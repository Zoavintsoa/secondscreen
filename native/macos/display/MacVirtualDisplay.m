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
- (void)pollForOnlineDisplay:(CGDirectDisplayID)candidateID
                  generation:(NSUInteger)generation
                    deadline:(CFAbsoluteTime)deadline;
@end

@implementation SecondScreenMacVirtualDisplayManager {
    CGVirtualDisplay *_virtualDisplay;
    CGDirectDisplayID _displayID;
    BOOL _isDisplayActive;
    uint32_t _width;
    uint32_t _height;
    NSUInteger _creationGeneration;
    BOOL _creationPending;
    SecondScreenVirtualDisplayCompletion _pendingCompletion;
}

@synthesize displayID = _displayID;
@synthesize isDisplayActive = _isDisplayActive;
@synthesize width = _width;
@synthesize height = _height;
@synthesize lastError = _lastError;

- (void)createVirtualDisplayWithWidth:(uint32_t)width
                               height:(uint32_t)height
                          refreshRate:(double)refreshRate
                                 name:(NSString *)name
                           completion:(SecondScreenVirtualDisplayCompletion)completion {
    // Keep all private display API calls and state transitions on the main queue.
    // The online-status wait itself is asynchronous so the UI is not blocked.
    dispatch_async(dispatch_get_main_queue(), ^{
        [self destroyVirtualDisplay];
        NSUInteger generation = self->_creationGeneration;

        NSString *displayName = [name stringByTrimmingCharactersInSet:
                                 [NSCharacterSet whitespaceAndNewlineCharacterSet]];
        if (width < 640 || width > 4096 || height < 480 || height > 2160 ||
            !isfinite(refreshRate) || refreshRate < 24.0 || refreshRate > 120.0 ||
            displayName.length == 0) {
            self.lastError = @"Configuration invalide. Utilisez une résolution de 640×480 à 4096×2160 et un rafraîchissement de 24 à 120 Hz.";
            if (completion != nil) {
                completion(NO, self.lastError);
            }
            return;
        }

        // Fail safely if this macOS build does not provide the private runtime classes.
        if (NSClassFromString(@"CGVirtualDisplayDescriptor") == Nil ||
            NSClassFromString(@"CGVirtualDisplay") == Nil ||
            NSClassFromString(@"CGVirtualDisplaySettings") == Nil ||
            NSClassFromString(@"CGVirtualDisplayMode") == Nil) {
            self.lastError = @"Cette version de macOS ne fournit pas les classes expérimentales de moniteur virtuel attendues.";
            NSLog(@"[SecondScreen macOS] Experimental virtual display unavailable: private runtime classes missing.");
            if (completion != nil) {
                completion(NO, self.lastError);
            }
            return;
        }

        CGVirtualDisplayDescriptor *descriptor = [[CGVirtualDisplayDescriptor alloc] init];
        if (descriptor == nil) {
            self.lastError = @"Impossible d'initialiser le descripteur du moniteur virtuel.";
            if (completion != nil) {
                completion(NO, self.lastError);
            }
            return;
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
                    SecondScreenVirtualDisplayCompletion pending = strongSelf->_pendingCompletion;
                    BOOL wasPending = strongSelf->_creationPending;

                    strongSelf->_creationGeneration += 1;
                    strongSelf->_pendingCompletion = nil;
                    strongSelf->_creationPending = NO;
                    strongSelf->_virtualDisplay = nil;
                    strongSelf->_displayID = 0;
                    strongSelf->_isDisplayActive = NO;
                    strongSelf->_width = 0;
                    strongSelf->_height = 0;
                    strongSelf.lastError = @"macOS a terminé le moniteur virtuel.";
                    NSLog(@"[SecondScreen macOS] Virtual display terminated by the system.");

                    if (wasPending && pending != nil) {
                        pending(NO, strongSelf.lastError);
                    }
                }
            });
        };

        CGVirtualDisplay *display = [[CGVirtualDisplay alloc] initWithDescriptor:descriptor];
        if (display == nil) {
            self.lastError = @"macOS a refusé la création du moniteur virtuel expérimental.";
            NSLog(@"[SecondScreen macOS] CGVirtualDisplay initialization failed.");
            if (completion != nil) {
                completion(NO, self.lastError);
            }
            return;
        }

        CGVirtualDisplayMode *mode = [[CGVirtualDisplayMode alloc]
                                      initWithWidth:width
                                      height:height
                                      refreshRate:(CGFloat)refreshRate];
        CGVirtualDisplaySettings *settings = [[CGVirtualDisplaySettings alloc] init];
        if (mode == nil || settings == nil) {
            self.lastError = @"Impossible de préparer le mode d'affichage.";
            if (completion != nil) {
                completion(NO, self.lastError);
            }
            return;
        }

        settings.modes = @[mode];
        settings.hiDPI = 0;
        if (![display applySettings:settings]) {
            self.lastError = @"macOS n'a pas accepté la résolution demandée pour le moniteur virtuel.";
            NSLog(@"[SecondScreen macOS] applySettings failed for %ux%u @ %.2f Hz.", width, height, refreshRate);
            if (completion != nil) {
                completion(NO, self.lastError);
            }
            return;
        }

        CGDirectDisplayID candidateID = display.displayID;
        if (candidateID == 0) {
            self.lastError = @"macOS a créé un objet, mais aucun identifiant d'écran valide n'a été fourni.";
            NSLog(@"[SecondScreen macOS] Invalid virtual display ID.");
            if (completion != nil) {
                completion(NO, self.lastError);
            }
            return;
        }

        // Retain the object while registration is pending. isDisplayActive remains
        // false until CoreGraphics confirms that the display is online.
        self->_virtualDisplay = display;
        self->_displayID = candidateID;
        self->_width = width;
        self->_height = height;
        self->_isDisplayActive = NO;
        self->_creationPending = YES;
        self->_pendingCompletion = [completion copy];
        self.lastError = @"";
        NSLog(@"[SecondScreen macOS] Display ID %u created; waiting for online status (max 2.0 s).",
              candidateID);

        [self pollForOnlineDisplay:candidateID
                        generation:generation
                          deadline:CFAbsoluteTimeGetCurrent() + 2.0];
    });
}

- (void)pollForOnlineDisplay:(CGDirectDisplayID)candidateID
                  generation:(NSUInteger)generation
                    deadline:(CFAbsoluteTime)deadline {
    // A destroy/recreate/termination invalidates previously scheduled polls.
    if (generation != _creationGeneration ||
        !_creationPending ||
        _virtualDisplay == nil ||
        _displayID != candidateID) {
        return;
    }

    if (CGDisplayIsOnline(candidateID)) {
        _isDisplayActive = YES;
        _creationPending = NO;
        self.lastError = @"";
        SecondScreenVirtualDisplayCompletion completion = _pendingCompletion;
        _pendingCompletion = nil;
        NSLog(@"[SecondScreen macOS] SUCCESS: virtual display ID %u is online (%ux%u).",
              _displayID, _width, _height);
        if (completion != nil) {
            completion(YES, nil);
        }
        return;
    }

    if (CFAbsoluteTimeGetCurrent() >= deadline) {
        NSString *error = @"Le moniteur virtuel n'a pas été confirmé en ligne après 2 secondes.";
        SecondScreenVirtualDisplayCompletion completion = _pendingCompletion;
        _pendingCompletion = nil;
        _creationPending = NO;
        _virtualDisplay = nil;
        _displayID = 0;
        _isDisplayActive = NO;
        _width = 0;
        _height = 0;
        self.lastError = error;
        NSLog(@"[SecondScreen macOS] TIMEOUT: display ID %u remained offline after 2.0 seconds.",
              candidateID);
        if (completion != nil) {
            completion(NO, error);
        }
        return;
    }

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(100 * NSEC_PER_MSEC)),
                   dispatch_get_main_queue(), ^{
        [self pollForOnlineDisplay:candidateID generation:generation deadline:deadline];
    });
}

- (void)destroyVirtualDisplay {
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self destroyVirtualDisplay];
        });
        return;
    }

    _creationGeneration += 1;
    SecondScreenVirtualDisplayCompletion pending = _pendingCompletion;
    BOOL wasPending = _creationPending;
    _pendingCompletion = nil;
    _creationPending = NO;
    _virtualDisplay = nil;
    _displayID = 0;
    _isDisplayActive = NO;
    _width = 0;
    _height = 0;
    self.lastError = @"";

    if (wasPending && pending != nil) {
        pending(NO, @"Création du moniteur virtuel annulée.");
    }
}

@end
