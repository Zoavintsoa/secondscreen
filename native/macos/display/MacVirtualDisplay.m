// SecondScreen macOS Virtual Display Engine
// Manages OS-level Display 2 allocation with technical truth documentation
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

/**
 * TECHNICAL TRUTH NOTE:
 * Apple provides virtual display mechanisms across different macOS tiers:
 * 1. macOS 12.3+ / 13 / 14 / 15: CGVirtualDisplay (CoreDisplay private framework)
 *    - Dynamic runtime allocation without kernel extensions
 *    - Used by production remote desktop applications under TCC Screen Recording permissions
 * 2. macOS DriverKit (IOUserGraphicsDriver / IOUserDisplayDriver):
 *    - Requires 'com.apple.developer.driverkit.user-client-access' entitlement from Apple Developer program.
 */

@interface CGVirtualDisplayDescriptor : NSObject
@property (nonatomic, copy) NSString *name;
@property (nonatomic) uint32_t maxPixelsWide;
@property (nonatomic) uint32_t maxPixelsHigh;
@property (nonatomic) CGSize sizeInMillimeters;
@property (nonatomic) uint32_t serialNum;
@property (nonatomic) uint32_t productID;
@property (nonatomic) uint32_t vendorID;
@property (nonatomic, strong) dispatch_queue_t queue;
@end

@interface CGVirtualDisplaySettings : NSObject
@property (nonatomic) uint32_t modesCount;
@end

@interface CGVirtualDisplayMode : NSObject
- (instancetype)initWithWidth:(uint32_t)width height:(uint32_t)height refreshRate:(double)refreshRate;
@end

@interface CGVirtualDisplay : NSObject
- (instancetype)initWithDescriptor:(CGVirtualDisplayDescriptor *)descriptor;
- (BOOL)applySettings:(CGVirtualDisplaySettings *)settings;
@property (nonatomic, readonly) CGDirectDisplayID displayID;
@end

@interface SecondScreenMacVirtualDisplayManager : NSObject

@property (nonatomic, strong) CGVirtualDisplay *virtualDisplay;
@property (nonatomic, readonly) CGDirectDisplayID displayID;
@property (nonatomic, readonly) BOOL isDisplayActive;
@property (nonatomic, readonly) uint32_t width;
@property (nonatomic, readonly) uint32_t height;

- (BOOL)createVirtualDisplayWithWidth:(uint32_t)width 
                               height:(uint32_t)height 
                          refreshRate:(double)refreshRate 
                                 name:(NSString *)name;

- (void)destroyVirtualDisplay;

@end

@implementation SecondScreenMacVirtualDisplayManager

- (BOOL)createVirtualDisplayWithWidth:(uint32_t)width 
                               height:(uint32_t)height 
                          refreshRate:(double)refreshRate 
                                 name:(NSString *)name {
    [self destroyVirtualDisplay];

    Class descriptorClass = NSClassFromString(@"CGVirtualDisplayDescriptor");
    Class displayClass = NSClassFromString(@"CGVirtualDisplay");

    if (!descriptorClass || !displayClass) {
        NSLog(@"[SecondScreen macOS] CGVirtualDisplay class symbols not dynamically resolved in runtime.");
        return NO;
    }

    CGVirtualDisplayDescriptor *descriptor = [[descriptorClass alloc] init];
    descriptor.name = name ?: @"SecondScreen Virtual Display (Display 2)";
    descriptor.maxPixelsWide = width;
    descriptor.maxPixelsHigh = height;
    descriptor.sizeInMillimeters = CGSizeMake(320, 200);
    descriptor.serialNum = 202601;
    descriptor.productID = 0x5343; // 'SC'
    descriptor.vendorID = 0x5332;  // 'S2'
    descriptor.queue = dispatch_get_main_queue();

    self.virtualDisplay = [[displayClass alloc] initWithDescriptor:descriptor];
    if (!self.virtualDisplay) {
        NSLog(@"[SecondScreen macOS] Failed to allocate virtual display instance.");
        return NO;
    }

    _displayID = self.virtualDisplay.displayID;
    _width = width;
    _height = height;
    _isDisplayActive = YES;

    NSLog(@"[SecondScreen macOS] Successfully allocated macOS Display 2 with CGDirectDisplayID: %u (%ux%u @ %.1fHz)", 
          _displayID, width, height, refreshRate);
    return YES;
}

- (void)destroyVirtualDisplay {
    if (self.virtualDisplay) {
        self.virtualDisplay = nil;
        _isDisplayActive = NO;
        _displayID = 0;
        NSLog(@"[SecondScreen macOS] Display 2 destroyed and detached from OS desktop topology.");
    }
}

- (void)dealloc {
    [self destroyVirtualDisplay];
}

@end
