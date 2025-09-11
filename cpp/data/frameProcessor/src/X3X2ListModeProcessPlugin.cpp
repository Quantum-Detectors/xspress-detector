#include <iostream>

#include "DataBlockFrame.h"
#include "DebugLevelLogger.h"

#include "X3X2ListModeProcessPlugin.h"
#include "FrameProcessorDefinitions.h"
#include "X3X2Definitions.h"


namespace FrameProcessor {

const std::string X3X2ListModeProcessPlugin::CONFIG_CHANNELS =           "channels";
const std::string X3X2ListModeProcessPlugin::CONFIG_RESET_ACQUISITION =  "reset";
const std::string X3X2ListModeProcessPlugin::CONFIG_FLUSH_ACQUISITION =  "flush";
const std::string X3X2ListModeProcessPlugin::CONFIG_FRAME_SIZE =         "frame_size";
const std::string X3X2ListModeProcessPlugin::CONFIG_TIME_FRAMES =        "time_frames";

X3X2ListModeProcessPlugin::X3X2ListModeProcessPlugin() :
  num_channels_(0),
  channel_offset_(0),
  num_time_frames_(0),
  num_events_(),
  completed_channels_(),
  acquisition_complete_(false)
{
  // Setup logging for the class
  logger_ = Logger::getLogger("FP.X3X2ListModeProcessPlugin");
  LOG4CXX_INFO(logger_, "X3X2ListModeProcessPlugin version " << this->get_version_long() << " loaded");
}

X3X2ListModeProcessPlugin::~X3X2ListModeProcessPlugin()
{
  LOG4CXX_TRACE(logger_, "X3X2ListModeProcessPlugin destructor.");
}

void X3X2ListModeProcessPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply) 
{
  if (config.has_param(X3X2ListModeProcessPlugin::CONFIG_CHANNELS)){
    std::stringstream ss;
    ss << "Configure process plugin for channels [";
    const rapidjson::Value& channels = config.get_param<const rapidjson::Value&>(X3X2ListModeProcessPlugin::CONFIG_CHANNELS);
    std::vector<uint32_t> channel_vec;
    size_t num_channels = channels.Size();
    for (size_t i = 0; i < num_channels; i++) {
      const rapidjson::Value& chan_value = channels[i];
      channel_vec.push_back(chan_value.GetInt());
      ss << chan_value.GetInt();
      if (i < num_channels - 1){
        ss << ", ";
      }
    }
    ss << "]";
    LOG4CXX_INFO(logger_, ss.str());
    this->set_channels(channel_vec);
  }

  if (config.has_param(X3X2ListModeProcessPlugin::CONFIG_RESET_ACQUISITION)){
    this->reset_acquisition();
  }

  if (config.has_param(X3X2ListModeProcessPlugin::CONFIG_FLUSH_ACQUISITION)){
    this->flush_close_acquisition();
  }

  if (config.has_param(X3X2ListModeProcessPlugin::CONFIG_FRAME_SIZE)){
    unsigned int frame_size = config.get_param<unsigned int>(X3X2ListModeProcessPlugin::CONFIG_FRAME_SIZE);
    this->set_frame_size(frame_size);
  }

  if (config.has_param(X3X2ListModeProcessPlugin::CONFIG_TIME_FRAMES)){
    num_time_frames_ = config.get_param<unsigned int>(X3X2ListModeProcessPlugin::CONFIG_TIME_FRAMES);
    LOG4CXX_INFO(logger_, "Number of time frames has been set to  " << num_time_frames_);
  }

}

// Version functions
int X3X2ListModeProcessPlugin::get_version_major() 
{
  return XSPRESS_DETECTOR_VERSION_MAJOR;
}

int X3X2ListModeProcessPlugin::get_version_minor()
{
  return XSPRESS_DETECTOR_VERSION_MINOR;
}

int X3X2ListModeProcessPlugin::get_version_patch()
{
  return XSPRESS_DETECTOR_VERSION_PATCH;
}

std::string X3X2ListModeProcessPlugin::get_version_short()
{
  return XSPRESS_DETECTOR_VERSION_STR_SHORT;
}

std::string X3X2ListModeProcessPlugin::get_version_long()
{
  return XSPRESS_DETECTOR_VERSION_STR;
}

void X3X2ListModeProcessPlugin::set_channels(std::vector<uint32_t> channels)
{
  channels_ = channels;
  num_channels_ = channels.size();

  reset_channel_statistics();

  // TCP frames only report as channels 0 and 1 - we need to store
  // the offset so we can report the correct system channels to the file
  // writer
  channel_offset_ = channels_[0];

  // We must reallocate memory blocks
  setup_memory_allocation();
}

void X3X2ListModeProcessPlugin::clear_timeframe_memory_blocks()
{
  std::map<uint32_t, boost::shared_ptr<X3X2ListModeTimeframeMemoryBlock> >::iterator iter;
  for (iter = timeframe_memory_ptrs_.begin(); iter != timeframe_memory_ptrs_.end(); ++iter){
    iter->second->reset_frame_count();
    iter->second->reset();
  }
}

void X3X2ListModeProcessPlugin::clear_timestamp_memory_blocks()
{
  std::map<uint32_t, boost::shared_ptr<X3X2ListModeTimestampMemoryBlock> >::iterator iter;
  for (iter = timestamp_memory_ptrs_.begin(); iter != timestamp_memory_ptrs_.end(); ++iter){
    iter->second->reset_frame_count();
    iter->second->reset();
  }
}

void X3X2ListModeProcessPlugin::clear_event_height_memory_blocks()
{
  std::map<uint32_t, boost::shared_ptr<X3X2ListModeEventHeightMemoryBlock> >::iterator iter;
  for (iter = event_height_memory_ptrs_.begin(); iter != event_height_memory_ptrs_.end(); ++iter){
    iter->second->reset_frame_count();
    iter->second->reset();
  }
}

void X3X2ListModeProcessPlugin::clear_reset_flag_memory_blocks()
{
  std::map<uint32_t, boost::shared_ptr<X3X2ListModeResetFlagMemoryBlock> >::iterator iter;
  for (iter = reset_flag_memory_ptrs_.begin(); iter != reset_flag_memory_ptrs_.end(); ++iter){
    iter->second->reset_frame_count();
    iter->second->reset();
  }
}

void X3X2ListModeProcessPlugin::reset_acquisition()
{
  LOG4CXX_INFO(logger_, "Resetting acquisition");

  clear_timeframe_memory_blocks();
  clear_timestamp_memory_blocks();
  clear_event_height_memory_blocks();
  clear_reset_flag_memory_blocks();

  acquisition_complete_ = false;

  reset_channel_statistics();
}

void X3X2ListModeProcessPlugin::flush_timeframe_memory_blocks()
{
  std::map<uint32_t, boost::shared_ptr<X3X2ListModeTimeframeMemoryBlock> >::iterator iter;
  for (iter = timeframe_memory_ptrs_.begin(); iter != timeframe_memory_ptrs_.end(); ++iter){
    LOG4CXX_DEBUG_LEVEL(0, logger_, "Flushing timeframe for channel " << iter->first);
    boost::shared_ptr <Frame> list_frame = iter->second->flush();
    if (list_frame){
      this->push(list_frame);
    }
  }
}

void X3X2ListModeProcessPlugin::flush_timestamp_memory_blocks()
{
  std::map<uint32_t, boost::shared_ptr<X3X2ListModeTimestampMemoryBlock> >::iterator iter;
  for (iter = timestamp_memory_ptrs_.begin(); iter != timestamp_memory_ptrs_.end(); ++iter){
    LOG4CXX_DEBUG_LEVEL(0, logger_, "Flushing timestamp for channel " << iter->first);
    boost::shared_ptr <Frame> list_frame = iter->second->flush();
    if (list_frame){
      this->push(list_frame);
    }
  }
}

void X3X2ListModeProcessPlugin::flush_event_height_memory_blocks()
{
  std::map<uint32_t, boost::shared_ptr<X3X2ListModeEventHeightMemoryBlock> >::iterator iter;
  for (iter = event_height_memory_ptrs_.begin(); iter != event_height_memory_ptrs_.end(); ++iter){
    LOG4CXX_DEBUG_LEVEL(0, logger_, "Flushing event height for channel " << iter->first);
    boost::shared_ptr <Frame> list_frame = iter->second->flush();
    if (list_frame){
      this->push(list_frame);
    }
  }
}

void X3X2ListModeProcessPlugin::flush_reset_flag_memory_blocks()
{
  std::map<uint32_t, boost::shared_ptr<X3X2ListModeResetFlagMemoryBlock> >::iterator iter;
  for (iter = reset_flag_memory_ptrs_.begin(); iter != reset_flag_memory_ptrs_.end(); ++iter){
    LOG4CXX_DEBUG_LEVEL(0, logger_, "Flushing event height for channel " << iter->first);
    boost::shared_ptr <Frame> list_frame = iter->second->flush();
    if (list_frame){
      this->push(list_frame);
    }
  }
}

void X3X2ListModeProcessPlugin::flush_close_acquisition()
{
  LOG4CXX_INFO(logger_, "Flushing and closing acquisition");

  flush_timeframe_memory_blocks();
  flush_timestamp_memory_blocks();
  flush_event_height_memory_blocks();
  flush_reset_flag_memory_blocks();

  this->notify_end_of_acquisition();
}

void X3X2ListModeProcessPlugin::set_frame_size(uint32_t num_bytes)
{
  LOG4CXX_INFO(logger_, "Setting frame size to " << num_bytes << " bytes");
  frame_size_bytes_ = num_bytes;
  // We must reallocate memory blocks
  setup_memory_allocation();
}

/**
 * Set up the channel memory blocks for a single channel
 * 
 * A separate block is used for each event field.
 * 
 * \param[in] channel - Channel number
 */
void X3X2ListModeProcessPlugin::setup_channel_memory_blocks(uint32_t channel)
{
  std::string timeframe_name = "ch" + std::to_string(channel) + "_time_frame";
  std::string timestamp_name = "ch" + std::to_string(channel) + "_time_stamp";
  std::string event_height_name = "ch" + std::to_string(channel) + "_event_height";
  std::string reset_flag_name = "ch" + std::to_string(channel) + "_reset_flag";

  // Create the memory blocks
  boost::shared_ptr<X3X2ListModeTimeframeMemoryBlock> frame_ptr =
    boost::shared_ptr<X3X2ListModeTimeframeMemoryBlock>(
      new X3X2ListModeTimeframeMemoryBlock(timeframe_name)
    );
  frame_ptr->set_size(frame_size_bytes_);
  timeframe_memory_ptrs_[channel] = frame_ptr;

  boost::shared_ptr<X3X2ListModeTimestampMemoryBlock> stamp_ptr =
    boost::shared_ptr<X3X2ListModeTimestampMemoryBlock>(
      new X3X2ListModeTimestampMemoryBlock(timestamp_name)
    );
  stamp_ptr->set_size(frame_size_bytes_);
  timestamp_memory_ptrs_[channel] = stamp_ptr;

  boost::shared_ptr<X3X2ListModeEventHeightMemoryBlock> height_ptr =
    boost::shared_ptr<X3X2ListModeEventHeightMemoryBlock>(
      new X3X2ListModeEventHeightMemoryBlock(event_height_name)
    );
  height_ptr->set_size(frame_size_bytes_);
  event_height_memory_ptrs_[channel] = height_ptr;

  boost::shared_ptr<X3X2ListModeResetFlagMemoryBlock> reset_ptr =
    boost::shared_ptr<X3X2ListModeResetFlagMemoryBlock>(
      new X3X2ListModeResetFlagMemoryBlock(reset_flag_name)
    );
  reset_ptr->set_size(frame_size_bytes_);
  reset_flag_memory_ptrs_[channel] = reset_ptr;

  // Setup the storage vectors for the packet header information
  std::vector<uint32_t> hdr(3, 0);
  packet_headers_[channel] = hdr;
}

/**
 * Reset the attributes used to track attributes per channel
 */
void X3X2ListModeProcessPlugin::reset_channel_statistics()
{
  completed_channels_.clear();
  num_events_.clear();
  for (auto const& chan : channels_)
  {
    completed_channels_[chan] = false;
    num_events_[chan] = 0;
  }
}

void X3X2ListModeProcessPlugin::setup_memory_allocation()
{
  // First clear out the memory vectors emptying any blocks
  timeframe_memory_ptrs_.clear();
  timestamp_memory_ptrs_.clear();
  event_height_memory_ptrs_.clear();
  reset_flag_memory_ptrs_.clear();

  // Allocate large enough blocks of memory to hold list mode frames
  // Allocate one block of memory for each event field for each channel
  std::vector<uint32_t>::iterator iter;
  for (iter = channels_.begin(); iter != channels_.end(); ++iter){
    setup_channel_memory_blocks(*iter);
  }
}

/**
 * Collate status information for the plugin. The status is added to the status IpcMessage object.
 *
 * \param[out] status - Reference to an IpcMessage value to store the status.
 */
void X3X2ListModeProcessPlugin::status(OdinData::IpcMessage& status)
{
  std::map<uint32_t, std::vector<uint32_t> >::iterator iter;
  for (iter = packet_headers_.begin(); iter != packet_headers_.end(); ++iter){
    std::stringstream ss;
    ss << "channel_" << iter->first;
    std::vector<uint32_t> hdr = iter->second;
    for (int index = 0; index < hdr.size(); index++){
      status.set_param(get_name() + "/" + ss.str() + "[]", hdr[index]);
    }
  }
}

void X3X2ListModeProcessPlugin::process_frame(boost::shared_ptr <Frame> frame) 
{
  // LOG4CXX_INFO(logger_, "Processing frame " << frame->get_frame_number() << " of size " << frame->get_data_size() << " at " << frame->get_data_ptr());

  // Packet arrived after we got the EOF for the desired time frame
  if (acquisition_complete_) return;

  uint16_t* frame_data = static_cast<uint16_t *>(frame->get_data_ptr());

  // TODO: remove after testing
  std::string ids("");
  std::string values("");

  // Event attributes
  uint16_t acquisition_number = 0;
  uint64_t time_frame = 0;
  uint64_t time_stamp = 0;

  uint16_t event_height = 0;

  bool dummy_event = 0;
  bool end_of_frame = 0;
  bool ttl_a = 0;
  bool ttl_b = 0;

  bool reset_flag = false;

  uint16_t channel = -1;

  // Track number of events
  unsigned int num_resets = 0;
  unsigned int num_padding = 0;

  uint16_t id;
  uint16_t value;
  uint64_t value_64;

  for (unsigned int field = 0; field < X3X2_MINI_FIELDS_PER_FRAME; field++)
  {
    // Decode field
    id = frame_data[field] >> 12;
    value = frame_data[field] & 0xFFF;
    value_64 = (uint64_t) value;

    // TODO: remove after testing
    //ids += std::to_string(id) + ",";
    //values += std::to_string(value) + ",";

    switch (id)
    {
      case 1:
        acquisition_number = value;
        break;
      case 4:
        end_of_frame = value & 0x1;
        ttl_a = value & 0x2;
        ttl_b = value & 0x4;
        dummy_event = value & 0x8;
        time_frame = (time_frame & 0xFFFFFFFFFFFFFF00) | ((value_64 &0xFF0) >> 4);
        break;
      case 5:
        time_frame = (time_frame & 0xFFFFFFFFFFF000FF) | (value_64 << 8);
        break;
      case 6:
        time_frame = (time_frame & 0xFFFFFFFF000FFFFF) | (value_64 << 20);
        break;
      case 7:
        time_frame = (time_frame & 0xFFFFF000FFFFFFFF) | (value_64 << 32);
        break;
      case 8:
        time_frame = (time_frame & 0xFF000FFFFFFFFFFF) | (value_64 << 44);
        break;
      case 9:
        // Calculate actual channel in system
        channel = (value >> 8) + channel_offset_;
        // Check we have the right channel (and ignore markers)
        if (timestamp_memory_ptrs_.find(channel) == timestamp_memory_ptrs_.end()) {
          // Ignore entire packet
          return;
        }
        time_frame = (time_frame & 0x00FFFFFFFFFFFFFF) | ((value_64 & 0xFF) << 56);
        break;
      case 10:
        time_stamp = (time_stamp & 0xFFFFFFFFF000) | value_64;
        break;
      case 11:
        time_stamp = (time_stamp & 0xFFFFFF000FFF) | (value_64 << 12);
        break;
      case 12:
        time_stamp = (time_stamp & 0xFFF000FFFFFF) | (value_64 << 24);
        break;
      case 13:
        time_stamp = (time_stamp & 0x000FFFFFFFFF) | (value_64 << 36);
        break;
      case 15:
        num_padding++;
        break;
      case 14:
        // Reset event width
        num_resets++;
      case 0:
        // Event height
        event_height = value;

        // TODO: remove when tested
        //LOG4CXX_INFO(logger_, "Got values: " << time_frame << ", " << time_stamp << ", " << event_height << " for first event ");
        //return

        // xspress3m_active_readout only counts events when not end of frame so we copy this logic here
        if (!end_of_frame)
        {
          if (dummy_event == 0) {
            boost::shared_ptr <Frame> list_frame = (timeframe_memory_ptrs_[channel])->add_timeframe(time_frame);
            // Memory block frame completed
            if (list_frame) this->push(list_frame);

            list_frame = (timestamp_memory_ptrs_[channel])->add_timestamp(time_stamp);
            // Memory block frame completed
            if (list_frame) this->push(list_frame);

            list_frame = (event_height_memory_ptrs_[channel])->add_event_height(event_height);
            // Memory block frame completed
            if (list_frame) this->push(list_frame);

            reset_flag = (id == 14) ? true : false;
            list_frame = (reset_flag_memory_ptrs_[channel])->add_reset_flag(reset_flag);
            // Memory block frame completed
            if (list_frame) this->push(list_frame);

            // Track overall number of recorded events in acquisition (including resets)
            num_events_[channel]++;
          }
        }
        else if (!acquisition_complete_)
        {
          // TODO: work out why we get more events after the end of frame marker is set and see if we need to
          // save them or ignore them (we ignore them here)
          LOG4CXX_INFO(logger_, "Channel " << channel << " got end of frame for frame " << time_frame);
          if (time_frame + 1 == num_time_frames_)
          {
            LOG4CXX_INFO(logger_, "Acquisition of " << num_time_frames_ << " frames complete for channel " << channel);
            completed_channels_[channel] = true;

            // Check if every channel is now finished
            uint16_t completed_channels = 0;
            for (auto const& it : completed_channels_)
            {
              if (it.second) completed_channels++;
            }
            if (completed_channels == num_channels_)
            {
              this->flush_close_acquisition();
              acquisition_complete_ = true;
              LOG4CXX_INFO(logger_, "Acquisition of " << num_time_frames_ << " frames completed for all channels");
              for (auto const& chan : channels_)
              {
                LOG4CXX_INFO(
                  logger_,
                  "Total events in channel " << chan << ": " << num_events_[chan]
                );
              }
              return;
            }
          }
        }

        break;
    }
  }

}

}
