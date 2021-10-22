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

namespace Xspress
{

/**
 * The XspressWrapper class provides an OO implementation based around the C
 * libxspress library.  This class is designed to abstract specific libxspress
 * calls.
 */
class LibXspressWrapper
{
public:
  LibXspressWrapper();
  virtual ~LibXspressWrapper();
  void setErrorString(const std::string& error);
  std::string getErrorString();
  void checkErrorCode(const std::string& prefix, int code);

  int configure(int num_cards,                 // Number of XSPRESS cards
                int num_frames,                // Number of 4096 energy bin spectra timeframes
                const std::string& ip_address, // Base IP address
                int port,                      // Base port number override (-1 does not override)
                int max_channels,              // Set the maximum number of channels
                int debug,                     // Enable debug messages
                int verbose                    // Enable verbose debug messages
                );

  int save_settings(const std::string& save_path);
  int restore_settings(const std::string& restore_path);
  int setup_resgrades(bool use_resgrades, int max_channels, int& num_aux_data);
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
                      std::vector<int>& dtci,
                      std::vector<double>& dtcd,
                      bool& parameters_updated);
  int write_dtc_params(int max_channels,
                       std::vector<int>& dtci,
                       std::vector<double>& dtcd);
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
  int get_num_frames_read(int64_t *furthest_frame);
  int histogram_start(int card);
  int histogram_arm(int card);

  static const int runFlag_MCA_SPECTRA_;
  static const int runFlag_SCALERS_ONLY_;
  static const int runFlag_PLAYB_MCA_SPECTRA_;


private:
  /** Pointer to the logging facility */
  log4cxx::LoggerPtr            logger_;

  /** Handle used by the libxspress library */
  int                           xsp_handle_;
  /** Last error string description */
  std::string                   error_string_;

};

} /* namespace Xspress */

#endif /* LibXspressWrapper_H_ */
