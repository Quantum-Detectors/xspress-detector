#include "DebugLevelLogger.h"

#include "X3X2ListModeProcessJob.h"
#include "X3X2Definitions.h"


namespace FrameProcessor {

X3X2ListModeProcessJob::X3X2ListModeProcessJob() : 
  channel_(0),
  event_qty_(0)
{
  // Setup logging for the class
  logger_ = Logger::getLogger("FP.X3X2ListModeProcessJob");

  timeframe_ = (uint64_t *)malloc(X3X2_MINI_FIELDS_PER_FRAME * sizeof(uint64_t));
  timestamp_ = (uint64_t *)malloc(X3X2_MINI_FIELDS_PER_FRAME * sizeof(uint64_t));
  event_height_ = (uint16_t *)malloc(X3X2_MINI_FIELDS_PER_FRAME * sizeof(uint64_t));
  reset_flag_ = (uint8_t *)malloc(X3X2_MINI_FIELDS_PER_FRAME * sizeof(uint64_t));
  tf_ptr_ = timeframe_;
  ts_ptr_ = timestamp_;
  eh_ptr_ = event_height_;
  rf_ptr_ = reset_flag_;
}

X3X2ListModeProcessJob::~X3X2ListModeProcessJob()
{
  LOG4CXX_TRACE(logger_, "X3X2ListModeProcessJob destructor.");
}

void X3X2ListModeProcessJob::init(uint32_t index, uint16_t *frame_data)
{
  index_ = index;
  data_ = frame_data;
  tf_ptr_ = timeframe_;
  ts_ptr_ = timestamp_;
  eh_ptr_ = event_height_;
  rf_ptr_ = reset_flag_;
  channel_ = -1;
  event_qty_ = 0;
}

uint32_t X3X2ListModeProcessJob::get_index() 
{
  return index_;
}

uint16_t X3X2ListModeProcessJob::get_channel() 
{
  return channel_;
}

uint16_t *X3X2ListModeProcessJob::get_data_ptr()
{
  return data_;
}

uint32_t X3X2ListModeProcessJob::get_event_qty()
{
  return event_qty_;
}

uint64_t *X3X2ListModeProcessJob::get_tf_ptr()
{
  return timeframe_;
}

uint64_t *X3X2ListModeProcessJob::get_ts_ptr()
{
  return timestamp_;
}

uint16_t *X3X2ListModeProcessJob::get_eh_ptr()
{
  return event_height_;
}

uint8_t *X3X2ListModeProcessJob::get_rf_ptr()
{
  return reset_flag_;
}

void X3X2ListModeProcessJob::process()
{
  LOG4CXX_TRACE(logger_, "Processing job index " << this->get_index());

  // Packet arrived after we have completed the acquisition - either by
  // receiving the EOF for the desired TF on all channels or it was manually
  // stopped using the Odin Data API
  uint16_t* frame_data = this->get_data_ptr();

  // Event attributes
  uint16_t acquisition_number = 0;
  uint64_t time_frame = 0;
  uint64_t time_stamp = 0;
  uint64_t prev_time_frame = 0;
  uint64_t prev_time_stamp = 0;

  uint16_t event_height = 0;

  bool dummy_event = 0;
  bool end_of_frame = 0;
  bool ttl_a = 0;
  bool ttl_b = 0;

  bool reset_flag = false;

//  uint16_t channel = -1;

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
        time_frame = (time_frame & 0xFFFFFFFFFFFFFF00) | ((value_64 & 0xFF0) >> 4);
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
        // Record this raw channel number:
        channel_ = (value >> 8);
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

        // Check for time frame and time stamp decreasing
        // this may be a sign that the receiver buffer
        // is being overwritten before being processed
        if (time_frame < prev_time_frame)
        {
          LOG4CXX_INFO(
            logger_,
            "Raw channel "
            << channel_
            << " stepped back from "
            << prev_time_frame
            << " to "
            << time_frame
            << " at field " << field
          );
        }
        if (time_stamp < prev_time_stamp)
        {
          LOG4CXX_INFO(
            logger_,
            "Raw channel "
            << channel_
            << " walked back timestamp at field "
            << field
            << " from "
            << prev_time_stamp
            << " to "
            << time_stamp
          );
        }
        prev_time_frame = time_frame;
        prev_time_stamp = time_stamp;

        // xspress3m_active_readout only counts events when not end of frame so we copy this logic here
        if (!end_of_frame)
        {
          if (!dummy_event) {
            *tf_ptr_ = time_frame;
            *ts_ptr_ = time_stamp;
            *eh_ptr_ = event_height;
            reset_flag = (id == 14) ? true : false;
            *rf_ptr_ = reset_flag;
            tf_ptr_++;
            ts_ptr_++;
            eh_ptr_++;
            rf_ptr_++;
            event_qty_++;
          }
        }
//        else if (!acquisition_complete_)
//        {
          // TODO: work out why we get more events after the end of frame marker is set and see if we need to
          // save them or ignore them (we ignore them here)
/*          if (time_frame + 1 == num_time_frames_)
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
              LOG4CXX_INFO(logger_, "Acquisition of " << num_time_frames_ << " frames completed for all channels");
              return;
            }
          }*/
//        }
        break;
    }
  }
}

}