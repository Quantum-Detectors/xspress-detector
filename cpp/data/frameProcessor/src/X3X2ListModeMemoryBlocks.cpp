#include <iostream>

#include "DataBlockFrame.h"
#include "DebugLevelLogger.h"

#include "X3X2ListModeMemoryBlocks.h"
#include "FrameProcessorDefinitions.h"

namespace FrameProcessor {

// ============================================================================
// Base memory block class
// ============================================================================

X3X2ListModeMemoryBlock::X3X2ListModeMemoryBlock(
    const std::string& name,
    DataType data_type,
    uint32_t num_bytes_per_event
  ) :
  ptr_(0),
  num_bytes_(0),
  filled_size_(0),
  frame_count_(0),
  data_type_(data_type),
  num_bytes_per_event_(num_bytes_per_event)
{
  name_ = name;

  // Setup logging for the class
  logger_ = Logger::getLogger("FP.X3X2ListModeProcessPlugin");
  LOG4CXX_INFO(logger_, "[" << name_ << "]" << "Created X3X2ListModeMemoryBlock");
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
  LOG4CXX_INFO(logger_, "[" << name_ << "]" << "Reallocating X3X2ListModeMemoryBlock to [" << num_bytes_ << "] bytes");
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

boost::shared_ptr <Frame> X3X2ListModeMemoryBlock::to_frame()
{
  LOG4CXX_INFO(logger_, "[" << name_ << "]" << "Flushing frame " << frame_count_ << " of " << num_bytes_ << " bytes");
  boost::shared_ptr <Frame> frame;

  // Create the frame around the current (partial) block
  dimensions_t dims;
  FrameMetaData list_metadata(frame_count_, name_, data_type_, "", dims);
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
  LOG4CXX_INFO(logger_, "[" << name_ << "]" << "Flushing partial frame " << frame_count_ << " of " << filled_size_ << " bytes");
  boost::shared_ptr <Frame> frame;

  // Create the frame around the current (partial) block
  dimensions_t dims;
  FrameMetaData list_metadata(frame_count_, name_, data_type_, "", dims);
  frame = boost::shared_ptr<Frame>(new DataBlockFrame(list_metadata, filled_size_));
  memcpy(frame->get_data_ptr(), ptr_, filled_size_);

  return frame;
}


// ============================================================================
// Timeframe memory block
// ============================================================================

X3X2ListModeTimeframeMemoryBlock::X3X2ListModeTimeframeMemoryBlock(const std::string& name) :
    X3X2ListModeMemoryBlock(name, raw_64bit, 8)
{
  LOG4CXX_INFO(logger_, "[" << name_ << "]" << "Created X3X2ListModeTimeframeMemoryBlock");
}

X3X2ListModeTimeframeMemoryBlock::~X3X2ListModeTimeframeMemoryBlock()
{
}

boost::shared_ptr <Frame> X3X2ListModeTimeframeMemoryBlock::add_timeframe(uint64_t timeframe)
{
  boost::shared_ptr <Frame> frame;

  // Calculate the current end of data pointer location
  char *dest = (char *)ptr_;
  dest += filled_size_;

  // Add the event
  *((uint64_t *)dest) = timeframe;

  filled_size_ += sizeof(uint64_t);

  // Final check, if we have a full buffer then send it out
  if (filled_size_ == num_bytes_){
    frame = this->to_frame();
  }

  return frame;
}


// ============================================================================
// Timestamp memory block
// ============================================================================

X3X2ListModeTimestampMemoryBlock::X3X2ListModeTimestampMemoryBlock(const std::string& name) :
    X3X2ListModeMemoryBlock(name, raw_64bit, 8)
{
  LOG4CXX_INFO(logger_, "[" << name_ << "]" << "Created X3X2ListModeTimestampMemoryBlock");
}

X3X2ListModeTimestampMemoryBlock::~X3X2ListModeTimestampMemoryBlock()
{
}

boost::shared_ptr <Frame> X3X2ListModeTimestampMemoryBlock::add_timestamp(uint64_t timestamp)
{
  boost::shared_ptr <Frame> frame;

  // Calculate the current end of data pointer location
  char *dest = (char *)ptr_;
  dest += filled_size_;

  // Add the event
  *((uint64_t *)dest) = timestamp;

  filled_size_ += sizeof(uint64_t);

  // Final check, if we have a full buffer then send it out
  if (filled_size_ == num_bytes_){
    frame = this->to_frame();
  }

  return frame;
}


// ============================================================================
// Timeframe memory block
// ============================================================================

X3X2ListModeEventHeightMemoryBlock::X3X2ListModeEventHeightMemoryBlock(const std::string& name) :
    X3X2ListModeMemoryBlock(name, raw_16bit, 2)
{
  LOG4CXX_INFO(logger_, "[" << name_ << "]" << "Created X3X2ListModeEventHeightMemoryBlock");
}

X3X2ListModeEventHeightMemoryBlock::~X3X2ListModeEventHeightMemoryBlock()
{
}

boost::shared_ptr <Frame> X3X2ListModeEventHeightMemoryBlock::add_event_height(uint16_t event_height)
{
  boost::shared_ptr <Frame> frame;

  // Calculate the current end of data pointer location
  char *dest = (char *)ptr_;
  dest += filled_size_;

  // Add the event
  *((uint16_t *)dest) = event_height;

  filled_size_ += sizeof(uint16_t);

  // Final check, if we have a full buffer then send it out
  if (filled_size_ == num_bytes_){
    frame = this->to_frame();
  }

  return frame;
}

}
