#include "X3X2ListModeProcessPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{
    /**
     * Registration of this process plugin through the ClassLoader.  This macro
     * registers the class without needing to worry about name mangling
     */
    REGISTER(FrameProcessorPlugin, X3X2ListModeProcessPlugin, "X3X2ListModeProcessPlugin");

} // namespace FrameReceiver

