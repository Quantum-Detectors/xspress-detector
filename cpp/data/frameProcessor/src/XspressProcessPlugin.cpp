//
// Created by hir12111 on 03/11/18.
//
#include <iostream>
#include <string>
#include "DataBlockFrame.h"
#include "XspressProcessPlugin.h"
#include "FrameProcessorDefinitions.h"
#include "XspressDefinitions.h"
#include "DebugLevelLogger.h"

#define MAX_SCALAR_MEM_BLOCK_SIZE 4096
#define DEFAULT_SCALAR_QTY 9
#define SCALAR_POST_TIME_MS 1000

namespace FrameProcessor {

const std::string XspressProcessPlugin::CONFIG_ACQ_ID = "acq_id";

const std::string XspressProcessPlugin::CONFIG_PROCESS = "process";
const std::string XspressProcessPlugin::CONFIG_PROCESS_NUMBER = "number";
const std::string XspressProcessPlugin::CONFIG_PROCESS_RANK = "rank";

const std::string XspressProcessPlugin::CONFIG_LIVE_VIEW_NAME = "live_view";

const std::string XspressProcessPlugin::CONFIG_FRAMES = "frames";
const std::string XspressProcessPlugin::CONFIG_DTC_FLAGS = "dtc/flags";
const std::string XspressProcessPlugin::CONFIG_DTC_PARAMS = "dtc/params";

const std::string XspressProcessPlugin::CONFIG_CHUNK = "chunks";

const std::string XspressProcessPlugin::CONFIG_OFFSET = "offset";

const std::string META_NAME = "xspress";
const std::string META_XSPRESS_CHUNK = "xspress_meta_chunk";
const std::string META_XSPRESS_SCALARS = "xspress_scalars";
const std::string META_XSPRESS_DTC = "xspress_dtc";
const std::string META_XSPRESS_INP_EST = "xspress_inp_est";
const std::string META_XSPRESS_SUM = "xspress_sum";

XspressMemoryBlock::XspressMemoryBlock() :
  ptr_(0),
  num_bytes_(0),
  filled_size_(0),
  frames_(0),
  max_frames_(0),
  frame_size_(0)
{
  // Setup logging for the class
  logger_ = Logger::getLogger("FP.XspressProcessPlugin");
  LOG4CXX_INFO(logger_, "Created XspressMemoryBlock");
}

XspressMemoryBlock::~XspressMemoryBlock()
{
  if (ptr_){
    free(ptr_);
  }
}

void XspressMemoryBlock::set_size(uint32_t frame_size, uint32_t max_frames)
{
  frame_size_ = frame_size;
  max_frames_ = max_frames;
  num_bytes_ = frame_size * max_frames;
  reallocate();
}

void XspressMemoryBlock::reallocate()
{
  LOG4CXX_INFO(logger_, "Reallocating XspressMemoryBlock to [" << num_bytes_ << "] bytes");
  if (ptr_){
    free(ptr_);
  }
  ptr_ = (char *)malloc(num_bytes_);
  reset();
}

void XspressMemoryBlock::reset()
{
  memset(ptr_, 0, num_bytes_);
  frames_ = 0;
  filled_size_ = 0;
}

void XspressMemoryBlock::add_frame(uint32_t frame_id, char *ptr)
{
//  LOG4CXX_INFO(logger_, "Adding frame [" << frame_id << "] to XspressMemoryBlock");
  // Work out the pointer offset
  uint32_t frame_offset = frame_id % max_frames_;
  char *dest = ptr_;
  dest += (frame_offset * frame_size_);
  memcpy(dest, ptr, frame_size_);
  frames_ += 1;
  filled_size_ = (frame_offset+1) * frame_size_;
//  LOG4CXX_INFO(logger_, "Frames [" << frames_ << " / " << max_frames_ << "]");
}

bool XspressMemoryBlock::check_full()
{
  bool full = false;
//  LOG4CXX_INFO(logger_, "Check full [" << frames_ << " / " << max_frames_ << "]");
  if (frames_ == max_frames_){
    full = true;
  }
  return full;
}

uint32_t XspressMemoryBlock::frames()
{
  return frames_;
}

uint32_t XspressMemoryBlock::size()
{
  return num_bytes_;
}

uint32_t XspressMemoryBlock::current_byte_size()
{
  return filled_size_;
}

char *XspressMemoryBlock::get_data_ptr()
{
  return ptr_;
}

XspressProcessPlugin::XspressProcessPlugin() :
  num_frames_(1),
  num_energy_bins_(4096),
  num_aux_(0),
  num_channels_(0),
  frames_per_block_(1),
  current_block_start_(0),
  concurrent_processes_(1),
  concurrent_rank_(0),
  acq_id_(""),
  live_view_name_(""),
  last_scalar_send_time_(boost::posix_time::min_date_time),
  scalar_memblock_(0),
  dtc_memblock_(0),
  inp_est_memblock_(0),
  sum_memblock_(0),
  num_scalars_recorded_(0),
  offset(0)
{
  // Setup logging for the class
  logger_ = Logger::getLogger("FP.XspressProcessPlugin");
  LOG4CXX_INFO(logger_, "XspressProcessPlugin version " << this->get_version_long() << " loaded");
}

XspressProcessPlugin::~XspressProcessPlugin()
{
  LOG4CXX_TRACE(logger_, "XspressProcessPlugin destructor.");
}

void XspressProcessPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  if (config.has_param(XspressProcessPlugin::CONFIG_FRAMES)){
    num_frames_ = config.get_param<unsigned int>(XspressProcessPlugin::CONFIG_FRAMES);
    LOG4CXX_INFO(logger_, "Number of frames has been set to  " << num_frames_);
  }

  // Check to see if we are configuring the process number and rank
  if (config.has_param(XspressProcessPlugin::CONFIG_PROCESS)) {
    OdinData::IpcMessage processConfig(config.get_param<const rapidjson::Value&>(XspressProcessPlugin::CONFIG_PROCESS));
    this->configureProcess(processConfig, reply);
  }

  // Check for the acquisition ID
  if (config.has_param(XspressProcessPlugin::CONFIG_ACQ_ID)) {
    this->acq_id_ = config.get_param<std::string>(XspressProcessPlugin::CONFIG_ACQ_ID);
    LOG4CXX_INFO(logger_, "Acquisition ID set to " << this->acq_id_);
  }

  /**
   *  If we receive the configuration with a chunk size different from the previous value we have to relocate the memory buffer
   */
  if (config.has_param(XspressProcessPlugin::CONFIG_CHUNK)){
    if (this->frames_per_block_ != config.get_param<uint32_t>(XspressProcessPlugin::CONFIG_CHUNK))
    {
      if (config.get_param<uint32_t>(XspressProcessPlugin::CONFIG_CHUNK) > MAX_SCALAR_MEM_BLOCK_SIZE)
      {
        this->frames_per_block_ = 4096;
        LOG4CXX_WARN(logger_, "Batch size configured to a value grater than the maximum allowed. Forcing it to " << MAX_SCALAR_MEM_BLOCK_SIZE);
      }
      else
      {
        this->frames_per_block_ = config.get_param<uint32_t>(XspressProcessPlugin::CONFIG_CHUNK);
      }
      LOG4CXX_INFO(logger_, "Number of frames per block changed from the previous iteration.");
      setup_memory_allocation();
    }

    rapidjson::Document meta_document;
    meta_document.SetObject();
    // Add Acquisition ID
    rapidjson::Value key_acq_id("acqID", meta_document.GetAllocator());
    rapidjson::Value value_acq_id;
    value_acq_id.SetString(this->acq_id_.c_str(), this->acq_id_.size(), meta_document.GetAllocator());
    meta_document.AddMember(key_acq_id, value_acq_id, meta_document.GetAllocator());
    // Add rank
    rapidjson::Value key_rank("rank", meta_document.GetAllocator());
    rapidjson::Value value_rank;
    value_rank.SetInt(this->concurrent_rank_);
    meta_document.AddMember(key_rank, value_rank, meta_document.GetAllocator());
    // Add frame_id
    rapidjson::Value key_frame_id("frame_id", meta_document.GetAllocator());
    rapidjson::Value value_frame_id;
    value_frame_id.SetInt(-1);
    meta_document.AddMember(key_frame_id, value_frame_id, meta_document.GetAllocator());
    // Add num_frames_
    rapidjson::Value key_num_frames("num_frames_", meta_document.GetAllocator());
    rapidjson::Value value_num_frames;
    value_num_frames.SetInt(num_frames_);
    meta_document.AddMember(key_num_frames, value_num_frames, meta_document.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    meta_document.Accept(writer);

    this->publish_meta(META_NAME,
                        META_XSPRESS_CHUNK,
                        boost::to_string(this->frames_per_block_),
                        buffer.GetString());

    LOG4CXX_INFO(logger_, "Number of frames per block set to " << this->frames_per_block_);
  }

  if(config.has_param(XspressProcessPlugin::CONFIG_OFFSET))
  {
    this->offset = config.get_param<uint32_t>(XspressProcessPlugin::CONFIG_OFFSET);
    LOG4CXX_INFO(logger_,"configured offset: " << offset);
  }

  // Check for the live view plugin name
  if (config.has_param(XspressProcessPlugin::CONFIG_LIVE_VIEW_NAME)) {
    this->live_view_name_ = config.get_param<std::string>(XspressProcessPlugin::CONFIG_LIVE_VIEW_NAME);
    LOG4CXX_INFO(logger_, "Live View destination name set to " << this->live_view_name_);
  }
}

void XspressProcessPlugin::requestConfiguration(OdinData::IpcMessage& reply)
{
  // Return the configuration of the LATRD process plugin
  reply.set_param(get_name() + "/" + XspressProcessPlugin::CONFIG_FRAMES, this->num_frames_);
  reply.set_param(get_name() + "/" + XspressProcessPlugin::CONFIG_CHUNK, this->frames_per_block_);
  reply.set_param(get_name() + "/" + XspressProcessPlugin::CONFIG_PROCESS + "/" +
                  XspressProcessPlugin::CONFIG_PROCESS_NUMBER, this->concurrent_processes_);
  reply.set_param(get_name() + "/" + XspressProcessPlugin::CONFIG_PROCESS + "/" +
                  XspressProcessPlugin::CONFIG_PROCESS_RANK, this->concurrent_rank_);
  reply.set_param(get_name() + "/" + XspressProcessPlugin::CONFIG_ACQ_ID, this->acq_id_);
  reply.set_param(get_name() + "/" + XspressProcessPlugin::CONFIG_LIVE_VIEW_NAME, this->live_view_name_);
  reply.set_param(get_name() + "/" + XspressProcessPlugin::CONFIG_OFFSET, this->offset);
}

/**
 * Set configuration options for the Xspress process count.
 *
 * This sets up the process plugin according to the configuration IpcMessage
 * objects that are received. The options are searched for:
 * CONFIG_PROCESS_NUMBER - Sets the number of writer processes executing
 * CONFIG_PROCESS_RANK - Sets the rank of this process
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void XspressProcessPlugin::configureProcess(OdinData::IpcMessage &config, OdinData::IpcMessage &reply)
{
  // Check for process number and rank number
  if (config.has_param(XspressProcessPlugin::CONFIG_PROCESS_NUMBER)) {
    this->concurrent_processes_ = config.get_param<size_t>(XspressProcessPlugin::CONFIG_PROCESS_NUMBER);
    LOG4CXX_INFO(logger_, "Concurrent processes changed to " << this->concurrent_processes_);
  }
  if (config.has_param(XspressProcessPlugin::CONFIG_PROCESS_RANK)) {
    this->concurrent_rank_ = config.get_param<size_t>(XspressProcessPlugin::CONFIG_PROCESS_RANK);
    LOG4CXX_INFO(logger_, "Process rank changed to " << this->concurrent_rank_);
  }
}

// Version functions
int XspressProcessPlugin::get_version_major()
{
  return XSPRESS_DETECTOR_VERSION_MAJOR;
}

int XspressProcessPlugin::get_version_minor()
{
  return XSPRESS_DETECTOR_VERSION_MINOR;
}

int XspressProcessPlugin::get_version_patch()
{
  return XSPRESS_DETECTOR_VERSION_PATCH;
}

std::string XspressProcessPlugin::get_version_short()
{
  return XSPRESS_DETECTOR_VERSION_STR_SHORT;
}

std::string XspressProcessPlugin::get_version_long()
{
  return XSPRESS_DETECTOR_VERSION_STR;
}

void XspressProcessPlugin::set_number_of_channels(uint32_t num_channels)
{
  num_channels_ = num_channels;
  // We must reallocate memory blocks
  setup_memory_allocation();
}

void XspressProcessPlugin::set_number_of_aux(uint32_t num_aux)
{
  num_aux_ = num_aux;
  // We must reallocate memory blocks
  setup_memory_allocation();
}

void XspressProcessPlugin::setup_memory_allocation()
{
  // First clear out the memory vector emptying any blocks
  memory_ptrs_.clear();

  // Allocate large enough blocks of memory to hold frames_per_block spectra
  // Allocate one block of memory for each channel
  uint32_t frame_size = num_energy_bins_ * num_aux_ * sizeof(uint32_t);
  LOG4CXX_DEBUG_LEVEL(3, logger_, "frames_per_block_ inside the setup_memory_allocation method: " << frames_per_block_);
  for (int index = 0; index <= num_channels_; index++){
    boost::shared_ptr<XspressMemoryBlock> ptr = boost::shared_ptr<XspressMemoryBlock>(new XspressMemoryBlock());
    ptr->set_size(frame_size, frames_per_block_);
    memory_ptrs_.push_back(ptr);
  }

  // Init the scalar memory allocation
  if (scalar_memblock_){
    free(scalar_memblock_);
  }
  scalar_memblock_ = malloc(sizeof(uint32_t) * this->frames_per_block_ * num_channels_ * DEFAULT_SCALAR_QTY);
  if (dtc_memblock_){
    free(dtc_memblock_);
  }
  dtc_memblock_ = malloc(sizeof(double) * this->frames_per_block_ * num_channels_);
  if (inp_est_memblock_){
    free(inp_est_memblock_);
  }
  inp_est_memblock_ = malloc(sizeof(double) * this->frames_per_block_ * num_channels_);

  if (sum_memblock_)
  {
    free(sum_memblock_);
  }
  sum_memblock_ = malloc(sizeof(double) * this->frames_per_block_);
}


/**
 * Calculate the sum for a frame
 *
 * This method will receive a frame, and sum each element of data on it.
 * The data is stored in the `sum_memblock_` array (allocated in the `setup_memory_allocation method`)
 * and each time this method is called it stores the result to this array at the offset `num_scalars_recorded_`, i.e.: when
 * `num_scalars_recorded_` is 0 we store the sum to index 0, when `num_scalars_recorded_` is 1 we store the sum to index 1.
 * The 'num_scalars_recorded_' variable is set back to zero each time the frame arrays are flushed.
 *  
 * \param[in] frame - shared_ptr for a Frame class that contains the data for the acquisition on each configured channel.
 */
void XspressProcessPlugin::calculate_sum(boost::shared_ptr<Frame> frame)
{
  double sum_value = 0;
  const uint32_t *data = static_cast<const uint32_t *>(frame->get_image_ptr());
  size_t elements_count = frame->get_image_size() / sizeof(uint32_t);
  for (size_t pixel_index = 0; pixel_index < elements_count; pixel_index++)
  {
    sum_value += data[pixel_index];
  }
  double *sum_ptr = (double *)sum_memblock_;
  sum_ptr += num_scalars_recorded_;
  memcpy(sum_ptr, &sum_value, sizeof(double));
}

void XspressProcessPlugin::process_frame(boost::shared_ptr <Frame> frame)
{
  char* frame_bytes = static_cast<char *>(frame->get_data_ptr());
  FrameHeader *header = reinterpret_cast<FrameHeader *>(frame_bytes);

  // Check the frame number
  uint32_t frame_id = header->frame_number;

  if (frame_id == 0){
    LOG4CXX_INFO(logger_, "First frame received");
    LOG4CXX_INFO(logger_, "  First channel index: " << header->first_channel);
    LOG4CXX_INFO(logger_, "  Number of channels: " << header->num_channels);
    LOG4CXX_INFO(logger_, "  Number of scalars: " << header->num_scalars);
    LOG4CXX_INFO(logger_, "  Number of resgrades: " << header->num_aux);

    // Reset all memory blocks holding mca data
    for (int index = 0; index < num_channels_; index++){
      memory_ptrs_[index]->reset();
    }

    // Reset the number of scalars recorded to zero
    num_scalars_recorded_ = 0;
  }

  // Check the number of channels.  If the number of channels is different
  // to our previously stored number we must reallocate the memory block
  if (header->num_channels != num_channels_){
    set_number_of_channels(header->num_channels);
  }

  // Check the number of aux values.  If the number of is different
  // to our previously stored number we must reallocate the memory block
  if (header->num_aux != num_aux_){
    set_number_of_aux(header->num_aux);
  }

  // If the frame number is greater than the current memory allocation clear out the memory
  // and update the starting block

  // Split out the frame data into channels and update the memory blocks.
  // If any memory blocks are full then create the frame object and push (this should happen
  // to all of the memory blocks at once).
  uint32_t mca_size = header->num_energy_bins * header->num_aux * sizeof(uint32_t);
  uint32_t num_scalar_values = header->num_scalars * header->num_channels;
  uint32_t num_dtc_factors = header->num_channels;
  uint32_t num_inp_est = header->num_channels;
  uint32_t first_channel_index = header->first_channel;

  char *raw_sca_ptr = frame_bytes;
  raw_sca_ptr += sizeof(FrameHeader);
  uint32_t *sca_ptr = (uint32_t *)raw_sca_ptr;
  char *raw_dtc_ptr = frame_bytes;
  raw_dtc_ptr += (sizeof(FrameHeader) + (num_scalar_values*sizeof(uint32_t)));
  double *dtc_ptr = (double *)raw_dtc_ptr;
  char *raw_inp_est_ptr = frame_bytes;
  raw_inp_est_ptr += (sizeof(FrameHeader) + (num_scalar_values*sizeof(uint32_t)) + (num_dtc_factors*sizeof(double)));
  double *inp_est_ptr = (double *)raw_inp_est_ptr;

  // Memcpy the scalars into the correct memory location
  uint32_t *dest_ptr = (uint32_t *)scalar_memblock_;
  dest_ptr += (num_scalar_values * num_scalars_recorded_);
  memcpy(dest_ptr, sca_ptr, num_scalar_values * sizeof(uint32_t));
  double *dest_dtc_ptr = (double *)dtc_memblock_;
  dest_dtc_ptr += (num_dtc_factors * num_scalars_recorded_);
  memcpy(dest_dtc_ptr, dtc_ptr, num_dtc_factors * sizeof(double));
  double *dest_inp_est_ptr = (double *)inp_est_memblock_;
  dest_inp_est_ptr += (num_inp_est * num_scalars_recorded_);
  memcpy(dest_inp_est_ptr, inp_est_ptr, num_inp_est * sizeof(double));

  char *mca_ptr = frame_bytes;
  mca_ptr += (sizeof(FrameHeader) +
             (num_scalar_values*sizeof(uint32_t)) +
             (num_dtc_factors*sizeof(double)) +
             (num_inp_est*sizeof(double))
             );

  // Create the live view frame and push it
  dimensions_t live_dims;
  live_dims.push_back(num_channels_);
  live_dims.push_back(header->num_aux);
  live_dims.push_back(header->num_energy_bins);
  FrameMetaData live_metadata(frame_id, "live", raw_32bit, "", live_dims);
  boost::shared_ptr<Frame> live_frame(new DataBlockFrame(live_metadata, (mca_size*num_channels_)));
  memcpy(live_frame->get_data_ptr(), mca_ptr, (mca_size*num_channels_));
  // Set the chunking size to dimension of 1
  live_frame->set_outer_chunk_size(1);
  // Push out the live MCA data to the live view plugin only
  this->push(live_view_name_, live_frame);
  calculate_sum(live_frame);

  num_scalars_recorded_ += 1;

  // Send scalars to be writen once we reach the desired number of frames.
  // Number has to be the same as the number of MCA frames for live processing reasons
  if ((num_scalars_recorded_ == this->frames_per_block_))
  {
    send_scalars(frame_id, header->num_scalars, header->first_channel, header->num_channels);
  }

  LOG4CXX_DEBUG_LEVEL(1, logger_, "FrameId = " << frame_id);
  for (int index = 0; index < num_channels_; index++){
    memory_ptrs_[index]->add_frame(frame_id, mca_ptr);
    mca_ptr += mca_size;

    // Check if the buffer is full
    if (memory_ptrs_[index]->check_full())
    {
      if (frame_id == (num_frames_ - 1))
      {
        LOG4CXX_DEBUG_LEVEL(1, logger_, "Sending the metadata as the last frame when the queue is full");
        send_scalars(frame_id, header->num_scalars, header->first_channel, header->num_channels);
      }
      // Create the frame and push it
      dimensions_t mca_dims;
      mca_dims.push_back(header->num_aux);
      mca_dims.push_back(header->num_energy_bins);
      std::stringstream ss;
      ss << "mca_" << index + first_channel_index;
      // Calculate the ID of the frame we need to push
      // This must be offset according to the rank and number of processes
      uint32_t push_frame_id = ((frame_id / frames_per_block_) * concurrent_processes_) + concurrent_rank_;
      FrameMetaData mca_metadata(push_frame_id, ss.str(), raw_32bit, "", mca_dims);
      boost::shared_ptr<Frame> mca_frame(new DataBlockFrame(mca_metadata, memory_ptrs_[index]->size()));
      memcpy(mca_frame->get_data_ptr(), memory_ptrs_[index]->get_data_ptr(), memory_ptrs_[index]->size());
      // Set the chunking size
      mca_frame->set_outer_chunk_size(frames_per_block_);
      // Push out the MCA data
      this->push(mca_frame);
      // Reset the memory block
      memory_ptrs_[index]->reset();
    }
    else
    {
      // Check if we are writing out the last block which is not full size
      if (frame_id == (num_frames_ - 1)){
        LOG4CXX_DEBUG_LEVEL(1, logger_, "Sending the metadata as the last frame when the queue is not full");
        // As this is the last frame block to send, post the scalar buffer as well
        send_scalars(frame_id, header->num_scalars, header->first_channel, header->num_channels);

        dimensions_t mca_dims;
        mca_dims.push_back(header->num_aux);
        mca_dims.push_back(header->num_energy_bins);
        std::stringstream ss;
        ss << "mca_" << index + first_channel_index;
        // Calculate the ID of the frame we need to push
        // This must be offset according to the rank and number of processes
        uint32_t push_frame_id = ((frame_id / frames_per_block_) * concurrent_processes_) + concurrent_rank_;
        FrameMetaData mca_metadata(push_frame_id, ss.str(), raw_32bit, "", mca_dims);
        boost::shared_ptr<Frame> mca_frame(new DataBlockFrame(mca_metadata, memory_ptrs_[index]->current_byte_size()));
        memcpy(mca_frame->get_data_ptr(), memory_ptrs_[index]->get_data_ptr(), memory_ptrs_[index]->current_byte_size());
        // Set the chunking size
        mca_frame->set_outer_chunk_size((int)memory_ptrs_[index]->frames());
        // Push out the MCA data
        this->push(mca_frame);
        // Reset the memory block
        memory_ptrs_[index]->reset();
        LOG4CXX_DEBUG_LEVEL(3, logger_, "Pushed partially full frame as required frame count reached");
      }
    }
  }
}

void XspressProcessPlugin::send_scalars(uint32_t last_frame_id, uint32_t num_scalars, uint32_t first_channel, uint32_t num_channels)
{
  uint32_t num_scalar_values = num_scalars * num_channels;
  uint32_t num_dtc_factors = num_channels;
  uint32_t num_inp_est = num_channels;
  uint32_t frame_id = (last_frame_id - num_scalars_recorded_ + 1)  + (this->offset * this->frames_per_block_); //Acquisition frame ID + Offset

  rapidjson::Document meta_document;
  meta_document.SetObject();

  // Add Acquisition ID
  rapidjson::Value key_acq_id("acqID", meta_document.GetAllocator());
  rapidjson::Value value_acq_id;
  value_acq_id.SetString(acq_id_.c_str(), acq_id_.size(), meta_document.GetAllocator());
  meta_document.AddMember(key_acq_id, value_acq_id, meta_document.GetAllocator());
  // Add rank
  rapidjson::Value key_rank("rank", meta_document.GetAllocator());
  rapidjson::Value value_rank;
  value_rank.SetInt(concurrent_rank_);
  meta_document.AddMember(key_rank, value_rank, meta_document.GetAllocator());
  // Add frame_id
  rapidjson::Value key_frame_id("frame_id", meta_document.GetAllocator());
  rapidjson::Value value_frame_id;
  value_frame_id.SetInt(frame_id);
  meta_document.AddMember(key_frame_id, value_frame_id, meta_document.GetAllocator());
  // Add number of data points
  rapidjson::Value key_qty("qty_scalars", meta_document.GetAllocator());
  rapidjson::Value value_qty;
  value_qty.SetInt(num_scalar_values);
  meta_document.AddMember(key_qty, value_qty, meta_document.GetAllocator());
  // Add channel index
  rapidjson::Value key_index("channel_index", meta_document.GetAllocator());
  rapidjson::Value value_index;
  value_index.SetInt(first_channel);
  meta_document.AddMember(key_index, value_index, meta_document.GetAllocator());
  // Add number of channels
  rapidjson::Value key_num("number_of_channels", meta_document.GetAllocator());
  rapidjson::Value value_num;
  value_num.SetInt(num_channels);
  meta_document.AddMember(key_num, value_num, meta_document.GetAllocator());
  rapidjson::Value key_num_frames("number_of_frames", meta_document.GetAllocator());
  rapidjson::Value value_num_frames;
  value_num_frames.SetInt(num_scalars_recorded_);
  meta_document.AddMember(key_num_frames, value_num_frames, meta_document.GetAllocator());

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  meta_document.Accept(writer);

  LOG4CXX_DEBUG_LEVEL(3, logger_, "Publishing MCA scalars: " << buffer.GetString());
  this->publish_meta(META_NAME,
                      META_XSPRESS_SCALARS,
                      scalar_memblock_,
                      num_scalar_values * num_scalars_recorded_ * sizeof(uint32_t),
                      buffer.GetString());

  this->publish_meta(META_NAME,
                      META_XSPRESS_DTC,
                      dtc_memblock_,
                      num_dtc_factors * num_scalars_recorded_ * sizeof(double),
                      buffer.GetString());

  this->publish_meta(META_NAME,
                      META_XSPRESS_INP_EST,
                      inp_est_memblock_,
                      num_inp_est * num_scalars_recorded_ * sizeof(double),
                      buffer.GetString());

    this->publish_meta(META_NAME,
                       META_XSPRESS_SUM,
                       sum_memblock_,
                       num_scalars_recorded_ * sizeof(double),
                       buffer.GetString());

  num_scalars_recorded_ = 0;
}
}
