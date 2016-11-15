//
//  TGMapViewController.mm
//  TangramiOS
//
//  Created by Matt Blair on 8/25/14.
//  Updated by Matt Smollinger on 7/29/16.
//  Copyright (c) 2016 Mapzen. All rights reserved.
//

#import "TGMapViewController.h"
#import "Helpers.h"

#include "platform_ios.h"
#include "data/propertyItem.h"
#include "tangram.h"

__CG_STATIC_ASSERT(sizeof(TGMapMarkerId) == sizeof(Tangram::MarkerID));
__CG_STATIC_ASSERT(sizeof(TGGeoPoint) == sizeof(Tangram::LngLat));

@interface TGMapViewController ()

@property (nullable, copy, nonatomic) NSString* scenePath;
@property (nullable, strong, nonatomic) EAGLContext* context;
@property (assign, nonatomic) CGFloat pixelScale;
@property (assign, nonatomic) BOOL renderRequested;
@property (assign, nonatomic, nullable) Tangram::Map* map;

@end

@implementation TGMapViewController

#pragma mark Scene loading interface

- (void)loadSceneFile:(NSString*)path
{
    if (!self.map) { return; }

    self.scenePath = path;
    self.map->loadScene([path UTF8String]);
    self.renderRequested = YES;
}

- (void)loadSceneFileAsync:(NSString*)path
{
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

#pragma mark Scene updates

- (void)queueSceneUpdate:(NSString*)componentPath withValue:(NSString*)value
{
    if (!self.map) { return; }

    self.map->queueSceneUpdate([componentPath UTF8String], [value UTF8String]);
}

- (void)applySceneUpdates
{
    if (!self.map) { return; }

    self.map->applySceneUpdates();
}

#pragma mark Longitude/Latitude - Screen position conversions

- (CGPoint)lngLatToScreenPosition:(TGGeoPoint)lngLat
{
    static const CGPoint nullCGPoint = {(CGFloat)NAN, (CGFloat)NAN};

    if (!self.map) { return nullCGPoint; }

    double screenPosition[2];
    if (self.map->lngLatToScreenPosition(lngLat.longitude, lngLat.latitude,
        &screenPosition[0], &screenPosition[1])) {

        screenPosition[0] /= self.pixelScale;
        screenPosition[1] /= self.pixelScale;

        return CGPointMake((CGFloat)screenPosition[0], (CGFloat)screenPosition[1]);
    }

    return nullCGPoint;
}

- (TGGeoPoint)screenPositionToLngLat:(CGPoint)screenPosition
{
    static const TGGeoPoint nullTangramGeoPoint = {NAN, NAN};

    if (!self.map) { return nullTangramGeoPoint; }

    screenPosition.x *= self.pixelScale;
    screenPosition.y *= self.pixelScale;

    TGGeoPoint lngLat;
    if (self.map->screenPositionToLngLat(screenPosition.x, screenPosition.y,
        &lngLat.longitude, &lngLat.latitude)) {
        return lngLat;
    }

    return nullTangramGeoPoint;
}

#pragma mark Feature picking

- (void)pickFeaturesAt:(CGPoint)screenPosition
{
    if (!self.map && !self.mapViewDelegate) { return; }

    screenPosition.x *= self.pixelScale;
    screenPosition.y *= self.pixelScale;

    self.map->pickFeaturesAt(screenPosition.x, screenPosition.y, [screenPosition, self](const auto& items) {
        NSMutableDictionary* dictionary = [[NSMutableDictionary alloc] init];
        CGPoint position = CGPointMake(screenPosition.x / self.pixelScale, screenPosition.y / self.pixelScale);

        if (items.size() > 0) {
            const auto& result = items[0];
            const auto& properties = result.properties;
            position = CGPointMake(result.position[0] / self.pixelScale, result.position[1] / self.pixelScale);

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

#pragma mark Marker implementation

- (TGMapMarkerId)markerAdd
{
    if (!self.map) { return 0; }

    return (TGMapMarkerId)self.map->markerAdd();
}

- (BOOL)markerRemove:(TGMapMarkerId)marker
{
    if (!self.map) { return NO; }

    return self.map->markerRemove(marker);
}

- (void)markerRemoveAll
{
    if (!self.map) { return; }

    self.map->markerRemoveAll();
}

- (BOOL)markerSetStyling:(TGMapMarkerId)identifier styling:(NSString *)styling
{
    if (!self.map) { return NO; }

    return self.map->markerSetStyling(identifier, [styling UTF8String]);
}

- (BOOL)markerSetPoint:(TGMapMarkerId)identifier coordinates:(TGGeoPoint)coordinate
{
    if (!self.map || !identifier) { return NO; }

    Tangram::LngLat lngLat(coordinate.longitude, coordinate.latitude);

    return self.map->markerSetPoint(identifier, lngLat);
}

- (BOOL)markerSetPointEased:(TGMapMarkerId)identifier coordinates:(TGGeoPoint)coordinate duration:(float)duration easeType:(TGEaseType)ease
{
    if (!self.map || !identifier) { return NO; }

    Tangram::LngLat lngLat(coordinate.longitude, coordinate.latitude);

    return self.map->markerSetPointEased(identifier, lngLat, duration, [Helpers convertEaseTypeFrom:ease]);
}

- (BOOL)markerSetPolyline:(TGMapMarkerId)identifier polyline:(TGGeoPolyline *)polyline
{
    if (polyline.count < 2 || !identifier) { return NO; }

    return self.map->markerSetPolyline(identifier, reinterpret_cast<Tangram::LngLat*>([polyline coordinates]), polyline.count);
}

- (BOOL)markerSetPolygon:(TGMapMarkerId)identifier polygon:(TGGeoPolygon *)polygon;
{
    if (polygon.count < 3 || !identifier) { return NO; }

    auto coords = reinterpret_cast<Tangram::LngLat*>([polygon coordinates]);

    return self.map->markerSetPolygon(identifier, coords, [polygon rings], [polygon ringsCount]);
}

- (BOOL)markerSetVisible:(TGMapMarkerId)identifier visible:(BOOL)visible
{
    if (!self.map) { return NO; }

    return self.map->markerSetVisible(identifier, visible);
}

#pragma mark Map position implementation

- (void)setPosition:(TGGeoPoint)position {
    if (!self.map) { return; }

    self.map->setPosition(position.longitude, position.latitude);
}

- (void)animateToPosition:(TGGeoPoint)position withDuration:(float)duration
{
    [self animateToPosition:position withDuration:duration withEaseType:TGEaseTypeCubic];
}

- (void)animateToPosition:(TGGeoPoint)position withDuration:(float)duration withEaseType:(TGEaseType)easeType
{
    if (!self.map) { return; }

    Tangram::EaseType ease = [Helpers convertEaseTypeFrom:easeType];
    self.map->setPositionEased(position.longitude, position.latitude, duration, ease);
}

- (TGGeoPoint)position
{
    static const TGGeoPoint nullTangramGeoPoint = {NAN, NAN};

    if (!self.map) { return nullTangramGeoPoint; }

    TGGeoPoint returnVal;

    self.map->getPosition(returnVal.longitude, returnVal.latitude);

    return returnVal;
}

- (void)setZoom:(float)zoom
{
    if (!self.map) { return; }

    self.map->setZoom(zoom);
}

- (void)animateToZoomLevel:(float)zoomLevel withDuration:(float)duration
{
    [self animateToZoomLevel:zoomLevel withDuration:duration withEaseType:TGEaseTypeCubic];
}

- (void)animateToZoomLevel:(float)zoomLevel withDuration:(float)duration withEaseType:(TGEaseType)easeType
{
    if (!self.map) { return; }

    Tangram::EaseType ease = [Helpers convertEaseTypeFrom:easeType];
    self.map->setZoomEased(zoomLevel, duration, ease);
}

- (float)zoom
{
    if (!self.map) { return 0.0; }

    return self.map->getZoom();
}

- (void)animateToRotation:(float)radians withDuration:(float)seconds
{
    [self animateToRotation:radians withDuration:seconds withEaseType:TGEaseTypeCubic];
}

- (void)animateToRotation:(float)radians withDuration:(float)seconds withEaseType:(TGEaseType)easeType
{
    if (!self.map) { return; }

    Tangram::EaseType ease = [Helpers convertEaseTypeFrom:easeType];
    self.map->setRotationEased(radians, seconds, ease);
}

- (void)setRotation:(float)radians
{
    if (!self.map) { return; }

    self.map->setRotation(radians);
}

- (float)rotation
{
    if (!self.map) { return 0.0; }

    return self.map->getRotation();
}

- (float)tilt
{
    if (!self.map) { return 0.0; }

    return self.map->getTilt();
}

- (void)setTilt:(float)radians
{
    if (!self.map) { return; }

    self.map->setTilt(radians);
}

- (void)animateToTilt:(float)radians withDuration:(float)seconds
{
    [self animateToTilt:radians withDuration:seconds withEaseType:TGEaseType::TGEaseTypeCubic];
}

- (void)animateToTilt:(float)radians withDuration:(float)seconds withEaseType:(TGEaseType)easeType
{
    if (!self.map) { return; }

    Tangram::EaseType ease = [Helpers convertEaseTypeFrom:easeType];
    self.map->setTiltEased(radians, seconds, ease);
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
    if (!self.map){ return; }

    self.map->setCameraType(cameraType);
}

#pragma mark Gestures

- (void)setupGestureRecognizers
{
    /* Construct Gesture Recognizers */
    //1. Tap
    UITapGestureRecognizer* tapRecognizer = [[UITapGestureRecognizer alloc]
                                             initWithTarget:self action:@selector(respondToTapGesture:)];
    tapRecognizer.numberOfTapsRequired = 1;
    // TODO: Figure a way to have a delay set for it not to tap gesture not to wait long enough for a doubletap gesture to be recognized
    tapRecognizer.delaysTouchesEnded = NO;

    //2. DoubleTap
    UITapGestureRecognizer* doubleTapRecognizer = [[UITapGestureRecognizer alloc]
                                                   initWithTarget:self action:@selector(respondToDoubleTapGesture:)];
    doubleTapRecognizer.numberOfTapsRequired = 2;
    // Distanle single tap when double tap occurs
    [tapRecognizer requireGestureRecognizerToFail:doubleTapRecognizer];

    //3. Pan
    UIPanGestureRecognizer* panRecognizer = [[UIPanGestureRecognizer alloc]
                                             initWithTarget:self action:@selector(respondToPanGesture:)];
    panRecognizer.maximumNumberOfTouches = 1;

    //4. Pinch
    UIPinchGestureRecognizer* pinchRecognizer = [[UIPinchGestureRecognizer alloc]
                                                 initWithTarget:self action:@selector(respondToPinchGesture:)];

    //5. Rotate
    UIRotationGestureRecognizer* rotationRecognizer = [[UIRotationGestureRecognizer alloc]
                                                       initWithTarget:self action:@selector(respondToRotationGesture:)];

    //6. Shove
    UIPanGestureRecognizer* shoveRecognizer = [[UIPanGestureRecognizer alloc]
                                               initWithTarget:self action:@selector(respondToShoveGesture:)];
    shoveRecognizer.minimumNumberOfTouches = 2;

    //7. Long press
    UILongPressGestureRecognizer* longPressRecognizer = [[UILongPressGestureRecognizer alloc]
                                                         initWithTarget:self action:@selector(respondToLongPressGesture:)];

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
    [self.view addGestureRecognizer:longPressRecognizer];
}

// Implement touchesBegan to catch down events
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    self.map->handlePanGesture(0.0f, 0.0f, 0.0f, 0.0f);
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer
{
    // make shove gesture exclusive
    if ([gestureRecognizer isKindOfClass:[UIPanGestureRecognizer class]]) {
        return [gestureRecognizer numberOfTouches] != 2;
    }
    if ([otherGestureRecognizer isKindOfClass:[UIPanGestureRecognizer class]]) {
        return [otherGestureRecognizer numberOfTouches] != 2;
    }
    return YES;
}

- (void)respondToLongPressGesture:(UILongPressGestureRecognizer *)longPressRecognizer
{
    CGPoint location = [longPressRecognizer locationInView:self.view];
    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizeLongPressGesture:)]) {
        [self.gestureDelegate mapView:self recognizer:longPressRecognizer didRecognizeLongPressGesture:location];
    }
}

- (void)respondToTapGesture:(UITapGestureRecognizer *)tapRecognizer
{
    CGPoint location = [tapRecognizer locationInView:self.view];
    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizeSingleTapGesture:)]) {
        [self.gestureDelegate mapView:self recognizer:tapRecognizer didRecognizeSingleTapGesture:location];
    }
}

- (void)respondToDoubleTapGesture:(UITapGestureRecognizer *)doubleTapRecognizer
{
    CGPoint location = [doubleTapRecognizer locationInView:self.view];
    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizeDoubleTapGesture:)]) {
        [self.gestureDelegate mapView:self recognizer:doubleTapRecognizer didRecognizeDoubleTapGesture:location];
    }
}

- (void)respondToPanGesture:(UIPanGestureRecognizer *)panRecognizer
{
    CGPoint displacement = [panRecognizer translationInView:self.view];

    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizePanGesture:)]) {
        [self.gestureDelegate mapView:self recognizer:panRecognizer didRecognizePanGesture:displacement];
    } else {
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
    }
}

- (void)respondToPinchGesture:(UIPinchGestureRecognizer *)pinchRecognizer
{
    CGPoint location = [pinchRecognizer locationInView:self.view];
    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizePinchGesture:)]) {
        [self.gestureDelegate mapView:self recognizer:pinchRecognizer didRecognizePinchGesture:location];
    } else {
        CGFloat scale = pinchRecognizer.scale;
        [pinchRecognizer setScale:1.0];
        self.map->handlePinchGesture(location.x * self.pixelScale, location.y * self.pixelScale, scale, pinchRecognizer.velocity);
    }
}

- (void)respondToRotationGesture:(UIRotationGestureRecognizer *)rotationRecognizer
{
    CGPoint position = [rotationRecognizer locationInView:self.view];
    CGFloat rotation = rotationRecognizer.rotation;
    [rotationRecognizer setRotation:0.0];
    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizeRotationGesture:)]) {
        [self.gestureDelegate mapView:self recognizer:rotationRecognizer didRecognizeRotationGesture:position];
    } else {
        self.map->handleRotateGesture(position.x * self.pixelScale, position.y * self.pixelScale, rotation);
    }
}

- (void)respondToShoveGesture:(UIPanGestureRecognizer *)shoveRecognizer
{
    CGPoint displacement = [shoveRecognizer translationInView:self.view];
    [shoveRecognizer setTranslation:{0, 0} inView:self.view];

    // don't trigger shove on single touch gesture
    if ([shoveRecognizer numberOfTouches] == 2) {
        if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(recognizer:didRecognizeShoveGesture:)]) {
            [self.gestureDelegate mapView:self recognizer:shoveRecognizer didRecognizeShoveGesture:displacement];
        } else {
            self.map->handleShoveGesture(displacement.y);
        }
    }
}

#pragma mark Map view lifecycle

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
    if (!self.map) { return; }

    delete self.map;
    self.map = nullptr;
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
    self.map->resize(size.width * self.pixelScale, size.height * self.pixelScale);

    [self renderOnce];
}

- (void)requestRender
{
    if (!self.map) { return; }

    self.renderRequested = YES;
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
