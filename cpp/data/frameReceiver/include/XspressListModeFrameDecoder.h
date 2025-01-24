/**
 * @file XspressListModeFrameDecoder.h
 * @author Alan Greer
 * @brief This decoder is used to handle list mode events for Xspress 4
 * @date 2025-01-02
 */

#ifndef SRC_XSPRESSLISTMODEDECODER_H
#define SRC_XSPRESSLISTMODEDECODER_H

#include <boost/shared_ptr.hpp>
#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameDecoderUDP.h"
#include "XspressDefinitions.h"
#include "IpcMessage.h"
#include "gettime.h"

namespace FrameReceiver {

class XspressListModeFrameDecoder : public FrameDecoderUDP {

  public:
    XspressListModeFrameDecoder();

    ~XspressListModeFrameDecoder();

    // version related functions
    int get_version_major();

    int get_version_minor();

    int get_version_patch();

    std::string get_version_short();

    std::string get_version_long();

    // decoder interface
    void init(LoggerPtr& logger, OdinData::IpcMessage& config_msg);
    void request_configuration(const std::string param_prefix, OdinData::IpcMessage& config_reply);

    const size_t get_frame_buffer_size(void) const;
    const size_t get_frame_header_size(void) const;

    inline const bool requires_header_peek(void) const { return true; };
    const size_t get_packet_header_size(void) const;
    void process_packet_header(size_t bytes_received, int port, struct sockaddr_in* from_addr);

    inline const bool trailer_mode(void) const { return false; };

    void* get_next_payload_buffer(void) const;
    size_t get_next_payload_size(void) const;
    FrameDecoder::FrameReceiveState process_packet(size_t bytes_received, int port, struct sockaddr_in* from_addr);

    void monitor_buffers(void);
    void get_status(const std::string param_prefix, OdinData::IpcMessage& status_msg);
    void reset_statistics(void);

    void* get_packet_header_buffer(void);

    //uint32_t get_frame_number(void) const;
    //uint32_t get_packet_number(void) const;


  private:
    boost::shared_ptr<void> current_raw_packet_header_;
    boost::shared_ptr<void> dropped_frame_buffer_;
    void *current_frame_buffer_;
    Xspress::ListFrameHeader *current_frame_header_;
    Xspress::ListPacketHeader *current_packet_header_;
    //void *dropped_frame_buffer_;
    int32_t current_frame_buffer_id_;
    uint32_t current_frame_number_;
    bool dropping_frame_data_;
    enum XspressState current_state;
    // statistics
    unsigned int frames_dropped_;
    std::map<std::string, uint32_t> channel_map_;
    struct sockaddr_in server_address_;
    int server_socket_;
    // Init time structure
    struct timespec init_time_;
    // Initialising flag
    bool initialising_;
  };

}

#endif //SRC_XSPRESSDECODER_H
