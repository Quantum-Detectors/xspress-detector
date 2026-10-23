#include <iostream>

#include "DataBlockFrame.h"
#include "DebugLevelLogger.h"

#include "X3X2ListModeProcessPlugin.h"
#include "FrameProcessorDefinitions.h"
#include "X3X2Definitions.h"


namespace FrameProcessor {

const std::string X3X2ListModeProcessPlugin::CONFIG_CHANNELS =           "channels";
const std::string X3X2ListModeProcessPlugin::CONFIG_MARKERS_ENABLED =    "markers";
const std::string X3X2ListModeProcessPlugin::CONFIG_RESET_ACQUISITION =  "reset";
const std::string X3X2ListModeProcessPlugin::CONFIG_FLUSH_ACQUISITION =  "flush";
const std::string X3X2ListModeProcessPlugin::CONFIG_FRAME_SIZE =         "frame_size";
const std::string X3X2ListModeProcessPlugin::CONFIG_TIME_FRAMES =        "time_frames";


X3X2ListModeProcessPlugin::X3X2ListModeProcessPlugin() :
  num_channels_(0),
  channel_offset_(0),
  marker_channels_enabled_(false),
  marker_channels_(),
  num_time_frames_(0),
  num_events_(),
  completed_channels_(),
  prev_time_frames_(),
  prev_time_stamps_(),
  frame_size_events_(524280),
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
  if (config.has_param(X3X2ListModeProcessPlugin::CONFIG_MARKERS_ENABLED)){
    // Marker channels are disabled by default
    const rapidjson::Value& markers = config.get_param<const rapidjson::Value&>(X3X2ListModeProcessPlugin::CONFIG_MARKERS_ENABLED);
    marker_channels_enabled_ = markers.GetBool();
    LOG4CXX_INFO(logger_, "Marker channels enabled: " << marker_channels_enabled_);
  }

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
    sch_.set_number_of_time_frames(num_time_frames_);
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

/**
 * Set the channels we are writing data for.
 * 
 * The channels should be based on the channel number in the system, rather than
 * relative to each ADC card (i.e. the first card has real channels 0,1 the
 * second has 2,3 etc.)
 * 
 * If the first channel is 0 then we also add the marker channels automatically
 *
 *  \param[in] channels Vector containing the channel numbers we are writing for
 */
void X3X2ListModeProcessPlugin::set_channels(std::vector<uint32_t> channels)
{
  // Each card reports TCP data starting from channel 0 - but in the fp<N>.json
  // we refer to each channel based on channel in the system. This tracks the
  // offset between the channel in the system and the first channel on the card
  channel_offset_ = channels[0];

  // If we have marker channels enabled then add them first
  if (marker_channels_enabled_) {
    LOG4CXX_INFO(logger_, "Adding marker channels");
    // Marker channels are always 3rd and 4th channel - so add based on offset
    // in system as this gets subtracted when decoding TCP packets
    channels.push_back(channel_offset_ + 2);
    channels.push_back(channel_offset_ + 3);

    // Track the marker channel numbers to give them different dataset names
    marker_channels_ = std::vector<uint32_t>();
    marker_channels_.push_back(channel_offset_ + 2);
    marker_channels_.push_back(channel_offset_ + 3);
  }

  channels_ = channels;
  num_channels_ = channels.size();

  LOG4CXX_INFO(logger_, "Configured for " << num_channels_ << " channels");

  // Send the channels to the scheduler
  sch_.set_channels(channels_, marker_channels_);

  reset_channel_statistics();

  // We must reallocate memory blocks
  setup_memory_allocation();
}

void X3X2ListModeProcessPlugin::reset_acquisition()
{
  LOG4CXX_INFO(logger_, "Resetting acquisition");

  acquisition_complete_ = false;
  sch_.reset_acquisition();

  reset_channel_statistics();
}

void X3X2ListModeProcessPlugin::flush_close_acquisition()
{
  // Only flush if we have a running acquisition
  if (!acquisition_complete_)
  {
    LOG4CXX_INFO(logger_, "Flushing and closing acquisition");

    // Mark acquisition as complete so we do not process any more data
    acquisition_complete_ = true;

    for (auto const& chan : channels_)
    {
      LOG4CXX_INFO(
        logger_,
        "Total events in channel " << chan << ": " << num_events_[chan]
      );
    }

    this->notify_end_of_acquisition();
  }
  else
  {
    LOG4CXX_INFO(logger_, "Skipping flush - no acquisition active");
  }
}

/**
 * Set the frame size in terms of number of events for each memory block
 *
 * This is different from Xspress 4 which uses bytes as we want a separate
 * dataset for each event field here.
 *
 *  \param[in] num_events - number of events to store per frame
 */
void X3X2ListModeProcessPlugin::set_frame_size(uint32_t num_events)
{
  LOG4CXX_INFO(logger_, "Setting frame size to " << num_events << " events");
  frame_size_events_ = num_events;
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
  // Names for memory block frames
  std::string prefix;
  std::string timeframe_name;
  std::string timestamp_name;
  std::string event_height_name;
  std::string reset_flag_name;

  // TODO: fix this logic
  if (std::find(marker_channels_.begin(), marker_channels_.end(), channel) == marker_channels_.end()) {
    prefix = "ch";
    timeframe_name = "ch" + std::to_string(channel) + "_time_frame";
    timestamp_name = "ch" + std::to_string(channel) + "_time_stamp";
    event_height_name = "ch" + std::to_string(channel) + "_event_height";
    reset_flag_name = "ch" + std::to_string(channel) + "_reset_flag";
  } else
  {
    // Use different dataset names for marker channels
    uint32_t marker_num = channel - channel_offset_ - 2;
    prefix = "marker";
    timeframe_name = "marker" + std::to_string(marker_num) + "_time_frame";
    timestamp_name = "marker" + std::to_string(marker_num) + "_time_stamp";
    event_height_name = "marker" + std::to_string(marker_num) + "_event_height";
    reset_flag_name = "marker" + std::to_string(marker_num) + "_reset_flag";
  }

  // Set up parallel frame blocks
  sch_.setup_frame_stores(channel, prefix, frame_size_events_);

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
  prev_time_frames_.clear();
  prev_time_stamps_.clear();
  num_events_.clear();
  for (auto const& chan : channels_)
  {
    completed_channels_[chan] = false;
    num_events_[chan] = 0;
    prev_time_frames_[chan] = 0;
    prev_time_stamps_[chan] = 0;
  }
}

void X3X2ListModeProcessPlugin::setup_memory_allocation()
{
  LOG4CXX_INFO(logger_, "Setting memory allocation for " << frame_size_events_ << " events");

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
  // Packet arrived after we have completed the acquisition - either by
  // receiving the EOF for the desired TF on all channels or it was manually
  // stopped using the Odin Data API
  if (acquisition_complete_) return;

  std::vector<boost::shared_ptr<Frame> > frames = sch_.process_frame(frame);
  LOG4CXX_DEBUG_LEVEL(2, logger_, "Parallel processing - Pushing " << frames.size() << " frames.");
  for (auto iter = frames.begin(); iter != frames.end(); iter++){
    this->push(*iter);
  }
  if (!acquisition_complete_ && sch_.get_acquisition_complete()){
    acquisition_complete_ = true;
    this->notify_end_of_acquisition();
  }
}

} // Namespace FrameProcessor
