//
//  TGMapViewController.mm
//  TangramMap
//
//  Created by Matt Blair on 8/25/14.
//  Updated by Matt Smollinger on 7/29/16.
//  Updated by Karim Naaji on 2/15/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGMapViewController+Internal.h"
#import "TGMapData+Internal.h"
#import "TGMarkerPickResult+Internal.h"
#import "TGLabelPickResult+Internal.h"
#import "TGMarker+Internal.h"
#import "TGHelpers.h"
#import "data/propertyItem.h"
#import "iosPlatform.h"
#import "map.h"

#import <unordered_map>
#import <functional>

__CG_STATIC_ASSERT(sizeof(TGGeoPoint) == sizeof(Tangram::LngLat));

@interface TGMapViewController () {
    BOOL shouldCaptureFrame;
    BOOL captureFrameWaitForViewComplete;
    BOOL viewComplete;
}

@property (nullable, strong, nonatomic) EAGLContext* context;
@property (assign, nonatomic) CGFloat contentScaleFactor;
@property (assign, nonatomic) BOOL renderRequested;
@property (strong, nonatomic) NSMutableDictionary* markersById;
@property (strong, nonatomic) NSMutableDictionary* dataLayersByName;

@end

@implementation TGMapViewController

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
    NSString* key = [NSString stringWithFormat:@"%d", (NSUInteger)marker.identifier];
    self.markersById[key] = marker;
    return marker;
}

- (void)markerRemove:(TGMarker *)marker
{
    NSString* key = [NSString stringWithFormat:@"%d", (NSUInteger)marker.identifier];
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

- (TGMapData *)addDataLayer:(NSString *)name
{
    return [self addDataLayer:name generateCentroid:false];
}

- (TGMapData *)addDataLayer:(NSString *)name generateCentroid:(bool)generateCentroid
{
    if (!self.map) { return nil; }

    std::string dataLayerName = std::string([name UTF8String]);
    auto source = std::make_shared<Tangram::ClientGeoJsonSource>(self.map->getPlatform(),
                    dataLayerName, "", generateCentroid);
    self.map->addTileSource(source);

    __weak TGMapViewController* weakSelf = self;
    TGMapData* clientData = [[TGMapData alloc] initWithMapView:weakSelf name:name source:source];
    self.dataLayersByName[name] = clientData;

    return clientData;
}

- (BOOL)removeDataSource:(std::shared_ptr<Tangram::TileSource>)tileSource name:(NSString *)name
{
    if (!self.map || !tileSource) { return; }

    [self.dataLayersByName removeObjectForKey:name];
    return self.map->removeTileSource(*tileSource);
}

- (void)clearDataSource:(std::shared_ptr<Tangram::TileSource>)tileSource
{
    if (!self.map || !tileSource) { return; }

    self.map->clearTileSource(*tileSource, true, true);
}

#pragma mark Scene loading interface

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
    __weak TGMapViewController* weakSelf = self;

    return [weakSelf](int sceneID, auto sceneError) {
        __strong TGMapViewController* strongSelf = weakSelf;

        if (!strongSelf) {
            return;
        }

        [strongSelf.markersById removeAllObjects];
        [strongSelf renderOnce];

        if (!strongSelf.mapViewDelegate || ![strongSelf.mapViewDelegate respondsToSelector:@selector(mapView:didLoadScene:withError:)]) {
            return;
        }

        NSError* error = nil;

        if (sceneError) {
            error = [TGHelpers errorFromSceneError:*sceneError];
        }

        [strongSelf.mapViewDelegate mapView:strongSelf didLoadScene:sceneID withError:error];
    };
}

- (int)loadSceneFromURL:(NSURL *)url
{
    return [self loadSceneFromURL:url withUpdates:nil];
}

- (int)loadSceneAsyncFromURL:(NSURL *)url
{
    return [self loadSceneAsyncFromURL:url withUpdates:nil];
}

- (int)loadSceneFromURL:(NSURL *)url withUpdates:(NSArray<TGSceneUpdate *> *)updates
{
    if (!self.map) { return -1; }

    auto sceneUpdates = unpackSceneUpdates(updates);

    self.map->setSceneReadyListener([self sceneReadyListener]);
    return self.map->loadScene([[url absoluteString] UTF8String], false, sceneUpdates);
}

- (int)loadSceneAsyncFromURL:(NSURL *)url withUpdates:(NSArray<TGSceneUpdate *> *)updates
{
    if (!self.map) { return -1; }

    auto sceneUpdates = unpackSceneUpdates(updates);

    self.map->setSceneReadyListener([self sceneReadyListener]);
    return self.map->loadSceneAsync([[url absoluteString] UTF8String], false, sceneUpdates);
}

- (int)loadSceneFromYAML:(NSString *)yaml relativeToURL:(NSURL *)url withUpdates:(NSArray<TGSceneUpdate *> *)updates
{
    if (!self.map) { return -1; }

    auto sceneUpdates = unpackSceneUpdates(updates);

    self.map->setSceneReadyListener([self sceneReadyListener]);
    return self.map->loadSceneYaml([yaml UTF8String], [[url absoluteString] UTF8String], false, sceneUpdates);
}

- (int)loadSceneAsyncFromYAML:(NSString *)yaml relativeToURL:(NSURL *)url withUpdates:(NSArray<TGSceneUpdate *> *)updates
{
    if (!self.map) { return -1; }

    auto sceneUpdates = unpackSceneUpdates(updates);

    self.map->setSceneReadyListener([self sceneReadyListener]);
    return self.map->loadSceneYamlAsync([yaml UTF8String], [[url absoluteString] UTF8String], false, sceneUpdates);
}

#pragma mark Scene updates

- (int)updateSceneAsync:(NSArray<TGSceneUpdate *> *)updates
{
    if (!self.map) { return -1; }

    if (!updates || ![updates count]) {
        return -1;
    }

    auto sceneUpdates = unpackSceneUpdates(updates);

    self.map->setSceneReadyListener([self sceneReadyListener]);
    return self.map->updateSceneAsync(sceneUpdates);
}

#pragma mark Longitude/Latitude - Screen position conversions

- (CGPoint)lngLatToScreenPosition:(TGGeoPoint)lngLat
{
    static const CGPoint nullCGPoint = {(CGFloat)NAN, (CGFloat)NAN};

    if (!self.map) { return nullCGPoint; }

    double screenPosition[2];
    if (self.map->lngLatToScreenPosition(lngLat.longitude, lngLat.latitude,
        &screenPosition[0], &screenPosition[1])) {

        screenPosition[0] /= self.contentScaleFactor;
        screenPosition[1] /= self.contentScaleFactor;

        return CGPointMake((CGFloat)screenPosition[0], (CGFloat)screenPosition[1]);
    }

    return nullCGPoint;
}

- (TGGeoPoint)screenPositionToLngLat:(CGPoint)screenPosition
{
    static const TGGeoPoint nullTangramGeoPoint = {NAN, NAN};

    if (!self.map) { return nullTangramGeoPoint; }

    screenPosition.x *= self.contentScaleFactor;
    screenPosition.y *= self.contentScaleFactor;

    TGGeoPoint lngLat;
    if (self.map->screenPositionToLngLat(screenPosition.x, screenPosition.y,
        &lngLat.longitude, &lngLat.latitude)) {
        return lngLat;
    }

    return nullTangramGeoPoint;
}

#pragma mark Feature picking

- (void)setPickRadius:(float)logicalPixels
{
    if (!self.map) { return; }

    self.map->setPickRadius(logicalPixels);
}

- (void)pickFeatureAt:(CGPoint)screenPosition
{
    if (!self.map) { return; }

    screenPosition.x *= self.contentScaleFactor;
    screenPosition.y *= self.contentScaleFactor;

    __weak TGMapViewController* weakSelf = self;
    self.map->pickFeatureAt(screenPosition.x, screenPosition.y, [weakSelf](const Tangram::FeaturePickResult* featureResult) {
        __strong TGMapViewController* strongSelf = weakSelf;

        if (!strongSelf || !strongSelf.mapViewDelegate || ![strongSelf.mapViewDelegate respondsToSelector:@selector(mapView:didSelectFeature:atScreenPosition:)]) {
            return;
        }

        CGPoint position = CGPointMake(0.0, 0.0);

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

- (void)pickMarkerAt:(CGPoint)screenPosition
{
    if (!self.map) { return; }

    screenPosition.x *= self.contentScaleFactor;
    screenPosition.y *= self.contentScaleFactor;

    __weak TGMapViewController* weakSelf = self;
    self.map->pickMarkerAt(screenPosition.x, screenPosition.y, [weakSelf](const Tangram::MarkerPickResult* markerPickResult) {
        __strong TGMapViewController* strongSelf = weakSelf;

        if (!strongSelf || !strongSelf.mapViewDelegate || ![strongSelf.mapViewDelegate respondsToSelector:@selector(mapView:didSelectMarker:atScreenPosition:)]) {
            return;
        }

        CGPoint position = CGPointMake(0.0, 0.0);

        if (!markerPickResult) {
            [strongSelf.mapViewDelegate mapView:strongSelf didSelectMarker:nil atScreenPosition:position];
            return;
        }

        NSString* key = [NSString stringWithFormat:@"%d", (NSUInteger)markerPickResult->id];
        TGMarker* marker = [strongSelf.markersById objectForKey:key];

        if (!marker) {
            [strongSelf.mapViewDelegate mapView:strongSelf didSelectMarker:nil atScreenPosition:position];
            return;
        }

        position = CGPointMake(markerPickResult->position[0] / strongSelf.contentScaleFactor,
                               markerPickResult->position[1] / strongSelf.contentScaleFactor);

        TGGeoPoint coordinates = TGGeoPointMake(markerPickResult->coordinates.longitude,
                                                markerPickResult->coordinates.latitude);

        TGMarkerPickResult* result = [[TGMarkerPickResult alloc] initWithCoordinates:coordinates marker:marker];
        [strongSelf.mapViewDelegate mapView:strongSelf didSelectMarker:result atScreenPosition:position];
    });
}

- (void)pickLabelAt:(CGPoint)screenPosition
{
    if (!self.map) { return; }

    screenPosition.x *= self.contentScaleFactor;
    screenPosition.y *= self.contentScaleFactor;

    __weak TGMapViewController* weakSelf = self;
    self.map->pickLabelAt(screenPosition.x, screenPosition.y, [weakSelf](const Tangram::LabelPickResult* labelPickResult) {
        __strong TGMapViewController* strongSelf = weakSelf;

        if (!strongSelf || !strongSelf.mapViewDelegate || ![strongSelf.mapViewDelegate respondsToSelector:@selector(mapView:didSelectLabel:atScreenPosition:)]) {
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

        TGGeoPoint coordinates = TGGeoPointMake(labelPickResult->coordinates.longitude, labelPickResult->coordinates.latitude);
        TGLabelPickResult* tgLabelPickResult = [[TGLabelPickResult alloc] initWithCoordinates:coordinates
                                                                                         type:(TGLabelType)labelPickResult->type
                                                                                   properties:featureProperties];
        [strongSelf.mapViewDelegate mapView:strongSelf didSelectLabel:tgLabelPickResult atScreenPosition:position];
    });
}

#pragma mark Map position implementation

- (void)setPosition:(TGGeoPoint)position {
    if (!self.map) { return; }

    self.map->setPosition(position.longitude, position.latitude);
}

- (void)animateToPosition:(TGGeoPoint)position withDuration:(float)seconds
{
    [self animateToPosition:position withDuration:seconds withEaseType:TGEaseTypeCubic];
}

- (void)animateToPosition:(TGGeoPoint)position withDuration:(float)seconds withEaseType:(TGEaseType)easeType
{
    if (!self.map) { return; }

    Tangram::EaseType ease = [TGHelpers convertEaseTypeFrom:easeType];
    self.map->setPositionEased(position.longitude, position.latitude, seconds, ease);
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

- (void)animateToZoomLevel:(float)zoomLevel withDuration:(float)seconds
{
    [self animateToZoomLevel:zoomLevel withDuration:seconds withEaseType:TGEaseTypeCubic];
}

- (void)animateToZoomLevel:(float)zoomLevel withDuration:(float)seconds withEaseType:(TGEaseType)easeType
{
    if (!self.map) { return; }

    Tangram::EaseType ease = [TGHelpers convertEaseTypeFrom:easeType];
    self.map->setZoomEased(zoomLevel, seconds, ease);
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

    Tangram::EaseType ease = [TGHelpers convertEaseTypeFrom:easeType];
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

    Tangram::EaseType ease = [TGHelpers convertEaseTypeFrom:easeType];
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
    if (!self.map) { return; }

    self.map->setCameraType(cameraType);
}

#pragma mark Gestures

- (void)setupGestureRecognizers
{
    _tapGestureRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(respondToTapGesture:)];
    _tapGestureRecognizer.numberOfTapsRequired = 1;
    // TODO: Figure a way to have a delay set for it not to tap gesture not to wait long enough for a doubletap gesture to be recognized
    _tapGestureRecognizer.delaysTouchesEnded = NO;

    _doubleTapGestureRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(respondToDoubleTapGesture:)];
    _doubleTapGestureRecognizer.numberOfTapsRequired = 2;
    // Distanle single tap when double tap occurs
    [_tapGestureRecognizer requireGestureRecognizerToFail:_doubleTapGestureRecognizer];

    _panGestureRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(respondToPanGesture:)];
    _panGestureRecognizer.maximumNumberOfTouches = 1;

    _pinchGestureRecognizer = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(respondToPinchGesture:)];
    _rotationGestureRecognizer = [[UIRotationGestureRecognizer alloc] initWithTarget:self action:@selector(respondToRotationGesture:)];
    _shoveGestureRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(respondToShoveGesture:)];
    _shoveGestureRecognizer.minimumNumberOfTouches = 2;
    _longPressGestureRecognizer = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(respondToLongPressGesture:)];

    // Use the delegate method 'shouldRecognizeSimultaneouslyWithGestureRecognizer' for gestures that can be concurrent
    _panGestureRecognizer.delegate = self;
    _pinchGestureRecognizer.delegate = self;
    _rotationGestureRecognizer.delegate = self;

    [self.view addGestureRecognizer:_tapGestureRecognizer];
    [self.view addGestureRecognizer:_doubleTapGestureRecognizer];
    [self.view addGestureRecognizer:_panGestureRecognizer];
    [self.view addGestureRecognizer:_pinchGestureRecognizer];
    [self.view addGestureRecognizer:_rotationGestureRecognizer];
    [self.view addGestureRecognizer:_shoveGestureRecognizer];
    [self.view addGestureRecognizer:_longPressGestureRecognizer];
}

- (void)setTapGestureRecognizer:(UITapGestureRecognizer *)recognizer
{
    if (!recognizer) { return; }
    if (_tapGestureRecognizer) {
        [self.view removeGestureRecognizer:_tapGestureRecognizer];
    }
    _tapGestureRecognizer = recognizer;
    [self.view addGestureRecognizer:_tapGestureRecognizer];
}

- (void)setDoubleTapGestureRecognizer:(UITapGestureRecognizer *)recognizer
{
    if (!recognizer) { return; }
    if (_doubleTapGestureRecognizer) {
        [self.view removeGestureRecognizer:_doubleTapGestureRecognizer];
    }
    _doubleTapGestureRecognizer = recognizer;
    [self.view addGestureRecognizer:_doubleTapGestureRecognizer];
}

- (void)setPanGestureRecognizer:(UIPanGestureRecognizer *)recognizer
{
    if (!recognizer) { return; }
    if (_panGestureRecognizer) {
        [self.view removeGestureRecognizer:_panGestureRecognizer];
    }
    _panGestureRecognizer = recognizer;
    [self.view addGestureRecognizer:_panGestureRecognizer];
}

- (void)setPinchGestureRecognizer:(UIPinchGestureRecognizer *)recognizer
{
    if (!recognizer) { return; }
    if (_pinchGestureRecognizer) {
        [self.view removeGestureRecognizer:_pinchGestureRecognizer];
    }
    _pinchGestureRecognizer = recognizer;
    [self.view addGestureRecognizer:_pinchGestureRecognizer];
}

- (void)setRotationGestureRecognizer:(UIRotationGestureRecognizer *)recognizer
{
    if (!recognizer) { return; }
    if (_rotationGestureRecognizer) {
        [self.view removeGestureRecognizer:_rotationGestureRecognizer];
    }
    _rotationGestureRecognizer = recognizer;
    [self.view addGestureRecognizer:_rotationGestureRecognizer];
}

- (void)setShoveGestureRecognizer:(UIPanGestureRecognizer *)recognizer
{
    if (!recognizer) { return; }
    if (_shoveGestureRecognizer) {
        [self.view removeGestureRecognizer:_shoveGestureRecognizer];
    }
    _shoveGestureRecognizer = recognizer;
    [self.view addGestureRecognizer:_shoveGestureRecognizer];
}

- (void)setLongPressGestureRecognizer:(UILongPressGestureRecognizer *)recognizer
{
    if (!recognizer) { return; }
    if (_longPressGestureRecognizer) {
        [self.view removeGestureRecognizer:_longPressGestureRecognizer];
    }
    _longPressGestureRecognizer = recognizer;
    [self.view addGestureRecognizer:_longPressGestureRecognizer];
}

- (SEL)respondToTapGestureAction
{
    return @selector(respondToTapGesture:);
}

- (SEL)respondToDoubleTapGestureAction
{
    return @selector(respondToDoubleTapGesture:);
}

- (SEL)respondToPanGestureAction
{
    return @selector(respondToPanGesture:);
}

- (SEL)respondToPinchGestureAction
{
    return @selector(respondToPinchGesture:);
}

- (SEL)respondToRotationGestureAction
{
    return @selector(respondToRotationGesture:);
}

- (SEL)respondToShoveGestureAction
{
    return @selector(respondToShoveGesture:);
}

- (SEL)respondToLongPressGestureAction
{
    return @selector(respondToLongPressGesture:);
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

#pragma mark - Gesture Recognizer Delegate Methods

- (void)respondToLongPressGesture:(UILongPressGestureRecognizer *)longPressRecognizer
{
    CGPoint location = [longPressRecognizer locationInView:self.view];
    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:shouldRecognizeLongPressGesture:)] ) {
        if (![self.gestureDelegate mapView:self recognizer:longPressRecognizer shouldRecognizeLongPressGesture:location]) { return; }
    }

    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizeLongPressGesture:)]) {
        [self.gestureDelegate mapView:self recognizer:longPressRecognizer didRecognizeLongPressGesture:location];
    }
}

- (void)respondToTapGesture:(UITapGestureRecognizer *)tapRecognizer
{
    CGPoint location = [tapRecognizer locationInView:self.view];
    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:shouldRecognizeSingleTapGesture:)]) {
        if (![self.gestureDelegate mapView:self recognizer:tapRecognizer shouldRecognizeSingleTapGesture:location]) { return; }
    }

    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizeSingleTapGesture:)]) {
        [self.gestureDelegate mapView:self recognizer:tapRecognizer didRecognizeSingleTapGesture:location];
    }
}

- (void)respondToDoubleTapGesture:(UITapGestureRecognizer *)doubleTapRecognizer
{
    CGPoint location = [doubleTapRecognizer locationInView:self.view];
    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:shouldRecognizeDoubleTapGesture:)]) {
        if (![self.gestureDelegate mapView:self recognizer:doubleTapRecognizer shouldRecognizeDoubleTapGesture:location]) { return; }
    }

    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizeDoubleTapGesture:)]) {
        [self.gestureDelegate mapView:self recognizer:doubleTapRecognizer didRecognizeDoubleTapGesture:location];
    }
}

- (void)respondToPanGesture:(UIPanGestureRecognizer *)panRecognizer
{
    CGPoint displacement = [panRecognizer translationInView:self.view];

    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:shouldRecognizePanGesture:)]) {
        if (![self.gestureDelegate mapView:self recognizer:panRecognizer shouldRecognizePanGesture:displacement]) {
            return;
        }
    }

    CGPoint velocity = [panRecognizer velocityInView:self.view];
    CGPoint end = [panRecognizer locationInView:self.view];
    CGPoint start = {end.x - displacement.x, end.y - displacement.y};

    [panRecognizer setTranslation:CGPointZero inView:self.view];

    switch (panRecognizer.state) {
        case UIGestureRecognizerStateChanged:
            self.map->handlePanGesture(start.x * self.contentScaleFactor, start.y * self.contentScaleFactor, end.x * self.contentScaleFactor, end.y * self.contentScaleFactor);
            break;
        case UIGestureRecognizerStateEnded:
            self.map->handleFlingGesture(end.x * self.contentScaleFactor, end.y * self.contentScaleFactor, velocity.x * self.contentScaleFactor, velocity.y * self.contentScaleFactor);
            break;
        default:
            break;
    }

    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizePanGesture:)]) {
        [self.gestureDelegate mapView:self recognizer:panRecognizer didRecognizePanGesture:displacement];
    }
}

- (void)respondToPinchGesture:(UIPinchGestureRecognizer *)pinchRecognizer
{
    CGPoint location = [pinchRecognizer locationInView:self.view];
    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:shouldRecognizePinchGesture:)]) {
        if (![self.gestureDelegate mapView:self recognizer:pinchRecognizer shouldRecognizePinchGesture:location]) {
            return;
        }
    }

    CGFloat scale = pinchRecognizer.scale;
    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(pinchFocus:recognizer:)]) {
        CGPoint focusPosition = [self.gestureDelegate pinchFocus:self recognizer:pinchRecognizer];
        self.map->handlePinchGesture(focusPosition.x * self.contentScaleFactor, focusPosition.y * self.contentScaleFactor, scale, pinchRecognizer.velocity);
    } else {
        self.map->handlePinchGesture(location.x * self.contentScaleFactor, location.y * self.contentScaleFactor, scale, pinchRecognizer.velocity);
    }

    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizePinchGesture:)]) {
        [self.gestureDelegate mapView:self recognizer:pinchRecognizer didRecognizePinchGesture:location];
    }

    [pinchRecognizer setScale:1.0];
}

- (void)respondToRotationGesture:(UIRotationGestureRecognizer *)rotationRecognizer
{
    CGPoint position = [rotationRecognizer locationInView:self.view];
    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:shouldRecognizeRotationGesture:)]) {
        if (![self.gestureDelegate mapView:self recognizer:rotationRecognizer shouldRecognizeRotationGesture:position]) {
            return;
        }
    }

    CGFloat rotation = rotationRecognizer.rotation;
    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(rotationFocus:recognizer:)]) {
        CGPoint focusPosition = [self.gestureDelegate rotationFocus:self recognizer:rotationRecognizer];
        self.map->handleRotateGesture(focusPosition.x * self.contentScaleFactor, focusPosition.y * self.contentScaleFactor, rotation);
    } else {
        self.map->handleRotateGesture(position.x * self.contentScaleFactor, position.y * self.contentScaleFactor, rotation);
    }

    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(mapView:recognizer:didRecognizeRotationGesture:)]) {
        [self.gestureDelegate mapView:self recognizer:rotationRecognizer didRecognizeRotationGesture:position];
    }

    [rotationRecognizer setRotation:0.0];
}

- (void)respondToShoveGesture:(UIPanGestureRecognizer *)shoveRecognizer
{
    CGPoint displacement = [shoveRecognizer translationInView:self.view];
    [shoveRecognizer setTranslation:{0, 0} inView:self.view];

    if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(recognizer:shouldRecognizeShoveGesture:)]) {
        if (![self.gestureDelegate mapView:self recognizer:shoveRecognizer shouldRecognizeShoveGesture:displacement]) {
            return;
        }
    }

    // don't trigger shove on single touch gesture
    if ([shoveRecognizer numberOfTouches] == 2) {
        self.map->handleShoveGesture(displacement.y);

        if (self.gestureDelegate && [self.gestureDelegate respondsToSelector:@selector(recognizer:didRecognizeShoveGesture:)]) {
            [self.gestureDelegate mapView:self recognizer:shoveRecognizer didRecognizeShoveGesture:displacement];
        }
    }
}
#pragma mark Standard Initializer

- (instancetype)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self != nil) {
        __weak TGMapViewController* weakSelf = self;
        std::shared_ptr<Tangram::Platform> platform(new Tangram::iOSPlatform(weakSelf));
        self.map = new Tangram::Map(platform);
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self != nil) {
        __weak TGMapViewController* weakSelf = self;
        std::shared_ptr<Tangram::Platform> platform(new Tangram::iOSPlatform(weakSelf));
        self.map = new Tangram::Map(platform);
    }
    return self;
}

#pragma mark Map view lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }

    self->viewComplete = NO;
    self->captureFrameWaitForViewComplete = YES;
    self->shouldCaptureFrame = NO;
    self.renderRequested = YES;
    self.continuous = NO;
    self.markersById = [[NSMutableDictionary alloc] init];
    self.dataLayersByName = [[NSMutableDictionary alloc] init];
    self.resourceRoot = [[NSBundle mainBundle] resourceURL];

    if (!self.httpHandler) {
        self.httpHandler = [[TGHttpHandler alloc] initWithCachePath:@"/tangram_cache"
                                                cacheMemoryCapacity:4*1024*1024
                                                  cacheDiskCapacity:30*1024*1024];
    }

    GLKView* view = (GLKView *)self.view;
    view.context = self.context;

    view.drawableColorFormat = GLKViewDrawableColorFormatRGBA8888;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    view.drawableStencilFormat = GLKViewDrawableStencilFormat8;
    view.drawableMultisample = GLKViewDrawableMultisampleNone;

    self.contentScaleFactor = view.contentScaleFactor;

    [self setupGestureRecognizers];
    [self setupGL];
}

- (void)dealloc
{
    if (self.map) {
        delete self.map;
    }

    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];

    if (self.map) {
        self.map->onMemoryWarning();
    }
}

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];

    self.map->setupGL();
    self.map->setPixelScale(self.contentScaleFactor);

    // Query background color set in UI designer
    GLKView* view = (GLKView *)self.view;

    self.preferredFramesPerSecond = 60;

    UIColor* backgroundColor = view.backgroundColor;

    if (backgroundColor != nil) {
        CGFloat red = 0.0, green = 0.0, blue = 0.0, alpha = 0.0;
        [backgroundColor getRed:&red green:&green blue:&blue alpha:&alpha];
        self.map->setDefaultBackgroundColor(red, green, blue);
    }
}

-(void)viewWillLayoutSubviews {
    [super viewWillLayoutSubviews];
    int width = self.view.bounds.size.width;
    int height = self.view.bounds.size.height;
    self.map->resize(width * self.contentScaleFactor, height * self.contentScaleFactor);
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
    self.map->resize(size.width * self.contentScaleFactor, size.height * self.contentScaleFactor);

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

- (void)captureScreenshot:(BOOL)waitForViewComplete
{
    self->captureFrameWaitForViewComplete = waitForViewComplete;
    self->shouldCaptureFrame = YES;
}

- (void)update
{
    self->viewComplete = self.map->update([self timeSinceLastUpdate]);

    if (viewComplete && [self.mapViewDelegate respondsToSelector:@selector(mapViewDidCompleteLoading:)]) {
        [self.mapViewDelegate mapViewDidCompleteLoading:self];
    }

    if (!self.continuous && !self.renderRequested) {
        self.paused = YES;
    }

    self.renderRequested = NO;
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    self.map->render();

    if (self.mapViewDelegate && [self.mapViewDelegate respondsToSelector:@selector(mapView:didCaptureScreenshot:)]) {
        if (self->shouldCaptureFrame && (!self->captureFrameWaitForViewComplete || self->viewComplete)) {
            UIGraphicsBeginImageContext(self.view.frame.size);
            [self.view drawViewHierarchyInRect:self.view.frame afterScreenUpdates:YES];
            UIImage* screenshot = UIGraphicsGetImageFromCurrentImageContext();
            UIGraphicsEndImageContext();

            [self.mapViewDelegate mapView:self didCaptureScreenshot:screenshot];

            self->shouldCaptureFrame = NO;
        }
    }
}

@end
