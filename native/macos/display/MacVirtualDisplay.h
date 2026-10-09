// SecondScreen — Zoavintsoa
// Experimental macOS virtual-display prototype.
// Uses undocumented CoreGraphics runtime classes. Not for App Store distribution.
// Copyright (c) 2026 Zoavintsoa. All rights reserved.

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

NS_ASSUME_NONNULL_BEGIN

@interface SecondScreenMacVirtualDisplayManager : NSObject

@property(nonatomic, readonly) CGDirectDisplayID displayID;
@property(nonatomic, readonly) BOOL isDisplayActive;
@property(nonatomic, readonly) uint32_t width;
@property(nonatomic, readonly) uint32_t height;
@property(nonatomic, copy, readonly) NSString *lastError;

- (BOOL)createVirtualDisplayWithWidth:(uint32_t)width
                               height:(uint32_t)height
                          refreshRate:(double)refreshRate
                                 name:(NSString *)name;
- (void)destroyVirtualDisplay;

@end

NS_ASSUME_NONNULL_END
