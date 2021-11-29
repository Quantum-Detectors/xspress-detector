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
#include "XspressDAQ.h"

using namespace log4cxx;
using namespace log4cxx::helpers;

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
  void setErrorString(const std::string& error);
  std::string getErrorString();
  void checkErrorCode(const std::string& prefix, int code);
  int connect();
  int checkConnected();
  int setupChannels();
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

  // Getter and Setters
  void setXspNumCards(int num_cards);
  int getXspNumCards();
  void setXspNumTf(int num_tf);
  int getXspNumTf();
  void setXspBaseIP(const std::string& address);
  std::string getXspBaseIP();
  void setXspMaxChannels(int max_channels);
  int getXspMaxChannels();
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
  void setXspTriggerMode(const std::string& mode);
  std::string getXspTriggerMode();
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
  void setXspDAQEndpoints(std::vector<std::string> endpoints);
  std::vector<std::string> getXspDAQEndpoints();
  
private:
  /** libxspress wrapper object */
  LibXspressWrapper             detector_;
  /** Pointer to DAQ object */
  boost::shared_ptr<XspressDAQ>   daq_;
  /** Simulation flag for this wrapper */
  bool                          simulated_;
  /** Connected flag */
  bool                          connected_;
  /** Is the detector acquiring */
  bool                          acquiring_;
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
  std::string                   xsp_trigger_mode_;
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


  std::vector<int>              pDTCi_;
  std::vector<double>           pDTCd_;
  
  std::vector<std::pair <int, int> > cardChanMap;

  /** Pointer to the logging facility */
  log4cxx::LoggerPtr            logger_;

  /** Last error string description */
  std::string                   error_string_;
};

} /* namespace Xspress */

#endif /* XspressDetector_H_ */
