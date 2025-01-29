/**
 * @file X3X2ListModeFrameDecoder.h
 * @author Ben Bradnick (ben@quantumdetectors.com)
 * @brief Odin receiver plugin for handling X3X2 list mode TCP data
 * @date 2024-11-06
 */

#ifndef INCLUDE_X3X2LISTMODEFRAMEDECODER_H_
#define INCLUDE_X3X2LISTMODEFRAMEDECODER_H_

#include <iostream>
#include <stdint.h>
#include <time.h>

#include "FrameDecoderTCP.h"

namespace FrameReceiver {

namespace X3X2ListModeFrameDecoderDefaults {
  const int frame_number = 0;
  const int buffer_id = 0;
  const size_t max_size = 8192; // Each TCP frame is 8192 bytes of 4096 16 bit words
  const size_t header_size = 0; // Initially we do not need a header
  const int num_buffers = 5;
} // namespace X3X2ListModeFrameDecoderDefaults

class X3X2ListModeFrameDecoder : public FrameDecoderTCP {
public:
  X3X2ListModeFrameDecoder();
  ~X3X2ListModeFrameDecoder();

  int get_version_major();
  int get_version_minor();
  int get_version_patch();
  std::string get_version_short();
  std::string get_version_long();

  void monitor_buffers(void);
  void get_status(const std::string param_prefix,
                  OdinData::IpcMessage &status_msg);

  void init(LoggerPtr &logger, OdinData::IpcMessage &config_msg);
  void request_configuration(const std::string param_prefix,
                             OdinData::IpcMessage &config_reply);

  void *get_next_message_buffer(void);
  const size_t get_next_message_size(void) const;
  FrameDecoder::FrameReceiveState process_message(size_t bytes_received);

  const size_t get_frame_buffer_size(void) const;
  const size_t get_frame_header_size(void) const;

  void reset_statistics(void);

  void *get_packet_header_buffer(void);

  uint32_t get_frame_number(void) const;
  uint32_t get_packet_number(void) const;

private:
  boost::shared_ptr<void> frame_buffer_;
  size_t frames_dropped_;
  size_t frames_sent_;
  size_t read_so_far_;
  int current_frame_number_;
  int current_frame_buffer_id_;
  size_t buffer_size_;
  size_t header_size_;
  size_t frame_size_;
  unsigned int num_buffers_;
  FrameDecoder::FrameReceiveState receive_state_;
};

} // namespace FrameReceiver
#endif /* INCLUDE_X3X2LISTMODEFRAMEDECODER_H_ */
