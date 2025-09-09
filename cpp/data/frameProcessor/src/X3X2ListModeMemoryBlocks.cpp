#include <iostream>

#include "DataBlockFrame.h"
#include "DebugLevelLogger.h"

#include "X3X2ListModeMemoryBlocks.h"
#include "FrameProcessorDefinitions.h"

namespace FrameProcessor {

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

}
