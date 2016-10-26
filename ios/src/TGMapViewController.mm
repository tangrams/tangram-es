//
//  TGMapViewController.mm
//  TangramiOS
//
//  Created by Matt Blair on 8/25/14.
//  Updated by Matt Smollinger on 7/29/16.
//  Copyright (c) 2016 Mapzen. All rights reserved.
//

#import "TGMapViewController.h"
#include "platform_ios.h"
#include "data/propertyItem.h"
#include "tangram.h"

@interface TGMapViewController ()

@property (nullable, copy, nonatomic) NSString* scenePath;
@property (nullable, strong, nonatomic) EAGLContext* context;
@property (assign, nonatomic) CGFloat pixelScale;
@property (assign, nonatomic) BOOL renderRequested;
@property (assign, nonatomic, nullable) Tangram::Map* map;

@end


@implementation TGMapViewController

- (void)loadSceneFile:(NSString*)path {
    if (!self.map) {
        return;
    }

    self.scenePath = path;
    self.map->loadScene([path UTF8String]);
    self.renderRequested = YES;
}

- (void)loadSceneFileAsync:(NSString*)path {
    if (!self.map) { return; }

    self.scenePath = path;

    MapReady onReadyCallback = [self, path](void* _userPtr) -> void {
        if (self.mapViewDelegate && [self.mapViewDelegate respondsToSelector:@selector(mapView:didLoadSceneAsync:)]) {
            [self.mapViewDelegate mapView:self didLoadSceneAsync:path];
        }

        self.renderRequested = YES;
    };

    self.map->loadSceneAsync([path UTF8String], false, onReadyCallback, nullptr);
}

- (void)queueSceneUpdate:(NSString*)componentPath withValue:(NSString*)value {
    if (!self.map) { return; }

    self.map->queueSceneUpdate([componentPath UTF8String], [componentPath UTF8String]);
}

- (void)requestRender {
    if (!self.map) { return; }

    self.renderRequested = YES;
}

- (CGPoint)lngLatToScreenPosition:(TGGeoPoint)lngLat {
    static const CGPoint nullCGPoint = {(CGFloat)NAN, (CGFloat)NAN};

    if (!self.map) { return nullCGPoint; }

    double screenPosition[2];
    if (self.map->lngLatToScreenPosition(lngLat.longitude, lngLat.latitude,
        &screenPosition[0], &screenPosition[1])) {
        return CGPointMake((CGFloat)screenPosition[0], (CGFloat)screenPosition[1]);
    }

    return nullCGPoint;
}

- (TGGeoPoint)screenPositionToLngLat:(CGPoint)screenPosition {
    static const TGGeoPoint nullTangramGeoPoint = {NAN, NAN};

    if (!self.map) { return nullTangramGeoPoint; }

    TGGeoPoint lngLat;
    if (self.map->screenPositionToLngLat(screenPosition.x, screenPosition.y,
        &lngLat.longitude, &lngLat.latitude)) {
        return lngLat;
    }

    return nullTangramGeoPoint;
}

- (void)pickFeaturesAt:(CGPoint)screenPosition {
    if (!self.map && !self.mapViewDelegate) { return; }

    self.map->pickFeaturesAt(screenPosition.x, screenPosition.y, [self](const auto& items) {
        NSMutableDictionary* dictionary = [[NSMutableDictionary alloc] init];
        CGPoint position = CGPointMake(0.0, 0.0);

        if (items.size() > 0) {
            const auto& result = items[0];
            const auto& properties = result.properties;
            position = CGPointMake(result.position[0], result.position[1]);

            for (const auto& item : properties->items()) {
                NSString* key = [NSString stringWithUTF8String:item.key.c_str()];
                NSString* value = [NSString stringWithUTF8String:properties->asString(item.value).c_str()];
                dictionary[key] = value;
            }
        }

        if ([self.mapViewDelegate respondsToSelector:@selector(mapView:didSelectFeatures:atScreenPosition:)]) {
            [self.mapViewDelegate mapView:self didSelectFeatures:dictionary atScreenPosition:position];
        }
    });
}

- (void)setPosition:(TGGeoPoint)position {
    if (self.map) {
        self.map->setPosition(position.longitude, position.latitude);
    }
}

- (void)animateToPosition:(TGGeoPoint)position withDuration:(float)duration {
    [self animateToPosition:position withDuration:duration withEaseType:TGEaseTypeCubic];
}

- (void)animateToPosition:(TGGeoPoint)position withDuration:(float)duration withEaseType:(TGEaseType)easeType {

    if (self.map) {
        Tangram::EaseType ease = [self convertEaseTypeFrom:easeType];
        self.map->setPositionEased(position.longitude, position.latitude, duration, ease);
    }
}

- (TGGeoPoint)position {
    TGGeoPoint returnVal;
    if (self.map){
        self.map->getPosition(returnVal.longitude, returnVal.latitude);
        return returnVal;
    }
    //Null Island
    returnVal.latitude = 0.0;
    returnVal.longitude = 0.0;
    return returnVal;
}

- (void)setZoom:(float)zoom {
    if (self.map) {
        self.map->setZoom(zoom);
    }
}

- (Tangram::EaseType)convertEaseTypeFrom:(TGEaseType)ease {
    switch (ease) {
        case TGEaseTypeLinear:
            return Tangram::EaseType::linear;
        case TGEaseTypeSine:
            return Tangram::EaseType::sine;
        case TGEaseTypeQuint:
            return Tangram::EaseType::quint;
        case TGEaseTypeCubic:
            return Tangram::EaseType::cubic;
        default:
            return Tangram::EaseType::cubic;
    }
}

- (void)animateToZoomLevel:(float)zoomLevel withDuration:(float)duration {
    [self animateToZoomLevel:zoomLevel withDuration:duration withEaseType:TGEaseTypeCubic];
}

- (void)animateToZoomLevel:(float)zoomLevel withDuration:(float)duration withEaseType:(TGEaseType)easeType {
    if (self.map) {
        Tangram::EaseType ease = [self convertEaseTypeFrom:easeType];
        self.map->setZoomEased(zoomLevel, duration, ease);
    }
}

- (float)zoom {
    if (self.map) {
        return self.map->getZoom();
    }
    return 0.0;
}

- (void)animateToRotation:(float)radians withDuration:(float)seconds {
    [self animateToRotation:radians withDuration:seconds withEaseType:TGEaseTypeCubic];
}

- (void)animateToRotation:(float)radians withDuration:(float)seconds withEaseType:(TGEaseType)easeType {
    if (self.map) {
        Tangram::EaseType ease = [self convertEaseTypeFrom:easeType];
        self.map->setRotationEased(radians, seconds, ease);
    }
}

- (void)setRotation:(float)radians {
    if (self.map) {
        self.map->setRotation(radians);
    }
}

- (float)rotation {
    if (self.map) {
        return self.map->getRotation();
    }
    return 0.0;
}

- (float)tilt {
    if (self.map) {
        return self.map->getTilt();
    }
    return 0.0;
}

- (void)setTilt:(float)radians {
    if (self.map) {
        self.map->setTilt(radians);
    }
}

- (void)animateToTilt:(float)radians withDuration:(float)seconds {
  [self animateToTilt:radians withDuration:seconds withEaseType:TGEaseType::TGEaseTypeCubic];
}

- (void)animateToTilt:(float)radians withDuration:(float)seconds withEaseType:(TGEaseType)easeType {
  if (self.map) {
    Tangram::EaseType ease = [self convertEaseTypeFrom:easeType];
    self.map->setTiltEased(radians, seconds, ease);
  }
}

- (TGCameraType)cameraType {
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

- (void)setCameraType:(TGCameraType)cameraType {
    if (self.map){
        self.map->setCameraType(cameraType);
    }
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }
    self.pixelScale = [[UIScreen mainScreen] scale];
    self.renderRequested = YES;
    self.continuous = NO;

    init(self);

    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    view.drawableMultisample = GLKViewDrawableMultisample4X;

    [self setupGestureRecognizers];
    [self setupGL];

}

- (void)setupGestureRecognizers {

    /* Construct Gesture Recognizers */
    //1. Tap
    UITapGestureRecognizer *tapRecognizer = [[UITapGestureRecognizer alloc]
                                             initWithTarget:self action:@selector(respondToTapGesture:)];
    tapRecognizer.numberOfTapsRequired = 1;
    // TODO: Figure a way to have a delay set for it not to tap gesture not to wait long enough for a doubletap gesture to be recognized
    tapRecognizer.delaysTouchesEnded = NO;

    //2. DoubleTap
    UITapGestureRecognizer *doubleTapRecognizer = [[UITapGestureRecognizer alloc]
                                                   initWithTarget:self action:@selector(respondToDoubleTapGesture:)];
    doubleTapRecognizer.numberOfTapsRequired = 2;
    // Distanle single tap when double tap occurs
    [tapRecognizer requireGestureRecognizerToFail:doubleTapRecognizer];

    //3. Pan
    UIPanGestureRecognizer *panRecognizer = [[UIPanGestureRecognizer alloc]
                                             initWithTarget:self action:@selector(respondToPanGesture:)];
    panRecognizer.maximumNumberOfTouches = 1;

    //4. Pinch
    UIPinchGestureRecognizer *pinchRecognizer = [[UIPinchGestureRecognizer alloc]
                                                 initWithTarget:self action:@selector(respondToPinchGesture:)];

    //5. Rotate
    UIRotationGestureRecognizer *rotationRecognizer = [[UIRotationGestureRecognizer alloc]
                                                       initWithTarget:self action:@selector(respondToRotationGesture:)];

    //6. Shove
    UIPanGestureRecognizer *shoveRecognizer = [[UIPanGestureRecognizer alloc]
                                               initWithTarget:self action:@selector(respondToShoveGesture:)];
    shoveRecognizer.minimumNumberOfTouches = 2;

    // Use the delegate method 'shouldRecognizeSimultaneouslyWithGestureRecognizer' for gestures that can be concurrent
    panRecognizer.delegate = self;
    pinchRecognizer.delegate = self;
    rotationRecognizer.delegate = self;

    /* Setup gesture recognizers */
    [self.view addGestureRecognizer:tapRecognizer];
    [self.view addGestureRecognizer:doubleTapRecognizer];
    [self.view addGestureRecognizer:panRecognizer];
    [self.view addGestureRecognizer:pinchRecognizer];
    [self.view addGestureRecognizer:rotationRecognizer];
    [self.view addGestureRecognizer:shoveRecognizer];
}

// Implement touchesBegan to catch down events
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    self.map->handlePanGesture(0.0f, 0.0f, 0.0f, 0.0f);
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    // make shove gesture exclusive
    if ([gestureRecognizer isKindOfClass:[UIPanGestureRecognizer class]]) {
        return [gestureRecognizer numberOfTouches] != 2;
    }
    if ([otherGestureRecognizer isKindOfClass:[UIPanGestureRecognizer class]]) {
        return [otherGestureRecognizer numberOfTouches] != 2;
    }
    return YES;
}

- (void)respondToTapGesture:(UITapGestureRecognizer *)tapRecognizer {
    CGPoint location = [tapRecognizer locationInView:self.view];
    self.map->handleTapGesture(location.x * self.pixelScale, location.y * self.pixelScale);
    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(recognizer:didRecognizeSingleTap:)]) {
        [self.gestureDelegate recognizer:tapRecognizer didRecognizeSingleTap:[tapRecognizer locationInView:self.view]];
    }
}

- (void)respondToDoubleTapGesture:(UITapGestureRecognizer *)doubleTapRecognizer {
    CGPoint location = [doubleTapRecognizer locationInView:self.view];
    self.map->handleDoubleTapGesture(location.x * self.pixelScale, location.y * self.pixelScale);
    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(recognizer:didRecognizeDoubleTap:)]) {
        [self.gestureDelegate recognizer:doubleTapRecognizer didRecognizeDoubleTap:[doubleTapRecognizer locationInView:self.view]];
    }
}

- (void)respondToPanGesture:(UIPanGestureRecognizer *)panRecognizer {
    CGPoint displacement = [panRecognizer translationInView:self.view];
    CGPoint velocity = [panRecognizer velocityInView:self.view];
    CGPoint end = [panRecognizer locationInView:self.view];
    CGPoint start = {end.x - displacement.x, end.y - displacement.y};

    [panRecognizer setTranslation:CGPointZero inView:self.view];

    switch (panRecognizer.state) {
        case UIGestureRecognizerStateChanged:
            self.map->handlePanGesture(start.x * self.pixelScale, start.y * self.pixelScale, end.x * self.pixelScale, end.y * self.pixelScale);
            break;
        case UIGestureRecognizerStateEnded:
            self.map->handleFlingGesture(end.x * self.pixelScale, end.y * self.pixelScale, velocity.x * self.pixelScale, velocity.y * self.pixelScale);
            break;
        default:
            break;
    }

    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(recognizer:didRecognizePanGesture:)]) {
        [self.gestureDelegate recognizer:panRecognizer didRecognizePanGesture:[panRecognizer locationInView:self.view]];
    }
}

- (void)respondToPinchGesture:(UIPinchGestureRecognizer *)pinchRecognizer {
    CGPoint location = [pinchRecognizer locationInView:self.view];
    CGFloat scale = pinchRecognizer.scale;
    [pinchRecognizer setScale:1.0];
    self.map->handlePinchGesture(location.x * self.pixelScale, location.y * self.pixelScale, scale, pinchRecognizer.velocity);
    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(recognizer:didRecognizePinchGesture:)]) {
        [self.gestureDelegate recognizer:pinchRecognizer didRecognizePinchGesture:[pinchRecognizer locationInView:self.view]];
    }
}

- (void)respondToRotationGesture:(UIRotationGestureRecognizer *)rotationRecognizer {
    CGPoint position = [rotationRecognizer locationInView:self.view];
    CGFloat rotation = rotationRecognizer.rotation;
    [rotationRecognizer setRotation:0.0];
    self.map->handleRotateGesture(position.x * self.pixelScale, position.y * self.pixelScale, rotation);
    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(recognizer:didRecognizeRotationGesture:)]) {
        [self.gestureDelegate recognizer:rotationRecognizer didRecognizeRotationGesture:[rotationRecognizer locationInView:self.view]];
    }
}

- (void)respondToShoveGesture:(UIPanGestureRecognizer *)shoveRecognizer {
    CGPoint displacement = [shoveRecognizer translationInView:self.view];
    [shoveRecognizer setTranslation:{0, 0} inView:self.view];

    // don't trigger shove on single touch gesture
    if ([shoveRecognizer numberOfTouches] == 2) {
        self.map->handleShoveGesture(displacement.y);
        if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(recognizer:didRecognizeShoveGesture:)]) {
            [self.gestureDelegate recognizer:shoveRecognizer didRecognizeShoveGesture:[shoveRecognizer locationInView:self.view]];
        }
    }
}

- (void)dealloc
{
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];

    if ([self isViewLoaded] && ([[self view] window] == nil)) {
        self.view = nil;

        if ([EAGLContext currentContext] == self.context) {
            [EAGLContext setCurrentContext:nil];
        }
        self.context = nil;
    }

    // Dispose of any resources that can be recreated.
}

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];

    if (!self.map) {
        self.map = new Tangram::Map();
    }

    self.map->setupGL();

    int width = self.view.bounds.size.width;
    int height = self.view.bounds.size.height;

    self.map->resize(width * self.pixelScale, height * self.pixelScale);

    self.map->setPixelScale(self.pixelScale);
}

- (void)tearDownGL
{
    if (self.map) {
        delete self.map;
        self.map = nullptr;
    }
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
    self.map->resize(size.width * self.pixelScale, size.height * self.pixelScale);
    [self renderOnce];
}

- (void)renderOnce
{
    if (!self.continuous) {
        self.renderRequested = YES;
        self.paused = NO;
    }
}

- (void)setContinuous:(BOOL)c
{
    _continuous = c;
    self.paused = !c;
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    self.map->update([self timeSinceLastUpdate]);

    if (!self.continuous && !self.renderRequested) {
        self.paused = YES;
    }
    self.renderRequested = NO;
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    self.map->render();
}

@end
