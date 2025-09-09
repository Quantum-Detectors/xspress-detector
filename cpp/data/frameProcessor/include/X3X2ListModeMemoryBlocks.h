#ifndef SRC_X3X2LISTMODEMEMORYBLOCK_H
#define SRC_X3X2LISTMODEMEMORYBLOCK_H

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
  class X3X2ListModeMemoryBlock
  {
  public:
    X3X2ListModeMemoryBlock(const std::string& name, uint32_t num_bytes_per_event);
    virtual ~X3X2ListModeMemoryBlock();
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

    uint32_t num_bytes_per_event_;

    /** Pointer to logger */
    LoggerPtr logger_;
  };

  /**
   * Specific class for managing timestamp memory blocks
   */
  class X3X2ListModeTimestampMemoryBlock :  public X3X2ListModeMemoryBlock
  {
  public:
    X3X2ListModeTimestampMemoryBlock(const std::string& name);
    virtual ~X3X2ListModeTimestampMemoryBlock();

    boost::shared_ptr <Frame> add_timestamp(uint64_t timestamp);
  };

}

#endif //SRC_X3X2LISTMODEMEMORYBLOCK_H
