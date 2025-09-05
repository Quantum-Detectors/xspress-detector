#include <iostream>
#include "DataBlockFrame.h"
#include "X3X2ListModeProcessPlugin.h"
#include "FrameProcessorDefinitions.h"
#include "X3X2Definitions.h"
#include "DebugLevelLogger.h"


namespace FrameProcessor {

const std::string X3X2ListModeProcessPlugin::CONFIG_CHANNELS =           "channels";
const std::string X3X2ListModeProcessPlugin::CONFIG_RESET_ACQUISITION =  "reset";
const std::string X3X2ListModeProcessPlugin::CONFIG_FLUSH_ACQUISITION =  "flush";
const std::string X3X2ListModeProcessPlugin::CONFIG_FRAME_SIZE =         "frame_size";
const std::string X3X2ListModeProcessPlugin::CONFIG_TIME_FRAMES =        "time_frames";

X3X2ListModeMemoryBlock::X3X2ListModeMemoryBlock(const std::string& name) :
  ptr_(0),
  num_bytes_(0),
  filled_size_(0),
  frame_count_(0)
{
  // Setup logging for the class
  logger_ = Logger::getLogger("FP.X3X2ListModeProcessPlugin");
  LOG4CXX_INFO(logger_, "Created X3X2ListModeMemoryBlock");

  name_ = name;
}

X3X2ListModeMemoryBlock::~X3X2ListModeMemoryBlock()
{
  if (ptr_){
    free(ptr_);
  }
}

void X3X2ListModeMemoryBlock::set_size(uint32_t bytes)
{
  // Round allocation down to number of whole events
  num_bytes_ = (bytes/num_bytes_per_event_)*num_bytes_per_event_;
  reallocate();
}

void X3X2ListModeMemoryBlock::reallocate()
{
  LOG4CXX_INFO(logger_, "Reallocating X3X2ListModeMemoryBlock to [" << num_bytes_ << "] bytes");
  if (ptr_){
    free(ptr_);
  }
  ptr_ = (char *)malloc(num_bytes_);
  reset();
}

void X3X2ListModeMemoryBlock::reset()
{
  memset(ptr_, 0, num_bytes_);
  filled_size_ = 0;
}

void X3X2ListModeMemoryBlock::reset_frame_count()
{
  frame_count_ = 0;
}

boost::shared_ptr <Frame> X3X2ListModeMemoryBlock::add_event(uint64_t time_stamp)
{
  boost::shared_ptr <Frame> frame;

  // Calculate the current end of data pointer location
  char *dest = (char *)ptr_;
  dest += filled_size_;

  // Add the event
  *((uint64_t *)dest) = time_stamp;

  filled_size_ += sizeof(uint64_t);

  // Final check, if we have a full buffer then send it out
  if (filled_size_ == num_bytes_){
    frame = this->to_frame();
  }

  return frame;
}

boost::shared_ptr <Frame> X3X2ListModeMemoryBlock::to_frame()
{
  boost::shared_ptr <Frame> frame;

  // Create the frame around the current (partial) block
  dimensions_t dims;
  FrameMetaData list_metadata(frame_count_, name_, raw_64bit, "", dims);
  frame = boost::shared_ptr<Frame>(new DataBlockFrame(list_metadata, num_bytes_));
  memcpy(frame->get_data_ptr(), ptr_, num_bytes_);

  // Reset the block
  reset();

  // Add 1 to the frame count
  frame_count_++;

  return frame;
}

boost::shared_ptr <Frame> X3X2ListModeMemoryBlock::flush()
{
  boost::shared_ptr <Frame> frame;

  // Create the frame around the current (partial) block
  dimensions_t dims;
  FrameMetaData list_metadata(frame_count_, name_, raw_64bit, "", dims);
  frame = boost::shared_ptr<Frame>(new DataBlockFrame(list_metadata, filled_size_));
  memcpy(frame->get_data_ptr(), ptr_, filled_size_);

  return frame;
}

X3X2ListModeProcessPlugin::X3X2ListModeProcessPlugin() :
  num_channels_(0),
  channel_offset_(0),
  num_time_frames_(0),
  num_completed_channels_(0),
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
  // TCP frames only report as channels 0 and 1 - we need to store
  // the offset so we can report the correct system channels to the file
  // writer
  channel_offset_ = channels_[0];
  // We must reallocate memory blocks
  setup_memory_allocation();
}

void X3X2ListModeProcessPlugin::reset_acquisition()
{
  LOG4CXX_INFO(logger_, "Resetting acquisition");
  std::map<uint32_t, boost::shared_ptr<X3X2ListModeMemoryBlock> >::iterator iter;
  for (iter = memory_ptrs_.begin(); iter != memory_ptrs_.end(); ++iter){
    iter->second->reset_frame_count();
    iter->second->reset();
  }

  num_completed_channels_ = 0;
  acquisition_complete_ = false;
}

void X3X2ListModeProcessPlugin::flush_close_acquisition()
{
  LOG4CXX_INFO(logger_, "Flushing and closing acquisition");
  std::map<uint32_t, boost::shared_ptr<X3X2ListModeMemoryBlock> >::iterator iter;
  for (iter = memory_ptrs_.begin(); iter != memory_ptrs_.end(); ++iter){
    LOG4CXX_DEBUG_LEVEL(0, logger_, "Flushing frame for channel " << iter->first);
    boost::shared_ptr <Frame> list_frame = iter->second->to_frame();
    if (list_frame){
      this->push(list_frame);
    }
  }
  this->notify_end_of_acquisition();
}

void X3X2ListModeProcessPlugin::set_frame_size(uint32_t num_bytes)
{
  LOG4CXX_INFO(logger_, "Setting frame size to " << num_bytes << " bytes");
  frame_size_bytes_ = num_bytes;
  // We must reallocate memory blocks
  setup_memory_allocation();
}

void X3X2ListModeProcessPlugin::setup_memory_allocation()
{
  // First clear out the memory vector emptying any blocks
  memory_ptrs_.clear();

  // Allocate large enough blocks of memory to hold list mode frames
  // Allocate one block of memory for each channel
  std::vector<uint32_t>::iterator iter;
  for (iter = channels_.begin(); iter != channels_.end(); ++iter){
    std::stringstream ss;
    ss << "ch" << *iter << "_time_stamp";
    boost::shared_ptr<X3X2ListModeMemoryBlock> ptr = boost::shared_ptr<X3X2ListModeMemoryBlock>(new X3X2ListModeMemoryBlock(ss.str()));
    ptr->set_size(frame_size_bytes_);
    memory_ptrs_[*iter] = ptr;
    // Setup the storage vectors for the packet header information
    std::vector<uint32_t> hdr(3, 0);
    packet_headers_[*iter] = hdr;
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

  uint16_t* frame_data = static_cast<uint16_t *>(frame->get_data_ptr());

  // TODO: remove after testing
  std::string ids("");
  std::string values("");

  // Event attributes
  uint16_t acquisition_number = 0;
  uint64_t time_frame = 0;
  uint64_t time_stamp = 0;

  uint64_t event_height = 0;

  bool dummy_event = 0;
  bool end_of_frame = 0;
  bool ttl_a = 0;
  bool ttl_b = 0;

  uint16_t channel = -1;

  // Track number of events
  unsigned int num_events = 0;
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
        if (memory_ptrs_.find(channel) == memory_ptrs_.end()) {
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

        // Add event
        if (dummy_event == 0) {
          boost::shared_ptr <Frame> list_frame = (memory_ptrs_[channel])->add_event(time_stamp);

          // Memory block frame completed
          if (list_frame){
            // LOG4CXX_DEBUG_LEVEL(1, logger_, "Completed frame for channel " << channel << ", pushing");
            // There is a full frame available for pushing
            this->push(list_frame);
          }
        }

        // xspress3m_active_readout only counts events when not end of frame so we copy this logic here
        if (!end_of_frame)
        {
          if (dummy_event == 0) {
            boost::shared_ptr <Frame> list_frame = (memory_ptrs_[channel])->add_event(time_stamp);

            // Memory block frame completed
            if (list_frame){
              // LOG4CXX_DEBUG_LEVEL(1, logger_, "Completed frame for channel " << channel << ", pushing");
              // There is a full frame available for pushing
              this->push(list_frame);
            }
          }
        }
        else
        {
          LOG4CXX_INFO(logger_, "Channel " << channel << " got end of frame for frame " << time_frame);
          if (time_frame + 1 == num_time_frames_)
          {
            LOG4CXX_INFO(logger_, "Acquisition of " << num_time_frames_ << " frames complete for channel " << channel);
            num_completed_channels_++;
          }
        }

        // This counts resets along with normal events because no break for case 14.
        num_events++;
        break;
    }
  }

  if (num_completed_channels_ == num_channels_ && acquisition_complete_ == false)
  {
    LOG4CXX_INFO(logger_, "Acquisition of " << num_time_frames_ << " frames completed for all channels");
    this->flush_close_acquisition();
    acquisition_complete_ = true;
  }

  // TODO: remove after testing
  //LOG4CXX_INFO(logger_, "Got ids " << ids << " for channel " << channel);
  //LOG4CXX_INFO(logger_, "Got values " << values << " for channel " << channel);
  //LOG4CXX_INFO(logger_, "Time frame: " << time_frame);
  //LOG4CXX_INFO(logger_, "Events: " << num_events << ", Resets: " << num_resets << ", pad: " << num_padding);

}

}
