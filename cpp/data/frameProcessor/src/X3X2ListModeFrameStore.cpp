#include <iostream>

#include "DataBlockFrame.h"
#include "DebugLevelLogger.h"

#include "X3X2ListModeFrameStore.h"
#include "FrameProcessorDefinitions.h"

namespace FrameProcessor {

 X3X2ListModeFrameStore::X3X2ListModeFrameStore
(
  const std::string& name,
  DataType data_type,
  uint32_t data_size_bytes
) :
  ptr_(0),
  num_bytes_(0),
  filled_size_(0),
  max_size_(0),
  frame_count_(0),
  data_type_(data_type),
  data_size_bytes_(data_size_bytes)
{
  name_ = name;

  // Setup logging for the class
  logger_ = Logger::getLogger("FP.X3X2ListModeProcessPlugin");
  LOG4CXX_INFO(logger_, "[" << name_ << "]" << " Created X3X2ListModeMemoryBlock");
}

X3X2ListModeFrameStore::~X3X2ListModeFrameStore()
{
  if (ptr_){
    free(ptr_);
  }
}

/**
 * Set the size of the memory block in bytes.
 */
void X3X2ListModeFrameStore::set_size(uint32_t bytes)
{
  // Round allocation down to number of whole events
  num_bytes_ = (bytes / data_size_bytes_) * data_size_bytes_;
  reallocate();
}

void X3X2ListModeFrameStore::reallocate()
{
  LOG4CXX_INFO(logger_, "[" << name_ << "]" << " Reallocating X3X2ListModeFrameStore to [" << num_bytes_ << "] bytes");
  if (ptr_){
    free(ptr_);
  }
  ptr_ = (char *)malloc(num_bytes_);
  max_size_ = num_bytes_ / data_size_bytes_;
  reset();
}

void X3X2ListModeFrameStore::reset()
{
  memset(ptr_, 0, num_bytes_);
  filled_size_ = 0;
}

void X3X2ListModeFrameStore::reset_frame_count()
{
  frame_count_ = 0;
}

boost::shared_ptr <Frame> X3X2ListModeFrameStore::to_frame()
{
  LOG4CXX_INFO(logger_, "[" << name_ << "]" << " Getting complete frame " << frame_count_ << " of " << num_bytes_ << " bytes");
  boost::shared_ptr <Frame> frame;

  // Create the frame around the complete block
  dimensions_t dims;
  FrameMetaData list_metadata(frame_count_, name_, data_type_, "", dims);
  frame = boost::shared_ptr<Frame>(new DataBlockFrame(list_metadata, ptr_, num_bytes_));

  // Reset the block
  reset();

  // Add 1 to the frame count
  frame_count_++;

  return frame;
}

boost::shared_ptr <Frame> X3X2ListModeFrameStore::flush()
{
  LOG4CXX_INFO(logger_, "[" << name_ << "]" << " Flushing partial frame " << frame_count_ << " of " << filled_size_ << " bytes");
  boost::shared_ptr <Frame> frame;

  // Create the frame around the current (partial) block
  dimensions_t dims;
  FrameMetaData list_metadata(frame_count_, name_, data_type_, "", dims);
  frame = boost::shared_ptr<Frame>(new DataBlockFrame(list_metadata, ptr_, filled_size_));

  // We don't reset here as this is called at the end of an acquisition by
  // flush_close_acquisition. Resetting should reset the memory block before
  // the next one starts

  return frame;
}




X3X2ListModeFrameStoreTimeframe::X3X2ListModeFrameStoreTimeframe(const std::string& name) :
  X3X2ListModeFrameStore(name, raw_64bit, sizeof(uint64_t))
{
  LOG4CXX_INFO(logger_, "[" << name_ << "]" << " Created X3X2ListModeFrameStoreTimeframe");
}

X3X2ListModeFrameStoreTimeframe::~X3X2ListModeFrameStoreTimeframe()
{
}

boost::shared_ptr <Frame> X3X2ListModeFrameStoreTimeframe::add_timeframe(uint64_t *timeframe, uint32_t qty)
{
  boost::shared_ptr <Frame> frame;
  uint32_t block_size = qty;
  char *dest = (char *)ptr_;
  dest += (filled_size_ * sizeof(uint64_t));

  // Check if the block is bigger than the space allocated
  if ((filled_size_ + qty) >= max_size_){
    // Calculate the size to copy
    block_size = max_size_ - filled_size_;

    // Make the copy
    memcpy(dest, (char *)timeframe, block_size * sizeof(uint64_t));

    // Create the frame
    frame = this->to_frame();

    // Now copy the remaining block
    char *src = (char *)timeframe;
    src += (block_size * sizeof(uint64_t));
    block_size = qty - block_size;
    memcpy(ptr_, src, block_size * sizeof(uint64_t));
    filled_size_ = block_size;
  } else {
    // The block fits completely inside the memory so one simple copy
    memcpy(dest, (char *)timeframe, qty * sizeof(uint64_t));
    filled_size_ += qty;
  }

  return frame;
}


X3X2ListModeFrameStoreTimestamp::X3X2ListModeFrameStoreTimestamp(const std::string& name) :
  X3X2ListModeFrameStore(name, raw_64bit, sizeof(uint64_t))
{
  LOG4CXX_INFO(logger_, "[" << name_ << "]" << " Created X3X2ListModeFrameStoreTimestamp");
}

X3X2ListModeFrameStoreTimestamp::~X3X2ListModeFrameStoreTimestamp()
{
}

boost::shared_ptr <Frame> X3X2ListModeFrameStoreTimestamp::add_timestamp(uint64_t *timestamp, uint32_t qty)
{
  boost::shared_ptr <Frame> frame;

  uint32_t block_size = qty;
  char *dest = (char *)ptr_;
  dest += (filled_size_ * sizeof(uint64_t));

  // Check if the block is bigger than the space allocated
  if ((filled_size_ + qty) >= max_size_){
    // Calculate the size to copy
    block_size = max_size_ - filled_size_;

    // Make the copy
    memcpy(dest, (char *)timestamp, block_size * sizeof(uint64_t));

    // Create the frame
    frame = this->to_frame();

    // Now copy the remaining block
    char *src = (char *)timestamp;
    src += (block_size * sizeof(uint64_t));
    block_size = qty - block_size;
    memcpy(ptr_, src, block_size * sizeof(uint64_t));
    filled_size_ = block_size;
  } else {
    // The block fits completely inside the memory so one simple copy
    memcpy(dest, (char *)timestamp, qty * sizeof(uint64_t));
    filled_size_ += qty;
  }

  return frame;
}



X3X2ListModeFrameStoreEventHeight::X3X2ListModeFrameStoreEventHeight(const std::string& name) :
    X3X2ListModeFrameStore(name, raw_16bit, sizeof(uint16_t))
{
  LOG4CXX_INFO(logger_, "[" << name_ << "]" << " Created X3X2ListModeFrameStoreEventHeight");
}

X3X2ListModeFrameStoreEventHeight::~X3X2ListModeFrameStoreEventHeight()
{
}

boost::shared_ptr <Frame> X3X2ListModeFrameStoreEventHeight::add_event_height(uint16_t *event_height, uint32_t qty)
{
  boost::shared_ptr <Frame> frame;

  uint32_t block_size = qty;
  char *dest = (char *)ptr_;
  dest += (filled_size_ * sizeof(uint16_t));

  // Check if the block is bigger than the space allocated
  if ((filled_size_ + qty) >= max_size_){
    // Calculate the size to copy
    block_size = max_size_ - filled_size_;

    // Make the copy
    memcpy(dest, (char *)event_height, block_size * sizeof(uint16_t));

    // Create the frame
    frame = this->to_frame();

    // Now copy the remaining block
    char *src = (char *)event_height;
    src += (block_size * sizeof(uint16_t));
    block_size = qty - block_size;
    memcpy(ptr_, src, block_size * sizeof(uint16_t));
    filled_size_ = block_size;
  } else {
    // The block fits completely inside the memory so one simple copy
    memcpy(dest, (char *)event_height, qty * sizeof(uint16_t));
    filled_size_ += qty;
  }

  return frame;
}


X3X2ListModeFrameStoreResetFlag::X3X2ListModeFrameStoreResetFlag(const std::string& name) :
    X3X2ListModeFrameStore(name, raw_8bit, sizeof(uint8_t))
{
  LOG4CXX_INFO(logger_, "[" << name_ << "]" << " Created X3X2ListModeFrameStoreResetFlag");
}

X3X2ListModeFrameStoreResetFlag::~X3X2ListModeFrameStoreResetFlag()
{
}

boost::shared_ptr <Frame> X3X2ListModeFrameStoreResetFlag::add_reset_flag(uint8_t *reset_flag, uint32_t qty)
{
  boost::shared_ptr <Frame> frame;

  uint32_t block_size = qty;
  char *dest = (char *)ptr_;
  dest += (filled_size_ * sizeof(uint8_t));

  // Check if the block is bigger than the space allocated
  if ((filled_size_ + qty) >= max_size_){
    // Calculate the size to copy
    block_size = max_size_ - filled_size_;

    // Make the copy
    memcpy(dest, (char *)reset_flag, block_size * sizeof(uint8_t));

    // Create the frame
    frame = this->to_frame();

    // Now copy the remaining block
    char *src = (char *)reset_flag;
    src += (block_size * sizeof(uint8_t));
    block_size = qty - block_size;
    memcpy(ptr_, src, block_size * sizeof(uint8_t));
    filled_size_ = block_size;
  } else {
    // The block fits completely inside the memory so one simple copy
    memcpy(dest, (char *)reset_flag, qty * sizeof(uint8_t));
    filled_size_ += qty;
  }

  return frame;
}


}
