#ifndef SRC_X3X2LISTMODEFRAMESTORE_H
#define SRC_X3X2LISTMODEFRAMESTORE_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameProcessorPlugin.h"
#include "gettime.h"

namespace FrameProcessor
{

  /**
   * Generic class for managing a block of memory containing a single field
   * of list mode data.
   */
  class X3X2ListModeFrameStore
  {
  public:
    X3X2ListModeFrameStore(const std::string& name, DataType data_type, uint32_t size_bytes);
    virtual ~X3X2ListModeFrameStore();
    void set_size(uint32_t bytes);
    void reallocate();
    void reset();
    void reset_frame_count();
    boost::shared_ptr <Frame> to_frame();
    boost::shared_ptr <Frame> flush();

  protected:
    void *ptr_;
    std::string name_;
    uint32_t num_bytes_;
    uint32_t filled_size_;
    uint32_t frame_count_;
    uint32_t max_size_;
    uint32_t data_size_bytes_;

    DataType data_type_;

    /** Pointer to logger */
    LoggerPtr logger_;
  };


  /**
   * Specific class for managing timeframe memory blocks
   */
  class X3X2ListModeFrameStoreTimeframe :  public X3X2ListModeFrameStore
  {
  public:
    X3X2ListModeFrameStoreTimeframe(const std::string& name);
    virtual ~X3X2ListModeFrameStoreTimeframe();

    boost::shared_ptr <Frame> add_timeframe(uint64_t *timeframe, uint32_t qty);
  };



  /**
   * Specific class for managing timestamp memory blocks
   */
  class X3X2ListModeFrameStoreTimestamp :  public X3X2ListModeFrameStore
  {
  public:
    X3X2ListModeFrameStoreTimestamp(const std::string& name);
    virtual ~X3X2ListModeFrameStoreTimestamp();

    boost::shared_ptr <Frame> add_timestamp(uint64_t *timestamp, uint32_t qty);
  };

  /**
   * Specific class for managing event height memory blocks
   */
  class X3X2ListModeFrameStoreEventHeight :  public X3X2ListModeFrameStore
  {
  public:
    X3X2ListModeFrameStoreEventHeight(const std::string& name);
    virtual ~X3X2ListModeFrameStoreEventHeight();

    boost::shared_ptr <Frame> add_event_height(uint16_t *event_height, uint32_t qty);
  };

  /**
   * Specific class for managing event reset flag memory blocks
   */
  class X3X2ListModeFrameStoreResetFlag :  public X3X2ListModeFrameStore
  {
  public:
    X3X2ListModeFrameStoreResetFlag(const std::string& name);
    virtual ~X3X2ListModeFrameStoreResetFlag();

    boost::shared_ptr <Frame> add_reset_flag(uint8_t *reset_flag, uint32_t qty);
  };

}

#endif //SRC_X3X2LISTMODEMEMORYBLOCK_H
