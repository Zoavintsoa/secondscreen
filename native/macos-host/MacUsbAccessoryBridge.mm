#import "MacUsbAccessoryBridge.h"
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/usb/USB.h>
#include <CoreFoundation/CoreFoundation.h>
#include <os/log.h>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

namespace {
constexpr UInt16 kGoogleVendor = 0x18D1;
constexpr UInt16 kAccessoryPid = 0x2D00;
constexpr UInt16 kAccessoryAdbPid = 0x2D01;
constexpr UInt8 kAoAGetProtocol = 51;
constexpr UInt8 kAoASendString = 52;
constexpr UInt8 kAoAStart = 53;
constexpr UInt8 kUsbDirIn = 0x80;
constexpr UInt8 kUsbDirOut = 0x00;
constexpr UInt8 kUsbTypeVendor = 0x40;
constexpr UInt8 kUsbRecipientDevice = 0x00;

struct AccessoryState {
    IOUSBDeviceInterface182** device = nullptr;
    IOUSBInterfaceInterface182** iface = nullptr;
    UInt8 inPipe = 0;
    UInt8 outPipe = 0;
    std::mutex mutex;
    bool ready = false;
};
std::atomic<bool> gRunning{false};
std::thread gThread;
AccessoryState g;
os_log_t gLog = os_log_create("com.zoavintsoa.secondscreen", "USB");

void releaseAccessoryLocked() {
    if (g.iface) {
        g.iface->USBInterfaceClose(g.iface);
        g.iface->Release(g.iface);
        g.iface = nullptr;
    }
    if (g.device) {
        g.device->USBDeviceClose(g.device);
        g.device->Release(g.device);
        g.device = nullptr;
    }
    g.inPipe = g.outPipe = 0;
    g.ready = false;
}

bool deviceRequest(IOUSBDeviceInterface** device, UInt8 type, UInt8 request,
                   UInt16 value, UInt16 index, void* data, UInt16 length) {
    IOUSBDevRequest req{};
    req.bmRequestType = type;
    req.bRequest = request;
    req.wValue = value;
    req.wIndex = index;
    req.wLength = length;
    req.pData = data;
    req.wLenDone = 0;
    req.completionTimeout = 1000;
    req.noDataTimeout = 1000;
    return device->DeviceRequestTO(device, &req) == kIOReturnSuccess;
}

bool sendString(IOUSBDeviceInterface** device, UInt16 id, const char* value) {
    std::vector<char> data(value, value + std::strlen(value) + 1);
    if (data.size() > 255) return false;
    return deviceRequest(device, kUsbDirOut | kUsbTypeVendor | kUsbRecipientDevice,
                         kAoASendString, 0, id, data.data(), static_cast<UInt16>(data.size()));
}

bool startAccessoryMode(IOUSBDeviceInterface** device) {
    UInt16 protocol = 0;
    if (!deviceRequest(device, kUsbDirIn | kUsbTypeVendor | kUsbRecipientDevice,
                       kAoAGetProtocol, 0, 0, &protocol, sizeof(protocol)) || protocol < 1)
        return false;
    if (!sendString(device, 0, "Zoavintsoa")) return false;
    if (!sendString(device, 1, "SecondScreen")) return false;
    if (!sendString(device, 2, "SecondScreen USB Display")) return false;
    if (!sendString(device, 3, "1")) return false;
    if (!sendString(device, 4, "https://github.com/Zoavintsoa/secondscreen")) return false;
    if (!sendString(device, 5, "secondscreen-usb")) return false;
    return deviceRequest(device, kUsbDirOut | kUsbTypeVendor | kUsbRecipientDevice,
                         kAoAStart, 0, 0, nullptr, 0);
}

bool createDeviceInterface(io_service_t service, IOUSBDeviceInterface182*** out) {
    IOCFPlugInInterface** plugin = nullptr; SInt32 score = 0;
    if (IOCreatePlugInInterfaceForService(service, kIOUSBDeviceUserClientTypeID,
        kIOCFPlugInInterfaceID, &plugin, &score) != kIOReturnSuccess || !plugin) return false;
    HRESULT result = (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID182),
        reinterpret_cast<LPVOID*>(out));
    (*plugin)->Release(plugin);
    return result == S_OK && *out;
}
bool createInterface(io_service_t service, IOUSBInterfaceInterface182*** out) {
    IOCFPlugInInterface** plugin = nullptr; SInt32 score = 0;
    if (IOCreatePlugInInterfaceForService(service, kIOUSBInterfaceUserClientTypeID,
        kIOCFPlugInInterfaceID, &plugin, &score) != kIOReturnSuccess || !plugin) return false;
    HRESULT result = (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID182),
        reinterpret_cast<LPVOID*>(out));
    (*plugin)->Release(plugin);
    return result == S_OK && *out;
}
bool openAccessoryDevice(io_service_t service) {
    IOUSBDeviceInterface** device = nullptr;
    if (!createDeviceInterface(service, &device)) return false;
    UInt16 vendor = 0, product = 0;
    if (device->GetDeviceVendor(device, &vendor) != kIOReturnSuccess ||
        device->GetDeviceProduct(device, &product) != kIOReturnSuccess ||
        vendor != kGoogleVendor || (product != kAccessoryPid && product != kAccessoryAdbPid)) {
        device->Release(device); return false;
    }
    if (device->USBDeviceOpenSeize(device) != kIOReturnSuccess ||
        device->SetConfiguration(device, 1) != kIOReturnSuccess) {
        device->USBDeviceClose(device); device->Release(device); return false;
    }
    io_iterator_t iterator = IO_OBJECT_NULL;
    IOUSBFindInterfaceRequest interfaceRequest{};
    interfaceRequest.bInterfaceClass = kIOUSBFindInterfaceDontCare;
    interfaceRequest.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
    interfaceRequest.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
    interfaceRequest.bAlternateSetting = kIOUSBFindInterfaceDontCare;
    if (device->CreateInterfaceIterator(device, &interfaceRequest, &iterator) != kIOReturnSuccess) {
        device->USBDeviceClose(device); device->Release(device); return false;
    }
    IOUSBInterfaceInterface** chosen = nullptr; UInt8 inPipe = 0, outPipe = 0;
    while (io_service_t intfService = IOIteratorNext(iterator)) {
        IOUSBInterfaceInterface182** candidate = nullptr;
        if (createInterface(intfService, &candidate)) {
            UInt8 count = 0;
            if (candidate->GetNumEndpoints(candidate, &count) == kIOReturnSuccess) {
                UInt8 cin = 0, cout = 0;
                for (UInt8 pipe = 1; pipe <= count; ++pipe) {
                    UInt8 direction = 0, number = 0, type = 0, interval = 0; UInt16 packet = 0;
                    if (candidate->GetPipeProperties(candidate, pipe, &direction, &number, &type,
                                                     &packet, &interval) != kIOReturnSuccess) continue;
                    if (type != kUSBBulk) continue;
                    if (direction == kUSBIn && !cin) cin = pipe;
                    if (direction == kUSBOut && !cout) cout = pipe;
                }
                if (cin && cout && candidate->USBInterfaceOpenSeize(candidate) == kIOReturnSuccess) {
                    chosen = candidate; inPipe = cin; outPipe = cout; break;
                }
            }
        }
        if (candidate && candidate != chosen) candidate->Release(candidate);
        IOObjectRelease(intfService);
    }
    IOObjectRelease(iterator);
    if (!chosen) {
        device->USBDeviceClose(device); device->Release(device); return false;
    }
    std::lock_guard<std::mutex> lock(g.mutex);
    releaseAccessoryLocked();
    g.device = device; g.iface = chosen; g.inPipe = inPipe; g.outPipe = outPipe; g.ready = true;
    os_log(gLog, "SecondScreen USB accessory ready");
    return true;
}
bool findAccessory(io_service_t* result) {
    CFMutableDictionaryRef matching = IOServiceMatching(kIOUSBDeviceClassName);
    if (!matching) return false;
    io_iterator_t iterator = IO_OBJECT_NULL;
    if (IOServiceGetMatchingServices(kIOMasterPortDefault, matching, &iterator) != kIOReturnSuccess) return false;
    io_service_t service = IOIteratorNext(iterator);
    while (service) {
        IOUSBDeviceInterface182** device = nullptr; bool match = false;
        if (createDeviceInterface(service, &device)) {
            UInt16 v = 0, p = 0;
            if (device->GetDeviceVendor(device, &v) == kIOReturnSuccess &&
                device->GetDeviceProduct(device, &p) == kIOReturnSuccess)
                match = v == kGoogleVendor && (p == kAccessoryPid || p == kAccessoryAdbPid);
            device->Release(device);
        }
        io_object_t next = IOIteratorNext(iterator);
        if (match) {
            *result = service;
            if (next) IOObjectRelease(next);
            IOObjectRelease(iterator);
            return true;
        }
        IOObjectRelease(service); service = next;
    }
    IOObjectRelease(iterator); return false;
}
void requestAccessoryMode() {
    CFMutableDictionaryRef matching = IOServiceMatching(kIOUSBDeviceClassName);
    if (!matching) return;
    io_iterator_t iterator = IO_OBJECT_NULL;
    if (IOServiceGetMatchingServices(kIOMasterPortDefault, matching, &iterator) != kIOReturnSuccess) return;
    io_service_t service = IOIteratorNext(iterator);
    while (service && gRunning.load()) {
        IOUSBDeviceInterface** device = nullptr;
        if (createDeviceInterface(service, &device)) {
            UInt16 v = 0, p = 0;
            if (device->GetDeviceVendor(device, &v) == kIOReturnSuccess &&
                device->GetDeviceProduct(device, &p) == kIOReturnSuccess &&
                v != kGoogleVendor && startAccessoryMode(device)) {
                os_log(gLog, "Android accepted Open Accessory request");
                device->Release(device); IOObjectRelease(service); break;
            }
            device->Release(device);
        }
        IOObjectRelease(service); service = IOIteratorNext(iterator);
    }
    if (iterator != IO_OBJECT_NULL) IOObjectRelease(iterator);
}
void worker() {
    while (gRunning.load()) {
        io_service_t service = IO_OBJECT_NULL;
        if (findAccessory(&service)) {
            openAccessoryDevice(service); IOObjectRelease(service);
        } else {
            requestAccessoryMode();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
} // namespace

extern "C" void SecondScreenStartUsbAccessory(void) {
    if (gRunning.exchange(true)) return;
    gThread = std::thread(worker);
}
extern "C" void SecondScreenStopUsbAccessory(void) {
    if (!gRunning.exchange(false)) return;
    if (gThread.joinable()) gThread.join();
    std::lock_guard<std::mutex> lock(g.mutex);
    releaseAccessoryLocked();
}
extern "C" bool SecondScreenUsbAccessorySend(const std::uint8_t* data, std::size_t size) {
    if (!gRunning.load()) return false;
    std::lock_guard<std::mutex> lock(g.mutex);
    if (!g.ready || !g.iface || !data || size == 0 || size > UINT32_MAX) return false;
    const IOReturn result = g.iface->WritePipeTO(g.iface, g.outPipe,
        const_cast<std::uint8_t*>(data), static_cast<UInt32>(size), 100, 1000);
    if (result != kIOReturnSuccess) {
        releaseAccessoryLocked(); return false;
    }
    return true;
}
