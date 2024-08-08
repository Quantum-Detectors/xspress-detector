/*
 * LibXspressWrapper.cpp
 *
 *  Created on: 24 Sep 2021
 *      Author: Diamond Light Source
 */

#include <stdio.h>
#include "dirent.h"

#include "LibXspressWrapper.h"
#include "DebugLevelLogger.h"

// handle to xspress from libxspress
extern XSP3Path Xsp3Sys[];


namespace Xspress
{
const int N_RESGRADES = 16;

//Definitions of static class data members
const int LibXspressWrapper::runFlag_MCA_SPECTRA_ = 0;
const int LibXspressWrapper::runFlag_SCALERS_ONLY_ = 1;
const int LibXspressWrapper::runFlag_PLAYB_MCA_SPECTRA_ = 2;

/** Construct a new LibXspressWrapper class.
 *
 * The constructor sets up logging used within the class, and initialises
 * variables.
 */
LibXspressWrapper::LibXspressWrapper() :
    xsp_handle_(-1)
{
  logger_ = log4cxx::Logger::getLogger("Xspress.LibXspressWrapper");
  OdinData::configure_logging_mdc(OdinData::app_path.c_str());
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Constructing LibXspressWrapper");

  // Create the trigger mode map
  trigger_modes_[TM_SOFTWARE_STR] = TM_SOFTWARE;
  trigger_modes_[TM_TTL_RISING_EDGE_STR] = TM_TTL_RISING_EDGE;
  trigger_modes_[TM_BURST_STR] = TM_BURST;
  trigger_modes_[TM_TTL_VETO_ONLY_STR] = TM_TTL_VETO_ONLY;
  trigger_modes_[TM_SOFTWARE_START_STOP_STR] = TM_SOFTWARE_START_STOP;
  trigger_modes_[TM_IDC_STR] = TM_IDC;
  trigger_modes_[TM_TTL_BOTH_STR] = TM_TTL_BOTH;
  trigger_modes_[TM_LVDS_VETO_ONLY_STR] = TM_LVDS_VETO_ONLY;
  trigger_modes_[TM_LVDS_BOTH_STR] = TM_LVDS_BOTH;
}

/** Destructor for XspressDetector class.
 *
 */
LibXspressWrapper::~LibXspressWrapper()
{

}

// TODO: Feature set required

std::string LibXspressWrapper::getVersionString()
{
  int version_no = xsp3_get_revision(xsp_handle_);
  std::stringstream version_ss;
  version_ss << XSP3_REVISION_GET_DETECTOR(version_no)
             << "."
             << XSP3_REVISION_GET_MAJOR(version_no)
             << "." 
             << XSP3_REVISION_GET_MINOR(version_no);
  return version_ss.str();
}

void LibXspressWrapper::checkErrorCode(const std::string& prefix, int code)
{
  checkErrorCode(prefix, code, true);
}

void LibXspressWrapper::checkErrorCode(const std::string& prefix, int code, bool add_xsp_error)
{
  std::stringstream err;
  switch (code){
    case XSP3_OK:
      // No error here, pass through
      break;
    case XSP3_ERROR:
      err << prefix << " error [" << code << "] XSP3_ERROR";
      break;
    case XSP3_INVALID_PATH:
      err << prefix << " error [" << code << "] XSP3_INVALID_PATH";
      break;
    case XSP3_ILLEGAL_CARD:
      err << prefix << " error [" << code << "] XSP3_ILLEGAL_CARD";
      break;
    case XSP3_ILLEGAL_SUBPATH:
      err << prefix << " error [" << code << "] XSP3_ILLEGAL_SUBPATH";
      break;
    case XSP3_INVALID_DMA_STREAM:
      err << prefix << " error [" << code << "] XSP3_INVALID_DMA_STREAM";
      break;
    case XSP3_RANGE_CHECK:
      err << prefix << " error [" << code << "] XSP3_RANGE_CHECK";
      break;
    case XSP3_INVALID_SCOPE_MOD:
      err << prefix << " error [" << code << "] XSP3_INVALID_SCOPE_MOD";
      break;
    case XSP3_OUT_OF_MEMORY:
      err << prefix << " error [" << code << "] XSP3_OUT_OF_MEMORY";
      break;
    case XSP3_ERR_DEV_NOT_FOUND:
      err << prefix << " error [" << code << "] XSP3_ERR_DEV_NOT_FOUND";
      break;
    case XSP3_CANNOT_OPEN_FILE:
      err << prefix << " error [" << code << "] XSP3_CANNOT_OPEN_FILE";
      break;
    case XSP3_FILE_READ_FAILED:
      err << prefix << " error [" << code << "] XSP3_FILE_READ_FAILED";
      break;
    case XSP3_FILE_WRITE_FAILED:
      err << prefix << " error [" << code << "] XSP3_FILE_WRITE_FAILED";
      break;
    case XSP3_FILE_RENAME_FAILED:
      err << prefix << " error [" << code << "] XSP3_FILE_RENAME_FAILED";
      break;
    case XSP3_LOG_FILE_MISSING:
      err << prefix << " error [" << code << "] XSP3_LOG_FILE_MISSING";
      break;
    default:
      // Uknown error code here
      err << prefix << " error [" << code << "] Unknown error code";
  }
  if (code != XSP3_OK){
    if (add_xsp_error){
      err << ": " << xsp3_get_error_message();
    }
    setErrorString(err.str());
  }
}

int LibXspressWrapper::configure_mca(int num_cards,                 // Number of XSPRESS cards
                                     int num_frames,                // Number of 4096 energy bin spectra timeframes
                                     const std::string& ip_address, // Base IP address
                                     int port,                      // Base port number override (-1 does not override)
                                     int max_channels,              // Set the maximum number of channels
                                     int debug,                     // Enable debug messages
                                     int verbose                    // Enable verbose debug messages
                                     )
{
  int status = XSP_STATUS_OK;
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Xspress wrapper calling xsp3_config");
  xsp_handle_ = xsp3_config(
    num_cards,                              // Number of XSPRESS cards
    num_frames,                             // Number of 4096 energy bin spectra timeframes
    const_cast<char *>(ip_address.c_str()), // Base IP address
    port,                                   // Base port number override (-1 does not override)
    NULL,                                   // Base MAC override (NULL does not override)
    max_channels,                           // Set the maximum number of channels
    1,                                      // Don't create scope data module
    NULL,                                   // Override scope data module filename
    debug,                                  // Enable debug messages
    verbose                                 // Enable verbose debug messages
  );
  // Check the returned handle.  
  // If the handle is less than zero then set an error
  if (xsp_handle_ < 0){
    status = XSP_STATUS_ERROR;
    checkErrorCode("xsp3_config", xsp_handle_);
  }
  return status;
}

int LibXspressWrapper::configure_list(int num_cards,                 // Number of XSPRESS cards
                                      int num_frames,                // Number of 4096 energy bin spectra timeframes
                                      const std::string& ip_address, // Base IP address
                                      int port,                      // Base port number override (-1 does not override)
                                      int max_channels,              // Set the maximum number of channels
                                      int debug                      // Enable debug messages
                                      )
{
  int status = XSP_STATUS_OK;
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Xspress wrapper calling xsp3_config_init");

  // Setup initialisation flags to allow alternate UDP RX sockets using the default ports
  int do_init = Xsp3Init_Normal | Xsp3InitUDP_DisHistThreads;

  // Call the more detailed config init function
  xsp_handle_ = xsp3_config_init(
    num_cards,                              // Number of XSPRESS cards
    num_frames,                             // Number of 4096 energy bin spectra timeframes
    const_cast<char *>(ip_address.c_str()), // Base IP address
    port,                                   // Base port number override (-1 does not override)
    NULL,                                   // Base MAC override (NULL does not override)
    max_channels,                           // Set the maximum number of channels
    1,                                      // Don't create scope data module
    NULL,                                   // Override scope data module filename
    debug,                                  // Enable debug messages
    0,                                      // Card index
    (Xsp3Init)do_init,                      // Initialisation flags
    Xsp3mRd_Auto,                           // Xsp3mRd_Auto
    XspressReal                             // XspressReal
  );

  // Check the returned handle.  
  // If the handle is less than zero then set an error
  if (xsp_handle_ < 0){
    status = XSP_STATUS_ERROR;
    checkErrorCode("xsp3_config_init", xsp_handle_);
  }
  return status;
}

int LibXspressWrapper::close_connection()
{
  int status = XSP_STATUS_OK;
  int xsp_status = 0;
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Xspress wrapper calling xsp3_close");

  xsp_status = xsp3_close(xsp_handle_, Xsp3UnlinkAll);

  if (xsp_status != XSP3_OK) {
    checkErrorCode("xsp3_close", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  return status;
}

int LibXspressWrapper::save_settings(const std::string& save_path)
{
  int status = XSP_STATUS_OK;
  int xsp_status = 0;
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Xspress wrapper calling xsp3_save_settings");
  xsp_status = xsp3_save_settings(xsp_handle_, const_cast<char *>(save_path.c_str()));
  if (xsp_status != XSP3_OK) {
    checkErrorCode("xsp3_save_settings", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  return status;
}

int LibXspressWrapper::restore_settings(const std::string& restore_path)
{
  int status = XSP_STATUS_OK;
  int xsp_status = 0;
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Xspress wrapper calling xsp3_restore_settings_and_clock");
  xsp_status = xsp3_restore_settings_and_clock(
    xsp_handle_, 
    const_cast<char *>(restore_path.c_str()),
    0
  );
  if (xsp_status != XSP3_OK) {
    checkErrorCode("xsp3_restore_settings_and_clock", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  return status;
}

int LibXspressWrapper::setup_format_run_mode(bool list_mode, bool use_resgrades, int max_channels, int& num_aux_data)
{
  int status = XSP_STATUS_OK;
  int xsp_status = 0;
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Xspress wrapper setting up list mode, resgrades and calling xsp3_format_run");
  for (int chan = 0; chan < max_channels; chan++) {
    int aux_mode = 0;
    num_aux_data = 1;
    if (list_mode){
      aux_mode |= XSP3_FORMAT_AUX1_MODE_TIMESTAMPED;
    }
    if (use_resgrades){
      num_aux_data = N_RESGRADES;
      aux_mode |= XSP3_FORMAT_RES_MODE_MINDIV8;
    }
    xsp_status = xsp3_format_run(xsp_handle_, chan, aux_mode, 0, 0, 0, 0, 12);
    if (xsp_status < XSP3_OK) {
      checkErrorCode("xsp3_format_run", xsp_status);
      status = XSP_STATUS_ERROR;
    } else {
      LOG4CXX_INFO(logger_, "Channel: " << chan << ", Number of time frames configured: " << xsp_status);
    }
  }
  return status;
}

int LibXspressWrapper::set_run_flags(int run_flags)
{
  int status = XSP_STATUS_OK;
  int xsp_status = 0;
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Xspress wrapper calling xsp3_set_run_flags with " << run_flags);

  switch (run_flags)
  {
    case runFlag_SCALERS_ONLY_:
      // TODO: setting scaler only run flags doesn't seem to work (docs say it is possible but "not fully supported").
      //       doing so seems to result in no data being returned, or at least reported as available by
      //       xsp3_scaler_check_progress_details. Treat as per "runFlag_MCA_SPECTRA_" here for now.
      // xsp3_status = xsp3_set_run_flags(xsp3_handle_, XSP3_RUN_FLAGS_SCALERS | XSP3_RUN_FLAGS_CIRCULAR_BUFFER);
      // break;
    case runFlag_MCA_SPECTRA_:
      xsp_status = xsp3_set_run_flags(xsp_handle_, XSP3_RUN_FLAGS_SCALERS |
                                                   XSP3_RUN_FLAGS_HIST | 
                                                   XSP3_RUN_FLAGS_CIRCULAR_BUFFER);
      break;
    case runFlag_PLAYB_MCA_SPECTRA_:
      xsp_status = xsp3_set_run_flags(xsp_handle_, XSP3_RUN_FLAGS_PLAYBACK |
                                                   XSP3_RUN_FLAGS_SCALERS | 
                                                   XSP3_RUN_FLAGS_HIST |
                                                   XSP3_RUN_FLAGS_CIRCULAR_BUFFER);
      break;
    default:
      LOG4CXX_ERROR(logger_, "Invalid run flag option when trying to set xsp3_set_run_flags.");
      status = XSP_STATUS_ERROR;
      break;
  }

  if (xsp_status < XSP3_OK) {
    checkErrorCode("xsp3_set_run_flags", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  return status;
}

int LibXspressWrapper::set_dtc_energy(double dtc_energy)
{
  int status = XSP_STATUS_OK;
  int xsp_status = 0;
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Xspress wrapper calling xsp3_setDeadTimeCalculationEnergy");
  xsp_status = xsp3_setDeadtimeCalculationEnergy(xsp_handle_, dtc_energy);
  if (xsp_status != XSP3_OK) {
    checkErrorCode("xsp3_setDeadtimeCalculationEnergy", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  return status;
}

int LibXspressWrapper::get_clock_period(double& clock_period)
{
  int status = XSP_STATUS_OK;
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Xspress wrapper calling xsp3_get_clock_period");
  // Read the clock period
  clock_period = xsp3_get_clock_period(xsp_handle_, 0);
  return status;
}


/**
 * Read the SCA window limits (for SCA 5 and 6) and threshold for SCA 4, for each channel.
 */
int LibXspressWrapper::read_sca_params(int max_channels,
                                       std::vector<uint32_t>& sca5_low,
                                       std::vector<uint32_t>& sca5_high,
                                       std::vector<uint32_t>& sca6_low,
                                       std::vector<uint32_t>& sca6_high,
                                       std::vector<uint32_t>& sca4_threshold
                                       )
{
  int status = XSP_STATUS_OK;
  int xsp_status = 0;
  uint32_t xsp_sca_param1 = 0;
  uint32_t xsp_sca_param2 = 0;

  LOG4CXX_DEBUG_LEVEL(1, logger_, "Xspress wrapper calling xsp3_get_window and xsp3_get_good_thres");

  // Clear the arrays
  sca5_low.clear();
  sca5_high.clear();
  sca6_low.clear();
  sca6_high.clear();
  sca4_threshold.clear();

  for (int chan = 0; chan < max_channels; ++chan) {
    //SCA 5 window limits
    xsp_status = xsp3_get_window(xsp_handle_, chan, 0, &xsp_sca_param1, &xsp_sca_param2);
    if (xsp_status < XSP3_OK) {
      checkErrorCode("xsp3_get_window", xsp_status);
      status = XSP_STATUS_ERROR;
    } else {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Read back SCA5 window limits: " << xsp_sca_param1 << ", " << xsp_sca_param2);
      sca5_low.push_back(xsp_sca_param1);
      sca5_high.push_back(xsp_sca_param2);
    }
    //SCA 6 window limits
    xsp_status = xsp3_get_window(xsp_handle_, chan, 1, &xsp_sca_param1, &xsp_sca_param2);
    if (xsp_status < XSP3_OK) {
      checkErrorCode("xsp3_get_window", xsp_status);
      status = XSP_STATUS_ERROR;
    } else {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Read back SCA6 window limits: " << xsp_sca_param1 << ", " << xsp_sca_param2);
      sca6_low.push_back(xsp_sca_param1);
      sca6_high.push_back(xsp_sca_param2);
    }
    //SCA 4 threshold limit
    xsp_status = xsp3_get_good_thres(xsp_handle_, chan, &xsp_sca_param1);
    if (xsp_status < XSP3_OK) {
      checkErrorCode("xsp3_get_good thres", xsp_status);
      status = XSP_STATUS_ERROR;
    } else {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Read back SCA4 threshold limit: " << xsp_sca_param1);
      sca4_threshold.push_back(xsp_sca_param1);
    }
  }
  return status;
}

int LibXspressWrapper::check_connected_channels(std::vector<bool>& cards_connected, std::vector<int>& channels_connected)
{
  int status = XSP_STATUS_OK;
  if (::Xsp3Sys[xsp_handle_].type == FEM_COMPOSITE) {
    int found_chans = 0;
    if (cards_connected.size() != ::Xsp3Sys[xsp_handle_].num_cards){
      setErrorString("cards_connected vector has the incorrect dimension for the detector reported number of cards");
      status = XSP_STATUS_ERROR;
    } else if (channels_connected.size() != ::Xsp3Sys[xsp_handle_].num_cards){
      setErrorString("channels_connected vector has the incorrect dimension for the detector reported number of cards");
      status = XSP_STATUS_ERROR;
    } else {
      for (int card = 0; card < ::Xsp3Sys[xsp_handle_].num_cards; card++) {
        int thisPath = ::Xsp3Sys[xsp_handle_].sub_path[card];
        cards_connected[card] = true;
        channels_connected[card] = ::Xsp3Sys[thisPath].num_chan;
        found_chans += ::Xsp3Sys[thisPath].num_chan;
        LOG4CXX_INFO(logger_, "Card " << card << " connected with " << channels_connected[card] << " channels");
      }
    }
  }
  return status;
}

int LibXspressWrapper::read_frames(int max_channels, std::vector<int32_t>& frame_counters)
{
  int status = XSP_STATUS_OK;
  LOG4CXX_DEBUG_LEVEL(5, logger_, "Xspress wrapper calling xsp3_resolve_path and using Xsp3Sys[].histogram[].cur_tf_ext");

  if (frame_counters.size() != max_channels){
    setErrorString("Frame counter vector has a different dimension to the number of channels");
    status = XSP_STATUS_ERROR;
  }

  if (status == XSP_STATUS_OK){
    for (u_int32_t chan = 0; chan < max_channels; chan++) {
      int thisPath, chanIdx;
      xsp3_resolve_path(xsp_handle_, chan, &thisPath, &chanIdx);
      frame_counters[chan] = (int32_t)(Xsp3Sys[thisPath].histogram[chanIdx].cur_tf_ext);
    }  
  }
  return status;
}

int LibXspressWrapper::read_temperatures(std::vector<float>& t0,
                                         std::vector<float>& t1,
                                         std::vector<float>& t2,
                                         std::vector<float>& t3,
                                         std::vector<float>& t4,
                                         std::vector<float>& t5)
{
  int status = XSP_STATUS_OK;
  LOG4CXX_DEBUG_LEVEL(5, logger_, "Xspress wrapper calling xsp3_i2c_read_adc_temp and xsp3_i2c_read_fem_temp");
  int num_cards = Xsp3Sys[xsp_handle_].num_cards;
  if (t0.size() != num_cards){
    setErrorString("temperature vector 0 has a different size to the number of cards");
    status = XSP_STATUS_ERROR;
  }
  if (t1.size() != num_cards){
    setErrorString("temperature vector 1 has a different size to the number of cards");
    status = XSP_STATUS_ERROR;
  }
  if (t2.size() != num_cards){
    setErrorString("temperature vector 2 has a different size to the number of cards");
    status = XSP_STATUS_ERROR;
  }
  if (t3.size() != num_cards){
    setErrorString("temperature vector 3 has a different size to the number of cards");
    status = XSP_STATUS_ERROR;
  }
  if (t4.size() != num_cards){
    setErrorString("temperature vector 4 has a different size to the number of cards");
    status = XSP_STATUS_ERROR;
  }
  if (t5.size() != num_cards){
    setErrorString("temperature vector 5 has a different size to the number of cards");
    status = XSP_STATUS_ERROR;
  }

  if (status == XSP_STATUS_OK){
    for (int card = 0; card < num_cards; card++) {
      int thisPath;
      float temps[6];
      float *tempsPtr = temps;
      int xsp_status;
      thisPath = Xsp3Sys[xsp_handle_].sub_path[card];
      xsp_status = xsp3_i2c_read_adc_temp(xsp_handle_, card, tempsPtr);
      if (xsp_status == XSP3_OK) {
        t0[card] = temps[0];
        t1[card] = temps[1];
        t2[card] = temps[2];
        t3[card] = temps[3];
      }
      tempsPtr += 4;
      xsp_status = xsp3_i2c_read_fem_temp(xsp_handle_, card, tempsPtr);
      if (xsp_status == XSP3_OK) {
        t4[card] = temps[4];
        t5[card] = temps[5];
      }
    }
  }
  return status;
}

int LibXspressWrapper::read_dropped_frames(std::vector<int32_t>& dropped_frames)
{
  int status = XSP_STATUS_OK;
  LOG4CXX_DEBUG_LEVEL(5, logger_, "Xspress wrapper using Xsp3Sys[].histogram[].dropped_frames");
  int num_cards = Xsp3Sys[xsp_handle_].num_cards;
  if (dropped_frames.size() != num_cards){
    setErrorString("dropped frames vector has a different size to the number of cards");
    status = XSP_STATUS_ERROR;
  }

  if (status == XSP_STATUS_OK){
    for (int card = 0; card < num_cards; card++) {
      int thisPath;
      thisPath = Xsp3Sys[xsp_handle_].sub_path[card];
      dropped_frames[card] = Xsp3Sys[thisPath].histogram[0].dropped_frames;
    }
  }
  return status;
}

int LibXspressWrapper::read_dtc_params(int max_channels,
                                       std::vector<int>& dtc_flags,
                                       std::vector<double>& dtc_all_event_off,
                                       std::vector<double>& dtc_all_event_grad,
                                       std::vector<double>& dtc_all_event_rate_off,
                                       std::vector<double>& dtc_all_event_rate_grad,
                                       std::vector<double>& dtc_in_window_off,
                                       std::vector<double>& dtc_in_window_grad,
                                       std::vector<double>& dtc_in_window_rate_off,
                                       std::vector<double>& dtc_in_window_rate_grad)
{
  int status = XSP_STATUS_OK;
  int xsp_status = 0;
  int xsp_dtc_flags = 0;
  double xsp_dtc_all_event_off = 0.0;
  double xsp_dtc_all_event_grad = 0.0;
  double xsp_dtc_all_event_rate_off = 0.0;
  double xsp_dtc_all_event_rate_grad = 0.0;
  double xsp_dtc_in_window_off = 0.0;
  double xsp_dtc_in_window_grad = 0.0;
  double xsp_dtc_in_window_rate_off = 0.0;
  double xsp_dtc_in_window_rate_grad = 0.0;

  LOG4CXX_DEBUG_LEVEL(1, logger_, "Xspress wrapper calling xsp3_getDeadtimeCorrectionParameters2");

  // Clear the arrays
  dtc_flags.clear();
  dtc_all_event_off.clear();
  dtc_all_event_grad.clear();
  dtc_all_event_rate_off.clear();
  dtc_all_event_rate_grad.clear();
  dtc_in_window_off.clear();
  dtc_in_window_grad.clear();
  dtc_in_window_rate_off.clear();
  dtc_in_window_rate_grad.clear();

  for (int chan = 0; chan < max_channels; chan++) {
    xsp_status = xsp3_getDeadtimeCorrectionParameters2(xsp_handle_,
                                                       chan,
                                                       &xsp_dtc_flags,
                                                       &xsp_dtc_all_event_off,
                                                       &xsp_dtc_all_event_grad,
                                                       &xsp_dtc_all_event_rate_off,
                                                       &xsp_dtc_all_event_rate_grad,
                                                       &xsp_dtc_in_window_off,
                                                       &xsp_dtc_in_window_grad,
                                                       &xsp_dtc_in_window_rate_off,
                                                       &xsp_dtc_in_window_rate_grad);
    if (xsp_status < XSP3_OK){
      checkErrorCode("xsp3_getDeadtimeCorrectionParameters", xsp_status);
      status = XSP_STATUS_ERROR;
    } else {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Channel " << chan <<
                                      " Dead Time Correction Params: Flags: " << xsp_dtc_flags <<
                                      ", All Event Grad: " << xsp_dtc_all_event_grad <<
                                      ", All Event Off: " << xsp_dtc_all_event_off <<
                                      ", In Win Off: " << xsp_dtc_in_window_off <<
                                      ", In Win Grad: " << xsp_dtc_in_window_grad);

      dtc_flags.push_back(xsp_dtc_flags);
      dtc_all_event_off.push_back(xsp_dtc_all_event_off);
      dtc_all_event_grad.push_back(xsp_dtc_all_event_grad);
      dtc_all_event_rate_off.push_back(xsp_dtc_all_event_rate_off);
      dtc_all_event_rate_grad.push_back(xsp_dtc_all_event_rate_grad);
      dtc_in_window_off.push_back(xsp_dtc_in_window_off);
      dtc_in_window_grad.push_back(xsp_dtc_in_window_grad);
      dtc_in_window_rate_off.push_back(xsp_dtc_in_window_rate_off);
      dtc_in_window_rate_grad.push_back(xsp_dtc_in_window_rate_grad);
    }
  }
  return status;
}
/*
int LibXspressWrapper::read_dtc_params(int max_channels,
                                       std::vector<int>& dtci,
                                       std::vector<double>& dtcd,
                                       bool& parameters_updated)
{
  int status = XSP_STATUS_OK;
  int xsp_status = 0;
  int xsp_dtc_flags = 0;
  double xsp_dtc_all_event_off = 0.0;
  double xsp_dtc_all_event_grad = 0.0;
  double xsp_dtc_all_event_rate_off = 0.0;
  double xsp_dtc_all_event_rate_grad = 0.0;
  double xsp_dtc_in_window_off = 0.0;
  double xsp_dtc_in_window_grad = 0.0;
  double xsp_dtc_in_window_rate_off = 0.0;
  double xsp_dtc_in_window_rate_grad = 0.0;

  LOG4CXX_DEBUG_LEVEL(1, logger_, "Xspress wrapper calling xsp3_getDeadtimeCorrectionParameters2");
  if (dtci.size() != (max_channels * XSP3_NUM_DTC_INT_PARAMS)){
    setErrorString("DTC integer vector has a different size to the number of parameters");
    status = XSP_STATUS_ERROR;
  }

  if (dtcd.size() != (max_channels * XSP3_NUM_DTC_FLOAT_PARAMS)){
    setErrorString("DTC double vector has a different size to the number of parameters");
    status = XSP_STATUS_ERROR;
  }

  if (status == XSP_STATUS_OK){
    for (int chan = 0; chan < max_channels; chan++) {
      xsp_status = xsp3_getDeadtimeCorrectionParameters2(xsp_handle_,
                                                         chan,
                                                         &xsp_dtc_flags,
                                                         &xsp_dtc_all_event_off,
                                                         &xsp_dtc_all_event_grad,
                                                         &xsp_dtc_all_event_rate_off,
                                                         &xsp_dtc_all_event_rate_grad,
                                                         &xsp_dtc_in_window_off,
                                                         &xsp_dtc_in_window_grad,
                                                         &xsp_dtc_in_window_rate_off,
                                                         &xsp_dtc_in_window_rate_grad);
      if (xsp_status < XSP3_OK){
        checkErrorCode("xsp3_getDeadtimeCorrectionParameters", xsp_status);
        status = XSP_STATUS_ERROR;
      } else {
        LOG4CXX_DEBUG_LEVEL(1, logger_, "Channel " << chan <<
                                        " Dead Time Correction Params: Flags: " << xsp_dtc_flags <<
                                        ", All Event Grad: " << xsp_dtc_all_event_grad <<
                                        ", All Event Off: " << xsp_dtc_all_event_off <<
                                        ", In Win Off: " << xsp_dtc_in_window_off <<
                                        ", In Win Grad: " << xsp_dtc_in_window_grad);

        dtci[chan * XSP3_NUM_DTC_INT_PARAMS + XSP3_DTC_FLAGS] = xsp_dtc_flags;

        parameters_updated = parameters_updated ||
                          dtcd[chan * XSP3_NUM_DTC_FLOAT_PARAMS + XSP3_DTC_AEO] != xsp_dtc_all_event_off;
        dtcd[chan * XSP3_NUM_DTC_FLOAT_PARAMS + XSP3_DTC_AEO] = xsp_dtc_all_event_off;

        parameters_updated = parameters_updated ||
                          dtcd[chan * XSP3_NUM_DTC_FLOAT_PARAMS + XSP3_DTC_AEG] != xsp_dtc_all_event_grad;
        dtcd[chan * XSP3_NUM_DTC_FLOAT_PARAMS + XSP3_DTC_AEG] = xsp_dtc_all_event_grad;

        parameters_updated = parameters_updated ||
                          dtcd[chan * XSP3_NUM_DTC_FLOAT_PARAMS + XSP3_DTC_AERO] != xsp_dtc_all_event_rate_off;
        dtcd[chan * XSP3_NUM_DTC_FLOAT_PARAMS + XSP3_DTC_AERO] = xsp_dtc_all_event_rate_off;

        parameters_updated = parameters_updated ||
                          dtcd[chan * XSP3_NUM_DTC_FLOAT_PARAMS + XSP3_DTC_AERG] != xsp_dtc_all_event_rate_grad;
        dtcd[chan * XSP3_NUM_DTC_FLOAT_PARAMS + XSP3_DTC_AERG] = xsp_dtc_all_event_rate_grad;

        parameters_updated = parameters_updated ||
                          dtcd[chan * XSP3_NUM_DTC_FLOAT_PARAMS + XSP3_DTC_IWO] != xsp_dtc_in_window_off;
        dtcd[chan * XSP3_NUM_DTC_FLOAT_PARAMS + XSP3_DTC_IWO] = xsp_dtc_in_window_off;

        parameters_updated = parameters_updated ||
                          dtcd[chan * XSP3_NUM_DTC_FLOAT_PARAMS + XSP3_DTC_IWG] != xsp_dtc_in_window_grad;
        dtcd[chan * XSP3_NUM_DTC_FLOAT_PARAMS + XSP3_DTC_IWG] = xsp_dtc_in_window_grad;

        parameters_updated = parameters_updated ||
                          dtcd[chan * XSP3_NUM_DTC_FLOAT_PARAMS + XSP3_DTC_IWRO] != xsp_dtc_in_window_rate_off;
        dtcd[chan * XSP3_NUM_DTC_FLOAT_PARAMS + XSP3_DTC_IWRO] = xsp_dtc_in_window_rate_off;

        parameters_updated = parameters_updated ||
                          dtcd[chan * XSP3_NUM_DTC_FLOAT_PARAMS + XSP3_DTC_IWRG] != xsp_dtc_in_window_rate_grad;
        dtcd[chan * XSP3_NUM_DTC_FLOAT_PARAMS + XSP3_DTC_IWRG] = xsp_dtc_in_window_rate_grad;
      }
    }
  }
  return status;
}
*/


int LibXspressWrapper::write_dtc_params(int max_channels,
                                        std::vector<int>& dtc_flags,
                                        std::vector<double>& dtc_all_event_off,
                                        std::vector<double>& dtc_all_event_grad,
                                        std::vector<double>& dtc_all_event_rate_off,
                                        std::vector<double>& dtc_all_event_rate_grad,
                                        std::vector<double>& dtc_in_window_off,
                                        std::vector<double>& dtc_in_window_grad,
                                        std::vector<double>& dtc_in_window_rate_off,
                                        std::vector<double>& dtc_in_window_rate_grad)
{
  int status = XSP_STATUS_OK;
  int xsp_status = 0;
  int xsp_dtc_flags = 0;
  double xsp_dtc_all_event_off = 0.0;
  double xsp_dtc_all_event_grad = 0.0;
  double xsp_dtc_all_event_rate_off = 0.0;
  double xsp_dtc_all_event_rate_grad = 0.0;
  double xsp_dtc_in_window_off = 0.0;
  double xsp_dtc_in_window_grad = 0.0;
  double xsp_dtc_in_window_rate_off = 0.0;
  double xsp_dtc_in_window_rate_grad = 0.0;

  LOG4CXX_DEBUG_LEVEL(1, logger_, "Xspress wrapper calling xsp3_setDeadtimeCorrectionParameters2");
  for (int chan = 0; chan < max_channels; chan++) {
    xsp_dtc_flags = dtc_flags[chan];
    xsp_dtc_all_event_off = dtc_all_event_off[chan];
    xsp_dtc_all_event_grad = dtc_all_event_grad[chan];
    xsp_dtc_all_event_rate_off = dtc_all_event_rate_off[chan];
    xsp_dtc_all_event_rate_grad = dtc_all_event_rate_grad[chan];
    xsp_dtc_in_window_off = dtc_in_window_off[chan];
    xsp_dtc_in_window_grad = dtc_in_window_grad[chan];
    xsp_dtc_in_window_rate_off = dtc_in_window_rate_off[chan];
    xsp_dtc_in_window_rate_grad = dtc_in_window_rate_grad[chan];

    xsp_status = xsp3_setDeadtimeCorrectionParameters2(xsp_handle_,
                                                       chan,
                                                       xsp_dtc_flags,
                                                       xsp_dtc_all_event_off,
                                                       xsp_dtc_all_event_grad,
                                                       xsp_dtc_all_event_rate_off,
                                                       xsp_dtc_all_event_rate_grad,
                                                       xsp_dtc_in_window_off,
                                                       xsp_dtc_in_window_grad,
                                                       xsp_dtc_in_window_rate_off,
                                                       xsp_dtc_in_window_rate_grad);
    if (xsp_status < XSP3_OK) {
      checkErrorCode("xsp3_setDeadtimeCorrectionParameters", xsp_status);
      status = XSP_STATUS_ERROR;
    }
  }
  return status;
}

/**
 * Function to map the database timeframe source
 * value to the macros defined by the API.
 * @param mode The database value
 * @param apiMode This returns the correct value for the API
 * @return asynStatus
 */
int LibXspressWrapper::mapTimeFrameSource(Xsp3Timing *api_mode,
                                          int *api_itfg_mode,
                                          int trigger_mode,
                                          int debounce,
                                          int invert_f0,
                                          int invert_veto)
{
  int status = XSP_STATUS_OK;

  *api_itfg_mode = -1;

  switch (trigger_mode) {
    case TM_SOFTWARE_START_STOP:
      api_mode->t_src = XSP3_GTIMA_SRC_FIXED;
      break;
    case TM_SOFTWARE:
      api_mode->t_src = XSP3_GTIMA_SRC_INTERNAL;
      *api_itfg_mode = XSP3_ITFG_TRIG_MODE_SOFTWARE;
      break;
    case TM_TTL_RISING_EDGE:
      api_mode->t_src = XSP3_GTIMA_SRC_INTERNAL;
      *api_itfg_mode = XSP3_ITFG_TRIG_MODE_HARDWARE;
      break;
    case TM_BURST:
      api_mode->t_src = XSP3_GTIMA_SRC_INTERNAL;
      *api_itfg_mode = XSP3_ITFG_TRIG_MODE_BURST;
      break;
    case TM_IDC:
      api_mode->t_src = XSP3_GTIMA_SRC_IDC;
      break;
    case TM_TTL_VETO_ONLY:
      api_mode->t_src = XSP3_GTIMA_SRC_TTL_VETO_ONLY;
      break;
    case TM_TTL_BOTH:
      api_mode->t_src = XSP3_GTIMA_SRC_TTL_BOTH;
      break;
    case TM_LVDS_VETO_ONLY:
      api_mode->t_src = XSP3_GTIMA_SRC_LVDS_VETO_ONLY;
      break;
    case TM_LVDS_BOTH:
      api_mode->t_src = XSP3_GTIMA_SRC_LVDS_BOTH;
      break;

    default:
      LOG4CXX_ERROR(logger_, "Mapping an unknown timeframe source mode: " << trigger_mode);
      status = XSP_STATUS_ERROR;
      break;
  }

  if (invert_f0){
    api_mode->inv_f0 = 1;
  }
  if (invert_veto){
    api_mode->inv_veto = 1;
  }
  api_mode->debounce = debounce;

  return status;
}

int LibXspressWrapper::setTriggerMode(int frames,
                                      double exposure_time,
                                      double clock_period,
                                      int trigger_mode,
                                      int debounce,
                                      int invert_f0,
                                      int invert_veto)
{
  int status = XSP_STATUS_OK;
  Xsp3Timing xsp_trigger_mode = {0};
  int itfg_trig_mode;
  int xsp_status = XSP3_OK;

  LOG4CXX_DEBUG_LEVEL(1, logger_, "Xspress wrapper calling xsp3_itfg_setup and xsp3_set_timing");  

  status = mapTimeFrameSource(&xsp_trigger_mode, &itfg_trig_mode, trigger_mode, debounce, invert_f0, invert_veto);
  if (status == XSP_STATUS_OK){
    if (xsp_trigger_mode.t_src == XSP3_GTIMA_SRC_INTERNAL) {
      xsp_status = xsp3_itfg_setup(xsp_handle_, 
                                   0,
                                   frames,
                                   (u_int32_t)floor(exposure_time / clock_period + 0.5),
                                   itfg_trig_mode,
                                   XSP3_ITFG_GAP_MODE_1US);
      if (xsp_status != XSP3_OK) {
        checkErrorCode("xsp3_itfg_setup", xsp_status);
        status = XSP_STATUS_ERROR;
      }
    }
    if (status != XSP_STATUS_ERROR){
      xsp_status = xsp3_set_timing(xsp_handle_, &xsp_trigger_mode);
      if (xsp_status != XSP3_OK) {
        checkErrorCode("xsp3_set_timing", xsp_status);
        status = XSP_STATUS_ERROR;
      }
    }
  }
  return status;
}

int LibXspressWrapper::get_num_frames_read(int32_t *frames)
{
  int status = XSP_STATUS_OK;
  int64_t furthest_frame = 0;
  Xsp3ErrFlag flags;
  *frames = xsp3_scaler_check_progress_details(xsp_handle_, &flags, 0, &furthest_frame);
  if (*frames < XSP3_OK) {
    checkErrorCode("xsp3_scaler_check_progress_details", *frames);
    status = XSP_STATUS_ERROR;
  } else {
    if (flags != 0){
      std::stringstream ss;
      ss << "xsp3_scaler_check_progress_details reported error flags [" << flags << "]";
      setErrorString(ss.str());
      status = XSP_STATUS_ERROR;
    }
  }
  return status;
}

int LibXspressWrapper::get_num_scalars(uint32_t *num_scalars)
{
  *num_scalars = XSP3_SW_NUM_SCALERS;
}

int LibXspressWrapper::histogram_circ_ack(int channel,
                                          uint32_t frame_number,
                                          uint32_t number_of_frames,
                                          uint32_t max_channels)
{
  int status = XSP_STATUS_OK;
  int xsp_status = xsp3_histogram_circ_ack(xsp_handle_, channel, frame_number, max_channels, number_of_frames);
  if (xsp_status < XSP3_OK) {
    checkErrorCode("xsp3_histogram_circ_ack", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  return status;
}

int LibXspressWrapper::histogram_start(int card)
{
  int status = XSP_STATUS_OK;
  int xsp_status = xsp3_histogram_start(xsp_handle_, card);
  if (xsp_status < XSP3_OK) {
    checkErrorCode("xsp3_histogram_start", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  return status;
}

int LibXspressWrapper::histogram_arm(int card)
{
  int status = XSP_STATUS_OK;
  int xsp_status = xsp3_histogram_arm(xsp_handle_, card);
  if (xsp_status < XSP3_OK) {
    checkErrorCode("xsp3_histogram_arm", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  return status;
}

int LibXspressWrapper::histogram_continue(int card)
{
  int status = XSP_STATUS_OK;
  int xsp_status = xsp3_histogram_continue(xsp_handle_, card);
  if (xsp_status < XSP3_OK) {
    checkErrorCode("xsp3_histogram_continue", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  return status;
}

int LibXspressWrapper::histogram_pause(int card)
{
  int status = XSP_STATUS_OK;
  int xsp_status = xsp3_histogram_pause(xsp_handle_, card);
  if (xsp_status < XSP3_OK){
    checkErrorCode("xsp3_histogram_pause", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  return status;
}

int LibXspressWrapper::histogram_stop(int card)
{
  int status = XSP_STATUS_OK;
  int xsp_status = xsp3_histogram_stop(xsp_handle_, card);
  if (xsp_status < XSP3_OK){
    checkErrorCode("xsp3_histogram_stop", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  return status;
}

int LibXspressWrapper::string_trigger_mode_to_int(const std::string& mode)
{
  int trigger_mode = -1;
  if (trigger_modes_.count(mode) > 0){
    trigger_mode = trigger_modes_[mode];
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Converting trigger mode " << mode << " into integer: " << trigger_mode);
  } else {
    LOG4CXX_ERROR(logger_, "Invalid trigger mode requested: " << mode);
  }
  return trigger_mode;
}

int LibXspressWrapper::scaler_read(uint32_t *buffer,
                                   uint32_t tf,
                                   uint32_t num_tf,
                                   uint32_t start_chan,
                                   uint32_t num_chan)
{
  int status = XSP_STATUS_OK;
  int xsp_status = xsp3_scaler_read(xsp_handle_, buffer, 0, start_chan, tf, XSP3_SW_NUM_SCALERS, num_chan, num_tf);
  if (xsp_status < XSP3_OK){
    checkErrorCode("xsp3_scaler_read", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  return status;
}

int LibXspressWrapper::calculate_dtc_factors(uint32_t *scalers,
                                             double *dtc_factors,
                                             double *inp_est,
                                             uint32_t frames,
                                             uint32_t start_chan,
                                             uint32_t num_chan)
{
  int status = XSP_STATUS_OK;
  int xsp_status = xsp3_calculateDeadtimeCorrectionFactors(xsp_handle_,
                                                           scalers,
                                                           dtc_factors,
                                                           inp_est,
                                                           frames,
                                                           start_chan,
                                                           num_chan);
  if (xsp_status < XSP3_OK){
    checkErrorCode("xsp3_calculateDeadtimeCorrectionFactors", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  return status;
}

int LibXspressWrapper::histogram_memcpy(uint32_t *buffer,
                                        uint32_t tf, 
                                        uint32_t num_tf,
                                        uint32_t total_tf,
                                        uint32_t num_eng,
                                        uint32_t num_aux,
                                        uint32_t start_chan,
                                        uint32_t num_chan)
{
  int status = XSP_STATUS_OK;
  int xsp_status;
  uint32_t twrap;
  uint32_t *frame_ptr;
  int rc;
  int thisPath, chanIdx;
  bool circ_buffer;

  if (xsp_handle_ < 0 || xsp_handle_ >= XSP3_MAX_PATH || !Xsp3Sys[xsp_handle_].valid){
    checkErrorCode("histogram_memcpy", XSP3_INVALID_PATH);
    status = XSP_STATUS_ERROR;
  } else {
    circ_buffer = (bool)(Xsp3Sys[xsp_handle_].run_flags & XSP3_RUN_FLAGS_CIRCULAR_BUFFER);

    if (Xsp3Sys[xsp_handle_].features.generation == XspressGen3Mini){
      xsp_status = xsp3_histogram_read4d(xsp_handle_, buffer, 0, 0, start_chan, tf, num_eng, num_aux, num_chan, num_tf)
      // xsp_status = xsp3m_histogram_read_frames(xsp_handle_, buffer, 0, start_chan, tf, num_eng, num_chan, num_tf);
      if (xsp_status < XSP3_OK){
        checkErrorCode("xsp3_histogram_read_frames", xsp_status);
        status = XSP_STATUS_ERROR;
      }
    } else {
      if (tf > total_tf && !circ_buffer) {
        LOG4CXX_ERROR(logger_, "Requested timeframe " << tf << " lies beyond end of buffer (length " << total_tf <<")");
        checkErrorCode("xsp3_histogram_memcpy", XSP3_RANGE_CHECK);
        status = XSP_STATUS_ERROR;
      }
      for (uint32_t t = tf; t < tf + num_tf; t++) {
        if (circ_buffer){
          twrap = t % total_tf;
        } else {
          twrap = t;
        }
        for (uint32_t c = start_chan; c < start_chan + num_chan; c++) {
          if ((xsp_status = xsp3_resolve_path(xsp_handle_, c, &thisPath, &chanIdx)) < 0){
            checkErrorCode("xsp3_resolve_path", xsp_status);
            status = XSP_STATUS_ERROR;
          } else {
            frame_ptr = Xsp3Sys[thisPath].histogram[chanIdx].buffer;
            frame_ptr += num_eng * num_aux * twrap;
            memcpy(buffer, frame_ptr, num_eng * num_aux * sizeof(uint32_t));
            buffer += num_eng * num_aux;
          }
        }
      }
    }
  }
  return status;
}

int LibXspressWrapper::validate_histogram_dims(uint32_t num_eng,
                                               uint32_t num_aux,
                                               uint32_t start_chan,
                                               uint32_t num_chan,
                                               uint32_t *buffer_length)
{
  int status = XSP_STATUS_OK;
  int xsp_status;
  unsigned c;
  int total_tf;
  int nbins_eng, nbins_aux1, nbins_aux2;
  int thisPath, chanIdx;
  char old_message[XSP3_MAX_MSG_LEN + 2];

  LOG4CXX_DEBUG_LEVEL(1, logger_, "validate_histogram_dims called with num_eng=" << num_eng <<
                                  " num_aux=" << num_aux << " start_chan=" << start_chan <<
                                  " num_chan=" << num_chan);

  if (xsp_handle_ < 0 || xsp_handle_ >= XSP3_MAX_PATH || !Xsp3Sys[xsp_handle_].valid) {
    checkErrorCode("validate_histogram_dims", XSP3_INVALID_PATH);
    status = XSP_STATUS_ERROR;
  }

  if (status == XSP_STATUS_OK){
    if ((xsp_status = xsp3_get_format(xsp_handle_, start_chan, &nbins_eng, &nbins_aux1, &nbins_aux2, &total_tf)) < 0) {
      checkErrorCode("xsp3_get_format", xsp_status, true);
      status = XSP_STATUS_ERROR;
    }
  }

  if (status == XSP_STATUS_OK){
    if (num_chan > 1) {
      int ne, na1, na2, nt;
      for (c = start_chan; c < (start_chan + num_chan); c++) {
        if ((xsp_status = xsp3_get_format(xsp_handle_, c, &ne, &na1, &na2, &nt)) < 0) {
          checkErrorCode("xsp3_get_format", xsp_status, true);
          status = XSP_STATUS_ERROR;
        } else {
          if (ne != nbins_eng || na1 != nbins_aux1 || na2 != nbins_aux2 || nt != total_tf) {
            checkErrorCode("xsp3_histogram_read4d: If reading more than 1 channel formats must match", XSP3_ERROR);
            status = XSP_STATUS_ERROR;
          }
        }
      }
    }
  }

  if (status == XSP_STATUS_OK){
    if ((xsp_status = xsp3_resolve_path(xsp_handle_, start_chan, &thisPath, &chanIdx)) < 0){
      checkErrorCode("xsp3_resolve_path", xsp_status);
      status = XSP_STATUS_ERROR;
    }
  }

  if (status == XSP_STATUS_OK){
    if (num_eng == 0 || num_aux == 0 || num_chan == 0) {
      checkErrorCode("xsp3_histogram_read4d: no data requested", XSP3_RANGE_CHECK);
      status = XSP_STATUS_ERROR;
    } else {
      if (num_eng > (unsigned) nbins_eng || num_aux != (unsigned) nbins_aux1 * nbins_aux2
          || start_chan + num_chan > (unsigned) Xsp3Sys[xsp_handle_].num_chan) {
        checkErrorCode("xsp3_histogram_read4d: Requested region mismatch", XSP3_RANGE_CHECK);
        status = XSP_STATUS_ERROR;
      }
    }
  }

  if (status == XSP_STATUS_OK){
    *buffer_length = (uint32_t) (total_tf);
  }
  return status;
}

int LibXspressWrapper::set_window(int chan, int sca, int llm, int hlm)
{
  int status = XSP_STATUS_OK;
  int xsp_status;

  LOG4CXX_DEBUG_LEVEL(1, logger_, "set_window called with chan=" << chan <<
                                  " sca=" << sca << " llm=" << llm << " hlm=" << hlm);

  if (llm > hlm){
    checkErrorCode("set_window SCA low limit is higher than high limit", XSP3_RANGE_CHECK);
    status = XSP_STATUS_ERROR;
  } else {
    xsp_status = xsp3_set_window(xsp_handle_, chan, sca, llm, hlm);
    if (xsp_status != XSP3_OK) {
      checkErrorCode("xsp3_set_window", xsp_status);
      status = XSP_STATUS_ERROR;
    }
  }
  return status;
}

int LibXspressWrapper::set_sca_thresh(int chan, int value)
{
  int status = XSP_STATUS_OK;
  int xsp_status;

  LOG4CXX_DEBUG_LEVEL(1, logger_, "set_sca_thresh called with chan=" << chan <<
                                  " value=" << value);

  xsp_status = xsp3_set_good_thres(xsp_handle_, chan, value);
  if (xsp_status != XSP3_OK) {
    checkErrorCode("xsp3_set_good_thres", xsp_status, true);
    status = XSP_STATUS_ERROR;
  }
  return status;
}

int LibXspressWrapper::set_trigger_input(bool list_mode)
{
  int status = XSP_STATUS_OK;
  int xsp_status;
  Xsp3TriggerMux trig_mux;
  memset(&trig_mux, 0, sizeof(Xsp3TriggerMux));

  if (list_mode){
    trig_mux.trig_sel[0] = 0;
    trig_mux.trig_sel[1] = 2;
    trig_mux.trig_sel[2] = 1;
    trig_mux.trig_sel[3] = 0;
  } else {
    for (int i = 0; i < 4; i++){
      trig_mux.trig_sel[i] = i;
    }
  }

  return status;
}

} /* namespace Xspress */
