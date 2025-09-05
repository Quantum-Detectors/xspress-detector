
#ifndef SRC_X3X2LISTMODEPLUGIN_H
#define SRC_X3X2LISTMODEPLUGIN_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameProcessorPlugin.h"
#include "XspressDefinitions.h"
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

    const uint32_t num_bytes_per_event_ = 24;

    /** Pointer to logger */
    LoggerPtr logger_;
  };

  class X3X2ListModeProcessPlugin : public FrameProcessorPlugin 
  {
  public:
    X3X2ListModeProcessPlugin();

    virtual ~X3X2ListModeProcessPlugin();

    void configure(OdinData::IpcMessage &config, OdinData::IpcMessage &reply);

    // version related functions
    int get_version_major();

    int get_version_minor();

    int get_version_patch();

    std::string get_version_short();

    std::string get_version_long();

  private:

    void reset_acquisition();
    void flush_close_acquisition();
    void set_channels(std::vector<uint32_t> channels);
    void set_frame_size(uint32_t num_bytes);
    void setup_memory_allocation();
        
    // Plugin interface
    void status(OdinData::IpcMessage& status);
    void process_frame(boost::shared_ptr <Frame> frame);

    uint32_t frame_size_bytes_;
    std::vector<uint32_t> channels_;
    uint32_t num_channels_;
    uint32_t channel_offset_;

    uint32_t num_time_frames_;
    uint32_t num_completed_channels_;
    std::vector<bool> completed_channels;
    bool acquisition_complete_;

    std::map<uint32_t, boost::shared_ptr<X3X2ListModeMemoryBlock> > memory_ptrs_;
    std::map<uint32_t, std::vector<uint32_t> > packet_headers_;

    static const std::string CONFIG_CHANNELS;
    static const std::string CONFIG_RESET_ACQUISITION;
    static const std::string CONFIG_FLUSH_ACQUISITION;
    static const std::string CONFIG_FRAME_SIZE;
    static const std::string CONFIG_TIME_FRAMES;

    /** Pointer to logger */
    LoggerPtr logger_;
  };

}


#endif //SRC_X3X2LISTMODEPLUGIN_H
