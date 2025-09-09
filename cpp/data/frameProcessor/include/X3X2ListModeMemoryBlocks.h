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

  class X3X2ListModeMemoryBlock
  {
  public:
    X3X2ListModeMemoryBlock(const std::string& name);
    virtual ~X3X2ListModeMemoryBlock();
    void set_size(uint32_t bytes);
    void reallocate();
    void reset();
    void reset_frame_count();
    boost::shared_ptr <Frame> add_event(uint64_t time_stamp);
    boost::shared_ptr <Frame> to_frame();
    boost::shared_ptr <Frame> flush();

  private:
    void *ptr_;
    std::string name_;
    uint32_t num_bytes_;
    uint32_t filled_size_;  // Filled size in bytes
    uint32_t frame_count_;

    const uint32_t num_bytes_per_event_ = 8;

    /** Pointer to logger */
    LoggerPtr logger_;
  };

}

#endif //SRC_X3X2LISTMODEMEMORYBLOCK_H
