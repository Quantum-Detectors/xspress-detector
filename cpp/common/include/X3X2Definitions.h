/**
 * Definitions specific to the X3X2 cards
 */

#ifndef _X3X2DEFINITIONS_H
#define _X3X2DEFINITIONS_H

#define X3X2_MINI_TCP_FRAME_SIZE    8192
#define X3X2_MINI_FIELDS_PER_FRAME  4096

#define PKTS_PER_FRAME 20

namespace X3X2
{
  typedef struct
  {
    struct timespec frame_start_time;
    uint32_t packets_received;
  } X3X2ListFrameHeader;

  inline const std::size_t max_frame_size(void)
  {
    std::size_t max_frame_size = sizeof(X3X2ListFrameHeader) + (X3X2_MINI_TCP_FRAME_SIZE * PKTS_PER_FRAME);
    return max_frame_size;
  }
}

#endif
