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
#include "tangram.h"

@interface TGMapViewController ()

@property (nullable, strong, nonatomic) EAGLContext *context;
@property (assign, nonatomic) CGFloat pixelScale;
@property (assign, nonatomic) BOOL renderRequested;
@property (assign, nonatomic, nullable) Tangram::Map* map;

@end


@implementation TGMapViewController

- (void)setPosition:(TangramGeoPoint)position {
    if (self.map) {
        self.map->setPosition(position.longitude, position.latitude);
    }
}

- (void)animateToPosition:(TangramGeoPoint)position withDuration:(float)duration {
    [self animateToPosition:position withDuration:duration withEaseType:TangramEaseTypeCubic];
}

- (void)animateToPosition:(TangramGeoPoint)position withDuration:(float)duration withEaseType:(TangramEaseType)easeType {

    if (self.map) {
        Tangram::EaseType ease = [self convertEaseTypeFrom:easeType];
        self.map->setPositionEased(position.longitude, position.latitude, duration, ease);
    }
}

- (TangramGeoPoint)position {
    TangramGeoPoint returnVal;
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

- (Tangram::EaseType)convertEaseTypeFrom:(TangramEaseType)ease {
    switch (ease) {
        case TangramEaseTypeLinear:
            return Tangram::EaseType::linear;
        case TangramEaseTypeSine:
            return Tangram::EaseType::sine;
        case TangramEaseTypeQuint:
            return Tangram::EaseType::quint;
        case TangramEaseTypeCubic:
            return Tangram::EaseType::cubic;
        default:
            return Tangram::EaseType::cubic;
    }
}

- (void)animateToZoomLevel:(float)zoomLevel withDuration:(float)duration {
    [self animateToZoomLevel:zoomLevel withDuration:duration withEaseType:TangramEaseTypeCubic];
}

- (void)animateToZoomLevel:(float)zoomLevel withDuration:(float)duration withEaseType:(TangramEaseType)easeType {
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
    [self animateToRotation:radians withDuration:seconds withEaseType:TangramEaseTypeCubic];
}

- (void)animateToRotation:(float)radians withDuration:(float)seconds withEaseType:(TangramEaseType)easeType {
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
  [self animateToTilt:radians withDuration:seconds withEaseType:TangramEaseTypeCubic];
}

- (void)animateToTilt:(float)radians withDuration:(float)seconds withEaseType:(TangramEaseType)easeType {
  if (self.map) {
    Tangram::EaseType ease = [self convertEaseTypeFrom:easeType];
    self.map->setTiltEased(radians, seconds, ease);
  }
}

- (TangramCameraType)cameraType {
    switch (self.map->getCameraType()) {
        case 0:
            return TangramCameraTypePerspective;
        case 1:
            return TangramCameraTypeIsometric;
        case 2:
            return TangramCameraTypeFlat;
        default:
            return TangramCameraTypePerspective;
    }
}

- (void)setCameraType:(TangramCameraType)cameraType {
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
        self.map->loadSceneAsync("scene.yaml");
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
