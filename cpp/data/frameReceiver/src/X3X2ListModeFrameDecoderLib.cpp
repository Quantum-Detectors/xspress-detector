/**
 * @file X3X2ListModeFrameDecoderLib.cpp
 * @author Ben Bradnick (ben@quantumdetectors.com)
 * @brief Registrar for X3X2 list mode receiver plugin
 * @date 2024-11-06
 */

#include "X3X2ListModeFrameDecoder.h"
#include "ClassLoader.h"

namespace FrameReceiver
{
  /**
   * Registration of this decoder through the ClassLoader.  This macro
   * registers the class without needing to worry about name mangling
   */
  REGISTER(FrameDecoder, X3X2ListModeFrameDecoder, "X3X2ListModeFrameDecoder");
}
// namespace FrameReceiver
