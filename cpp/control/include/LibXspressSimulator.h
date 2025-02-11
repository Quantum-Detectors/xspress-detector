/*
 * LibXspressSimulator.h
 *
 *  Created on: 22 Sep 2021
 *      Author: Diamond Light Source
 */

#ifndef LibXspressSimulator_H_
#define LibXspressSimulator_H_

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

#include "logging.h"
#include "ILibXspress.h"

using namespace log4cxx;
using namespace log4cxx::helpers;


namespace Xspress
{

/**
 * The XspressWrapper class provides an OO implementation based around the C
 * libxspress library.  This class is designed to abstract specific libxspress
 * calls.
 */
class LibXspressSimulator : public ILibXspress
{
public:
  LibXspressSimulator();
  virtual ~LibXspressSimulator();
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
  int histogram_is_any_busy();
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
  int enable_list_mode_resets();

private:
  /** String representation of trigger modes */
  std::map<std::string, int>    trigger_modes_;

  /** Simulated storage variables */
  int                           num_cards_;
  int                           max_channels_;
  bool                          use_resgrades_;
  std::vector<uint32_t>         sca5_low_;
  std::vector<uint32_t>         sca5_high_;
  std::vector<uint32_t>         sca6_low_;
  std::vector<uint32_t>         sca6_high_;
  std::vector<uint32_t>         sca4_threshold_;
  int32_t                       num_frames_;
  int32_t                       max_frames_;
  double                        exposure_time_;
  bool                          acquisition_state_;
  boost::posix_time::ptime      acq_start_time_;
  uint32_t                      simulated_mca_[4096];
};

} /* namespace Xspress */

#endif /* LibXspressSimulator_H_ */
