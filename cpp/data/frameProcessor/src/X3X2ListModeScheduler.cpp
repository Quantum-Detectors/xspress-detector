#include "DebugLevelLogger.h"

#include "X3X2ListModeScheduler.h"
#include "X3X2Definitions.h"
#include <chrono>
#include <thread>


namespace FrameProcessor {

X3X2ListModeScheduler::X3X2ListModeScheduler()
{
  // Setup logging for the class
  logger_ = Logger::getLogger("FP.X3X2ListModeScheduler");
  LOG4CXX_INFO(logger_, "LATRDProcessCoordinator constructor.");

  // Create the work queue for processing jobs
  job_queue_ = boost::shared_ptr<WorkQueue<boost::shared_ptr<X3X2ListModeProcessJob> > >(new WorkQueue<boost::shared_ptr<X3X2ListModeProcessJob> >);
  // Create the work queue for completed jobs
  res_queue_ = boost::shared_ptr<WorkQueue<boost::shared_ptr<X3X2ListModeProcessJob> > >(new WorkQueue<boost::shared_ptr<X3X2ListModeProcessJob> >);

  // Configure threads for processing
  // Now start the worker thread to monitor the queue
  for (size_t index = 0; index < PROCESS_THREADS; index++){
    thread_[index] = new boost::thread(&X3X2ListModeScheduler::process_task, this);
  }
}

X3X2ListModeScheduler::~X3X2ListModeScheduler()
{
  LOG4CXX_TRACE(logger_, "X3X2ListModeScheduler destructor.");
}

void X3X2ListModeScheduler::set_number_of_time_frames(uint32_t time_frames)
{
  num_time_frames_ = time_frames;
}

void X3X2ListModeScheduler::set_channels(std::vector<uint32_t> channels)
{
  std::stringstream ss;
  ss << "Registering channels [";
  for (auto iter = channels.begin(); iter != channels.end(); iter++){
    ss << " " << *iter;
  }
  ss << "]";
  LOG4CXX_INFO(logger_, ss.str());
  channels_ = channels;
  channel_offset_ = channels[0];
  LOG4CXX_INFO(logger_, "Setting channels offset to [" << channel_offset_ << "].");

  this->reset_time_stores();
}

void X3X2ListModeScheduler::setup_frame_stores(uint32_t channel, const std::string& prefix, uint32_t frame_event_qty)
{
  LOG4CXX_INFO(logger_, "Setting up frame store channel [" << channel << "] with prefix '" << prefix << "' for [" << frame_event_qty << "] events.");
  std::string timeframe_name = prefix + std::to_string(channel) + "_time_frame";
  std::string timestamp_name = prefix + std::to_string(channel) + "_time_stamp";
  std::string event_height_name = prefix + std::to_string(channel) + "_event_height";
  std::string reset_flag_name = prefix + std::to_string(channel) + "_reset_flag";

  // Size of each memory block in bytes based on the number of events we want to
  // store in each frame
  uint32_t frame_size_tf_bytes = frame_event_qty * sizeof(uint64_t);
  uint32_t frame_size_ts_bytes = frame_event_qty * sizeof(uint64_t);
  uint32_t frame_size_eh_bytes = frame_event_qty * sizeof(uint16_t);
  uint32_t frame_size_rf_bytes = frame_event_qty * sizeof(uint8_t);

  boost::shared_ptr<X3X2ListModeFrameStoreTimeframe> tf_store = 
    boost::shared_ptr<X3X2ListModeFrameStoreTimeframe>(new X3X2ListModeFrameStoreTimeframe(timeframe_name));
  tf_store->set_size(frame_size_tf_bytes);
  tf_store_ptrs_[channel] = tf_store;

  boost::shared_ptr<X3X2ListModeFrameStoreTimestamp> ts_store = 
    boost::shared_ptr<X3X2ListModeFrameStoreTimestamp>(new X3X2ListModeFrameStoreTimestamp(timestamp_name));
  ts_store->set_size(frame_size_ts_bytes);
  ts_store_ptrs_[channel] = ts_store;

  boost::shared_ptr<X3X2ListModeFrameStoreEventHeight> eh_store = 
    boost::shared_ptr<X3X2ListModeFrameStoreEventHeight>(new X3X2ListModeFrameStoreEventHeight(event_height_name));
  eh_store->set_size(frame_size_eh_bytes);
  eh_store_ptrs_[channel] = eh_store;

  boost::shared_ptr<X3X2ListModeFrameStoreResetFlag> rf_store = 
    boost::shared_ptr<X3X2ListModeFrameStoreResetFlag>(new X3X2ListModeFrameStoreResetFlag(reset_flag_name));
  rf_store->set_size(frame_size_rf_bytes);
  rf_store_ptrs_[channel] = rf_store;
}

void X3X2ListModeScheduler::reset_time_stores()
{
  last_timeframe_store_.clear();
  last_timestamp_store_.clear();
  completed_channels_.clear();
  for (auto iter = channels_.begin(); iter != channels_.end(); iter++){
    last_timeframe_store_[*iter] = 0;
    last_timestamp_store_[*iter] = 0;
    completed_channels_[*iter] = false;
  }
}

boost::shared_ptr<X3X2ListModeProcessJob> X3X2ListModeScheduler::get_job()
{
  boost::shared_ptr<X3X2ListModeProcessJob> job;
  // Check if we have a job available
  LOG4CXX_DEBUG_LEVEL(2, logger_, "job_stack_ size [" << job_stack_.size() << "]");
  if (job_stack_.size() > 0){
    job = job_stack_.top();
    job_stack_.pop();
  } else{
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Creating new job instance");
    // No job available so create a new one
    job = boost::shared_ptr<X3X2ListModeProcessJob>(new X3X2ListModeProcessJob());
  }
  return job;
}

void X3X2ListModeScheduler::release_job(boost::shared_ptr<X3X2ListModeProcessJob> job)
{
  // Place the job back on the stack ready for re-use
  job_stack_.push(job);
}

std::vector<boost::shared_ptr<Frame> > X3X2ListModeScheduler::process_frame(boost::shared_ptr<Frame> frame)
{
  LOG4CXX_TRACE(logger_, "Scheduler process_frame called...");
  // Deconstruct the frame into pointers to each packet
  char *raw_data = static_cast<char *>(frame->get_data_ptr());
  X3X2::X3X2ListFrameHeader* frame_header = reinterpret_cast<X3X2::X3X2ListFrameHeader*>(raw_data);
  raw_data += sizeof(X3X2::X3X2ListFrameHeader);
  uint16_t* frame_data = reinterpret_cast<uint16_t *>(raw_data);

  uint16_t packets_received = frame_header->packets_received;

  std::vector<boost::shared_ptr<Frame> > complete_frames;

  LOG4CXX_DEBUG_LEVEL(2, logger_, "Frame processing with " << packets_received << " packets");
  // Wrap a Job around each of the packets
  // Add the job to the jobQueue for processing
  for (uint32_t index = 0; index < packets_received; index++){
    boost::shared_ptr<X3X2ListModeProcessJob> job = this->get_job();
    job->init(index, frame_data);
    job_queue_->add(job);
    frame_data += X3X2_MINI_FIELDS_PER_FRAME;
  }
  LOG4CXX_TRACE(logger_, "Jobs added to job queue");
  
  std::vector<boost::shared_ptr <X3X2ListModeProcessJob> > completed_jobs(packets_received);

  // Wait for all of the results to return (checking the resQueue)
  uint32_t processed_jobs = 0;
  while (processed_jobs < packets_received){
    // Place each result in order into the vector
    boost::shared_ptr<X3X2ListModeProcessJob> job = res_queue_->remove();
    completed_jobs[job->get_index()] = job;
    processed_jobs++;
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Scheduled job [" << job->get_index() << "] has completed, total of " << processed_jobs << " are now complete");
  }

  // Once we reach here we have a vector of completed packets processed in order
  // We can now add the results into the storage blocks
  boost::shared_ptr<Frame> reply_frame;
  for (auto iter = completed_jobs.begin(); iter != completed_jobs.end(); iter++){
    uint16_t channel = (*iter)->get_channel() + channel_offset_;
    // Check we have the right channel (and ignore marker channels for now)
    if (std::find(channels_.begin(), channels_.end(), channel) == channels_.end()) {
      // Ignore entire packet
      continue;
    }

    // Check the first timeframe and timestamp of this job against the last saved timeframe and timestamp
    if ((*iter)->get_first_timeframe() < last_timeframe_store_[channel])
    {
      LOG4CXX_INFO(
        logger_,
        "Channel "
        << channel
        << " stepped back from "
        << last_timeframe_store_[channel]
        << " to "
        << (*iter)->get_first_timeframe()
        //<< " at field " << field
      );
    }
    if ((*iter)->get_first_timestamp() < last_timestamp_store_[channel])
    {
      LOG4CXX_INFO(
        logger_,
        "Channel "
        << channel
        << " walked back timestamp"
        //<< field
        << " from "
        << last_timestamp_store_[channel]
        << " to "
        << (*iter)->get_first_timestamp()
      );
    }
    // Now update the stored last time values
    last_timeframe_store_[channel] = (*iter)->get_last_timeframe();
    last_timestamp_store_[channel] = (*iter)->get_last_timestamp();

    if (tf_store_ptrs_.count(channel)){
      reply_frame = tf_store_ptrs_[channel]->add_timeframe((*iter)->get_tf_ptr(), (*iter)->get_event_qty());
      if (reply_frame){
        complete_frames.push_back(reply_frame);
      }
    } else {
      LOG4CXX_ERROR(logger_, "Incorrect channel detected [" << channel << "] for time frame store.");
    }
    if (ts_store_ptrs_.count(channel)){
      reply_frame = ts_store_ptrs_[channel]->add_timestamp((*iter)->get_ts_ptr(), (*iter)->get_event_qty());
      if (reply_frame){
        complete_frames.push_back(reply_frame);
      }
    } else {
      LOG4CXX_ERROR(logger_, "Incorrect channel detected [" << channel << "] for timestamp store.");
    }
    if (eh_store_ptrs_.count(channel)){
      reply_frame = eh_store_ptrs_[channel]->add_event_height((*iter)->get_eh_ptr(), (*iter)->get_event_qty());
      if (reply_frame){
        complete_frames.push_back(reply_frame);
      }
    } else {
      LOG4CXX_ERROR(logger_, "Incorrect channel detected [" << channel << "] for event height store.");
    }
    if (rf_store_ptrs_.count(channel)){
      reply_frame = rf_store_ptrs_[channel]->add_reset_flag((*iter)->get_rf_ptr(), (*iter)->get_event_qty());
      if (reply_frame){
        complete_frames.push_back(reply_frame);
      }
    } else {
      LOG4CXX_ERROR(logger_, "Incorrect channel detected [" << channel << "] for reset flag store.");
    }

    // Now check for an end of frame marker
    if ((*iter)->get_eof_marker()){
      if ((*iter)->get_last_timeframe() + 1 >= num_time_frames_){
        LOG4CXX_INFO(logger_, "Acquisition of " << num_time_frames_ << " frames complete for channel " << channel);
        completed_channels_[channel] = true;

        // Check if every channel is now finished
        uint16_t completed_channels = 0;
        for (auto const& it : completed_channels_){
          if (it.second) completed_channels++;
        }
        if (completed_channels == channels_.size()){
          LOG4CXX_INFO(logger_, "FLUSHING !!!!  Acquisition of " << num_time_frames_ << " frames completed for all channels");
          std::vector<boost::shared_ptr<Frame> > flushed_frames = this->flush();
          complete_frames.insert(complete_frames.end(), flushed_frames.begin(), flushed_frames.end());
          this->reset_acquisition();
        }
      }
    }

    this->release_job(*iter);
  }
  return complete_frames;
}

void X3X2ListModeScheduler::process_task()
{
  LOG4CXX_INFO(logger_, "Starting processing task with ID [" << boost::this_thread::get_id() << "]");
  bool executing = true;
  while (executing){
    // Wait for jobs to arrive on the queue
    boost::shared_ptr<X3X2ListModeProcessJob> job = job_queue_->remove();

    // Perform the invdividual job processing
    job->process();

    // Add the results onto the results queue
    res_queue_->add(job, true);
  }
}

std::vector<boost::shared_ptr<Frame> > X3X2ListModeScheduler::flush()
{
  std::vector<boost::shared_ptr<Frame> > complete_frames;

  for (auto iter = tf_store_ptrs_.begin(); iter != tf_store_ptrs_.end(); ++iter){
    LOG4CXX_DEBUG_LEVEL(0, logger_, "Flushing timeframe for channel " << iter->first);
    boost::shared_ptr <Frame> frame = iter->second->to_frame();
    if (frame){
      complete_frames.push_back(frame);
    }
  }
  for (auto iter = ts_store_ptrs_.begin(); iter != ts_store_ptrs_.end(); ++iter){
    LOG4CXX_DEBUG_LEVEL(0, logger_, "Flushing timestamp for channel " << iter->first);
    boost::shared_ptr <Frame> frame = iter->second->to_frame();
    if (frame){
      complete_frames.push_back(frame);
    }
  }
  for (auto iter = eh_store_ptrs_.begin(); iter != eh_store_ptrs_.end(); ++iter){
    LOG4CXX_DEBUG_LEVEL(0, logger_, "Flushing event height for channel " << iter->first);
    boost::shared_ptr <Frame> frame = iter->second->to_frame();
    if (frame){
      complete_frames.push_back(frame);
    }
  }
  for (auto iter = rf_store_ptrs_.begin(); iter != rf_store_ptrs_.end(); ++iter){
    LOG4CXX_DEBUG_LEVEL(0, logger_, "Flushing reset flag for channel " << iter->first);
    boost::shared_ptr <Frame> frame = iter->second->to_frame();
    if (frame){
      complete_frames.push_back(frame);
    }
  }
  return complete_frames;
}

void X3X2ListModeScheduler::reset_acquisition()
{
  for (auto iter = tf_store_ptrs_.begin(); iter != tf_store_ptrs_.end(); ++iter){
    iter->second->reset_frame_count();
  }
  for (auto iter = ts_store_ptrs_.begin(); iter != ts_store_ptrs_.end(); ++iter){
    iter->second->reset_frame_count();
  }
  for (auto iter = eh_store_ptrs_.begin(); iter != eh_store_ptrs_.end(); ++iter){
    iter->second->reset_frame_count();
  }
  for (auto iter = rf_store_ptrs_.begin(); iter != rf_store_ptrs_.end(); ++iter){
    iter->second->reset_frame_count();
  }
  this->reset_time_stores();
}

}
