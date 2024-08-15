/*
 * LibXspressSimulator.cpp
 *
 *  Created on: 24 Sep 2021
 *      Author: Diamond Light Source
 */

#include <stdio.h>
#include "dirent.h"

#include "LibXspressSimulator.h"
#include "DebugLevelLogger.h"


namespace Xspress
{
const int N_RESGRADES = 16;

double rand_gen() {
   // return a uniformly distributed random value
   return ( (double)(rand()) + 1. )/( (double)(RAND_MAX) + 1. );
}
double normalRandom() {
   // return a normally distributed random value
   double v1=rand_gen();
   double v2=rand_gen();
   return cos(2*3.14*v2)*sqrt(-2.*log(v1));
}

/** Construct a new LibXspressSimulator class.
 *
 * The constructor sets up logging used within the class, and initialises
 * variables.
 */
LibXspressSimulator::LibXspressSimulator() :
  num_frames_(0),
  max_frames_(1),
  exposure_time_(1.0),
  acquisition_state_(false)
{
  logger_ = log4cxx::Logger::getLogger("Xspress.LibXspressSimulator");
  OdinData::configure_logging_mdc(OdinData::app_path.c_str());
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Constructing LibXspressSimulator");

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

  // Create simulated MCA
  memset(simulated_mca_, 0, 4096*sizeof(int32_t));
  for (int i=0; i<20000; ++i) {
    double number = normalRandom()*50.0 + 2000.0;
    if ((number>=0.0)&&(number<4096.0)) ++simulated_mca_[int(number)];
  }
}

/** Destructor for XspressDetector class.
 *
 */
LibXspressSimulator::~LibXspressSimulator()
{

}

// TODO: Feature set required

std::string LibXspressSimulator::getVersionString()
{
  std::string version = "sim_0.0.0";
  return version;
}

void LibXspressSimulator::checkErrorCode(const std::string& prefix, int code)
{
  checkErrorCode(prefix, code, false);
}

void LibXspressSimulator::checkErrorCode(const std::string& prefix, int code, bool add_xsp_error)
{
  std::stringstream err;
  if (code != XSP3_OK){
    err << prefix << " [SIM] error [" << code << "]";
    setErrorString(err.str());
  }
}

int LibXspressSimulator::configure_mca(int num_cards,                 // Number of XSPRESS cards
                                     int num_frames,                // Number of 4096 energy bin spectra timeframes
                                     const std::string& ip_address, // Base IP address
                                     int port,                      // Base port number override (-1 does not override)
                                     int max_channels,              // Set the maximum number of channels
                                     int debug,                     // Enable debug messages
                                     int verbose                    // Enable verbose debug messages
                                     )
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "[SIM] Xspress wrapper calling xsp3_config");
  num_cards_ = num_cards;
  max_channels_ = max_channels;
  return XSP_STATUS_OK;
}


int LibXspressSimulator::configure_list(int num_cards,                 // Number of XSPRESS cards
                                      int num_frames,                // Number of 4096 energy bin spectra timeframes
                                      const std::string& ip_address, // Base IP address
                                      int port,                      // Base port number override (-1 does not override)
                                      int max_channels,              // Set the maximum number of channels
                                      int debug                      // Enable debug messages
                                      )
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "[SIM] Xspress wrapper calling xsp3_config (list mode)");
  num_cards_ = num_cards;
  max_channels_ = max_channels;
  return XSP_STATUS_OK;
}


int LibXspressSimulator::close_connection()
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "[SIM] Xspress wrapper calling xsp3_close");
  return XSP_STATUS_OK;
}

int LibXspressSimulator::save_settings(const std::string& save_path)
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "[SIM] Xspress wrapper calling xsp3_save_settings");
  return XSP_STATUS_OK;
}

int LibXspressSimulator::restore_settings(const std::string& restore_path)
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "[SIM] Xspress wrapper calling xsp3_restore_settings_and_clock");
  return XSP_STATUS_OK;
}

int LibXspressSimulator::setup_format_run_mode(bool list_mode, bool use_resgrades, int max_channels, int& num_aux_data)
{
  int status = XSP_STATUS_OK;
  LOG4CXX_DEBUG_LEVEL(1, logger_, "[SIM] Xspress wrapper setting up list mode, resgrades and calling xsp3_format_run");
  if (use_resgrades){
    num_aux_data = N_RESGRADES;
  } else {
    num_aux_data = 1;
  }
  return status;
}

/*
int LibXspressSimulator::setup_resgrades(bool use_resgrades, int max_channels, int& num_aux_data)
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "[SIM] Xspress wrapper setting up resgrades and calling xsp3_format_run");
  if (use_resgrades){
    num_aux_data = N_RESGRADES;
  } else {
    num_aux_data = 1;
  }
  use_resgrades_ = use_resgrades;
  return XSP_STATUS_OK;
}
*/

int LibXspressSimulator::set_run_flags(int run_flags)
{
  LOG4CXX_INFO(logger_, "\n\n[SIM] Xspress wrapper calling xsp3_set_run_flags with " << run_flags << "\n\n");
  return XSP_STATUS_OK;
}

int LibXspressSimulator::set_dtc_energy(double dtc_energy)
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "[SIM] Xspress wrapper calling xsp3_setDeadTimeCalculationEnergy with " << dtc_energy);
  return XSP_STATUS_OK;
}

int LibXspressSimulator::get_clock_period(double& clock_period)
{
  int status = XSP_STATUS_OK;
  LOG4CXX_DEBUG_LEVEL(1, logger_, "[SIM] Xspress wrapper calling xsp3_get_clock_period");
  // Read the clock period
  clock_period = 0.0;
  return status;
}


/**
 * Read the SCA window limits (for SCA 5 and 6) and threshold for SCA 4, for each channel.
 */
int LibXspressSimulator::read_sca_params(int max_channels,
                                       std::vector<uint32_t>& sca5_low,
                                       std::vector<uint32_t>& sca5_high,
                                       std::vector<uint32_t>& sca6_low,
                                       std::vector<uint32_t>& sca6_high,
                                       std::vector<uint32_t>& sca4_threshold
                                       )
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "[SIM] Xspress wrapper calling xsp3_get_window and xsp3_get_good_thres");

  sca5_low = sca5_low_;
  sca5_high = sca5_high_;
  sca6_low = sca6_low_;
  sca6_high = sca6_high_;
  sca4_threshold = sca4_threshold_;

  return XSP_STATUS_OK;
}

int LibXspressSimulator::check_connected_channels(std::vector<bool>& cards_connected, std::vector<int>& channels_connected)
{
  int status = XSP_STATUS_OK;

  int found_chans = 0;
  if (cards_connected.size() != num_cards_){
    LOG4CXX_DEBUG_LEVEL(0, logger_, "cards_connected.size(): " << cards_connected.size() << "num_cards: " << num_cards_);
    setErrorString("cards_connected vector has the incorrect dimension for the detector reported number of cards");
    status = XSP_STATUS_ERROR;
  } else if (channels_connected.size() != num_cards_){
    setErrorString("channels_connected vector has the incorrect dimension for the detector reported number of cards");
    status = XSP_STATUS_ERROR;
  } else {
    if(max_channels_ < 10)
    {
      int chan_per_card = int(max_channels_/num_cards_);
      for (int card = 0; card < num_cards_; card++) {
        cards_connected[card] = true;
        channels_connected[card] = chan_per_card;
        found_chans += channels_connected[card];
        LOG4CXX_INFO(logger_, "[SIM] Card " << card << " connected with " << channels_connected[card] << " channels");
      }
    }
    else
    {
      for (int card = 0; card < num_cards_; card++) {
        cards_connected[card] = true;
        if (card == num_cards_ - 1){
          channels_connected[card] = int(max_channels_ % 10);
        } else {
          channels_connected[card] = 10;
        }
        found_chans += channels_connected[card];
        LOG4CXX_INFO(logger_, "[SIM] Card " << card << " connected with " << channels_connected[card] << " channels");
      }
    }
  }
  return status;
}

int LibXspressSimulator::read_frames(int max_channels, std::vector<int32_t>& frame_counters)
{
  int status = XSP_STATUS_OK;
  LOG4CXX_DEBUG_LEVEL(3, logger_, "[SIM] Xspress wrapper calling xsp3_resolve_path and using Xsp3Sys[].histogram[].cur_tf_ext");

  if (frame_counters.size() != max_channels){
    setErrorString("Frame counter vector has a different dimension to the number of channels");
    status = XSP_STATUS_ERROR;
  }

  if (status == XSP_STATUS_OK){
      LOG4CXX_DEBUG_LEVEL(3, logger_, "[SIM] num_frames_ : " << num_frames_);
    for (u_int32_t chan = 0; chan < max_channels; chan++) {
      frame_counters[chan] = num_frames_;
    }  
  }
  return status;
}

int LibXspressSimulator::read_temperatures(std::vector<float>& t0,
                                         std::vector<float>& t1,
                                         std::vector<float>& t2,
                                         std::vector<float>& t3,
                                         std::vector<float>& t4,
                                         std::vector<float>& t5)
{
  int status = XSP_STATUS_OK;
  LOG4CXX_DEBUG_LEVEL(5, logger_, "[SIM] Xspress wrapper calling xsp3_i2c_read_adc_temp and xsp3_i2c_read_fem_temp");
  if (t0.size() != num_cards_){
    setErrorString("temperature vector 0 has a different size to the number of cards");
    status = XSP_STATUS_ERROR;
  }
  if (t1.size() != num_cards_){
    setErrorString("temperature vector 1 has a different size to the number of cards");
    status = XSP_STATUS_ERROR;
  }
  if (t2.size() != num_cards_){
    setErrorString("temperature vector 2 has a different size to the number of cards");
    status = XSP_STATUS_ERROR;
  }
  if (t3.size() != num_cards_){
    setErrorString("temperature vector 3 has a different size to the number of cards");
    status = XSP_STATUS_ERROR;
  }
  if (t4.size() != num_cards_){
    setErrorString("temperature vector 4 has a different size to the number of cards");
    status = XSP_STATUS_ERROR;
  }
  if (t5.size() != num_cards_){
    setErrorString("temperature vector 5 has a different size to the number of cards");
    status = XSP_STATUS_ERROR;
  }

  if (status == XSP_STATUS_OK){
    for (int card = 0; card < num_cards_; card++) {
      t0[card] = 20.0 + (float(card)*10.0);
      t1[card] = 21.0 + (float(card)*10.0);
      t2[card] = 22.0 + (float(card)*10.0);
      t3[card] = 23.0 + (float(card)*10.0);
      t4[card] = 24.0 + (float(card)*10.0);
      t5[card] = 25.0 + (float(card)*10.0);
    }
  }
  return status;
}

int LibXspressSimulator::read_dropped_frames(std::vector<int32_t>& dropped_frames)
{
  int status = XSP_STATUS_OK;
  LOG4CXX_DEBUG_LEVEL(5, logger_, "[SIM] Xspress wrapper using Xsp3Sys[].histogram[].dropped_frames");
  if (dropped_frames.size() != num_cards_){
    setErrorString("dropped frames vector has a different size to the number of cards");
    status = XSP_STATUS_ERROR;
  }

  if (status == XSP_STATUS_OK){
    for (int card = 0; card < num_cards_; card++) {
      dropped_frames[card] = 0;
    }
  }
  return status;
}

int LibXspressSimulator::read_dtc_params(int max_channels,
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

  LOG4CXX_DEBUG_LEVEL(1, logger_, "[SIM] Xspress wrapper calling xsp3_getDeadtimeCorrectionParameters2");

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
  return status;
}
/*
int LibXspressSimulator::read_dtc_params(int max_channels,
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


int LibXspressSimulator::write_dtc_params(int max_channels,
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

  LOG4CXX_DEBUG_LEVEL(1, logger_, "[SIM] Xspress wrapper calling xsp3_setDeadtimeCorrectionParameters2");
  /*
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
  */
  return status;
}

int LibXspressSimulator::setTriggerMode(int frames,
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

  LOG4CXX_DEBUG_LEVEL(1, logger_, "[SIM] Xspress wrapper calling xsp3_itfg_setup and xsp3_set_timing");  

  // Record the number of frames to be acquired
  max_frames_ = frames;
  // Record exposure time
  exposure_time_ = exposure_time;

  /*
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
  */
  return status;
}

int LibXspressSimulator::get_num_frames_read(int32_t *frames)
{
  int status = XSP_STATUS_OK;
  // Calculate the current acquisition duration and from that the
  // number of frames that have acquired
  if (acquisition_state_){
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    int32_t elapsed_time = (now - acq_start_time_).total_milliseconds();
    num_frames_ = (int32_t)floor((double)elapsed_time / 1000.0 / exposure_time_);
    if (num_frames_ >= max_frames_){
      num_frames_ = max_frames_;
      acquisition_state_ = false;
    }
  }
  *frames = num_frames_;
  return status;
}

int LibXspressSimulator::get_num_scalars(uint32_t *num_scalars)
{
  *num_scalars = XSP3_SW_NUM_SCALERS;
}

int LibXspressSimulator::histogram_circ_ack(int channel,
                                          uint32_t frame_number,
                                          uint32_t number_of_frames,
                                          uint32_t max_channels)
{
  int status = XSP_STATUS_OK;
  /*
  int xsp_status = xsp3_histogram_circ_ack(xsp_handle_, channel, frame_number, max_channels, number_of_frames);
  if (xsp_status < XSP3_OK) {
    checkErrorCode("xsp3_histogram_circ_ack", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  */
  return status;
}

int LibXspressSimulator::histogram_start(int card)
{
  int status = XSP_STATUS_OK;
  num_frames_=0;
  // Hook into the final start (card = 0)
  if (card == 0){
    // Set the number of frames to 0
    num_frames_ = 0;
    // Set the acquisition state to true
    acquisition_state_ = true;
    // Record the start time so we know how many frames have acquired
    acq_start_time_ = boost::posix_time::microsec_clock::local_time();
  }

  /*
  int xsp_status = xsp3_histogram_start(xsp_handle_, card);
  if (xsp_status < XSP3_OK) {
    checkErrorCode("xsp3_histogram_start", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  */
  return status;
}

int LibXspressSimulator::histogram_arm(int card)
{
  int status = XSP_STATUS_OK;
  /*
  int xsp_status = xsp3_histogram_arm(xsp_handle_, card);
  if (xsp_status < XSP3_OK) {
    checkErrorCode("xsp3_histogram_arm", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  */
  return status;
}

int LibXspressSimulator::histogram_continue(int card)
{
  int status = XSP_STATUS_OK;
  num_frames_++;
  /*
  int xsp_status = xsp3_histogram_continue(xsp_handle_, card);
  if (xsp_status < XSP3_OK) {
    checkErrorCode("xsp3_histogram_continue", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  */
  return status;
}

int LibXspressSimulator::histogram_pause(int card)
{
  int status = XSP_STATUS_OK;
  /*
  int xsp_status = xsp3_histogram_pause(xsp_handle_, card);
  if (xsp_status < XSP3_OK){
    checkErrorCode("xsp3_histogram_pause", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  */
  return status;
}

int LibXspressSimulator::histogram_stop(int card)
{
  int status = XSP_STATUS_OK;
  /*
  int xsp_status = xsp3_histogram_stop(xsp_handle_, card);
  if (xsp_status < XSP3_OK){
    checkErrorCode("xsp3_histogram_stop", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  */
  return status;
}

int LibXspressSimulator::string_trigger_mode_to_int(const std::string& mode)
{
  int trigger_mode = -1;
  if (trigger_modes_.count(mode) > 0){
    trigger_mode = trigger_modes_[mode];
    LOG4CXX_DEBUG_LEVEL(1, logger_, "[SIM] Converting trigger mode " << mode << " into integer: " << trigger_mode);
  } else {
    LOG4CXX_ERROR(logger_, "[SIM] Invalid trigger mode requested: " << mode);
  }
  return trigger_mode;
}

int LibXspressSimulator::scaler_read(uint32_t *buffer,
                                   uint32_t tf,
                                   uint32_t num_tf,
                                   uint32_t start_chan,
                                   uint32_t num_chan)
{
  for (int index = 0; index < num_tf * num_chan; index++){
    buffer[index] = 0;
  }
  int status = XSP_STATUS_OK;
  /*
  int xsp_status = xsp3_scaler_read(xsp_handle_, buffer, 0, start_chan, tf, XSP3_SW_NUM_SCALERS, num_chan, num_tf);
  if (xsp_status < XSP3_OK){
    checkErrorCode("xsp3_scaler_read", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  */
  return status;
}

int LibXspressSimulator::calculate_dtc_factors(uint32_t *scalers,
                                             double *dtc_factors,
                                             double *inp_est,
                                             uint32_t frames,
                                             uint32_t start_chan,
                                             uint32_t num_chan)
{
  for (int index = 0; index < num_chan; index++){
    dtc_factors[index] = index + 1.0;
    inp_est[index] = index - 1.0;
  }
  int status = XSP_STATUS_OK;
  /*
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
  */
  return status;
}

int LibXspressSimulator::histogram_memcpy(uint32_t *buffer,
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

  uint32_t *local_buffer;
  // uint32_t local_buffer[4096];
  for(int chan=start_chan; chan < start_chan + num_chan; chan++)
  {
    // for (int i = 0; i < 4096; i++){
    //   local_buffer[i] = (uint32_t)((double)simulated_mca_[i] * (9.0 + rand_gen()));    
    // }
    local_buffer = (uint32_t *)(simulated_mca_);    
    memcpy(buffer, local_buffer, 4096*sizeof(int32_t));
    buffer+=num_eng*num_aux;
  }


  /*
  if (xsp_handle_ < 0 || xsp_handle_ >= XSP3_MAX_PATH || !Xsp3Sys[xsp_handle_].valid){
    checkErrorCode("histogram_memcpy", XSP3_INVALID_PATH);
    status = XSP_STATUS_ERROR;
  } else {
    circ_buffer = (bool)(Xsp3Sys[xsp_handle_].run_flags & XSP3_RUN_FLAGS_CIRCULAR_BUFFER);

    if (Xsp3Sys[xsp_handle_].features.generation == XspressGen3Mini){
      xsp_status = xsp3m_histogram_read_frames(xsp_handle_, buffer, 0, start_chan, tf, num_eng, num_chan, num_tf);
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
  */
  return status;
}

int LibXspressSimulator::validate_histogram_dims(uint32_t num_eng,
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

  /*
  LOG4CXX_DEBUG_LEVEL(1, logger_, "validate_histogram_dims called with num_eng=" << num_eng <<
                                  " num_aux=" << num_aux << " start_chan=" << start_chan <<
                                  " num_chan=" << num_chan);

  if (xsp_handle_ < 0 || xsp_handle_ >= XSP3_MAX_PATH || !Xsp3Sys[xsp_handle_].valid) {
    checkErrorCode("validate_histogram_dims", XSP3_INVALID_PATH);
    status = XSP_STATUS_ERROR;
  }

  if (status == XSP_STATUS_OK){
    if ((xsp_status = xsp3_get_format(xsp_handle_, start_chan, &nbins_eng, &nbins_aux1, &nbins_aux2, &total_tf)) < 0) {
      checkErrorCode("xsp3_get_format", xsp_status);
      status = XSP_STATUS_ERROR;
    }
  }

  if (status == XSP_STATUS_OK){
    if (num_chan > 1) {
      int ne, na1, na2, nt;
      for (c = start_chan; c < (start_chan + num_chan); c++) {
        if ((xsp_status = xsp3_get_format(xsp_handle_, c, &ne, &na1, &na2, &nt)) < 0) {
          checkErrorCode("xsp3_get_format", xsp_status);
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
  */
  return status;
}

int LibXspressSimulator::set_window(int chan, int sca, int llm, int hlm)
{
  int status = XSP_STATUS_OK;
  int xsp_status;

  LOG4CXX_DEBUG_LEVEL(1, logger_, "[SIM] set_window called with chan=" << chan <<
                                  " sca=" << sca << " llm=" << llm << " hlm=" << hlm);

  if (llm > hlm){
    checkErrorCode("[SIM] set_window SCA low limit is higher than high limit", XSP3_RANGE_CHECK);
    status = XSP_STATUS_ERROR;
  } else {
    /*
    xsp_status = xsp3_set_window(xsp_handle_, chan, sca, llm, hlm);
    if (xsp_status != XSP3_OK) {
      checkErrorCode("xsp3_set_window", xsp_status);
      status = XSP_STATUS_ERROR;
    }
    */
  }
  return status;
}

int LibXspressSimulator::set_sca_thresh(int chan, int value)
{
  int status = XSP_STATUS_OK;
  int xsp_status;

  LOG4CXX_DEBUG_LEVEL(1, logger_, "[SIM] set_sca_thresh called with chan=" << chan <<
                                  " value=" << value);

  /*
  xsp_status = xsp3_set_good_thres(xsp_handle_, chan, value);
  if (xsp_status != XSP3_OK) {
    checkErrorCode("xsp3_set_good_thres", xsp_status);
    status = XSP_STATUS_ERROR;
  }
  */
  return status;
}

int LibXspressSimulator::set_trigger_input(bool list_mode)
{
  // This is a no op for the simulator
  return XSP_STATUS_OK;
}

int LibXspressSimulator::setup_clocks(int num_cards)
{
  // This is a no op for the simulator
  return XSP_STATUS_OK;
}

} /* namespace Xspress */
