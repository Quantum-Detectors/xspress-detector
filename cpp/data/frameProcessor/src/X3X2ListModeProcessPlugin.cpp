//
// Created by hir12111 on 03/11/18.
//
#include <iostream>
#include "DataBlockFrame.h"
#include "XspressListModeProcessPlugin.h"
#include "FrameProcessorDefinitions.h"
#include "XspressDefinitions.h"
#include "DebugLevelLogger.h"

namespace FrameProcessor {

const std::string XspressListModeProcessPlugin::CONFIG_CHANNELS =           "channels";
const std::string XspressListModeProcessPlugin::CONFIG_RESET_ACQUISITION =  "reset";
const std::string XspressListModeProcessPlugin::CONFIG_FLUSH_ACQUISITION =  "flush";
const std::string XspressListModeProcessPlugin::CONFIG_FRAME_SIZE =         "frame_size";

#define XSP3_10GTX_SOF 0x80000000
#define XSP3_10GTX_EOF 0x40000000
#define XSP3_10GTX_PAD 0x20000000
#define XSP3_10GTX_PACKET_MASK 0x0FFFFFFF


#define XSP3_HGT64_SOF_GET_FRAME(x)       (((x)>>0)&0xFFFFFF)     //!< Get time frame from first (header) word
#define XSP3_HGT64_SOF_GET_PREV_TIME(x)   (((x)>>24)&0xFFFFFFFF)  //!< Get total integration time from previous time frame from first (header) word
#define XSP3_HGT64_SOF_GET_CHAN(x)        (((x)>>60)&0xF)         //!< Get channel number from first (header) word

#define XSP3_HGT64_MASK_END_OF_FRAME			(1L<<59)

XspressListModeMemoryBlock::XspressListModeMemoryBlock(const std::string& name) :
  ptr_(0),
  num_bytes_(0),
  num_words_(0),
  filled_size_(0),
  frame_count_(0)
{
  // Setup logging for the class
  logger_ = Logger::getLogger("FP.XspressListModeProcessPlugin");
  LOG4CXX_INFO(logger_, "Created XspressListModeMemoryBlock");

  name_ = name;
}

XspressListModeMemoryBlock::~XspressListModeMemoryBlock()
{
  if (ptr_){
    free(ptr_);
  }
}

void XspressListModeMemoryBlock::set_size(uint32_t bytes)
{
  num_bytes_ = bytes;
  num_words_ = num_bytes_ / sizeof(uint64_t);
  reallocate();
}

void XspressListModeMemoryBlock::reallocate()
{
  LOG4CXX_INFO(logger_, "Reallocating XspressListModeMemoryBlock to [" << num_bytes_ << "] bytes");
  if (ptr_){
    free(ptr_);
  }
  ptr_ = (char *)malloc(num_bytes_);
  reset();
}

void XspressListModeMemoryBlock::reset()
{
  memset(ptr_, 0, num_bytes_);
  filled_size_ = 0;
}

void XspressListModeMemoryBlock::reset_frame_count()
{
  frame_count_ = 0;
}

boost::shared_ptr <Frame> XspressListModeMemoryBlock::add_block(uint32_t bytes, void *ptr)
{
  boost::shared_ptr <Frame> frame;

  // Calculate the current end of data pointer location
  char *dest = (char *)ptr_;
  dest += filled_size_;
  // Set the number of packet words as a 64bit variable
  uint64_t pkt_words = (uint64_t)(bytes / sizeof(uint64_t));

  if (filled_size_ == num_bytes_){
    // Buffer is already full (this shouldn't really be possible but best to check)
    frame = this->to_frame();
  }

  // Copy the number of words into the dataset
  memcpy(dest, &pkt_words, sizeof(uint64_t));
  dest += sizeof(uint64_t);
  filled_size_ += sizeof(uint64_t);

  // Work out if adding the block will result in a full frame
  if (filled_size_ + bytes < num_bytes_){
    // We can copy the entire block into the store
    memcpy(dest, ptr, bytes);
    filled_size_ += bytes;
  } else {
    // Fill up the remainder of the block, create the frame and then copy over 
    // any remaining bytes
    uint32_t bytes_to_full = num_bytes_ - filled_size_;
    if (bytes_to_full > 0){
      memcpy(dest, ptr, bytes_to_full);
    }

    frame = this->to_frame();

    // Copy any remaining data
    uint32_t remaining_bytes = bytes - bytes_to_full;
    if (remaining_bytes > 0){
      dest = (char *)ptr_;
      char *src = (char *)ptr;
      src += bytes_to_full;
      memcpy(dest, src, remaining_bytes);
      filled_size_ += remaining_bytes;
    }
  }
  
  // Final check, if we have a full buffer then send it out
  if (filled_size_ == num_bytes_){
    frame = this->to_frame();
  }

  return frame;
}

boost::shared_ptr <Frame> XspressListModeMemoryBlock::to_frame()
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

boost::shared_ptr <Frame> XspressListModeMemoryBlock::flush()
{
  boost::shared_ptr <Frame> frame;

  // Create the frame around the current (partial) block
  dimensions_t dims;
  FrameMetaData list_metadata(frame_count_, name_, raw_64bit, "", dims);
  frame = boost::shared_ptr<Frame>(new DataBlockFrame(list_metadata, filled_size_));
  memcpy(frame->get_data_ptr(), ptr_, filled_size_);

  return frame;
}

XspressListModeProcessPlugin::XspressListModeProcessPlugin() :
  num_channels_(0)
{
  // Setup logging for the class
  logger_ = Logger::getLogger("FP.XspressListModeProcessPlugin");
  LOG4CXX_INFO(logger_, "XspressListModeProcessPlugin version " << this->get_version_long() << " loaded");
}

XspressListModeProcessPlugin::~XspressListModeProcessPlugin()
{
  LOG4CXX_TRACE(logger_, "XspressListModeProcessPlugin destructor.");
}

void XspressListModeProcessPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply) 
{
  if (config.has_param(XspressListModeProcessPlugin::CONFIG_CHANNELS)){
    std::stringstream ss;
    ss << "Configure process plugin for channels [";
    const rapidjson::Value& channels = config.get_param<const rapidjson::Value&>(XspressListModeProcessPlugin::CONFIG_CHANNELS);
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

  if (config.has_param(XspressListModeProcessPlugin::CONFIG_RESET_ACQUISITION)){
    this->reset_acquisition();
  }

  if (config.has_param(XspressListModeProcessPlugin::CONFIG_FLUSH_ACQUISITION)){
    this->flush_close_acquisition();
  }

  if (config.has_param(XspressListModeProcessPlugin::CONFIG_FRAME_SIZE)){
    unsigned int frame_size = config.get_param<unsigned int>(XspressListModeProcessPlugin::CONFIG_FRAME_SIZE);
    this->set_frame_size(frame_size);
  }
}

// Version functions
int XspressListModeProcessPlugin::get_version_major() 
{
  return XSPRESS_DETECTOR_VERSION_MAJOR;
}

int XspressListModeProcessPlugin::get_version_minor()
{
  return XSPRESS_DETECTOR_VERSION_MINOR;
}

int XspressListModeProcessPlugin::get_version_patch()
{
  return XSPRESS_DETECTOR_VERSION_PATCH;
}

std::string XspressListModeProcessPlugin::get_version_short()
{
  return XSPRESS_DETECTOR_VERSION_STR_SHORT;
}

std::string XspressListModeProcessPlugin::get_version_long()
{
  return XSPRESS_DETECTOR_VERSION_STR;
}

void XspressListModeProcessPlugin::set_channels(std::vector<uint32_t> channels)
{
  channels_ = channels;
  num_channels_ = channels.size();
  // We must reallocate memory blocks
  setup_memory_allocation();
}

void XspressListModeProcessPlugin::reset_acquisition()
{
  LOG4CXX_INFO(logger_, "Resetting acquisition");
  std::map<uint32_t, boost::shared_ptr<XspressListModeMemoryBlock> >::iterator iter;
  for (iter = memory_ptrs_.begin(); iter != memory_ptrs_.end(); ++iter){
    iter->second->reset_frame_count();
    iter->second->reset();
  }
}

void XspressListModeProcessPlugin::flush_close_acquisition()
{
  LOG4CXX_INFO(logger_, "Flushing and closing acquisition");
  std::map<uint32_t, boost::shared_ptr<XspressListModeMemoryBlock> >::iterator iter;
  for (iter = memory_ptrs_.begin(); iter != memory_ptrs_.end(); ++iter){
    LOG4CXX_DEBUG_LEVEL(0, logger_, "Flushing frame for channel " << iter->first);
    boost::shared_ptr <Frame> list_frame = iter->second->to_frame();
    if (list_frame){
      this->push(list_frame);
    }
  }
  this->notify_end_of_acquisition();
}

void XspressListModeProcessPlugin::set_frame_size(uint32_t num_bytes)
{
  LOG4CXX_INFO(logger_, "Setting frame size to " << num_bytes << " bytes");
  frame_size_bytes_ = num_bytes;
  // We must reallocate memory blocks
  setup_memory_allocation();
}

void XspressListModeProcessPlugin::setup_memory_allocation()
{
  // First clear out the memory vector emptying any blocks
  memory_ptrs_.clear();

  // Allocate large enough blocks of memory to hold list mode frames
  // Allocate one block of memory for each channel
  std::vector<uint32_t>::iterator iter;
  for (iter = channels_.begin(); iter != channels_.end(); ++iter){
    std::stringstream ss;
    ss << "raw_" << *iter;
    boost::shared_ptr<XspressListModeMemoryBlock> ptr = boost::shared_ptr<XspressListModeMemoryBlock>(new XspressListModeMemoryBlock(ss.str()));
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
void XspressListModeProcessPlugin::status(OdinData::IpcMessage& status)
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

void XspressListModeProcessPlugin::process_frame(boost::shared_ptr <Frame> frame) 
{
  LOG4CXX_INFO(logger_, "Processing frame " << frame->get_frame_number());
  // char* frame_bytes = static_cast<char *>(frame->get_data_ptr());

  /**
  Xspress::ListFrameHeader *header = reinterpret_cast<Xspress::ListFrameHeader *>(frame_bytes);

  LOG4CXX_DEBUG_LEVEL(2, logger_, "Received frame with " << header->packets_received << " packets");

  for (int packet_index = 0; packet_index < header->packets_received; packet_index++){

    char *data_ptr = frame_bytes + sizeof(Xspress::ListFrameHeader) + (packet_index * Xspress::xspress_packet_size);

    // Obtain the packet size and the channel number
    uint32_t pkt_size = header->packet_headers[packet_index].packet_size;
    uint32_t channel = header->packet_headers[packet_index].channel;

    LOG4CXX_DEBUG_LEVEL(3, logger_, "Received " << pkt_size << " bytes from channel " << channel);

    // Peek at incoming words
    uint64_t *peek_ptr = (uint64_t *)data_ptr;

    LOG4CXX_DEBUG_LEVEL(3, logger_, " Ch: " << channel
    << " FRAME: " << std::dec << XSP3_HGT64_SOF_GET_FRAME(peek_ptr[0])
    << " PREV_TIME: " << std::dec << XSP3_HGT64_SOF_GET_PREV_TIME(peek_ptr[0])
    << " CHAN: " << std::dec << XSP3_HGT64_SOF_GET_CHAN(peek_ptr[0]));

    if (packet_headers_.count(channel) > 0){
      packet_headers_[channel].clear();
      packet_headers_[channel].push_back(XSP3_HGT64_SOF_GET_FRAME(peek_ptr[0]));
      packet_headers_[channel].push_back(XSP3_HGT64_SOF_GET_PREV_TIME(peek_ptr[0]));
      packet_headers_[channel].push_back(XSP3_HGT64_SOF_GET_CHAN(peek_ptr[0]));

      // Place the bytes into the store
      boost::shared_ptr <Frame> list_frame = (memory_ptrs_[channel])->add_block(pkt_size, data_ptr);

      if (list_frame){
        LOG4CXX_DEBUG_LEVEL(1, logger_, "Completed frame for channel " << channel << ", pushing");
        // There is a full frame available for pushing
        this->push(list_frame);
      }

      if ((XSP3_HGT64_MASK_END_OF_FRAME&peek_ptr[0]) == XSP3_HGT64_MASK_END_OF_FRAME){
        LOG4CXX_DEBUG_LEVEL(1, logger_, " Ch: " << channel << " EOF marker registered");
      }

    } else {
      LOG4CXX_ERROR(logger_, "Bad channel, this plugin is not set up for channel " << channel);
    }
  }
  */
}

}
