// SecondScreen — Zoavintsoa
// Experimental virtual-display adapter using undocumented CoreGraphics runtime classes.
// Keep this implementation isolated: it is not an Apple-supported API and may break
// across macOS releases. Do not ship through the Mac App Store without policy review.

#import "MacVirtualDisplay.h"
#import <math.h>
#import <dispatch/dispatch.h>

// Private, undocumented CoreGraphics interfaces. These declarations only describe
// runtime classes; they do not link against exported private symbols.
@interface CGVirtualDisplayMode : NSObject
- (instancetype)initWithWidth:(NSUInteger)width
                       height:(NSUInteger)height
                  refreshRate:(CGFloat)refreshRate;
@end

@interface CGVirtualDisplaySettings : NSObject
@property(nonatomic, retain) NSArray *modes;
@property(nonatomic) unsigned int hiDPI;
- (instancetype)init;
@end

@class CGVirtualDisplay;
@interface CGVirtualDisplayDescriptor : NSObject
@property(nonatomic, retain) dispatch_queue_t queue;
@property(nonatomic, copy) NSString *name;
@property(nonatomic) unsigned int maxPixelsHigh;
@property(nonatomic) unsigned int maxPixelsWide;
@property(nonatomic) CGSize sizeInMillimeters;
@property(nonatomic) unsigned int serialNum;
@property(nonatomic) unsigned int productID;
@property(nonatomic) unsigned int vendorID;
@property(nonatomic, copy) void (^terminationHandler)(id display, CGVirtualDisplay *mode);
- (instancetype)init;
@end

@interface CGVirtualDisplay : NSObject
@property(nonatomic, readonly) CGDirectDisplayID displayID;
- (instancetype)initWithDescriptor:(CGVirtualDisplayDescriptor *)descriptor;
- (BOOL)applySettings:(CGVirtualDisplaySettings *)settings;
@end

@interface SecondScreenMacVirtualDisplayManager ()
@property(nonatomic, copy, readwrite) NSString *lastError;
@end

@implementation SecondScreenMacVirtualDisplayManager {
    CGVirtualDisplay *_virtualDisplay;
    CGDirectDisplayID _displayID;
    BOOL _isDisplayActive;
    uint32_t _width;
    uint32_t _height;
}

@synthesize displayID = _displayID;
@synthesize isDisplayActive = _isDisplayActive;
@synthesize width = _width;
@synthesize height = _height;
@synthesize lastError = _lastError;

- (BOOL)createVirtualDisplayWithWidth:(uint32_t)width
                               height:(uint32_t)height
                          refreshRate:(double)refreshRate
                                 name:(NSString *)name {
    [self destroyVirtualDisplay];

    NSString *displayName = [name stringByTrimmingCharactersInSet:
                             [NSCharacterSet whitespaceAndNewlineCharacterSet]];
    if (width < 640 || width > 4096 || height < 480 || height > 2160 ||
        !isfinite(refreshRate) || refreshRate < 24.0 || refreshRate > 120.0 ||
        displayName.length == 0) {
        self.lastError = @"Configuration invalide. Utilisez une résolution de 640×480 à 4096×2160 et un rafraîchissement de 24 à 120 Hz.";
        return NO;
    }

    // Fail safely if this macOS build does not provide the private runtime classes.
    if (NSClassFromString(@"CGVirtualDisplayDescriptor") == Nil ||
        NSClassFromString(@"CGVirtualDisplay") == Nil ||
        NSClassFromString(@"CGVirtualDisplaySettings") == Nil ||
        NSClassFromString(@"CGVirtualDisplayMode") == Nil) {
        self.lastError = @"Cette version de macOS ne fournit pas les classes expérimentales de moniteur virtuel attendues.";
        NSLog(@"[SecondScreen macOS] Experimental virtual display unavailable: private runtime classes missing.");
        return NO;
    }

    CGVirtualDisplayDescriptor *descriptor = [[CGVirtualDisplayDescriptor alloc] init];
    if (descriptor == nil) {
        self.lastError = @"Impossible d'initialiser le descripteur du moniteur virtuel.";
        return NO;
    }

    descriptor.name = displayName;
    descriptor.maxPixelsWide = width;
    descriptor.maxPixelsHigh = height;
    descriptor.sizeInMillimeters = CGSizeMake(530.0, 300.0);
    descriptor.vendorID = 0x5A56;  // "ZV" — SecondScreen prototype identifier
    descriptor.productID = 0x0001;
    descriptor.serialNum = 0x0001;
    descriptor.queue = dispatch_get_main_queue();

    __weak SecondScreenMacVirtualDisplayManager *weakSelf = self;
    descriptor.terminationHandler = ^(id terminatedDisplay, CGVirtualDisplay *mode) {
        (void)mode;
        dispatch_async(dispatch_get_main_queue(), ^{
            SecondScreenMacVirtualDisplayManager *strongSelf = weakSelf;
            if (strongSelf != nil && strongSelf->_virtualDisplay == terminatedDisplay) {
                strongSelf->_virtualDisplay = nil;
                strongSelf->_displayID = 0;
                strongSelf->_isDisplayActive = NO;
                strongSelf->_width = 0;
                strongSelf->_height = 0;
                strongSelf.lastError = @"macOS a terminé le moniteur virtuel.";
                NSLog(@"[SecondScreen macOS] Virtual display terminated by the system.");
            }
        });
    };

    CGVirtualDisplay *display = [[CGVirtualDisplay alloc] initWithDescriptor:descriptor];
    if (display == nil) {
        self.lastError = @"macOS a refusé la création du moniteur virtuel expérimental.";
        NSLog(@"[SecondScreen macOS] CGVirtualDisplay initialization failed.");
        return NO;
    }

    CGVirtualDisplayMode *mode = [[CGVirtualDisplayMode alloc]
                                  initWithWidth:width
                                  height:height
                                  refreshRate:(CGFloat)refreshRate];
    CGVirtualDisplaySettings *settings = [[CGVirtualDisplaySettings alloc] init];
    if (mode == nil || settings == nil) {
        self.lastError = @"Impossible de préparer le mode d'affichage.";
        return NO;
    }

    settings.modes = @[mode];
    settings.hiDPI = 0;
    if (![display applySettings:settings]) {
        self.lastError = @"macOS n'a pas accepté la résolution demandée pour le moniteur virtuel.";
        NSLog(@"[SecondScreen macOS] applySettings failed for %ux%u @ %.2f Hz.", width, height, refreshRate);
        return NO;
    }

    CGDirectDisplayID candidateID = display.displayID;
    if (candidateID == 0 || !CGDisplayIsOnline(candidateID)) {
        self.lastError = @"Le moniteur n'a pas été confirmé en ligne par CoreGraphics.";
        NSLog(@"[SecondScreen macOS] Virtual display was not reported online.");
        return NO;
    }

    _virtualDisplay = display;
    _displayID = candidateID;
    _isDisplayActive = YES;
    _width = width;
    _height = height;
    self.lastError = @"";

    NSLog(@"[SecondScreen macOS] Experimental virtual display created: ID %u, %ux%u @ %.2f Hz.",
          _displayID, _width, _height, refreshRate);
    return YES;
}

- (void)destroyVirtualDisplay {
    // The private API ties the display lifetime to the CGVirtualDisplay object.
    // Releasing our strong reference requests teardown; macOS may take a moment
    // to update the display topology after the object is released.
    _virtualDisplay = nil;
    _displayID = 0;
    _isDisplayActive = NO;
    _width = 0;
    _height = 0;
    self.lastError = @"";
}

@end
