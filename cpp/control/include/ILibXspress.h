/*
 * ILibXspress.h
 *
 *  Created on: 06 Jun 2022
 *      Author: Diamond Light Source
 */

#ifndef ILibXspress_H_
#define ILibXspress_H_

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

#include "logging.h"
#include "xspress3.h"

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
class ILibXspress
{
public:
  virtual std::string getVersionString() = 0;
  void setErrorString(const std::string& error);
  std::string getErrorString();
  virtual void checkErrorCode(const std::string& prefix, int code) = 0;
  virtual void checkErrorCode(const std::string& prefix, int code, bool add_xsp_error) = 0;

  virtual int configure_mca(int num_cards,                 // Number of XSPRESS cards
                    int num_frames,                // Number of 4096 energy bin spectra timeframes
                    const std::string& ip_address, // Base IP address
                    int port,                      // Base port number override (-1 does not override)
                    int max_channels,              // Set the maximum number of channels
                    int debug,                     // Enable debug messages
                    int verbose                    // Enable verbose debug messages
                    ) = 0;

  virtual int configure_list(int num_cards,                 // Number of XSPRESS cards
                     int num_frames,                // Number of 4096 energy bin spectra timeframes
                     const std::string& ip_address, // Base IP address
                     int port,                      // Base port number override (-1 does not override)
                     int max_channels,              // Set the maximum number of channels
                     int debug                      // Enable debug messages
                     ) = 0;

  virtual int close_connection() = 0;

  virtual int save_settings(const std::string& save_path) = 0;
  virtual int restore_settings(const std::string& restore_path) = 0;
  virtual int setup_format_run_mode(bool list_mode, bool use_resgrades, int max_channels, int& num_aux_data) = 0;
  virtual int set_run_flags(int run_flags) = 0;
  virtual int set_dtc_energy(double dtc_energy) = 0;
  virtual int get_clock_period(double& clock_period) = 0;
  virtual int read_sca_params(int max_channels,
                      std::vector<uint32_t>& sca5_low,
                      std::vector<uint32_t>& sca5_high,
                      std::vector<uint32_t>& sca6_low,
                      std::vector<uint32_t>& sca6_high,
                      std::vector<uint32_t>& sca4_threshold
                      ) = 0;
  virtual int check_connected_channels(std::vector<bool>& cards_connected, std::vector<int>& channels_connected) = 0;
  virtual int read_frames(int max_channels, std::vector<int32_t>& frame_counters) = 0;
  virtual int read_temperatures(std::vector<float>& t0,
                        std::vector<float>& t1,
                        std::vector<float>& t2,
                        std::vector<float>& t3,
                        std::vector<float>& t4,
                        std::vector<float>& t5) = 0;
  virtual int read_dropped_frames(std::vector<int32_t>& dropped_frames) = 0;
  virtual int read_dtc_params(int max_channels,
                      std::vector<int>& dtc_flags,
                      std::vector<double>& dtc_all_event_off,
                      std::vector<double>& dtc_all_event_grad,
                      std::vector<double>& dtc_all_event_rate_off,
                      std::vector<double>& dtc_all_event_rate_grad,
                      std::vector<double>& dtc_in_window_off,
                      std::vector<double>& dtc_in_window_grad,
                      std::vector<double>& dtc_in_window_rate_off,
                      std::vector<double>& dtc_in_window_rate_grad) = 0;
  virtual int write_dtc_params(int max_channels,
                       std::vector<int>& dtc_flags,
                       std::vector<double>& dtc_all_event_off,
                       std::vector<double>& dtc_all_event_grad,
                       std::vector<double>& dtc_all_event_rate_off,
                       std::vector<double>& dtc_all_event_rate_grad,
                       std::vector<double>& dtc_in_window_off,
                       std::vector<double>& dtc_in_window_grad,
                       std::vector<double>& dtc_in_window_rate_off,
                       std::vector<double>& dtc_in_window_rate_grad) = 0;
  virtual int setTriggerMode(int frames,
                     double exposure_time,
                     double clock_period,
                     int trigger_mode,
                     int debounce,
                     int invert_f0,
                     int invert_veto) = 0;
  virtual int get_num_frames_read(int32_t *frames) = 0;
  virtual int get_num_scalars(uint32_t *num_scalars) = 0;
  virtual int histogram_circ_ack(int channel,
                         uint32_t frame_number,
                         uint32_t number_of_frames,
                         uint32_t max_channels) = 0;
  virtual int histogram_start(int card) = 0;
  virtual int histogram_arm(int card) = 0;
  virtual int histogram_continue(int card) = 0;
  virtual int histogram_pause(int card) = 0;
  virtual int histogram_stop(int card) = 0;
  virtual int histogram_is_any_busy() = 0;
  virtual int string_trigger_mode_to_int(const std::string& mode) = 0;
  virtual int scaler_read(uint32_t *buffer,
                  uint32_t tf,
                  uint32_t num_tf,
                  uint32_t start_chan,
                  uint32_t num_chan) = 0;
  virtual int calculate_dtc_factors(uint32_t *scalers,
                            double *dtc_factors,
                            double *inp_est,
                            uint32_t frames,
                            uint32_t start_chan,
                            uint32_t num_chan) = 0;
  virtual int histogram_memcpy(uint32_t *buffer,
                       uint32_t tf, 
                       uint32_t num_tf,
                       uint32_t total_tf,
                       uint32_t num_eng,
                       uint32_t num_aux,
                       uint32_t start_chan,
                       uint32_t num_chan) = 0;
  virtual int validate_histogram_dims(uint32_t num_eng,
                              uint32_t num_aux,
                              uint32_t start_chan,
                              uint32_t num_chan,
                              uint32_t *buffer_length) = 0;
  virtual int set_window(int chan, int sca, int llm, int hlm) = 0;
  virtual int set_sca_thresh(int chan, int value) = 0;
  virtual int set_trigger_input(bool list_mode) = 0;
  virtual int setup_clocks(int num_cards) = 0;
  virtual int enable_list_mode_resets() = 0;
  virtual int set_channel_sources(int run_flags) = 0;

  static const int runFlag_MCA_SPECTRA_;
  static const int runFlag_SCALERS_ONLY_;
  static const int runFlag_PLAYB_MCA_SPECTRA_;

  protected:
    /** Last error string description */
    std::string                   error_string_;
    /** Pointer to the logging facility */
    log4cxx::LoggerPtr            logger_;


};

} /* namespace Xspress */

#endif /* ILibXspress_H_ */
