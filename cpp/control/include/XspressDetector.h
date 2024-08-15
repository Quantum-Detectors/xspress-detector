/*
 * XspressDetector.h
 *
 *  Created on: 22 Sep 2021
 *      Author: Diamond Light Source
 */

#ifndef XspressDetector_H_
#define XspressDetector_H_

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

#include "logging.h"
#include "xspress3.h"
#include "LibXspressWrapper.h"
#include "LibXspressSimulator.h"
#include "ILibXspress.h"
#include "XspressDAQ.h"

using namespace log4cxx;
using namespace log4cxx::helpers;

#define XSP_MODE_MCA "mca"
#define XSP_MODE_LIST "list"

#define XSP_SCA5_LIM 0
#define XSP_SCA6_LIM 1

#define DEFAULT_MAX_CHANNELS 8

namespace Xspress
{

/**
 * The XspressDetector class provides an OO implementation based around the C
 * libxspress library.  This class is designed to abstract specific libxspress
 * calls.
 * 
 * This class provides a control interface and a mechanism for reading out
 * data.  The data interface is optional in case another method is used to 
 * collect data directly from the detector hardware.
 */
class XspressDetector
{
public:
  XspressDetector(bool simulation=false);
  virtual ~XspressDetector();
  std::string getVersionString();
  void setErrorString(const std::string& error);
  std::string getErrorString();
  void checkErrorCode(const std::string& prefix, int code);
  int connect();
  int connect_mca_mode();
  int connect_list_mode();
  bool checkConnected();
  int disconnect();
  int setupChannels();
  int setupClocks();
  int enableDAQ();
  int checkSaveDir(const std::string& dir_name);
  int saveSettings();
  int restoreSettings();
  int readSCAParams();
  int readDTCParams();
  void readFemStatus();
  int writeDTCParams();
  int setTriggerMode();
  int startAcquisition();
  int stopAcquisition();
  int sendSoftwareTrigger();
  void reconnectRequired();
  bool getReconnectStatus();

  // Getter and Setters
  void setXspNumCards(int num_cards);
  int getXspNumCards();
  void setXspNumTf(int num_tf);
  int getXspNumTf();
  void setXspBaseIP(const std::string& address);
  std::string getXspBaseIP();
  void setXspMaxChannels(int max_channels);
  int getXspMaxChannels();
  void setXspMcaChannels(int mca_channels);
  int getXspMcaChannels();
  void setXspMaxSpectra(int max_spectra);
  int getXspMaxSpectra();
  void setXspDebug(int debug);
  int getXspDebug();
  void setXspConfigPath(const std::string& config_path);
  std::string getXspConfigPath();
  void setXspConfigSavePath(const std::string& config_save_path);
  std::string getXspConfigSavePath();
  void setXspUseResgrades(bool use);
  bool getXspUseResgrades();
  void setXspRunFlags(int flags);
  int getXspRunFlags();
  void setXspDTCEnergy(double energy);
  double getXspDTCEnergy();
  void setXspTriggerMode(int mode);
  int getXspTriggerMode();
  void setXspInvertF0(int invert_f0);
  int getXspInvertF0();
  void setXspInvertVeto(int invert_veto);
  int getXspInvertVeto();
  void setXspDebounce(int debounce);
  int getXspDebounce();
  void setXspExposureTime(double exposure);
  double getXspExposureTime();
  void setXspFrames(int frames);
  int getXspFrames();
  void setXspMode(const std::string& mode);
  std::string getXspMode();
  void setXspDAQEndpoints(std::vector<std::string> endpoints);
  std::vector<std::string> getXspDAQEndpoints();
  int setSca5LowLimits(std::vector<uint32_t> sca5_low_limit);
  std::vector<uint32_t> getSca5LowLimits();
  int setSca5HighLimits(std::vector<uint32_t> sca5_high_limit);
  std::vector<uint32_t> getSca5HighLimits();
  int setSca6LowLimits(std::vector<uint32_t> sca6_low_limit);
  std::vector<uint32_t> getSca6LowLimits();
  int setSca6HighLimits(std::vector<uint32_t> sca6_high_limit);
  std::vector<uint32_t> getSca6HighLimits();
  int setSca4Thresholds(std::vector<uint32_t> sca4_thresholds);
  std::vector<uint32_t> getSca4Thresholds();
  std::vector<int> getDtcFlags();
  std::vector<double> getDtcAllEventOff();
  std::vector<double> getDtcAllEventGrad();
  std::vector<double> getDtcAllEventRateOff();
  std::vector<double> getDtcAllEventRateGrad();
  std::vector<double> getDtcInWindowOff();
  std::vector<double> getDtcInWindowGrad();
  std::vector<double> getDtcInWindowRateOff();
  std::vector<double> getDtcInWindowRateGrad();
  std::vector<uint32_t> getLiveScalars(uint32_t index);
  std::vector<double> getLiveDtcFactors();
  std::vector<double> getLiveInpEst();
  bool getXspAcquiring();
  bool getXspAcqFailed();
  void resetXspAcqFailed();
  uint32_t getXspFramesRead();
  std::vector<float> getTemperature0();
  std::vector<float> getTemperature1();
  std::vector<float> getTemperature2();
  std::vector<float> getTemperature3();
  std::vector<float> getTemperature4();
  std::vector<float> getTemperature5();
  std::vector<int32_t> getXspFEMFramesRead();
  std::vector<int32_t> getXspFEMDroppedFrames();
  std::vector<int32_t> getChannelsConnected();
  std::vector<bool> getCardsConnected();
  
private:
  /** libxspress wrapper object */
  boost::shared_ptr<ILibXspress>  detector_;
  /** Pointer to DAQ object */
  boost::shared_ptr<XspressDAQ>   daq_;
  /** Simulation flag for this wrapper */
  bool                          simulated_;
  /** Connected flag */
  bool                          connected_;
  /** Is the detector acquiring */
  bool                          acquiring_;
  /** Did the acquisition fail */
  bool                          acq_failed_;
  /** Card connected flags */
  std::vector<bool>             cards_connected_;
  /** Channels connected through each card */
  std::vector<int>              channels_connected_;
  /** Reconnection required flag */
  bool                          reconnect_required_;
  /** Number of XSPRESS cards */
  int                           xsp_num_cards_;
  /** Number of 4096 energy bin spectra timeframes */
  int                           xsp_num_tf_;
  /** Base IP address */
  std::string                   xsp_base_IP_;
  /** Set the maximum number of channels */
  int                           xsp_max_channels_;
  /** Set the number of channels in mca mode */
  int                           xsp_mca_channels_;
  /** Set the maximum number of spectra (eg 4096) */
  int                           xsp_max_spectra_;
  /** Enable debug messages */
  int                           xsp_debug_;
  /** Path to Xspress configuration files */
  std::string                   xsp_config_path_;
  /** Path to Xspress configuration files */
  std::string                   xsp_config_save_path_;
  /** Use resgrades? */
  bool                          xsp_use_resgrades_;
  /** Number of auxiliary data items */
  int                           xsp_num_aux_data_;
  /** Number of auxiliary data items */
  int                           xsp_run_flags_;
  /** Have the DTC params been updated */
  bool                          xsp_dtc_params_updated_;
  /** DTC energy */
  double                        xsp_dtc_energy_;
  /** Clock period */
  double                        xsp_clock_period_;
  /** Trigger mode */
  int                           xsp_trigger_mode_;
  /** Invert f0 */
  int                           xsp_invert_f0_;
  /** Invert veto */
  int                           xsp_invert_veto_;
  /** Debounce */
  int                           xsp_debounce_;
  /** Exposure time */
  double                        xsp_exposure_time_;
  /** Number of frames */
  int                           xsp_frames_;
  /** Mode of operation */
  std::string                   xsp_mode_;
  /** DAQ endpoints */
  std::vector<std::string>      xsp_daq_endpoints_;
  
  /** Number of frames read out by each channel */
  std::vector<int32_t>          xsp_status_frames_;
  /** Number of dropped frames for each channel */
  std::vector<int32_t>          xsp_status_dropped_frames_;
  /** Temperature status items for each card */
  std::vector<float>            xsp_status_temperature_0_;
  std::vector<float>            xsp_status_temperature_1_;
  std::vector<float>            xsp_status_temperature_2_;
  std::vector<float>            xsp_status_temperature_3_;
  std::vector<float>            xsp_status_temperature_4_;
  std::vector<float>            xsp_status_temperature_5_;


  std::vector<uint32_t>         xsp_chan_sca5_low_lim_;
  std::vector<uint32_t>         xsp_chan_sca5_high_lim_;
  std::vector<uint32_t>         xsp_chan_sca6_low_lim_;
  std::vector<uint32_t>         xsp_chan_sca6_high_lim_;

  std::vector<uint32_t>         xsp_chan_sca4_threshold_;


  /** Dead time correction parameters */
  std::vector<int>              xsp_dtc_flags_;
  std::vector<double>           xsp_dtc_all_event_off_;
  std::vector<double>           xsp_dtc_all_event_grad_;
  std::vector<double>           xsp_dtc_all_event_rate_off_;
  std::vector<double>           xsp_dtc_all_event_rate_grad_;
  std::vector<double>           xsp_dtc_in_window_off_;
  std::vector<double>           xsp_dtc_in_window_grad_;
  std::vector<double>           xsp_dtc_in_window_rate_off_;
  std::vector<double>           xsp_dtc_in_window_rate_grad_;

  std::vector<std::pair <int, int> > cardChanMap;

  /** Pointer to the logging facility */
  log4cxx::LoggerPtr            logger_;

  /** Last error string description */
  std::string                   error_string_;

  /** StartAcquisition mutex for locking */
  boost::mutex                  start_acq_mutex_;

};

} /* namespace Xspress */

#endif /* XspressDetector_H_ */
