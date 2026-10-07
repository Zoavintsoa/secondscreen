#import <CoreGraphics/CoreGraphics.h>

#include "LegacyCapture.h"

namespace secondscreen::macos {

bool LegacyCapture::isSupported() {
    // A display being present is not enough to claim that the legacy capture
    // backend is implemented. Keep this false until real frame acquisition is
    // wired and validated on Catalina/Big Sur hardware.
    return false;
}

std::string LegacyCapture::backendName() {
    return "CoreGraphics legacy capture boundary (not implemented)";
}

} // namespace secondscreen::macos
