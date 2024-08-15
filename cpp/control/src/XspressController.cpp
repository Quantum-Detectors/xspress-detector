/*
 * XspressController.cpp
 *
 *  Created on: 22 Sep 2021
 *      Author: Diamond Light Source
 */

#include <stdio.h>

#include "XspressController.h"
#include "DebugLevelLogger.h"
#include "version.h"

// API version numbers for XSP control wrapper interface
// After first release any changes to the API should result
// in an API increment
#define XSP_API_MAJOR 0
#define XSP_API_MINOR 1
#define XSP_API_PATCH 0

namespace Xspress
{
const std::string XspressController::API                              = "api";

const std::string XspressController::CONFIG_APP                       = "app";
const std::string XspressController::CONFIG_APP_SHUTDOWN              = "shutdown";
const std::string XspressController::CONFIG_APP_DEBUG                 = "debug_level";
const std::string XspressController::CONFIG_APP_CTRL_ENDPOINT         = "ctrl_endpoint";

const std::string XspressController::CONFIG_XSP                       = "config";
const std::string XspressController::CONFIG_XSP_NUM_CARDS             = "num_cards";
const std::string XspressController::CONFIG_XSP_NUM_TF                = "num_tf";
const std::string XspressController::CONFIG_XSP_BASE_IP               = "base_ip";
const std::string XspressController::CONFIG_XSP_MAX_CHANNELS          = "max_channels";
const std::string XspressController::CONFIG_XSP_MCA_CHANNELS          = "mca_channels";
const std::string XspressController::CONFIG_XSP_MAX_SPECTRA           = "max_spectra";
const std::string XspressController::CONFIG_XSP_DEBUG                 = "debug";
const std::string XspressController::CONFIG_XSP_CONFIG_PATH           = "config_path";
const std::string XspressController::CONFIG_XSP_CONFIG_SAVE_PATH      = "config_save_path";
const std::string XspressController::CONFIG_XSP_USE_RESGRADES         = "use_resgrades";
const std::string XspressController::CONFIG_XSP_RUN_FLAGS             = "run_flags";
const std::string XspressController::CONFIG_XSP_DTC_ENERGY            = "dtc_energy";
const std::string XspressController::CONFIG_XSP_TRIGGER_MODE          = "trigger_mode";
const std::string XspressController::CONFIG_XSP_INVERT_F0             = "invert_f0";
const std::string XspressController::CONFIG_XSP_INVERT_VETO           = "invert_veto";
const std::string XspressController::CONFIG_XSP_DEBOUNCE              = "debounce";
const std::string XspressController::CONFIG_XSP_EXPOSURE_TIME         = "exposure_time";
const std::string XspressController::CONFIG_XSP_FRAMES                = "num_images";
const std::string XspressController::CONFIG_XSP_MODE                  = "mode";
const std::string XspressController::CONFIG_XSP_SCA5_LOW              = "sca5_low_lim";
const std::string XspressController::CONFIG_XSP_SCA5_HIGH             = "sca5_high_lim";
const std::string XspressController::CONFIG_XSP_SCA6_LOW              = "sca6_low_lim";
const std::string XspressController::CONFIG_XSP_SCA6_HIGH             = "sca6_high_lim";
const std::string XspressController::CONFIG_XSP_SCA4_THRESH           = "sca4_threshold";

const std::string XspressController::CONFIG_XSP_DTC_FLAGS             = "dtc_flags";
const std::string XspressController::CONFIG_XSP_DTC_ALL_EVT_OFF       = "dtc_all_evt_off";
const std::string XspressController::CONFIG_XSP_DTC_ALL_EVT_GRAD      = "dtc_all_evt_grad";
const std::string XspressController::CONFIG_XSP_DTC_ALL_EVT_RATE_OFF  = "dtc_all_evt_rate_off";
const std::string XspressController::CONFIG_XSP_DTC_ALL_EVT_RATE_GRAD = "dtc_all_evt_rate_grad";
const std::string XspressController::CONFIG_XSP_DTC_IN_WIN_OFF        = "dtc_in_win_off";
const std::string XspressController::CONFIG_XSP_DTC_IN_WIN_GRAD       = "dtc_in_win_grad";
const std::string XspressController::CONFIG_XSP_DTC_IN_WIN_RATE_OFF   = "dtc_in_win_rate_off";
const std::string XspressController::CONFIG_XSP_DTC_IN_WIN_RATE_GRAD  = "dtc_in_win_rate_grad";

const std::string XspressController::CONFIG_DAQ                       = "daq";
const std::string XspressController::CONFIG_DAQ_ENABLED               = "enabled";
const std::string XspressController::CONFIG_DAQ_ZMQ_ENDPOINTS         = "endpoints";

const std::string XspressController::CONFIG_CMD                       = "command";
const std::string XspressController::CONFIG_CMD_CONNECT               = "connect";
const std::string XspressController::CONFIG_CMD_DISCONNECT            = "disconnect";
const std::string XspressController::CONFIG_CMD_SAVE                  = "save";
const std::string XspressController::CONFIG_CMD_RESTORE               = "restore";
const std::string XspressController::CONFIG_CMD_START                 = "start";
const std::string XspressController::CONFIG_CMD_STOP                  = "stop";
const std::string XspressController::CONFIG_CMD_TRIGGER               = "trigger";

const std::string XspressController::CONFIG_XSP_MODE_MCA              = XSP_MODE_MCA;
const std::string XspressController::CONFIG_XSP_MODE_LIST             = XSP_MODE_LIST;


const std::string XspressController::STATUS                           = "status";
const std::string XspressController::STATUS_ERROR                     = "error";
const std::string XspressController::STATUS_STATE                     = "state";
const std::string XspressController::STATUS_CONNECTED                 = "connected";
const std::string XspressController::STATUS_RECONNECT_REQUIRED        = "reconnect_required";
const std::string XspressController::STATUS_ACQ_COMPLETE              = "acquisition_complete";
const std::string XspressController::STATUS_FRAMES                    = "frames_acquired";
const std::string XspressController::STATUS_CHANNELS_CONNECTED        = "num_ch_connected";
const std::string XspressController::STATUS_CARDS_CONNECTED           = "cards_connected";
const std::string XspressController::STATUS_CHANNEL_FRAMES            = "ch_frames_acquired";
const std::string XspressController::STATUS_FEM_DROPPED_FRAMES        = "fem_dropped_frames";
const std::string XspressController::STATUS_LIVE_SCALAR[]             = {"scalar_0",
                                                                         "scalar_1",
                                                                         "scalar_2",
                                                                         "scalar_3",
                                                                         "scalar_4",
                                                                         "scalar_5",
                                                                         "scalar_6",
                                                                         "scalar_7",
                                                                         "scalar_8"};
const std::string XspressController::STATUS_LIVE_DTC                  = "dtc";
const std::string XspressController::STATUS_LIVE_INP_EST              = "inp_est";

const std::string XspressController::STATUS_TEMPERATURE[]             = {"temp_0",
                                                                         "temp_1",
                                                                         "temp_2",
                                                                         "temp_3",
                                                                         "temp_4",
                                                                         "temp_5"};


/** Construct a new XspressController class.
 *
 * The constructor sets up logging used within the class, and starts the
 * IpcReactor thread.
 */
XspressController::XspressController(bool simulation) :
    logger_(log4cxx::Logger::getLogger("Xspress.XspressController")),
    runThread_(true),
    threadRunning_(false),
    threadInitError_(false),
    shutdown_(false),
    ipc_context_(OdinData::IpcContext::Instance(1)),
    ctrlThread_(boost::bind(&XspressController::runIpcService, this)),
    ctrlChannelEndpoint_(""),
    ctrlChannel_(ZMQ_ROUTER),
    error_(""),
    state_("")
{
  OdinData::configure_logging_mdc(OdinData::app_path.c_str());
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Constructing XspressController");

  xsp_ = boost::shared_ptr<XspressDetector>(new XspressDetector(simulation));

  // Wait for the thread service to initialise and be running properly, so that
  // this constructor only returns once the object is fully initialised (RAII).
  // Monitor the thread error flag and throw an exception if initialisation fails
  while (!threadRunning_)
  {
    if (threadInitError_) {
      ctrlThread_.join();
      throw std::runtime_error(threadInitMsg_);
    }
  }
}

/**
 * Destructor.
 */
XspressController::~XspressController()
{
  // Make sure we shutdown cleanly if an exception was thrown
  shutdown();
}

/** Set the error message for the control wrapper
 *
 * If the error string is different to the existing error then
 * a log message is recorded.
 *
 * @param[in] error - the error string to set
 */
void XspressController::setError(const std::string& error)
{
  if (error != error_){
    if (error != ""){
      LOG4CXX_ERROR(logger_, "ControlWrapper error: " << error);
    }
    error_ = error;
  }
}

/** Handle an incoming configuration message.
 *
 * This method is called by the IpcReactor when a configuration IpcMessage
 * has been received. The raw message is read and parsed into an IpcMessage
 * for further processing. The configure method is called, and once
 * configuration has completed a response IpcMessage is sent back on the
 * control channel.
 */
void XspressController::handleCtrlChannel()
{
  // Receive a message from the main thread channel
  std::string clientIdentity;
  std::string ctrlMsgEncoded = ctrlChannel_.recv(&clientIdentity);
  unsigned int msg_id = 0;
  
  LOG4CXX_DEBUG_LEVEL(3, logger_, "Control thread called with message: " << ctrlMsgEncoded);

  // Parse and handle the message
  try {
    OdinData::IpcMessage ctrlMsg(ctrlMsgEncoded.c_str());
    OdinData::IpcMessage replyMsg;  // Instantiate default IpmMessage
    replyMsg.set_msg_val(ctrlMsg.get_msg_val());
    msg_id = ctrlMsg.get_msg_id();
    replyMsg.set_msg_id(msg_id);

    if ((ctrlMsg.get_msg_type() == OdinData::IpcMessage::MsgTypeCmd) &&
        (ctrlMsg.get_msg_val()  == OdinData::IpcMessage::MsgValCmdConfigure)) {
      replyMsg.set_msg_type(OdinData::IpcMessage::MsgTypeAck);
      this->configure(ctrlMsg, replyMsg);
      LOG4CXX_DEBUG_LEVEL(3, logger_, "Control thread reply message (configure): "
                             << replyMsg.encode());
    }
    else if ((ctrlMsg.get_msg_type() == OdinData::IpcMessage::MsgTypeCmd) &&
             (ctrlMsg.get_msg_val() == OdinData::IpcMessage::MsgValCmdRequestConfiguration)) {
        replyMsg.set_msg_type(OdinData::IpcMessage::MsgTypeAck);
        this->requestConfiguration(replyMsg);
        LOG4CXX_DEBUG_LEVEL(3, logger_, "Control thread reply message (request configuration): "
                               << replyMsg.encode());
    }
    else if ((ctrlMsg.get_msg_type() == OdinData::IpcMessage::MsgTypeCmd) &&
             (ctrlMsg.get_msg_val() == OdinData::IpcMessage::MsgValCmdResetStatistics)) {
      replyMsg.set_msg_type(OdinData::IpcMessage::MsgTypeAck);
      this->resetStatistics(replyMsg);
      LOG4CXX_DEBUG_LEVEL(3, logger_, "Control thread reply message (reset statistics): "
              << replyMsg.encode());
    }
    else if ((ctrlMsg.get_msg_type() == OdinData::IpcMessage::MsgTypeCmd) &&
             (ctrlMsg.get_msg_val() == OdinData::IpcMessage::MsgValCmdShutdown)) {
      replyMsg.set_msg_type(OdinData::IpcMessage::MsgTypeAck);
      exitCondition_.notify_all();
      LOG4CXX_DEBUG_LEVEL(3, logger_, "Control thread reply message (shutdown): "
              << replyMsg.encode());
    }
    else {
      std::stringstream ss;
      ss << "Invalid control message: " << ctrlMsgEncoded;
      replyMsg.set_param("error", ss.str());
      replyMsg.set_msg_type(OdinData::IpcMessage::MsgTypeNack);
      setError(ss.str());
    }
    ctrlChannel_.send(replyMsg.encode(), 0, clientIdentity);
  }
  catch (OdinData::IpcMessageException& e)
  {
    LOG4CXX_ERROR(logger_, "Error decoding control channel request: " << e.what());
  }
  catch (std::runtime_error& e)
  {
    std::stringstream ss;
    ss << "Bad control message: " << std::string(e.what());
    OdinData::IpcMessage replyMsg(OdinData::IpcMessage::MsgTypeNack, OdinData::IpcMessage::MsgValCmdConfigure);
    replyMsg.set_param<std::string>("error", ss.str());
    replyMsg.set_msg_id(msg_id);
    setError(ss.str());
    ctrlChannel_.send(replyMsg.encode());
  }
}

/** Provide status information to requesting clients.
 *
 * This is called in response to a status request from a connected client. The reply to the
 * request is populated with status information from the shared memory controller and all the
 * plugins currently loaded, and with any error messages currently stored.
 *
 * @param[in,out] reply - response IPC message to be populated with status parameters
 */
void XspressController::provideStatus(OdinData::IpcMessage& reply)
{
  // Check the acquisition failed state
  if (xsp_->getXspAcqFailed()){
    setError(xsp_->getErrorString());
  }
  // Return the current error status
  reply.set_param(XspressController::STATUS + "/" +
    XspressController::STATUS_ERROR, error_);
  // Return the current state
  reply.set_param(XspressController::STATUS + "/" +
    XspressController::STATUS_STATE, state_);
  // Check if we are connected to the hardware
  reply.set_param(XspressController::STATUS + "/" +
    XspressController::STATUS_CONNECTED, xsp_->checkConnected());
  // Check if we need to perform a reconnection to the hardware
  reply.set_param(XspressController::STATUS + "/" +
    XspressController::STATUS_RECONNECT_REQUIRED, xsp_->getReconnectStatus());
  // Clients expect the acq complete status, which is the inverse of the acquiring method
  reply.set_param(XspressController::STATUS + "/" +
    XspressController::STATUS_ACQ_COMPLETE, !xsp_->getXspAcquiring());
  // Number of frames read for current acquisition
  reply.set_param(XspressController::STATUS + "/" +
    XspressController::STATUS_FRAMES, xsp_->getXspFramesRead());
  // Number of channels connected per card
  std::vector<int32_t> ch_con = xsp_->getChannelsConnected();
  for (int index = 0; index < ch_con.size(); index++){
    reply.set_param(XspressController::STATUS + "/" +
                    XspressController::STATUS_CHANNELS_CONNECTED + "[]", ch_con[index]);
  }
  // Cards connected status
  std::vector<bool> cards_con = xsp_->getCardsConnected();
  for (int index = 0; index < cards_con.size(); index++){
    reply.set_param(XspressController::STATUS + "/" +
                    XspressController::STATUS_CARDS_CONNECTED + "[]", (int32_t)cards_con[index]);
  }
  // Number of frames per FEM
  std::vector<int32_t> ch_frames = xsp_->getXspFEMFramesRead();
  for (int index = 0; index < ch_frames.size(); index++){
    reply.set_param(XspressController::STATUS + "/" +
                    XspressController::STATUS_CHANNEL_FRAMES + "[]", ch_frames[index]);
  }
  // Number of dropped frames per FEM
  std::vector<int32_t> dropped_frames = xsp_->getXspFEMDroppedFrames();
  for (int index = 0; index < dropped_frames.size(); index++){
    reply.set_param(XspressController::STATUS + "/" +
                    XspressController::STATUS_FEM_DROPPED_FRAMES + "[]", dropped_frames[index]);
  }

  // Live scalar values from latest MCA
  for (int sc_index = 0; sc_index < NUMBER_OF_SCALARS; sc_index++){
    std::vector<uint32_t> live_scalars = xsp_->getLiveScalars(sc_index);
    for (int index = 0; index < live_scalars.size(); index++){
      reply.set_param(XspressController::STATUS + "/" +
                      XspressController::STATUS_LIVE_SCALAR[sc_index] + "[]", live_scalars[index]);
    }
  }
  // Live DTC factors from latest MCA
  std::vector<double> live_dtc = xsp_->getLiveDtcFactors();
  for (int index = 0; index < live_dtc.size(); index++){
    reply.set_param(XspressController::STATUS + "/" +
                    XspressController::STATUS_LIVE_DTC + "[]", live_dtc[index]);
  }
  // Live input estimates from latest MCA
  std::vector<double> live_inp_est = xsp_->getLiveInpEst();
  for (int index = 0; index < live_inp_est.size(); index++){
    reply.set_param(XspressController::STATUS + "/" +
                    XspressController::STATUS_LIVE_INP_EST + "[]", live_inp_est[index]);
  }

  // Temperatures
  std::vector<float> temp_0 = xsp_->getTemperature0();
  for (int index = 0; index < temp_0.size(); index++){
    reply.set_param(XspressController::STATUS + "/" +
                    XspressController::STATUS_TEMPERATURE[0] + "[]", (double)temp_0[index]);
  }
  std::vector<float> temp_1 = xsp_->getTemperature1();
  for (int index = 0; index < temp_1.size(); index++){
    reply.set_param(XspressController::STATUS + "/" +
                    XspressController::STATUS_TEMPERATURE[1] + "[]", (double)temp_1[index]);
  }
  std::vector<float> temp_2 = xsp_->getTemperature2();
  for (int index = 0; index < temp_2.size(); index++){
    reply.set_param(XspressController::STATUS + "/" +
                    XspressController::STATUS_TEMPERATURE[2] + "[]", (double)temp_2[index]);
  }
  std::vector<float> temp_3 = xsp_->getTemperature3();
  for (int index = 0; index < temp_3.size(); index++){
    reply.set_param(XspressController::STATUS + "/" +
                    XspressController::STATUS_TEMPERATURE[3] + "[]", (double)temp_3[index]);
  }
  std::vector<float> temp_4 = xsp_->getTemperature4();
  for (int index = 0; index < temp_4.size(); index++){
    reply.set_param(XspressController::STATUS + "/" +
                    XspressController::STATUS_TEMPERATURE[4] + "[]", (double)temp_4[index]);
  }
  std::vector<float> temp_5 = xsp_->getTemperature5();
  for (int index = 0; index < temp_5.size(); index++){
    reply.set_param(XspressController::STATUS + "/" +
                    XspressController::STATUS_TEMPERATURE[5] + "[]", (double)temp_5[index]);
  }
}

/** Provide version information to requesting clients.
 *
 * This is called in response to a version request from a connected client. The reply to the
 * request is populated with version information from application and all the
 * plugins currently loaded.
 *
 * @param[in,out] reply - response IPC message to be populated with version information
 */
void XspressController::provideVersion(OdinData::IpcMessage& reply)
{
  // Populate the reply with top-level odin-data application version information
  reply.set_param("version/xspress-detector/major", XSPRESS_DETECTOR_VERSION_MAJOR);
  reply.set_param("version/xspress-detector/minor", XSPRESS_DETECTOR_VERSION_MINOR);
  reply.set_param("version/xspress-detector/patch", XSPRESS_DETECTOR_VERSION_PATCH);
  reply.set_param("version/xspress-detector/short", std::string(XSPRESS_DETECTOR_VERSION_STR_SHORT));
  reply.set_param("version/xspress-detector/full", std::string(XSPRESS_DETECTOR_VERSION_STR)); 
}

/** Provide API information to requesting clients.
 *
 * @param[in,out] reply - response IPC message to be populated with version information
 */
void XspressController::provideAPIVersion(OdinData::IpcMessage& reply)
{
  std::stringstream ss;
  ss << XSP_API_MAJOR << "." << XSP_API_MINOR << "." << XSP_API_PATCH;
  reply.set_param(XspressController::API, ss.str());
}

/**
 * Set configuration options for the Control Wrapper application.
 *
 * Sets up the overall control application according to the
 * configuration IpcMessage objects that are received. The objects
 * are searched for:
 * CONFIG_APP - Sets up the control application
 * CONFIG_XSP - Sets up the Xspress hardware
 * CONFIG_DAQ - Sets up the DAQ for MCA mode
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void XspressController::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Configuration submitted: " << config.encode());
  // First clear out any existing error message
  setError("");
  // Reset any failed acquisition error messages
  xsp_->resetXspAcqFailed();

  // Check to see if we are configuring this control application
  if (config.has_param(XspressController::CONFIG_APP)) {
    OdinData::IpcMessage appConfig(config.get_param<const rapidjson::Value&>(XspressController::CONFIG_APP));
    this->configureApp(appConfig, reply);
  }

  // Check to see if we are configuring the Xspress hardware
  if (config.has_param(XspressController::CONFIG_XSP)) {
    OdinData::IpcMessage xspConfig(config.get_param<const rapidjson::Value&>(XspressController::CONFIG_XSP));
    this->configureXsp(xspConfig, reply);
  }

  // Check to see if we are configuring the DAQ interface
  if (config.has_param(XspressController::CONFIG_DAQ)) {
    OdinData::IpcMessage daqConfig(config.get_param<const rapidjson::Value&>(XspressController::CONFIG_DAQ));
    this->configureDAQ(daqConfig, reply);
  }

  // Check to see if we have received a command
  if (config.has_param(XspressController::CONFIG_CMD)) {
    OdinData::IpcMessage cmdConfig(config.get_param<const rapidjson::Value&>(XspressController::CONFIG_CMD));
    this->configureCommand(cmdConfig, reply);
  }

}

/**
 * Set configuration options for this control application.
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void XspressController::configureApp(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  // Check for a debug level change
  if (config.has_param(XspressController::CONFIG_APP_DEBUG)) {
    unsigned int debug = config.get_param<unsigned int>(XspressController::CONFIG_APP_DEBUG);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Debug level set to  " << debug);
    set_debug_level(debug);
  }

  if (config.has_param(XspressController::CONFIG_APP_CTRL_ENDPOINT)) {
    std::string endpoint = config.get_param<std::string>(XspressController::CONFIG_APP_CTRL_ENDPOINT);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting control end point to  " << endpoint);
    this->setupControlInterface(endpoint);
  }
}

/**
 * Set configuration options for the Xspress detector.
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void XspressController::configureXsp(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  int status = XSP_STATUS_OK;

  // Check for the number of cards
  if (config.has_param(XspressController::CONFIG_XSP_NUM_CARDS)){
    int num_cards = config.get_param<int>(XspressController::CONFIG_XSP_NUM_CARDS);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "num_cards set to  " << num_cards);
    xsp_->setXspNumCards(num_cards);
  }

  // Check for the number of 4096 energy bin spectra timeframes
  if (config.has_param(XspressController::CONFIG_XSP_NUM_TF)) {
    int num_tf = config.get_param<int>(XspressController::CONFIG_XSP_NUM_TF);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "num_tf set to  " << num_tf);
    xsp_->setXspNumTf(num_tf);
  }

  // Check for the base IP address of the Xspress detector
  if (config.has_param(XspressController::CONFIG_XSP_BASE_IP)) {
    std::string base_ip = config.get_param<std::string>(XspressController::CONFIG_XSP_BASE_IP);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "base_ip set to  " << base_ip);
    xsp_->setXspBaseIP(base_ip);
  }

  // Check for the maximum number of channels available in the Xspress
  if (config.has_param(XspressController::CONFIG_XSP_MAX_CHANNELS)) {
    int max_channels = config.get_param<int>(XspressController::CONFIG_XSP_MAX_CHANNELS);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "max_channels set to  " << max_channels);
    xsp_->setXspMaxChannels(max_channels);
  }

  // Check for the maximum number of channels available in the Xspress
  if (config.has_param(XspressController::CONFIG_XSP_MCA_CHANNELS)) {
    int mca_channels = config.get_param<int>(XspressController::CONFIG_XSP_MCA_CHANNELS);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "mca_channels set to  " << mca_channels);
    xsp_->setXspMcaChannels(mca_channels);
  }

  // Check for the maximum number of spectra available in the Xspress
  if (config.has_param(XspressController::CONFIG_XSP_MAX_SPECTRA)) {
    int max_spectra = config.get_param<int>(XspressController::CONFIG_XSP_MAX_SPECTRA);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "max_spectra set to  " << max_spectra);
    xsp_->setXspMaxSpectra(max_spectra);
  }

  // Check for the libxspress debug level
  if (config.has_param(XspressController::CONFIG_XSP_DEBUG)) {
    int debug = config.get_param<int>(XspressController::CONFIG_XSP_DEBUG);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "debug set to  " << debug);
    xsp_->setXspDebug(debug);
  }

  // Check for the configuration path for loading configurations
  if (config.has_param(XspressController::CONFIG_XSP_CONFIG_PATH)) {
    std::string config_path = config.get_param<std::string>(XspressController::CONFIG_XSP_CONFIG_PATH);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "config_path set to  " << config_path);
    xsp_->setXspConfigPath(config_path);
  }

  // Check for the configuration path for saving configurations
  if (config.has_param(XspressController::CONFIG_XSP_CONFIG_SAVE_PATH)) {
    std::string config_save_path = config.get_param<std::string>(XspressController::CONFIG_XSP_CONFIG_SAVE_PATH);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "config_save_path set to  " << config_save_path);
    xsp_->setXspConfigSavePath(config_save_path);
  }

  // Check for use resgrades flag
  if (config.has_param(XspressController::CONFIG_XSP_USE_RESGRADES)) {
    bool use_resgrades = config.get_param<bool>(XspressController::CONFIG_XSP_USE_RESGRADES);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "use_resgrades set to  " << use_resgrades);
    xsp_->setXspUseResgrades(use_resgrades);
  }

  // Check for run flags parameter
  if (config.has_param(XspressController::CONFIG_XSP_RUN_FLAGS)) {
    int run_flags = config.get_param<int>(XspressController::CONFIG_XSP_RUN_FLAGS);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "run_flags set to  " << run_flags);
    xsp_->setXspRunFlags(run_flags);
  }

  // Check for dead time correction energy parameter
  if (config.has_param(XspressController::CONFIG_XSP_DTC_ENERGY)) {
    double dtc_energy = config.get_param<double>(XspressController::CONFIG_XSP_DTC_ENERGY);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "dtc_energy set to  " << dtc_energy);
    xsp_->setXspDTCEnergy(dtc_energy);
  }

  // Check for trigger mode parameter
  if (config.has_param(XspressController::CONFIG_XSP_TRIGGER_MODE)) {
    int trigger_mode = config.get_param<int>(XspressController::CONFIG_XSP_TRIGGER_MODE);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "trigger_mode set to  " << trigger_mode);
    xsp_->setXspTriggerMode(trigger_mode);
  }

  // Check for invert_f0 parameter
  if (config.has_param(XspressController::CONFIG_XSP_INVERT_F0)) {
    int invert_f0 = config.get_param<int>(XspressController::CONFIG_XSP_INVERT_F0);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "invert_f0 set to  " << invert_f0);
    xsp_->setXspInvertF0(invert_f0);
  }

  // Check for invert_veto parameter
  if (config.has_param(XspressController::CONFIG_XSP_INVERT_VETO)) {
    int invert_veto = config.get_param<int>(XspressController::CONFIG_XSP_INVERT_VETO);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "invert_veto set to  " << invert_veto);
    xsp_->setXspInvertVeto(invert_veto);
  }

  // Check for debounce parameter
  if (config.has_param(XspressController::CONFIG_XSP_DEBOUNCE)) {
    int debounce = config.get_param<int>(XspressController::CONFIG_XSP_DEBOUNCE);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "debounce set to  " << debounce);
    xsp_->setXspDebounce(debounce);
  }

  // Check for exposure time parameter
  if (config.has_param(XspressController::CONFIG_XSP_EXPOSURE_TIME)) {
    double exposure_time = config.get_param<double>(XspressController::CONFIG_XSP_EXPOSURE_TIME);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "exposure_time set to  " << exposure_time);
    xsp_->setXspExposureTime(exposure_time);
  }

  // Check for number of frames parameter
  if (config.has_param(XspressController::CONFIG_XSP_FRAMES)) {
    int frames = config.get_param<int>(XspressController::CONFIG_XSP_FRAMES);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "frames set to  " << frames);
    xsp_->setXspFrames(frames);
  }

  // Check for mode parameter
  if (config.has_param(XspressController::CONFIG_XSP_MODE)) {
    std::string mode = config.get_param<std::string>(XspressController::CONFIG_XSP_MODE);
    if (mode != XSP_MODE_MCA && mode != XSP_MODE_LIST){
      std::stringstream ss;
      ss << "Invalid mode requested: " << mode;
      setError(ss.str());
      reply.set_msg_type(OdinData::IpcMessage::MsgTypeNack);
      reply.set_param("error", ss.str());
    } else {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "mode set to " << mode);
      xsp_->setXspMode(mode);
    }
  }

  // Check for sc5 low limit array parameter
  if (config.has_param(XspressController::CONFIG_XSP_SCA5_LOW)){
    const rapidjson::Value& val = config.get_param<const rapidjson::Value&>(XspressController::CONFIG_XSP_SCA5_LOW);
    // Create the vector of limits
    std::vector<uint32_t> sca5low;
    for (rapidjson::SizeType i = 0; i < val.Size(); i++) {
      const rapidjson::Value& ll = val[i];
      uint32_t ival = ll.GetInt();
      LOG4CXX_DEBUG_LEVEL(0, logger_, "Setting Scalar 5 low limit [" << i << "] = " << ival);
      sca5low.push_back(ival);
    }
    status = xsp_->setSca5LowLimits(sca5low);
    if (status != XSP_STATUS_OK){
      // Command failed, return error with any error string
      reply.set_nack(xsp_->getErrorString());
      setError(xsp_->getErrorString());
    }
  }

  // Check for sc5 high limit array parameter
  if (config.has_param(XspressController::CONFIG_XSP_SCA5_HIGH)){
    const rapidjson::Value& val = config.get_param<const rapidjson::Value&>(XspressController::CONFIG_XSP_SCA5_HIGH);
    // Create the vector of limits
    std::vector<uint32_t> sca5high;
    for (rapidjson::SizeType i = 0; i < val.Size(); i++) {
      const rapidjson::Value& hl = val[i];
      uint32_t ival = hl.GetInt();
      LOG4CXX_DEBUG_LEVEL(0, logger_, "Setting Scalar 5 high limit [" << i << "] = " << ival);
      sca5high.push_back(ival);
    }
    status = xsp_->setSca5HighLimits(sca5high);
    if (status != XSP_STATUS_OK){
      // Command failed, return error with any error string
      reply.set_nack(xsp_->getErrorString());
      setError(xsp_->getErrorString());
    }
  }

  // Check for sc6 low limit array parameter
  if (config.has_param(XspressController::CONFIG_XSP_SCA6_LOW)){
    const rapidjson::Value& val = config.get_param<const rapidjson::Value&>(XspressController::CONFIG_XSP_SCA6_LOW);
    // Create the vector of limits
    std::vector<uint32_t> sca6low;
    for (rapidjson::SizeType i = 0; i < val.Size(); i++) {
      const rapidjson::Value& ll = val[i];
      uint32_t ival = ll.GetInt();
      LOG4CXX_DEBUG_LEVEL(0, logger_, "Setting Scalar 6 low limit [" << i << "] = " << ival);
      sca6low.push_back(ival);
    }
    status = xsp_->setSca6LowLimits(sca6low);
    if (status != XSP_STATUS_OK){
      // Command failed, return error with any error string
      reply.set_nack(xsp_->getErrorString());
      setError(xsp_->getErrorString());
    }
  }

  // Check for sc6 high limit array parameter
  if (config.has_param(XspressController::CONFIG_XSP_SCA6_HIGH)){
    const rapidjson::Value& val = config.get_param<const rapidjson::Value&>(XspressController::CONFIG_XSP_SCA6_HIGH);
    // Create the vector of limits
    std::vector<uint32_t> sca6high;
    for (rapidjson::SizeType i = 0; i < val.Size(); i++) {
      const rapidjson::Value& hl = val[i];
      uint32_t ival = hl.GetInt();
      LOG4CXX_DEBUG_LEVEL(0, logger_, "Setting Scalar 6 high limit [" << i << "] = " << ival);
      sca6high.push_back(ival);
    }
    status = xsp_->setSca6HighLimits(sca6high);
    if (status != XSP_STATUS_OK){
      // Command failed, return error with any error string
      reply.set_nack(xsp_->getErrorString());
      setError(xsp_->getErrorString());
    }
  }

  // Check for sc6 high limit array parameter
  if (config.has_param(XspressController::CONFIG_XSP_SCA4_THRESH)){
    const rapidjson::Value& val = config.get_param<const rapidjson::Value&>(XspressController::CONFIG_XSP_SCA4_THRESH);
    // Create the vector of limits
    std::vector<uint32_t> sca4t;
    for (rapidjson::SizeType i = 0; i < val.Size(); i++) {
      const rapidjson::Value& th = val[i];
      uint32_t ival = th.GetInt();
      LOG4CXX_DEBUG_LEVEL(0, logger_, "Setting Scalar 4 threshold [" << i << "] = " << ival);
      sca4t.push_back(ival);
    }
    status = xsp_->setSca4Thresholds(sca4t);
    if (status != XSP_STATUS_OK){
      // Command failed, return error with any error string
      reply.set_nack(xsp_->getErrorString());
      setError(xsp_->getErrorString());
    }
  }

}

/**
 * Set configuration options for the Xspress DAQ class.
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void XspressController::configureDAQ(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  // Check if the DAQ endpoints have been specified
  if (config.has_param(XspressController::CONFIG_DAQ_ZMQ_ENDPOINTS)) {
    const rapidjson::Value& val = config.get_param<const rapidjson::Value&>(XspressController::CONFIG_DAQ_ZMQ_ENDPOINTS);
    // Create the vector of endpoints
    std::vector<std::string> eps;
    for (rapidjson::SizeType i = 0; i < val.Size(); i++) {
      const rapidjson::Value& ep = val[i];
      std::string sep = ep.GetString();
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Adding DAQ endpoint [" << sep << "]");
      eps.push_back(sep);
    }
    xsp_->setXspDAQEndpoints(eps);
  }

  // Check if DAQ is to be enabled
  if (config.has_param(XspressController::CONFIG_DAQ_ENABLED)){
    bool enable_daq = config.get_param<bool>(XspressController::CONFIG_DAQ_ENABLED);
    if (enable_daq){
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Enable DAQ requested");
      xsp_->enableDAQ();
    }
  }
}

/**
 * Send commands to the Xspress interface library.
 *
 * \param[in] config - IpcMessage containing command data.
 * \param[out] reply - Response IpcMessage.
 */
void XspressController::configureCommand(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  // Check for a connect command
  if (config.has_param(XspressController::CONFIG_CMD_CONNECT)){
    LOG4CXX_DEBUG_LEVEL(1, logger_, "connect command executing");
    // Attempt connection to the hardware
    int status = xsp_->connect();
    if (status != XSP_STATUS_OK){
      // Command failed, return error with any error string
      reply.set_nack(xsp_->getErrorString());
      setError(xsp_->getErrorString());
    } else {
      // Log the Xspress version
      LOG4CXX_INFO(logger_, "Connected to Xspress version: " << xsp_->getVersionString());
      // Attempt to restore the settings
      status = xsp_->restoreSettings();
    }
    if (status != XSP_STATUS_OK){
      // Command failed, return error with any error string
      reply.set_nack(xsp_->getErrorString());
      setError(xsp_->getErrorString());
    } else {
      // Check the connected channels and cards
      status = xsp_->setupChannels();
    }
    if (status != XSP_STATUS_OK){
      // Command failed, return error with any error string
      reply.set_nack(xsp_->getErrorString());
      setError(xsp_->getErrorString());
    } else {
      // Configure the clock signals (important for Mk2)
      status = xsp_->setupClocks();
    }
    if (status != XSP_STATUS_OK){
      // Command failed, return error with any error string
      reply.set_nack(xsp_->getErrorString());
      setError(xsp_->getErrorString());
    }
  }

  // Check for a disconnect command
  if (config.has_param(XspressController::CONFIG_CMD_DISCONNECT)){
    LOG4CXX_DEBUG_LEVEL(1, logger_, "disconnect command executing");
    // Attempt connection to the hardware
    int status = xsp_->disconnect();
    if (status != XSP_STATUS_OK){
      // Command failed, return error with any error string
      reply.set_nack(xsp_->getErrorString());
      setError(xsp_->getErrorString());
    }
  }

  // Check for a start acquisition command
  if (config.has_param(XspressController::CONFIG_CMD_START)){
    LOG4CXX_DEBUG_LEVEL(1, logger_, "start acquisition command executing");
    // Attempt to start an acquisition
    int status = xsp_->startAcquisition();
    if (status != XSP_STATUS_OK){
      // Command failed, return error with any error string
      reply.set_nack(xsp_->getErrorString());
      setError(xsp_->getErrorString());
    }
  }

  // Check for a stop acquisition command
  if (config.has_param(XspressController::CONFIG_CMD_STOP)){
    LOG4CXX_DEBUG_LEVEL(1, logger_, "stop acquisition command executing");
    // Attempt to stop an acquisition
    int status = xsp_->stopAcquisition();
    if (status != XSP_STATUS_OK){
      // Command failed, return error with any error string
      reply.set_nack(xsp_->getErrorString());
      setError(xsp_->getErrorString());
    }
  }

  // Check for a software trigger command
  if (config.has_param(XspressController::CONFIG_CMD_TRIGGER)){
    LOG4CXX_DEBUG_LEVEL(1, logger_, "software trigger command executing");
    // Send a software trigger
    int status = xsp_->sendSoftwareTrigger();
    if (status != XSP_STATUS_OK){
      // Command failed, return error with any error string
      reply.set_nack(xsp_->getErrorString());
      setError(xsp_->getErrorString());
    }
  }
}

/**
 * Request the current configuration of the FrameProcessorController.
 *
 * The method also searches through all loaded plugins.  Each plugin is
 * also sent a request for its configuration.
 *
 * \param[out] reply - Response IpcMessage with the current configuration.
 */
void XspressController::requestConfiguration(OdinData::IpcMessage& reply)
{
  LOG4CXX_DEBUG_LEVEL(3, logger_, "Request for configuration made");

  // Add local configuration parameter values to the reply
  reply.set_param(XspressController::CONFIG_APP + "/" +
                  XspressController::CONFIG_APP_DEBUG, debug_level);
  reply.set_param(XspressController::CONFIG_APP + "/" +
                  XspressController::CONFIG_APP_CTRL_ENDPOINT, ctrlChannelEndpoint_);
  // Add Xspress configuration parameter values to the reply
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_NUM_CARDS, xsp_->getXspNumCards());
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_NUM_TF, xsp_->getXspNumTf());
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_BASE_IP, xsp_->getXspBaseIP());
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_MAX_CHANNELS, xsp_->getXspMaxChannels());
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_MCA_CHANNELS, xsp_->getXspMcaChannels());
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_MAX_SPECTRA, xsp_->getXspMaxSpectra());
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_DEBUG, xsp_->getXspDebug());
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_CONFIG_PATH, xsp_->getXspConfigPath());
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_CONFIG_SAVE_PATH, xsp_->getXspConfigSavePath());
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_USE_RESGRADES, xsp_->getXspUseResgrades());
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_RUN_FLAGS, xsp_->getXspRunFlags());
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_DTC_ENERGY, xsp_->getXspDTCEnergy());
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_TRIGGER_MODE, xsp_->getXspTriggerMode());
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_INVERT_F0, xsp_->getXspInvertF0());
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_INVERT_VETO, xsp_->getXspInvertVeto());
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_DEBOUNCE, xsp_->getXspDebounce());
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_EXPOSURE_TIME, xsp_->getXspExposureTime());
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_FRAMES, xsp_->getXspFrames());
  reply.set_param(XspressController::CONFIG_XSP + "/" +
                  XspressController::CONFIG_XSP_MODE, xsp_->getXspMode());

  std::vector<uint32_t> sca5ll = xsp_->getSca5LowLimits();
  for (int index = 0; index < sca5ll.size(); index++){
    reply.set_param(XspressController::CONFIG_XSP + "/" +
                    XspressController::CONFIG_XSP_SCA5_LOW + "[]", sca5ll[index]);
  }
  std::vector<uint32_t> sca5hl = xsp_->getSca5HighLimits();
  for (int index = 0; index < sca5hl.size(); index++){
    reply.set_param(XspressController::CONFIG_XSP + "/" +
                    XspressController::CONFIG_XSP_SCA5_HIGH + "[]", sca5hl[index]);
  }
  std::vector<uint32_t> sca6ll = xsp_->getSca6LowLimits();
  for (int index = 0; index < sca6ll.size(); index++){
    reply.set_param(XspressController::CONFIG_XSP + "/" +
                    XspressController::CONFIG_XSP_SCA6_LOW + "[]", sca6ll[index]);
  }
  std::vector<uint32_t> sca6hl = xsp_->getSca6HighLimits();
  for (int index = 0; index < sca6hl.size(); index++){
    reply.set_param(XspressController::CONFIG_XSP + "/" +
                    XspressController::CONFIG_XSP_SCA6_HIGH + "[]", sca6hl[index]);
  }
  std::vector<uint32_t> sca4t = xsp_->getSca4Thresholds();
  for (int index = 0; index < sca4t.size(); index++){
    reply.set_param(XspressController::CONFIG_XSP + "/" +
                    XspressController::CONFIG_XSP_SCA4_THRESH + "[]", sca4t[index]);
  }
  std::vector<int> dtc_flags = xsp_->getDtcFlags();
  for (int index = 0; index < dtc_flags.size(); index++){
    reply.set_param(XspressController::CONFIG_XSP + "/" +
                    XspressController::CONFIG_XSP_DTC_FLAGS + "[]", dtc_flags[index]);
  }

  std::vector<double> dtc_all_event_off = xsp_->getDtcAllEventOff();
  for (int index = 0; index < dtc_all_event_off.size(); index++){
    reply.set_param(XspressController::CONFIG_XSP + "/" +
                    XspressController::CONFIG_XSP_DTC_ALL_EVT_OFF + "[]", dtc_all_event_off[index]);
  }

  std::vector<double> dtc_all_event_grad = xsp_->getDtcAllEventGrad();
  for (int index = 0; index < dtc_all_event_grad.size(); index++){
    reply.set_param(XspressController::CONFIG_XSP + "/" +
                    XspressController::CONFIG_XSP_DTC_ALL_EVT_GRAD + "[]", dtc_all_event_grad[index]);
  }

  std::vector<double> dtc_all_event_rate_off = xsp_->getDtcAllEventRateOff();
  for (int index = 0; index < dtc_all_event_rate_off.size(); index++){
    reply.set_param(XspressController::CONFIG_XSP + "/" +
                    XspressController::CONFIG_XSP_DTC_ALL_EVT_RATE_OFF + "[]", dtc_all_event_rate_off[index]);
  }

  std::vector<double> dtc_all_event_rate_grad = xsp_->getDtcAllEventRateGrad();
  for (int index = 0; index < dtc_all_event_rate_grad.size(); index++){
    reply.set_param(XspressController::CONFIG_XSP + "/" +
                    XspressController::CONFIG_XSP_DTC_ALL_EVT_RATE_GRAD + "[]", dtc_all_event_rate_grad[index]);
  }

  std::vector<double> dtc_in_window_off = xsp_->getDtcInWindowOff();
  for (int index = 0; index < dtc_in_window_off.size(); index++){
    reply.set_param(XspressController::CONFIG_XSP + "/" +
                    XspressController::CONFIG_XSP_DTC_IN_WIN_OFF + "[]", dtc_in_window_off[index]);
  }

  std::vector<double> dtc_in_window_grad = xsp_->getDtcInWindowGrad();
  for (int index = 0; index < dtc_in_window_grad.size(); index++){
    reply.set_param(XspressController::CONFIG_XSP + "/" +
                    XspressController::CONFIG_XSP_DTC_IN_WIN_GRAD + "[]", dtc_in_window_grad[index]);
  }

  std::vector<double> dtc_in_window_rate_off = xsp_->getDtcInWindowRateOff();
  for (int index = 0; index < dtc_in_window_rate_off.size(); index++){
    reply.set_param(XspressController::CONFIG_XSP + "/" +
                    XspressController::CONFIG_XSP_DTC_IN_WIN_RATE_OFF + "[]", dtc_in_window_rate_off[index]);
  }

  std::vector<double> dtc_in_window_rate_grad = xsp_->getDtcInWindowRateGrad();
  for (int index = 0; index < dtc_in_window_rate_grad.size(); index++){
    reply.set_param(XspressController::CONFIG_XSP + "/" +
                    XspressController::CONFIG_XSP_DTC_IN_WIN_RATE_GRAD + "[]", dtc_in_window_rate_grad[index]);
  }

  std::vector<std::string> eps = xsp_->getXspDAQEndpoints();
  for (int index = 0; index < eps.size(); index++){
    reply.set_param(XspressController::CONFIG_DAQ + "/" +
                    XspressController::CONFIG_DAQ_ZMQ_ENDPOINTS + "[]", eps[index]);
  }
  provideStatus(reply);
  provideVersion(reply);
  provideAPIVersion(reply);
}

/**
 * Reset statistics on all of the loaded plugins.
 *
 * The method calls reset statistics on all of the loaded plugins.
 *
 * \param[out] reply - Response IpcMessage with the current status.
 */
void XspressController::resetStatistics(OdinData::IpcMessage& reply)
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Reset statistics requested");
  bool reset_ok = true;
}

void XspressController::run() {

  LOG4CXX_INFO(logger_, "Running Xspress controller");

  // Now wait for the shutdown command from either the control interface or the worker thread
  waitForShutdown();
  shutdown();
}

void XspressController::shutdown() {
  if (!shutdown_) {
    LOG4CXX_INFO(logger_, "Received shutdown command");

    // Stop worker thread (for IFrameCallback) and reactor
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Stopping Xspress Controller IPCReactor");
    reactor_->stop();

    // Close control IPC channel
    closeControlInterface();

    shutdown_ = true;
    LOG4CXX_INFO(logger_, "Shutting Down");
  }
}

/**
 * Wait for the exit condition before returning.
 */
void XspressController::waitForShutdown()
{
  boost::unique_lock<boost::mutex> lock(exitMutex_);
  exitCondition_.wait(lock);
}

/** Set up the control interface.
 *
 * This method binds the control IpcChannel to the provided endpoint,
 * creating a socket for controlling applications to connect to. This
 * socket is used for sending configuration IpcMessages.
 *
 * \param[in] ctrlEndpointString - Name of the control endpoint.
 */
void XspressController::setupControlInterface(const std::string& ctrlEndpointString)
{
  try {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Connecting control channel to endpoint: " << ctrlEndpointString);
    ctrlChannel_.bind(ctrlEndpointString.c_str());
    ctrlChannelEndpoint_ = ctrlEndpointString;
  }
  catch (zmq::error_t& e) {
    //std::stringstream ss;
    //ss << "RX channel connect to endpoint " << config_.rx_channel_endpoint_ << " failed: " << e.what();
    // TODO: What to do here, I think throw it up
    throw std::runtime_error(e.what());
  }

  // Add the control channel to the reactor
  reactor_->register_channel(ctrlChannel_, boost::bind(&XspressController::handleCtrlChannel, this));
}

/** Close the control interface.
 */
void XspressController::closeControlInterface()
{
  try {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Closing control endpoint socket.");
    ctrlThread_.join();
    reactor_->remove_channel(ctrlChannel_);
    ctrlChannel_.close();
  }
  catch (zmq::error_t& e) {
    // TODO: What to do here, I think throw it up
    throw std::runtime_error(e.what());
  }
}

/** Start the Ipc service running.
 *
 * Sets up a tick timer and runs the Ipc reactor.
 * Currently the tick timer does not perform any processing.
 */
void XspressController::runIpcService(void)
{
  // Configure logging for this thread
  OdinData::configure_logging_mdc(OdinData::app_path.c_str());

  LOG4CXX_DEBUG_LEVEL(1, logger_, "Running IPC thread service");

  // Create the reactor
  reactor_ = boost::shared_ptr<OdinData::IpcReactor>(new OdinData::IpcReactor());

  // Add the tick timer to the reactor
  int tick_timer_id = reactor_->register_timer(1000, 0, boost::bind(&XspressController::tickTimer, this));

  // Set thread state to running, allows constructor to return
  threadRunning_ = true;

  // Run the reactor event loop
  reactor_->run();

  // Cleanup - remove channels, sockets and timers from the reactor and close the receive socket
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Terminating IPC thread service");
}

/** Tick timer task called by IpcReactor.
 *
 * This currently performs no processing.
 */
void XspressController::tickTimer(void)
{
  if (!runThread_)
  {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "IPC thread terminate detected in timer");
    reactor_->stop();
  } else {
    // We will read the FEM status in this timer execution
    xsp_->readFemStatus();
  }
}

} /* namespace Xspress */
