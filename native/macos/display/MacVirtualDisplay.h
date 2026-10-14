// SecondScreen — Zoavintsoa
// Experimental macOS virtual-display prototype.
// Uses undocumented CoreGraphics runtime classes. Not for App Store distribution.
// Copyright (c) 2026 Zoavintsoa. All rights reserved.

#import <Foundation/Foundation.h>
#include <stdint.h>
#import <CoreGraphics/CoreGraphics.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^SecondScreenVirtualDisplayCompletion)(BOOL success,
                                                     NSString * _Nullable errorMessage);

@interface SecondScreenMacVirtualDisplayManager : NSObject

@property(nonatomic, readonly) CGDirectDisplayID displayID;
@property(nonatomic, readonly) BOOL isDisplayActive;
@property(nonatomic, readonly) uint32_t width;
@property(nonatomic, readonly) uint32_t height;
@property(nonatomic, copy, readonly) NSString *lastError;

// Completion is called on the main queue. Display creation starts on the main queue,
// but online confirmation is polled asynchronously to avoid blocking the UI.
- (void)createVirtualDisplayWithWidth:(uint32_t)width
                               height:(uint32_t)height
                          refreshRate:(double)refreshRate
                                 name:(NSString *)name
                           completion:(SecondScreenVirtualDisplayCompletion)completion;
- (void)destroyVirtualDisplay;

@end

NS_ASSUME_NONNULL_END
