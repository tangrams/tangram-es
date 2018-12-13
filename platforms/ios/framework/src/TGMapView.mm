//
//  TGMapView.mm
//  TangramMap
//
//  Created by Matt Blair on 7/10/18.
//

#import "TGMapView.h"
#import "TGMapView+Internal.h"
#import "TGCameraPosition.h"
#import "TGCameraPosition+Internal.h"
#import "TGURLHandler.h"
#import "TGLabelPickResult.h"
#import "TGLabelPickResult+Internal.h"
#import "TGMapData.h"
#import "TGMapData+Internal.h"
#import "TGMapViewDelegate.h"
#import "TGMarkerPickResult.h"
#import "TGMarkerPickResult+Internal.h"
#import "TGMarker.h"
#import "TGMarker+Internal.h"
#import "TGRecognizerDelegate.h"
#import "TGSceneUpdate.h"
#import "TGTypes+Internal.h"
#import <GLKit/GLKit.h>

#include "data/propertyItem.h"
#include "iosPlatform.h"
#include "map.h"
#include <unordered_map>
#include <functional>

inline float convertBearingDegreesToRotationRadians(CLLocationDirection bearing) {
    return -TGRadiansFromDegrees(bearing);
}

inline CLLocationDirection convertRotationRadiansToBearingDegrees(float rotation) {
    return TGDegreesFromRadians(-rotation);
}

/**
 Map region change states
 Used to determine map region change transitions when animating or responding to input gestures.
 IDLE: Map is still, no state transitions happening
 Jumping: Map region changing without animating
 Animating: Map region changing with animation
 */
typedef NS_ENUM(NSInteger, TGMapRegionChangeState) {
    TGMapRegionIdle = 0,
    TGMapRegionJumping,
    TGMapRegionAnimating,
};

@interface TGMapView () <UIGestureRecognizerDelegate, GLKViewDelegate> {
    BOOL _shouldCaptureFrame;
    BOOL _captureFrameWaitForViewComplete;
    BOOL _prevMapViewComplete;
    BOOL _viewInBackground;
    BOOL _renderRequested;
}

@property (nullable, strong, nonatomic) EAGLContext *context;
@property (strong, nonatomic) GLKView *glView;
@property (strong, nonatomic) CADisplayLink *displayLink;
@property (strong, nonatomic) NSMutableDictionary<NSString *, TGMarker *> *markersById;
@property (strong, nonatomic) NSMutableDictionary<NSString *, TGMapData *> *dataLayersByName;
@property (nonatomic, copy, nullable) void (^cameraAnimationCallback)(BOOL);
@property TGMapRegionChangeState currentState;
@property BOOL prevCameraEasing;

@end // interface TGMapView

@implementation TGMapView

@synthesize displayLink = _displayLink;

#pragma mark Lifecycle Methods

- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [self setup];
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) {
        [self setup];
    }
    return self;
}

- (instancetype)initWithFrame:(CGRect)frame urlHandler:(id<TGURLHandler>)urlHandler
{
    self = [super initWithFrame:frame];
    if (self) {
        self.urlHandler = urlHandler;
        [self setup];
    }
    return self;
}

- (void)dealloc
{
    [self validateContext];
    if (_map) {
        delete _map;
    }

    if (_displayLink) {
        [_displayLink invalidate];
    }
    [self invalidateContext];
}

- (void)setup
{
    __weak TGMapView* weakSelf = self;
    _map = new Tangram::Map(std::make_unique<Tangram::iOSPlatform>(weakSelf));

    NSNotificationCenter* notificationCenter = [NSNotificationCenter defaultCenter];
    [notificationCenter addObserver:self
                           selector:@selector(didEnterBackground:)
                               name:UIApplicationDidEnterBackgroundNotification
                             object:nil];

    [notificationCenter addObserver:self
                           selector:@selector(didLeaveBackground:)
                               name:UIApplicationWillEnterForegroundNotification
                             object:nil];

    [notificationCenter addObserver:self
                           selector:@selector(didLeaveBackground:)
                               name:UIApplicationDidBecomeActiveNotification
                             object:nil];

    _prevMapViewComplete = NO;
    _captureFrameWaitForViewComplete = YES;
    _shouldCaptureFrame = NO;
    _viewInBackground = [UIApplication sharedApplication].applicationState == UIApplicationStateBackground;
    _renderRequested = YES;
    _continuous = NO;
    _preferredFramesPerSecond = 60;
    _markersById = [[NSMutableDictionary alloc] init];
    _dataLayersByName = [[NSMutableDictionary alloc] init];
    _resourceRoot = [[NSBundle mainBundle] resourceURL];
    _currentState = TGMapRegionIdle;
    _prevCameraEasing = false;

    self.clipsToBounds = YES;
    self.opaque = YES;
    self.autoresizesSubviews = YES;
    self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

    if (!_urlHandler) {
        _urlHandler = [[TGDefaultURLHandler alloc] init];
    }

    if(!_viewInBackground) {
        [self setupGL];
    }

    [self setupGestureRecognizers];

    self.map->setCameraAnimationListener([weakSelf](bool finished){
        void (^callback)(BOOL) = weakSelf.cameraAnimationCallback;
        if (callback) {
            callback(!finished);
            [weakSelf setMapRegionChangeState:TGMapRegionIdle];
        }
        weakSelf.cameraAnimationCallback = nil;
    });
}

- (void)didEnterBackground:(__unused NSNotification *)notification
{
    _viewInBackground = YES;
    _displayLink.paused = YES;
}

- (void)didLeaveBackground:(__unused NSNotification *)notification
{
    _viewInBackground = NO;
    if (!_context) {
        [self setupGL];
    }
    _displayLink.paused = NO;
}

- (void)validateContext
{
    if (![[EAGLContext currentContext] isEqual:_context]) {
        [EAGLContext setCurrentContext:_context];
    }
}

- (void)invalidateContext
{
    if ([[EAGLContext currentContext] isEqual:_context]) {
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)setupGL
{
    _context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!_context) {
        NSLog(@"Failed to create GLES context");
        return;
    }

    _glView = [[GLKView alloc] initWithFrame:self.bounds context:_context];

    _glView.drawableColorFormat = GLKViewDrawableColorFormatRGBA8888;
    _glView.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    _glView.drawableStencilFormat = GLKViewDrawableStencilFormat8;
    _glView.drawableMultisample = GLKViewDrawableMultisampleNone;
    _glView.opaque = self.opaque;
    _glView.delegate = self;
    _glView.autoresizingMask = self.autoresizingMask;

    [self validateContext];

    [_glView bindDrawable];

    [self insertSubview:_glView atIndex:0];

    // By default,  a GLKView's contentScaleFactor property matches the scale of
    // the screen that contains it, so the framebuffer is already configured for
    // rendering at the full resolution of the display.
    self.contentScaleFactor = _glView.contentScaleFactor;

    _map->setupGL();
    _map->setPixelScale(_glView.contentScaleFactor);

    [self trySetMapDefaultBackground:self.backgroundColor];
}

- (void)setupDisplayLink
{
    BOOL visible = self.superview && self.window;
    if (visible && !_displayLink) {
        _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(displayLinkUpdate:)];
        _displayLink.preferredFramesPerSecond = self.preferredFramesPerSecond;
        [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
        _renderRequested = YES;
    } else if (!visible && _displayLink) {
        [_displayLink invalidate];
        _displayLink = nil;
    }
}

- (void)setupGestureRecognizers
{
    _tapGestureRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(respondToTapGesture:)];
    _tapGestureRecognizer.numberOfTapsRequired = 1;
    // TODO: Figure a way to have a delay set for it not to tap gesture not to wait long enough for a doubletap gesture to be recognized
    _tapGestureRecognizer.delaysTouchesEnded = NO;

    _doubleTapGestureRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(respondToDoubleTapGesture:)];
    _doubleTapGestureRecognizer.numberOfTapsRequired = 2;
    // Ignore single tap when double tap occurs
    [_tapGestureRecognizer requireGestureRecognizerToFail:_doubleTapGestureRecognizer];

    _panGestureRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(respondToPanGesture:)];
    _panGestureRecognizer.maximumNumberOfTouches = 2;

    _pinchGestureRecognizer = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(respondToPinchGesture:)];
    _rotationGestureRecognizer = [[UIRotationGestureRecognizer alloc] initWithTarget:self action:@selector(respondToRotationGesture:)];
    _shoveGestureRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(respondToShoveGesture:)];
    _shoveGestureRecognizer.minimumNumberOfTouches = 2;
    _shoveGestureRecognizer.maximumNumberOfTouches = 2;
    _longPressGestureRecognizer = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(respondToLongPressGesture:)];

    // Use the delegate method 'shouldRecognizeSimultaneouslyWithGestureRecognizer' for gestures that can be concurrent
    _panGestureRecognizer.delegate = self;
    _pinchGestureRecognizer.delegate = self;
    _rotationGestureRecognizer.delegate = self;

    [_glView addGestureRecognizer:_tapGestureRecognizer];
    [_glView addGestureRecognizer:_doubleTapGestureRecognizer];
    [_glView addGestureRecognizer:_panGestureRecognizer];
    [_glView addGestureRecognizer:_pinchGestureRecognizer];
    [_glView addGestureRecognizer:_rotationGestureRecognizer];
    [_glView addGestureRecognizer:_shoveGestureRecognizer];
    [_glView addGestureRecognizer:_longPressGestureRecognizer];
}

#pragma mark UIView methods

- (void)setBackgroundColor:(UIColor *)backgroundColor
{
    [super setBackgroundColor:backgroundColor];
    [self trySetMapDefaultBackground:backgroundColor];
}

- (void)didMoveToWindow
{
    [self setupDisplayLink];
    [super didMoveToWindow];
}

- (void)didMoveToSuperview
{
    [self setupDisplayLink];
    [super didMoveToSuperview];
}

- (void)layoutSubviews
{
    [super layoutSubviews];

    CGSize size = self.bounds.size;
    self.map->resize(size.width * self.contentScaleFactor, size.height * self.contentScaleFactor);

    [self requestRender];
}

#pragma mark Memory Management

- (void)didReceiveMemoryWarning
{
    self.map->onMemoryWarning();
}

#pragma mark Rendering Behavior

- (void)requestRender
{
    _renderRequested = YES;
}

- (void)displayLinkUpdate:(CADisplayLink *)sender
{
    if (_renderRequested || self.continuous) {
        _renderRequested = NO;

        CFTimeInterval dt = _displayLink.targetTimestamp - _displayLink.timestamp;
        BOOL mapViewComplete = self.map->update(dt);
        BOOL viewComplete = mapViewComplete && !_prevMapViewComplete;

        // When invoking delegate selectors like this below, we don't need to check whether the delegate is `nil`. `nil` is
        // a valid object that returns `0`, `nil`, or `NO` from all messages, including `respondsToSelector`. So we can use
        // `respondsToSelector` to check for delegate nullity and selector response at the same time. MEB 2018.7.16

        if (viewComplete && [self.mapViewDelegate respondsToSelector:@selector(mapViewDidCompleteLoading:)]) {
            [self.mapViewDelegate mapViewDidCompleteLoading:self];
        }

        if ([self.mapViewDelegate respondsToSelector:@selector(mapView:didCaptureScreenshot:)]) {
            if (_shouldCaptureFrame && (!_captureFrameWaitForViewComplete || viewComplete)) {

                UIImage *screenshot = [_glView snapshot];

                // For now we only have the GLKView to capture. In the future, to capture a view heirarchy including any
                // other subviews, we can use the alternative approach below. MEB 2018.7.16
                // UIGraphicsBeginImageContext(self.frame.size);
                // [self drawViewHierarchyInRect:self.frame afterScreenUpdates:YES];
                // UIImage* screenshot = UIGraphicsGetImageFromCurrentImageContext();
                // UIGraphicsEndImageContext();

                [self.mapViewDelegate mapView:self didCaptureScreenshot:screenshot];

                _shouldCaptureFrame = NO;
            }
        }

        _prevMapViewComplete = mapViewComplete;

        [self.glView display];
    }
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    if (_viewInBackground) {
        return;
    }

    BOOL cameraEasing = self.map->render();
    if (cameraEasing) {
        [self setMapRegionChangeState:TGMapRegionAnimating];
    } else if (_prevCameraEasing) {
        [self setMapRegionChangeState:TGMapRegionIdle];
    }

    _prevCameraEasing = cameraEasing;
}

#pragma mark Screenshots

- (void)captureScreenshot:(BOOL)waitForViewComplete
{
    _captureFrameWaitForViewComplete = waitForViewComplete;
    _shouldCaptureFrame = YES;
    [self requestRender];
}

#pragma mark Markers

- (NSArray<TGMarker *> *)markers
{
    if (!self.map) {
        NSArray* values = [[NSArray alloc] init];
        return values;
    }

    return [self.markersById allValues];
}

- (TGMarker*)markerAdd
{
    TGMarker* marker = [[TGMarker alloc] initWithMap:self.map];
    NSString* key = [NSString stringWithFormat:@"%d", marker.identifier];
    self.markersById[key] = marker;
    return marker;
}

- (void)markerRemove:(TGMarker *)marker
{
    NSString* key = [NSString stringWithFormat:@"%d", marker.identifier];
    self.map->markerRemove(marker.identifier);
    [self.markersById removeObjectForKey:key];
    marker.identifier = 0;
}

- (void)markerRemoveAll
{
    if (!self.map) { return; }
    for (id markerId in self.markersById) {
        TGMarker* marker = [self.markersById objectForKey:markerId];
        marker.identifier = 0;
    }
    [self.markersById removeAllObjects];
    self.map->markerRemoveAll();
}

#pragma mark Debugging

- (void)setDebugFlag:(TGDebugFlag)debugFlag value:(BOOL)on
{
    Tangram::setDebugFlag((Tangram::DebugFlags)debugFlag, on);
}

- (BOOL)getDebugFlag:(TGDebugFlag)debugFlag
{
    return Tangram::getDebugFlag((Tangram::DebugFlags)debugFlag);
}

- (void)toggleDebugFlag:(TGDebugFlag)debugFlag
{
    Tangram::toggleDebugFlag((Tangram::DebugFlags)debugFlag);
}

#pragma mark Data Layers

- (TGMapData *)addDataLayer:(NSString *)name
{
    return [self addDataLayer:name generateCentroid:NO];
}

- (TGMapData *)addDataLayer:(NSString *)name generateCentroid:(BOOL)generateCentroid
{
    if (!self.map) { return nil; }

    std::string dataLayerName = std::string([name UTF8String]);
    auto source = std::make_shared<Tangram::ClientGeoJsonSource>(self.map->getPlatform(),
                    dataLayerName, "", generateCentroid);
    self.map->addTileSource(source);

    __weak TGMapView* weakSelf = self;
    TGMapData* clientData = [[TGMapData alloc] initWithMapView:weakSelf name:name source:source];
    self.dataLayersByName[name] = clientData;

    return clientData;
}

- (BOOL)removeDataSource:(std::shared_ptr<Tangram::TileSource>)tileSource name:(NSString *)name
{
    if (!self.map || !tileSource) { return NO; }

    [self.dataLayersByName removeObjectForKey:name];
    return self.map->removeTileSource(*tileSource);
}

- (void)clearDataSource:(std::shared_ptr<Tangram::TileSource>)tileSource
{
    if (!self.map || !tileSource) { return; }

    self.map->clearTileSource(*tileSource, true, true);
}

#pragma mark Loading Scenes

std::vector<Tangram::SceneUpdate> unpackSceneUpdates(NSArray<TGSceneUpdate *> *sceneUpdates)
{
    std::vector<Tangram::SceneUpdate> updates;
    if (sceneUpdates) {
        for (TGSceneUpdate* update in sceneUpdates) {
            updates.push_back({std::string([update.path UTF8String]), std::string([update.value UTF8String])});
        }
    }
    return updates;
}

- (Tangram::SceneReadyCallback)sceneReadyListener {
    __weak TGMapView* weakSelf = self;

    return [weakSelf](int sceneID, const Tangram::SceneError *sceneError) {
        __strong TGMapView* strongSelf = weakSelf;

        if (!strongSelf) {
            return;
        }

        [strongSelf.markersById removeAllObjects];
        [strongSelf requestRender];

        if (![strongSelf.mapViewDelegate respondsToSelector:@selector(mapView:didLoadScene:withError:)]) {
            return;
        }

        NSError* error = nil;

        if (sceneError) {
            error = TGConvertCoreSceneErrorToNSError(sceneError);
        }

        [strongSelf.mapViewDelegate mapView:strongSelf didLoadScene:sceneID withError:error];
    };
}

- (int)loadSceneAsyncFromURL:(NSURL *)url withUpdates:(nullable NSArray<TGSceneUpdate *> *)updates
{
    if (!self.map) { return -1; }

    auto sceneUpdates = unpackSceneUpdates(updates);

    self.map->setSceneReadyListener([self sceneReadyListener]);
    return self.map->loadSceneAsync([[url absoluteString] UTF8String], false, sceneUpdates);
}

- (int)loadSceneAsyncFromYAML:(NSString *)yaml relativeToURL:(NSURL *)url withUpdates:(nullable NSArray<TGSceneUpdate *> *)updates
{
    if (!self.map) { return -1; }

    auto sceneUpdates = unpackSceneUpdates(updates);

    self.map->setSceneReadyListener([self sceneReadyListener]);
    return self.map->loadSceneYamlAsync([yaml UTF8String], [[url absoluteString] UTF8String], false, sceneUpdates);
}

#pragma mark Coordinate Conversions

- (CGPoint)viewPositionFromCoordinate:(CLLocationCoordinate2D)coordinate
{
    if (!self.map) { return CGPointZero; }

    double viewPosition[2];
    self.map->lngLatToScreenPosition(coordinate.longitude, coordinate.latitude, &viewPosition[0], &viewPosition[1]);
    viewPosition[0] /= self.contentScaleFactor;
    viewPosition[1] /= self.contentScaleFactor;

    return CGPointMake((CGFloat)viewPosition[0], (CGFloat)viewPosition[1]);
}

- (CLLocationCoordinate2D)coordinateFromViewPosition:(CGPoint)viewPosition
{
    if (!self.map) { return kCLLocationCoordinate2DInvalid; }

    viewPosition.x *= self.contentScaleFactor;
    viewPosition.y *= self.contentScaleFactor;

    CLLocationCoordinate2D coordinate;
    if (self.map->screenPositionToLngLat(viewPosition.x, viewPosition.y,
        &coordinate.longitude, &coordinate.latitude)) {
        return coordinate;
    }

    return kCLLocationCoordinate2DInvalid;
}

#pragma mark Picking Map Objects

- (void)setPickRadius:(CGFloat)logicalPixels
{
    if (!self.map) { return; }

    self.map->setPickRadius(logicalPixels);
}

- (void)pickFeatureAt:(CGPoint)viewPosition
{
    if (!self.map) { return; }

    viewPosition.x *= self.contentScaleFactor;
    viewPosition.y *= self.contentScaleFactor;

    __weak TGMapView* weakSelf = self;
    self.map->pickFeatureAt(viewPosition.x, viewPosition.y, [weakSelf](const Tangram::FeaturePickResult* featureResult) {
        __strong TGMapView* strongSelf = weakSelf;

        if (!strongSelf || ![strongSelf.mapViewDelegate respondsToSelector:@selector(mapView:didSelectFeature:atScreenPosition:)]) {
            return;
        }

        CGPoint position = CGPointZero;

        if (!featureResult) {
            [strongSelf.mapViewDelegate mapView:strongSelf didSelectFeature:nil atScreenPosition:position];
            return;
        }

        NSMutableDictionary* featureProperties = [[NSMutableDictionary alloc] init];

        const auto& properties = featureResult->properties;
        position = CGPointMake(featureResult->position[0] / strongSelf.contentScaleFactor,
                               featureResult->position[1] / strongSelf.contentScaleFactor);

        for (const auto& item : properties->items()) {
            NSString* key = [NSString stringWithUTF8String:item.key.c_str()];
            NSString* value = [NSString stringWithUTF8String:properties->asString(item.value).c_str()];
            featureProperties[key] = value;
        }

        [strongSelf.mapViewDelegate mapView:strongSelf didSelectFeature:featureProperties atScreenPosition:position];
    });
}

- (void)pickMarkerAt:(CGPoint)viewPosition
{
    if (!self.map) { return; }

    viewPosition.x *= self.contentScaleFactor;
    viewPosition.y *= self.contentScaleFactor;

    __weak TGMapView* weakSelf = self;
    self.map->pickMarkerAt(viewPosition.x, viewPosition.y, [weakSelf](const Tangram::MarkerPickResult* markerPickResult) {
        __strong TGMapView* strongSelf = weakSelf;

        if (!strongSelf || ![strongSelf.mapViewDelegate respondsToSelector:@selector(mapView:didSelectMarker:atScreenPosition:)]) {
            return;
        }

        CGPoint position = CGPointZero;

        if (!markerPickResult) {
            [strongSelf.mapViewDelegate mapView:strongSelf didSelectMarker:nil atScreenPosition:position];
            return;
        }

        NSString* key = [NSString stringWithFormat:@"%d", markerPickResult->id];
        TGMarker* marker = [strongSelf.markersById objectForKey:key];

        if (!marker) {
            [strongSelf.mapViewDelegate mapView:strongSelf didSelectMarker:nil atScreenPosition:position];
            return;
        }

        position = CGPointMake(markerPickResult->position[0] / strongSelf.contentScaleFactor,
                               markerPickResult->position[1] / strongSelf.contentScaleFactor);

        CLLocationCoordinate2D coordinate = CLLocationCoordinate2DMake(markerPickResult->coordinates.latitude,
                                                                       markerPickResult->coordinates.longitude);

        TGMarkerPickResult* result = [[TGMarkerPickResult alloc] initWithCoordinate:coordinate marker:marker];
        [strongSelf.mapViewDelegate mapView:strongSelf didSelectMarker:result atScreenPosition:position];
    });
}

- (void)pickLabelAt:(CGPoint)viewPosition
{
    if (!self.map) { return; }

    viewPosition.x *= self.contentScaleFactor;
    viewPosition.y *= self.contentScaleFactor;

    __weak TGMapView* weakSelf = self;
    self.map->pickLabelAt(viewPosition.x, viewPosition.y, [weakSelf](const Tangram::LabelPickResult* labelPickResult) {
        __strong TGMapView* strongSelf = weakSelf;

        if (!strongSelf || ![strongSelf.mapViewDelegate respondsToSelector:@selector(mapView:didSelectLabel:atScreenPosition:)]) {
            return;
        }

        CGPoint position = CGPointMake(0.0, 0.0);

        if (!labelPickResult) {
            [strongSelf.mapViewDelegate mapView:strongSelf didSelectLabel:nil atScreenPosition:position];
            return;
        }

        NSMutableDictionary* featureProperties = [[NSMutableDictionary alloc] init];

        const auto& touchItem = labelPickResult->touchItem;
        const auto& properties = touchItem.properties;
        position = CGPointMake(touchItem.position[0] / strongSelf.contentScaleFactor,
                               touchItem.position[1] / strongSelf.contentScaleFactor);

        for (const auto& item : properties->items()) {
            NSString* key = [NSString stringWithUTF8String:item.key.c_str()];
            NSString* value = [NSString stringWithUTF8String:properties->asString(item.value).c_str()];
            featureProperties[key] = value;
        }

        CLLocationCoordinate2D coordinate = {labelPickResult->coordinates.latitude, labelPickResult->coordinates.longitude};
        TGLabelPickResult* tgLabelPickResult = [[TGLabelPickResult alloc] initWithCoordinate:coordinate
                                                                                         type:(TGLabelType)labelPickResult->type
                                                                                   properties:featureProperties];
        [strongSelf.mapViewDelegate mapView:strongSelf didSelectLabel:tgLabelPickResult atScreenPosition:position];
    });
}

#pragma mark Changing the Map Viewport

- (CGFloat)minimumZoomLevel
{
    if (!self.map) { return 0; }
    return self.map->getMinZoom();
}

- (void)setMinimumZoomLevel:(CGFloat)minimumZoomLevel
{
    if (!self.map) { return; }
    self.map->setMinZoom(minimumZoomLevel);
}

- (CGFloat)maximumZoomLevel
{
    if (!self.map) { return 0; }
    return self.map->getMaxZoom();
}

- (void)setMaximumZoomLevel:(CGFloat)maximumZoomLevel
{
    if (!self.map) { return; }
    self.map->setMaxZoom(maximumZoomLevel);
}

- (void)setPosition:(CLLocationCoordinate2D)position {
    if (!self.map) { return; }

    [self setMapRegionChangeState:TGMapRegionJumping];
    self.map->setPosition(position.longitude, position.latitude);
    [self setMapRegionChangeState:TGMapRegionIdle];
}

- (CLLocationCoordinate2D)position
{
    if (!self.map) { return kCLLocationCoordinate2DInvalid; }

    CLLocationCoordinate2D coordinate;
    self.map->getPosition(coordinate.longitude, coordinate.latitude);

    return coordinate;
}

- (void)setZoom:(CGFloat)zoom
{
    if (!self.map) { return; }

    [self setMapRegionChangeState:TGMapRegionJumping];
    self.map->setZoom(zoom);
    [self setMapRegionChangeState:TGMapRegionIdle];
}

- (CGFloat)zoom
{
    if (!self.map) { return 0.0; }

    return self.map->getZoom();
}

- (void)setBearing:(CLLocationDirection)bearing
{
    if (!self.map) { return; }

    [self setMapRegionChangeState:TGMapRegionJumping];
    float rotation = convertBearingDegreesToRotationRadians(bearing);
    self.map->setRotation(rotation);
    [self setMapRegionChangeState:TGMapRegionIdle];
}

- (CLLocationDirection)bearing
{
    if (!self.map) { return 0.0; }

    float rotation = self.map->getRotation();
    return convertRotationRadiansToBearingDegrees(rotation);
}

- (CGFloat)pitch
{
    if (!self.map) { return 0.f; }

    float tilt = self.map->getTilt();
    return TGDegreesFromRadians(tilt);
}

- (void)setPitch:(CGFloat)pitch
{
    if (!self.map) { return; }

    [self setMapRegionChangeState:TGMapRegionJumping];
    float tilt = TGRadiansFromDegrees(pitch);
    self.map->setTilt(tilt);
    [self setMapRegionChangeState:TGMapRegionIdle];
}

- (TGCameraPosition *)cameraPosition
{
    Tangram::CameraPosition camera = self.map->getCameraPosition();
    TGCameraPosition *result = [[TGCameraPosition alloc] initWithCoreCamera:&camera];
    return result;
}

- (void)setCameraPosition:(TGCameraPosition *)cameraPosition
{
    Tangram::CameraPosition result = [cameraPosition convertToCoreCamera];
    [self setMapRegionChangeState:TGMapRegionJumping];
    self.map->setCameraPosition(result);
    [self setMapRegionChangeState:TGMapRegionIdle];
}

- (void)setCameraPosition:(TGCameraPosition *)cameraPosition
             withDuration:(NSTimeInterval)duration
                 easeType:(TGEaseType)easeType
                 callback:(void (^)(BOOL))callback
{
    Tangram::CameraPosition camera = [cameraPosition convertToCoreCamera];
    Tangram::EaseType ease = TGConvertTGEaseTypeToCoreEaseType(easeType);
    if (duration > 0) {
        [self setMapRegionChangeState:TGMapRegionAnimating];
    } else {
        [self setMapRegionChangeState:TGMapRegionJumping];
    }
    self.map->setCameraPositionEased(camera, duration, ease);
    self.cameraAnimationCallback = callback;
}

- (void)flyToCameraPosition:(TGCameraPosition *)cameraPosition callback:(void (^)(BOOL))callback
{
    [self flyToCameraPosition:cameraPosition withDuration:-1.0 callback:callback];
}

- (void)flyToCameraPosition:(TGCameraPosition *)cameraPosition
               withDuration:(NSTimeInterval)duration
                   callback:(void (^)(BOOL))callback
{
    Tangram::CameraPosition camera = [cameraPosition convertToCoreCamera];
    if (duration > 0) {
        [self setMapRegionChangeState:TGMapRegionAnimating];
    } else {
        [self setMapRegionChangeState:TGMapRegionJumping];
    }
    self.map->flyTo(camera, duration);
    self.cameraAnimationCallback = callback;
}

- (TGCameraPosition *)cameraThatFitsBounds:(TGCoordinateBounds)bounds withPadding:(UIEdgeInsets)padding
{
    if (!self.map) {
        return nil;
    }
    Tangram::LngLat sw(bounds.sw.longitude, bounds.sw.latitude);
    Tangram::LngLat ne(bounds.ne.longitude, bounds.ne.latitude);
    Tangram::EdgePadding pad(padding.left, padding.top, padding.right, padding.bottom);
    Tangram::CameraPosition camera = self.map->getEnclosingCameraPosition(sw, ne, pad);
    return [[TGCameraPosition alloc] initWithCoreCamera:&camera];
}

#pragma mark Camera type

- (TGCameraType)cameraType
{
    switch (self.map->getCameraType()) {
        case 0:
            return TGCameraTypePerspective;
        case 1:
            return TGCameraTypeIsometric;
        case 2:
            return TGCameraTypeFlat;
        default:
            return TGCameraTypePerspective;
    }
}

- (void)setCameraType:(TGCameraType)cameraType
{
    if (!self.map) { return; }

    self.map->setCameraType(cameraType);
}

#pragma mark Gesture Recognizers

- (void)setTapGestureRecognizer:(UITapGestureRecognizer *)recognizer
{
    if (!recognizer) { return; }
    if (_tapGestureRecognizer) {
        [_glView removeGestureRecognizer:_tapGestureRecognizer];
    }
    _tapGestureRecognizer = recognizer;
    [_glView addGestureRecognizer:_tapGestureRecognizer];
}

- (void)setDoubleTapGestureRecognizer:(UITapGestureRecognizer *)recognizer
{
    if (!recognizer) { return; }
    if (_doubleTapGestureRecognizer) {
        [_glView removeGestureRecognizer:_doubleTapGestureRecognizer];
    }
    _doubleTapGestureRecognizer = recognizer;
    [_glView addGestureRecognizer:_doubleTapGestureRecognizer];
}

- (void)setPanGestureRecognizer:(UIPanGestureRecognizer *)recognizer
{
    if (!recognizer) { return; }
    if (_panGestureRecognizer) {
        [_glView removeGestureRecognizer:_panGestureRecognizer];
    }
    _panGestureRecognizer = recognizer;
    [_glView addGestureRecognizer:_panGestureRecognizer];
}

- (void)setPinchGestureRecognizer:(UIPinchGestureRecognizer *)recognizer
{
    if (!recognizer) { return; }
    if (_pinchGestureRecognizer) {
        [_glView removeGestureRecognizer:_pinchGestureRecognizer];
    }
    _pinchGestureRecognizer = recognizer;
    [_glView addGestureRecognizer:_pinchGestureRecognizer];
}

- (void)setRotationGestureRecognizer:(UIRotationGestureRecognizer *)recognizer
{
    if (!recognizer) { return; }
    if (_rotationGestureRecognizer) {
        [_glView removeGestureRecognizer:_rotationGestureRecognizer];
    }
    _rotationGestureRecognizer = recognizer;
    [_glView addGestureRecognizer:_rotationGestureRecognizer];
}

- (void)setShoveGestureRecognizer:(UIPanGestureRecognizer *)recognizer
{
    if (!recognizer) { return; }
    if (_shoveGestureRecognizer) {
        [_glView removeGestureRecognizer:_shoveGestureRecognizer];
    }
    _shoveGestureRecognizer = recognizer;
    [_glView addGestureRecognizer:_shoveGestureRecognizer];
}

- (void)setLongPressGestureRecognizer:(UILongPressGestureRecognizer *)recognizer
{
    if (!recognizer) { return; }
    if (_longPressGestureRecognizer) {
        [_glView removeGestureRecognizer:_longPressGestureRecognizer];
    }
    _longPressGestureRecognizer = recognizer;
    [_glView addGestureRecognizer:_longPressGestureRecognizer];
}

// Implement touchesBegan to catch down events
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    self.map->handlePanGesture(0.0f, 0.0f, 0.0f, 0.0f);
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer
{
    // make shove gesture exclusive
    if ([gestureRecognizer isEqual:_shoveGestureRecognizer] || [otherGestureRecognizer isEqual:_shoveGestureRecognizer]) {
        return NO;
    }
    return YES;
}

#pragma mark - Gesture Responders

- (void)respondToLongPressGesture:(UILongPressGestureRecognizer *)longPressRecognizer
{
    CGPoint location = [longPressRecognizer locationInView:_glView];
    if ([self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:shouldRecognizeLongPressGesture:)] ) {
        if (![self.gestureDelegate mapView:self recognizer:longPressRecognizer shouldRecognizeLongPressGesture:location]) { return; }
    }

    if ([self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizeLongPressGesture:)]) {
        [self.gestureDelegate mapView:self recognizer:longPressRecognizer didRecognizeLongPressGesture:location];
    }
}

- (void)respondToTapGesture:(UITapGestureRecognizer *)tapRecognizer
{
    CGPoint location = [tapRecognizer locationInView:_glView];
    if ([self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:shouldRecognizeSingleTapGesture:)]) {
        if (![self.gestureDelegate mapView:self recognizer:tapRecognizer shouldRecognizeSingleTapGesture:location]) { return; }
    }

    if ([self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizeSingleTapGesture:)]) {
        [self.gestureDelegate mapView:self recognizer:tapRecognizer didRecognizeSingleTapGesture:location];
    }
}

- (void)respondToDoubleTapGesture:(UITapGestureRecognizer *)doubleTapRecognizer
{
    CGPoint location = [doubleTapRecognizer locationInView:_glView];
    if ([self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:shouldRecognizeDoubleTapGesture:)]) {
        if (![self.gestureDelegate mapView:self recognizer:doubleTapRecognizer shouldRecognizeDoubleTapGesture:location]) { return; }
    }

    if ([self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizeDoubleTapGesture:)]) {
        [self.gestureDelegate mapView:self recognizer:doubleTapRecognizer didRecognizeDoubleTapGesture:location];
    }
}

- (void)respondToPanGesture:(UIPanGestureRecognizer *)panRecognizer
{
    CGPoint displacement = [panRecognizer translationInView:_glView];

    if ([self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:shouldRecognizePanGesture:)]) {
        if (![self.gestureDelegate mapView:self recognizer:panRecognizer shouldRecognizePanGesture:displacement]) {
            return;
        }
    }

    CGPoint velocity = [panRecognizer velocityInView:_glView];
    CGPoint end = [panRecognizer locationInView:_glView];
    CGPoint start = {end.x - displacement.x, end.y - displacement.y};

    switch (panRecognizer.state) {
        case UIGestureRecognizerStateBegan:
            [self setMapRegionChangeState:TGMapRegionJumping];
            break;
        case UIGestureRecognizerStateChanged:
            [self setMapRegionChangeState:TGMapRegionJumping];
            self.map->handlePanGesture(start.x * self.contentScaleFactor, start.y * self.contentScaleFactor, end.x * self.contentScaleFactor, end.y * self.contentScaleFactor);
            break;
        case UIGestureRecognizerStateEnded:
            self.map->handleFlingGesture(end.x * self.contentScaleFactor, end.y * self.contentScaleFactor, velocity.x * self.contentScaleFactor, velocity.y * self.contentScaleFactor);
            [self setMapRegionChangeState:TGMapRegionIdle];
            break;
        default:
            break;
    }

    if ([self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizePanGesture:)]) {
        [self.gestureDelegate mapView:self recognizer:panRecognizer didRecognizePanGesture:displacement];
    }

    // Reset translation to zero so that subsequent calls get relative value.
    [panRecognizer setTranslation:CGPointZero inView:_glView];
}

- (void)respondToPinchGesture:(UIPinchGestureRecognizer *)pinchRecognizer
{
    CGPoint location = [pinchRecognizer locationInView:_glView];
    if ([self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:shouldRecognizePinchGesture:)]) {
        if (![self.gestureDelegate mapView:self recognizer:pinchRecognizer shouldRecognizePinchGesture:location]) {
            return;
        }
    }

    CGFloat scale = pinchRecognizer.scale;
    switch (pinchRecognizer.state) {
        case UIGestureRecognizerStateBegan:
            [self setMapRegionChangeState:TGMapRegionJumping];
            break;
        case UIGestureRecognizerStateChanged:
            [self setMapRegionChangeState:TGMapRegionJumping];
            if ([self.gestureDelegate respondsToSelector:@selector(pinchFocus:recognizer:)]) {
                CGPoint focusPosition = [self.gestureDelegate pinchFocus:self recognizer:pinchRecognizer];
                self.map->handlePinchGesture(focusPosition.x * self.contentScaleFactor, focusPosition.y * self.contentScaleFactor, scale, pinchRecognizer.velocity);
            } else {
                self.map->handlePinchGesture(location.x * self.contentScaleFactor, location.y * self.contentScaleFactor, scale, pinchRecognizer.velocity);
            }
            break;
        case UIGestureRecognizerStateEnded:
            [self setMapRegionChangeState:TGMapRegionIdle];
            break;
        default:
            break;
    }

    if ([self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizePinchGesture:)]) {
        [self.gestureDelegate mapView:self recognizer:pinchRecognizer didRecognizePinchGesture:location];
    }

    // Reset scale to 1 so that subsequent calls get relative value.
    [pinchRecognizer setScale:1.0];
}

- (void)respondToRotationGesture:(UIRotationGestureRecognizer *)rotationRecognizer
{
    CGPoint position = [rotationRecognizer locationInView:_glView];
    if ([self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:shouldRecognizeRotationGesture:)]) {
        if (![self.gestureDelegate mapView:self recognizer:rotationRecognizer shouldRecognizeRotationGesture:position]) {
            return;
        }
    }

    CGFloat rotation = rotationRecognizer.rotation;
    switch (rotationRecognizer.state) {
        case UIGestureRecognizerStateBegan:
            [self setMapRegionChangeState:TGMapRegionJumping];
            break;
        case UIGestureRecognizerStateChanged:
            [self setMapRegionChangeState:TGMapRegionJumping];
            if ([self.gestureDelegate respondsToSelector:@selector(rotationFocus:recognizer:)]) {
                CGPoint focusPosition = [self.gestureDelegate rotationFocus:self recognizer:rotationRecognizer];
                self.map->handleRotateGesture(focusPosition.x * self.contentScaleFactor, focusPosition.y * self.contentScaleFactor, rotation);
            } else {
                self.map->handleRotateGesture(position.x * self.contentScaleFactor, position.y * self.contentScaleFactor, rotation);
            }
            break;
        case UIGestureRecognizerStateEnded:
            [self setMapRegionChangeState:TGMapRegionIdle];
            break;
        default:
            break;
    }

    if ([self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizeRotationGesture:)]) {
        [self.gestureDelegate mapView:self recognizer:rotationRecognizer didRecognizeRotationGesture:position];
    }

    // Reset rotation to zero so that subsequent calls get relative value.
    [rotationRecognizer setRotation:0.0];
}

- (void)respondToShoveGesture:(UIPanGestureRecognizer *)shoveRecognizer
{
    CGPoint displacement = [shoveRecognizer translationInView:_glView];

    if ([self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:shouldRecognizeShoveGesture:)]) {
        if (![self.gestureDelegate mapView:self recognizer:shoveRecognizer shouldRecognizeShoveGesture:displacement]) {
            return;
        }
    }

    switch (shoveRecognizer.state) {
        case UIGestureRecognizerStateBegan:
            [self setMapRegionChangeState:TGMapRegionJumping];
            break;
        case UIGestureRecognizerStateChanged:
            [self setMapRegionChangeState:TGMapRegionJumping];
            self.map->handleShoveGesture(displacement.y);
            if ([self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizeShoveGesture:)]) {
                [self.gestureDelegate mapView:self recognizer:shoveRecognizer didRecognizeShoveGesture:displacement];
            }
            break;
        case UIGestureRecognizerStateEnded:
            [self setMapRegionChangeState:TGMapRegionIdle];
            break;
        default:
            break;
    }

    // Reset translation to zero so that subsequent calls get relative value.
    [shoveRecognizer setTranslation:CGPointZero inView:_glView];
}

#pragma mark Map region change state notifiers

- (void)notifyGestureDidBegin
{
    [self setMapRegionChangeState:TGMapRegionAnimating];
}

- (void)notifyGestureIsChanging
{
    [self setMapRegionChangeState:TGMapRegionAnimating];
}

- (void)notifyGestureDidEnd
{
    [self setMapRegionChangeState:TGMapRegionIdle];
}

#pragma mark Internal Logic

- (void)setMapRegionChangeState:(TGMapRegionChangeState)state
{
    switch (_currentState) {
        case TGMapRegionIdle:
            if (state == TGMapRegionJumping) {
                [self regionWillChangeAnimated:NO];
            } else if (state == TGMapRegionAnimating) {
                [self regionWillChangeAnimated:YES];
            }
            break;
        case TGMapRegionJumping:
            if (state == TGMapRegionIdle) {
                [self regionDidChangeAnimated:NO];
            } else if (state == TGMapRegionJumping) {
                [self regionIsChanging];
            }
            break;
        case TGMapRegionAnimating:
            if (state == TGMapRegionIdle) {
                [self regionDidChangeAnimated:YES];
            } else if (state == TGMapRegionAnimating) {
                [self regionIsChanging];
            }
            break;
        default:
            break;
    }
    _currentState = state;
}

- (void)regionWillChangeAnimated:(BOOL)animated
{
    if ([self.mapViewDelegate respondsToSelector:@selector(mapView:regionWillChangeAnimated:)]) {
        [self.mapViewDelegate mapView:self regionWillChangeAnimated:animated];
    }
}

- (void)regionIsChanging
{
    if ([self.mapViewDelegate respondsToSelector:@selector(mapViewRegionIsChanging:)]) {
        [self.mapViewDelegate mapViewRegionIsChanging:self];
    }
}

- (void)regionDidChangeAnimated:(BOOL)animated
{
    if ([self.mapViewDelegate respondsToSelector:@selector(mapView:regionDidChangeAnimated:)]) {
        [self.mapViewDelegate mapView:self regionDidChangeAnimated:animated];
    }
}

- (void)trySetMapDefaultBackground:(UIColor *)backgroundColor
{
    if (_map) {
        CGFloat red = 0.0, green = 0.0, blue = 0.0, alpha = 0.0;
        [backgroundColor getRed:&red green:&green blue:&blue alpha:&alpha];
        _map->setDefaultBackgroundColor(red, green, blue);
    }
}

@end // implementation TGMapView
