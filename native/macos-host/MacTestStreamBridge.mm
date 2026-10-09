#import "MacTestStreamBridge.h"
#import "MacUsbAccessoryBridge.h"
#import <CoreGraphics/CoreGraphics.h>
#import <CoreVideo/CoreVideo.h>
#import <VideoToolbox/VideoToolbox.h>
#import <IOSurface/IOSurface.h>
#import <os/log.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

namespace {
constexpr uint16_t kDiscoveryPort = 49151;
constexpr uint16_t kVideoPort = 49153;
constexpr int kWidth = 1280;
constexpr int kHeight = 720;
constexpr int kFps = 30;
constexpr int kBitrate = 5 * 1000 * 1000;

struct State {
    std::atomic<bool> running{false};
    std::thread discoveryThread;
    std::thread serverThread;
    CGDisplayStreamRef stream = nullptr;
    VTCompressionSessionRef encoder = nullptr;
    std::mutex socketMutex;
    int client = -1;
    std::uint32_t frameId = 0;
};

State g;
os_log_t gLog = os_log_create("com.zoavintsoa.secondscreen", "TestStream");

bool sendAll(int fd, const std::uint8_t* data, size_t size) {
    while (size > 0) {
        const ssize_t n = send(fd, data, size, MSG_NOSIGNAL);
        if (n <= 0) return false;
        data += n;
        size -= static_cast<size_t>(n);
    }
    return true;
}

void putU16(std::vector<std::uint8_t>& b, std::uint16_t v) {
    b.push_back(static_cast<std::uint8_t>(v >> 8));
    b.push_back(static_cast<std::uint8_t>(v));
}
void putU32(std::vector<std::uint8_t>& b, std::uint32_t v) {
    b.push_back(static_cast<std::uint8_t>(v >> 24));
    b.push_back(static_cast<std::uint8_t>(v >> 16));
    b.push_back(static_cast<std::uint8_t>(v >> 8));
    b.push_back(static_cast<std::uint8_t>(v));
}
void putU64(std::vector<std::uint8_t>& b, std::uint64_t v) {
    for (int i = 7; i >= 0; --i) b.push_back(static_cast<std::uint8_t>(v >> (i * 8)));
}
void putStartCode(std::vector<std::uint8_t>& b) {
    b.push_back(0); b.push_back(0); b.push_back(0); b.push_back(1);
}

bool isKeyframe(CMSampleBufferRef sample) {
    CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray(sample, false);
    if (!attachments || CFArrayGetCount(attachments) == 0) return true;
    auto dict = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(attachments, 0));
    if (!dict) return true;
    auto notSync = static_cast<CFBooleanRef>(
        CFDictionaryGetValue(dict, kCMSampleAttachmentKey_NotSync));
    return !(notSync && CFBooleanGetValue(notSync));
}

std::vector<std::uint8_t> annexBFromSample(CMSampleBufferRef sample, bool keyframe) {
    std::vector<std::uint8_t> out;

    if (keyframe) {
        CMVideoFormatDescriptionRef format =
            CMSampleBufferGetFormatDescription(sample);
        if (format) {
            for (size_t i = 0; i < 2; ++i) {
                const std::uint8_t* parameterSet = nullptr;
                size_t parameterSetSize = 0;
                size_t parameterSetCount = 0;
                int nalHeaderLength = 0;
                if (CMVideoFormatDescriptionGetH264ParameterSetAtIndex(
                        format, i, &parameterSet, &parameterSetSize,
                        &parameterSetCount, &nalHeaderLength) == noErr &&
                    parameterSet && parameterSetSize > 0) {
                    putStartCode(out);
                    out.insert(out.end(), parameterSet, parameterSet + parameterSetSize);
                }
            }
        }
    }

    CMBlockBufferRef block = CMSampleBufferGetDataBuffer(sample);
    if (!block) return {};

    // CMBlockBufferGetDataPointer can fail when a sample is backed by multiple
    // non-contiguous memory blocks. CopyDataBytes handles both contiguous and
    // non-contiguous buffers and keeps the parser independent of encoder layout.
    const size_t length = CMBlockBufferGetDataLength(block);
    if (length == 0 || length > 16 * 1024 * 1024) return {};
    std::vector<std::uint8_t> data(length);
    if (CMBlockBufferCopyDataBytes(block, 0, length, data.data()) != kCMBlockBufferNoErr) {
        return {};
    }

    size_t offset = 0;
    while (offset + 4 <= length) {
        const std::uint32_t nalLength =
            (static_cast<std::uint32_t>(data[offset]) << 24) |
            (static_cast<std::uint32_t>(data[offset + 1]) << 16) |
            (static_cast<std::uint32_t>(data[offset + 2]) << 8) |
            static_cast<std::uint32_t>(data[offset + 3]);
        offset += 4;
        if (nalLength == 0 || static_cast<size_t>(nalLength) > length - offset) return {};
        putStartCode(out);
        out.insert(out.end(), data.begin() + static_cast<std::ptrdiff_t>(offset),
                   data.begin() + static_cast<std::ptrdiff_t>(offset + nalLength));
        offset += nalLength;
    }
    if (offset != length) return {};
    return out;
}

void encoderCallback(void* refcon,
                     void* sourceFrameRefcon,
                     OSStatus status,
                     VTEncodeInfoFlags infoFlags,
                     CMSampleBufferRef sample) {
    (void)refcon;
    (void)sourceFrameRefcon;
    (void)infoFlags;
    if (status != noErr || !sample || !CMSampleBufferDataIsReady(sample) || !g.running.load())
        return;

    const bool keyframe = isKeyframe(sample);
    auto payload = annexBFromSample(sample, keyframe);
    if (payload.empty() || payload.size() > 16 * 1024 * 1024) return;

    const auto pts = CMSampleBufferGetPresentationTimeStamp(sample);
    const std::uint64_t timestampUs =
        pts.timescale > 0 ? static_cast<std::uint64_t>(
            (static_cast<double>(pts.value) / pts.timescale) * 1000000.0) : 0;

    std::vector<std::uint8_t> frame;
    frame.reserve(20 + payload.size());
    frame.push_back('S'); frame.push_back('S'); frame.push_back('V'); frame.push_back('F');
    frame.push_back(1); // H.264
    frame.push_back(keyframe ? 1 : 0);
    putU16(frame, 0);
    putU64(frame, timestampUs);
    putU32(frame, static_cast<std::uint32_t>(payload.size()));
    frame.insert(frame.end(), payload.begin(), payload.end());

    {
        std::lock_guard<std::mutex> lock(g.socketMutex);
        if (g.client >= 0 && !sendAll(g.client, frame.data(), frame.size())) {
            close(g.client);
            g.client = -1;
        }
    }
    // USB is an additional transport. A connected Android accessory receives the
    // exact same SSVF frame, so decoding/recovery stays identical across LAN/USB.
    SecondScreenUsbAccessorySend(frame.data(), frame.size());
    ++g.frameId;
}

void discoveryLoop() {
    const char payload[] =
        "{\"service\":\"secondscreen\",\"name\":\"SecondScreen iMac\","
        "\"mode\":\"test\",\"controlPort\":49152,\"videoPort\":49153,"
        "\"width\":1280,\"height\":720,\"fps\":30}";

    while (g.running.load()) {
        ifaddrs* interfaces = nullptr;
        if (getifaddrs(&interfaces) != 0) {
            os_log_error(gLog, "getifaddrs failed: %{public}d", errno);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        for (ifaddrs* entry = interfaces; entry != nullptr && g.running.load(); entry = entry->ifa_next) {
            if (!entry->ifa_addr || entry->ifa_addr->sa_family != AF_INET) continue;
            if (!(entry->ifa_flags & IFF_UP) || !(entry->ifa_flags & IFF_RUNNING)) continue;
            if (entry->ifa_flags & IFF_LOOPBACK) continue;

            auto* address = reinterpret_cast<sockaddr_in*>(entry->ifa_addr);
            auto* netmask = reinterpret_cast<sockaddr_in*>(entry->ifa_netmask);
            auto* broadcast = reinterpret_cast<sockaddr_in*>(entry->ifa_broadaddr);

            sockaddr_in calculated{};
            if (!broadcast || broadcast->sin_addr.s_addr == 0) {
                if (!netmask) continue;
                calculated.sin_family = AF_INET;
                calculated.sin_addr.s_addr =
                    address->sin_addr.s_addr | ~netmask->sin_addr.s_addr;
                broadcast = &calculated;
            }

            const int fd = socket(AF_INET, SOCK_DGRAM, 0);
            if (fd < 0) {
                os_log_error(gLog, "Discovery socket creation failed on %{public}s: %{public}d",
                             entry->ifa_name, errno);
                continue;
            }

            int yes = 1;
            if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) < 0) {
                os_log_error(gLog, "SO_BROADCAST failed on %{public}s: %{public}d",
                             entry->ifa_name, errno);
                close(fd);
                continue;
            }

            // REMOVED: bind(fd, entry->ifa_addr, sizeof(sockaddr_in))
            // Reason: Binding to a specific host address on macOS often blocks UDP broadcast emission.

            sockaddr_in destination = *broadcast;
            destination.sin_family = AF_INET;
            destination.sin_port = htons(kDiscoveryPort);

            char broadcastText[INET_ADDRSTRLEN] = {};
            inet_ntop(AF_INET, &destination.sin_addr, broadcastText, sizeof(broadcastText));

            const ssize_t sent = sendto(
                fd, payload, sizeof(payload) - 1, 0,
                reinterpret_cast<sockaddr*>(&destination), sizeof(destination));

            if (sent < 0) {
                os_log_error(gLog,
                             "Discovery send failed on %{public}s to %{public}s:%{public}d: %{public}d",
                             entry->ifa_name, broadcastText, kDiscoveryPort, errno);
            } else {
                os_log(gLog,
                       "Discovery broadcast sent on %{public}s to %{public}s:%{public}d (%{public}ld bytes)",
                       entry->ifa_name, broadcastText, kDiscoveryPort, (long)sent);
            }

            close(fd);
        }

        freeifaddrs(interfaces);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void serverLoop() {
    const int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) return;

    int yes = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    const int flags = fcntl(listener, F_GETFL, 0);
    if (flags >= 0) fcntl(listener, F_SETFL, flags | O_NONBLOCK);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(kVideoPort);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        os_log_error(gLog, "Video TCP bind failed: %{public}d", errno);
        close(listener);
        return;
    }
    if (listen(listener, 1) < 0) {
        os_log_error(gLog, "Video TCP listen failed: %{public}d", errno);
        close(listener);
        return;
    }
    os_log(gLog, "Video TCP server listening on %{public}d", kVideoPort);

    while (g.running.load()) {
        sockaddr_in peer{};
        socklen_t peerLength = sizeof(peer);
        const int client = accept(listener, reinterpret_cast<sockaddr*>(&peer), &peerLength);
        if (client < 0) {
            if (!g.running.load()) break;
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            os_log_error(gLog, "Video TCP accept failed: %{public}d", errno);
            continue;
        }
        os_log(gLog, "Android video client connected");

        {
            std::lock_guard<std::mutex> lock(g.socketMutex);
            if (g.client >= 0) close(g.client);
            g.client = client;
        }

        while (g.running.load()) {
            char byte;
            const ssize_t n = recv(client, &byte, 1, MSG_PEEK);
            if (n == 0 || (n < 0 && errno != EAGAIN && errno != EINTR)) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        std::lock_guard<std::mutex> lock(g.socketMutex);
        if (g.client == client) {
            close(g.client);
            g.client = -1;
        } else {
            close(client);
        }
    }

    close(listener);
}

bool startEncoder() {
    os_log(gLog, "Starting VideoToolbox H.264 encoder");
    const OSStatus createStatus = VTCompressionSessionCreate(
        kCFAllocatorDefault, kWidth, kHeight, kCMVideoCodecType_H264,
        nullptr, nullptr, nullptr, encoderCallback, nullptr, &g.encoder);
    if (createStatus != noErr || !g.encoder) {
        os_log_error(gLog, "VTCompressionSessionCreate failed: %{public}d", (int)createStatus);
        return false;
    }

    const int32_t bitrate = kBitrate;
    const int32_t fps = kFps;
    const int32_t keyInterval = kFps;

    VTSessionSetProperty(g.encoder, kVTCompressionPropertyKey_RealTime, kCFBooleanTrue);

    CFNumberRef bitrateRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &bitrate);
    CFNumberRef fpsRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &fps);
    CFNumberRef keyRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &keyInterval);

    if (bitrateRef) {
        VTSessionSetProperty(g.encoder, kVTCompressionPropertyKey_AverageBitRate, bitrateRef);
        CFRelease(bitrateRef);
    }
    if (fpsRef) {
        VTSessionSetProperty(g.encoder, kVTCompressionPropertyKey_ExpectedFrameRate, fpsRef);
        CFRelease(fpsRef);
    }
    if (keyRef) {
        VTSessionSetProperty(g.encoder, kVTCompressionPropertyKey_MaxKeyFrameInterval, keyRef);
        CFRelease(keyRef);
    }

    VTSessionSetProperty(g.encoder, kVTCompressionPropertyKey_AllowFrameReordering, kCFBooleanFalse);
    const OSStatus prepareStatus = VTCompressionSessionPrepareToEncodeFrames(g.encoder);
    if (prepareStatus != noErr) {
        os_log_error(gLog, "VTCompressionSessionPrepareToEncodeFrames failed: %{public}d", (int)prepareStatus);
        VTCompressionSessionInvalidate(g.encoder);
        CFRelease(g.encoder);
        g.encoder = nullptr;
        return false;
    }
    os_log(gLog, "VideoToolbox encoder ready");
    return true;
}

void startCapture() {
    os_log(gLog, "Starting CGDisplayStream");
    const CGDirectDisplayID display = CGMainDisplayID();
    const double minimumFrameTime = 1.0 / kFps;
    CFNumberRef frameTime = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType,
                                           &minimumFrameTime);
    const void* keys[] = {kCGDisplayStreamMinimumFrameTime};
    const void* values[] = {frameTime};
    CFDictionaryRef properties = CFDictionaryCreate(
        kCFAllocatorDefault, keys, values, 1,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (frameTime) CFRelease(frameTime);

    g.stream = CGDisplayStreamCreateWithDispatchQueue(
        display, kWidth, kHeight, kCVPixelFormatType_32BGRA, properties,
        dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0),
        ^(CGDisplayStreamFrameStatus status, uint64_t displayTime, IOSurfaceRef frameSurface,
          CGDisplayStreamUpdateRef updateRef) {
            (void)updateRef;
            if (status != kCGDisplayStreamFrameStatusFrameComplete || !frameSurface ||
                !g.encoder || !g.running.load()) return;

            CVPixelBufferRef pixelBuffer = nullptr;
            if (CVPixelBufferCreateWithIOSurface(kCFAllocatorDefault, frameSurface, nullptr,
                                                 &pixelBuffer) != kCVReturnSuccess ||
                !pixelBuffer) return;

            // CGDisplayStream's displayTime uses the host clock's system units, not
    // nanoseconds. Convert it with CoreMedia so VideoToolbox receives valid PTS.
    CMTime pts = CMClockMakeHostTimeFromSystemUnits(displayTime);
            VTEncodeInfoFlags flags = 0;
            VTCompressionSessionEncodeFrame(g.encoder, pixelBuffer, pts,
                                            CMTimeMake(1, kFps), nullptr, nullptr, &flags);
            CVPixelBufferRelease(pixelBuffer);
        });

    if (properties) CFRelease(properties);
    if (!g.stream) {
        os_log_error(gLog, "CGDisplayStreamCreateWithDispatchQueue returned null");
        return;
    }
    const CGError startStatus = CGDisplayStreamStart(g.stream);
    os_log(gLog, "CGDisplayStreamStart returned: %{public}d", (int)startStatus);
    if (startStatus != kCGErrorSuccess) {
        os_log_error(gLog, "CGDisplayStreamStart failed; releasing capture stream");
        CFRelease(g.stream);
        g.stream = nullptr;
    }
}

} // namespace

extern "C" void SecondScreenStartTestStream(void) {
    if (g.running.exchange(true)) return;

    os_log(gLog, "SecondScreenStartTestStream");

    if (!CGPreflightScreenCaptureAccess()) {
        os_log(gLog, "Screen Recording permission is not granted; requesting access");
        const bool requested = CGRequestScreenCaptureAccess();
        os_log(gLog, "CGRequestScreenCaptureAccess returned: %{public}s", requested ? "true" : "false");
        g.running = false;
        return;
    }

    if (!startEncoder()) {
        g.running = false;
        return;
    }

    g.discoveryThread = std::thread(discoveryLoop);
    g.serverThread = std::thread(serverLoop);
    SecondScreenStartUsbAccessory();
    startCapture();

    if (!g.stream) {
        os_log_error(gLog, "Test stream aborted: display capture could not be started");
        // Use the same complete cleanup path as a normal stop. In particular,
        // also join the server thread and stop the USB worker after partial startup.
        SecondScreenStopTestStream();
    } else {
        os_log(gLog, "SecondScreen test stream is running");
    }
}

extern "C" void SecondScreenStopTestStream(void) {
    // Do not return just because startup already cleared running: startup can
    // fail after creating worker threads, and those threads still need joining.
    g.running.exchange(false);

    if (g.stream) {
        CGDisplayStreamStop(g.stream);
        CFRelease(g.stream);
        g.stream = nullptr;
    }
    if (g.encoder) {
        // Drain any callbacks already queued by VideoToolbox before invalidating
        // and releasing the compression session.
        VTCompressionSessionCompleteFrames(g.encoder, kCMTimeInvalid);
        VTCompressionSessionInvalidate(g.encoder);
        CFRelease(g.encoder);
        g.encoder = nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(g.socketMutex);
        if (g.client >= 0) {
            shutdown(g.client, SHUT_RDWR);
            close(g.client);
            g.client = -1;
        }
    }

    SecondScreenStopUsbAccessory();
    if (g.discoveryThread.joinable()) g.discoveryThread.join();
    if (g.serverThread.joinable()) g.serverThread.join();
}
