// SecondScreen macOS virtual-display boundary.
//
// IMPORTANT:
// Apple does not expose a supported public API that lets an ordinary application
// create an OS-level virtual monitor on the legacy macOS deployment targets we
// support. The previous version of this file declared private CGVirtualDisplay
// classes. That was architecturally incorrect for a distributable product.
//
// This file intentionally contains NO private CoreGraphics/CoreDisplay symbols.
// A future public/entitled implementation must be introduced behind this
// boundary only after its platform availability, signing and distribution model
// are verified.
//
// Copyright (c) 2026 Zoavintsoa. All rights reserved.

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

@interface SecondScreenMacVirtualDisplayManager : NSObject
@property(nonatomic, readonly) CGDirectDisplayID displayID;
@property(nonatomic, readonly) BOOL isDisplayActive;
@property(nonatomic, readonly) uint32_t width;
@property(nonatomic, readonly) uint32_t height;

- (BOOL)createVirtualDisplayWithWidth:(uint32_t)width
                               height:(uint32_t)height
                          refreshRate:(double)refreshRate
                                 name:(NSString *)name;
- (void)destroyVirtualDisplay;
@end

@implementation SecondScreenMacVirtualDisplayManager {
    CGDirectDisplayID _displayID;
    BOOL _isDisplayActive;
    uint32_t _width;
    uint32_t _height;
}

@synthesize displayID = _displayID;
@synthesize isDisplayActive = _isDisplayActive;
@synthesize width = _width;
@synthesize height = _height;

- (BOOL)createVirtualDisplayWithWidth:(uint32_t)width
                               height:(uint32_t)height
                          refreshRate:(double)refreshRate
                                 name:(NSString *)name {
    (void)width;
    (void)height;
    (void)refreshRate;
    (void)name;

    NSLog(@"[SecondScreen macOS] Public virtual-display API is not available on this target; virtual display creation remains disabled.");
    [self destroyVirtualDisplay];
    return NO;
}

- (void)destroyVirtualDisplay {
    _displayID = 0;
    _isDisplayActive = NO;
    _width = 0;
    _height = 0;
}

@end
