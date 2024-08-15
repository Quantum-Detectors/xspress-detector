/*
 * LibXspressWrapper.h
 *
 *  Created on: 22 Sep 2021
 *      Author: Diamond Light Source
 */

#ifndef LibXspressWrapper_H_
#define LibXspressWrapper_H_

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

#include "logging.h"
#include "xspress3.h"
#include "ILibXspress.h"

using namespace log4cxx;
using namespace log4cxx::helpers;

#define XSP_STATUS_OK     0
#define XSP_STATUS_ERROR -1

#define XSP3_NUM_DTC_FLOAT_PARAMS           8
#define XSP3_NUM_DTC_INT_PARAMS             1
#define XSP3_DTC_FLAGS                      0
#define XSP3_DTC_AEO                        0
#define XSP3_DTC_AEG                        1
#define XSP3_DTC_AERO                       2
#define XSP3_DTC_AERG                       3
#define XSP3_DTC_IWO                        4
#define XSP3_DTC_IWG                        5
#define XSP3_DTC_IWRO                       6
#define XSP3_DTC_IWRG                       7

// Trigger mode enum valeus
#define TM_SOFTWARE                         0
#define TM_TTL_RISING_EDGE                  1
#define TM_BURST                            2
#define TM_TTL_VETO_ONLY                    3
#define TM_SOFTWARE_START_STOP              4
#define TM_IDC                              5
#define TM_TTL_BOTH                         6
#define TM_LVDS_VETO_ONLY                   7
#define TM_LVDS_BOTH                        8

// Trigger mode strings values
#define TM_SOFTWARE_STR                     "software"
#define TM_TTL_RISING_EDGE_STR              "ttl_rising"
#define TM_BURST_STR                        "burst"
#define TM_TTL_VETO_ONLY_STR                "ttl_veto_only"
#define TM_SOFTWARE_START_STOP_STR          "software_start_stop"
#define TM_IDC_STR                          "idc"
#define TM_TTL_BOTH_STR                     "ttl_both"
#define TM_LVDS_VETO_ONLY_STR               "lvds_veto_only"
#define TM_LVDS_BOTH_STR                    "lvds_both"

namespace Xspress
{

/**
 * The XspressWrapper class provides an OO implementation based around the C
 * libxspress library.  This class is designed to abstract specific libxspress
 * calls.
 */
class LibXspressWrapper : public ILibXspress
{
public:
  LibXspressWrapper();
  virtual ~LibXspressWrapper();
  std::string getVersionString();
  void checkErrorCode(const std::string& prefix, int code);
  void checkErrorCode(const std::string& prefix, int code, bool add_xsp_error);

  int configure_mca(int num_cards,                 // Number of XSPRESS cards
                    int num_frames,                // Number of 4096 energy bin spectra timeframes
                    const std::string& ip_address, // Base IP address
                    int port,                      // Base port number override (-1 does not override)
                    int max_channels,              // Set the maximum number of channels
                    int debug,                     // Enable debug messages
                    int verbose                    // Enable verbose debug messages
                    );

  int configure_list(int num_cards,                 // Number of XSPRESS cards
                     int num_frames,                // Number of 4096 energy bin spectra timeframes
                     const std::string& ip_address, // Base IP address
                     int port,                      // Base port number override (-1 does not override)
                     int max_channels,              // Set the maximum number of channels
                     int debug                      // Enable debug messages
                     );

  int close_connection();

  int save_settings(const std::string& save_path);
  int restore_settings(const std::string& restore_path);
  int setup_format_run_mode(bool list_mode, bool use_resgrades, int max_channels, int& num_aux_data);
  int set_run_flags(int run_flags);
  int set_dtc_energy(double dtc_energy);
  int get_clock_period(double& clock_period);
  int read_sca_params(int max_channels,
                      std::vector<uint32_t>& sca5_low,
                      std::vector<uint32_t>& sca5_high,
                      std::vector<uint32_t>& sca6_low,
                      std::vector<uint32_t>& sca6_high,
                      std::vector<uint32_t>& sca4_threshold
                      );
  int check_connected_channels(std::vector<bool>& cards_connected, std::vector<int>& channels_connected);
  int read_frames(int max_channels, std::vector<int32_t>& frame_counters);
  int read_temperatures(std::vector<float>& t0,
                        std::vector<float>& t1,
                        std::vector<float>& t2,
                        std::vector<float>& t3,
                        std::vector<float>& t4,
                        std::vector<float>& t5);
  int read_dropped_frames(std::vector<int32_t>& dropped_frames);
  int read_dtc_params(int max_channels,
                      std::vector<int>& dtc_flags,
                      std::vector<double>& dtc_all_event_off,
                      std::vector<double>& dtc_all_event_grad,
                      std::vector<double>& dtc_all_event_rate_off,
                      std::vector<double>& dtc_all_event_rate_grad,
                      std::vector<double>& dtc_in_window_off,
                      std::vector<double>& dtc_in_window_grad,
                      std::vector<double>& dtc_in_window_rate_off,
                      std::vector<double>& dtc_in_window_rate_grad);
  int write_dtc_params(int max_channels,
                       std::vector<int>& dtc_flags,
                       std::vector<double>& dtc_all_event_off,
                       std::vector<double>& dtc_all_event_grad,
                       std::vector<double>& dtc_all_event_rate_off,
                       std::vector<double>& dtc_all_event_rate_grad,
                       std::vector<double>& dtc_in_window_off,
                       std::vector<double>& dtc_in_window_grad,
                       std::vector<double>& dtc_in_window_rate_off,
                       std::vector<double>& dtc_in_window_rate_grad);
  int mapTimeFrameSource(Xsp3Timing *api_mode,
                         int *api_itfg_mode,
                         int trigger_mode,
                         int debounce,
                         int invert_f0,
                         int invert_veto);
  int setTriggerMode(int frames,
                     double exposure_time,
                     double clock_period,
                     int trigger_mode,
                     int debounce,
                     int invert_f0,
                     int invert_veto);
  int get_num_frames_read(int32_t *frames);
  int get_num_scalars(uint32_t *num_scalars);
  int histogram_circ_ack(int channel,
                         uint32_t frame_number,
                         uint32_t number_of_frames,
                         uint32_t max_channels);
  int histogram_start(int card);
  int histogram_arm(int card);
  int histogram_continue(int card);
  int histogram_pause(int card);
  int histogram_stop(int card);
  int string_trigger_mode_to_int(const std::string& mode);
  int scaler_read(uint32_t *buffer,
                  uint32_t tf,
                  uint32_t num_tf,
                  uint32_t start_chan,
                  uint32_t num_chan);
  int calculate_dtc_factors(uint32_t *scalers,
                            double *dtc_factors,
                            double *inp_est,
                            uint32_t frames,
                            uint32_t start_chan,
                            uint32_t num_chan);
  int histogram_memcpy(uint32_t *buffer,
                       uint32_t tf, 
                       uint32_t num_tf,
                       uint32_t total_tf,
                       uint32_t num_eng,
                       uint32_t num_aux,
                       uint32_t start_chan,
                       uint32_t num_chan);
  int validate_histogram_dims(uint32_t num_eng,
                              uint32_t num_aux,
                              uint32_t start_chan,
                              uint32_t num_chan,
                              uint32_t *buffer_length);
  int set_window(int chan, int sca, int llm, int hlm);
  int set_sca_thresh(int chan, int value);
  int set_trigger_input(bool list_mode);
  int setup_clocks(int num_cards);

  static const int runFlag_MCA_SPECTRA_;
  static const int runFlag_SCALERS_ONLY_;
  static const int runFlag_PLAYB_MCA_SPECTRA_;


private:
  /** Handle used by the libxspress library */
  int                           xsp_handle_;
  /** String representation of trigger modes */
  std::map<std::string, int>    trigger_modes_;

};

} /* namespace Xspress */

#endif /* LibXspressWrapper_H_ */
