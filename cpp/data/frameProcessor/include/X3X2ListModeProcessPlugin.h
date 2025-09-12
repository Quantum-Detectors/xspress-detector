
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
#include "X3X2ListModeMemoryBlocks.h"
#include "gettime.h"

namespace FrameProcessor
{

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
    void set_frame_size(uint32_t num_events);
    void setup_memory_allocation();

    // Memory blocks
    void clear_timeframe_memory_blocks();
    void clear_timestamp_memory_blocks();
    void clear_event_height_memory_blocks();
    void clear_reset_flag_memory_blocks();

    void flush_timeframe_memory_blocks();
    void flush_timestamp_memory_blocks();
    void flush_event_height_memory_blocks();
    void flush_reset_flag_memory_blocks();

    void setup_channel_memory_blocks(uint32_t channel);

    void reset_channel_statistics();

    // Plugin interface
    void status(OdinData::IpcMessage& status);
    void process_frame(boost::shared_ptr <Frame> frame);

    uint32_t frame_size_events_;
    std::vector<uint32_t> channels_;
    uint32_t num_channels_;
    uint32_t channel_offset_;

    // Acquisition properties
    uint32_t num_time_frames_;
    bool acquisition_complete_;

    // Tracking per channel
    std::map<uint32_t, bool> completed_channels_;
    std::map<uint32_t, uint64_t> num_events_;

    // Memory blocks for event fields
    std::map<uint32_t, boost::shared_ptr<X3X2ListModeTimeframeMemoryBlock> > timeframe_memory_ptrs_;
    std::map<uint32_t, boost::shared_ptr<X3X2ListModeTimestampMemoryBlock> > timestamp_memory_ptrs_;
    std::map<uint32_t, boost::shared_ptr<X3X2ListModeEventHeightMemoryBlock> > event_height_memory_ptrs_;
    std::map<uint32_t, boost::shared_ptr<X3X2ListModeResetFlagMemoryBlock> > reset_flag_memory_ptrs_;

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
