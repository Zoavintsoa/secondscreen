#import <CoreGraphics/CoreGraphics.h>

#include "LegacyCapture.h"

namespace secondscreen::macos {

bool LegacyCapture::isSupported() {
    return CGMainDisplayID() != 0;
}

std::string LegacyCapture::backendName() {
    return "CoreGraphics legacy capture boundary";
}

} // namespace secondscreen::macos
