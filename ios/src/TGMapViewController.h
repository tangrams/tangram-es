//
//  TGMapViewController.h
//  TangramiOS
//
//  Created by Matt Blair on 8/25/14.
//  Updated by Matt Smollinger on 7/29/16.
//  Copyright (c) 2016 Mapzen. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

@class TGMapViewController;

#import "TGGeoPoint.h"
#import "TGGeoPolygon.h"
#import "TGGeoPolyline.h"

typedef NS_ENUM(NSInteger, TGCameraType) {
    TGCameraTypePerspective = 0,
    TGCameraTypeIsometric,
    TGCameraTypeFlat
};

typedef NS_ENUM(NSInteger, TGEaseType) {
    TGEaseTypeLinear = 0,
    TGEaseTypeCubic,
    TGEaseTypeQuint,
    TGEaseTypeSine
};

typedef uint32_t TGMapMarkerId;

NS_ASSUME_NONNULL_BEGIN

@protocol TGRecognizerDelegate <NSObject>
@optional
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeSingleTapGesture:(CGPoint)location;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeDoubleTapGesture:(CGPoint)location;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeLongPressGesture:(CGPoint)location;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizePanGesture:(CGPoint)location;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizePinchGesture:(CGPoint)location;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeRotationGesture:(CGPoint)location;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeShoveGesture:(CGPoint)location;
@end

@protocol TGMapViewDelegate <NSObject>
@optional
- (void)mapView:(TGMapViewController*)mapView didLoadSceneAsync:(NSString*)scene;
- (void)mapView:(TGMapViewController*)mapView didSelectFeatures:(NSDictionary*)features atScreenPosition:(CGPoint)position;
@end

NS_ASSUME_NONNULL_END

@interface TGMapViewController : GLKViewController <UIGestureRecognizerDelegate>

@property (assign, nonatomic) BOOL continuous;
@property (weak, nonatomic, nullable) id<TGRecognizerDelegate> gestureDelegate;
@property (weak, nonatomic, nullable) id<TGMapViewDelegate> mapViewDelegate;

// The following are computed property. They return sensible defaults when the above .map property is nil
@property (assign, nonatomic) TGCameraType cameraType;
@property (assign, nonatomic) TGGeoPoint position;
@property (assign, nonatomic) float zoom;
@property (assign, nonatomic) float rotation;
@property (assign, nonatomic) float tilt;

NS_ASSUME_NONNULL_BEGIN

#pragma mark Marker interface

- (void)markerRemoveAll;

- (TGMapMarkerId)markerAdd;

- (BOOL)markerSetStyling:(TGMapMarkerId)identifier styling:(NSString *)styling;

- (BOOL)markerSetPoint:(TGMapMarkerId)identifier coordinates:(TGGeoPoint)coordinate;

- (BOOL)markerSetPointEased:(TGMapMarkerId)identifier coordinates:(TGGeoPoint)coordinate duration:(float)duration easeType:(TGEaseType)ease;

- (BOOL)markerSetPolyline:(TGMapMarkerId)identifier polyline:(TGGeoPolyline *)polyline;

- (BOOL)markerSetPolygon:(TGMapMarkerId)identifier polygon:(TGGeoPolygon *)polygon;

- (BOOL)markerSetVisible:(TGMapMarkerId)identifier visible:(BOOL)visible;

- (BOOL)markerRemove:(TGMapMarkerId)marker;

#pragma mark Scene loading - updates interface

- (void)loadSceneFile:(NSString*)path;

- (void)loadSceneFileAsync:(NSString*)path;

- (void)queueSceneUpdate:(NSString*)componentPath withValue:(NSString*)value;

- (void)applySceneUpdates;

#pragma mark Feature picking interface

- (void)pickFeaturesAt:(CGPoint)screenPosition;

#pragma mark Map View lifecycle

- (void)requestRender;

- (void)renderOnce;

#pragma mark Longitude/Latitude - Screen position conversions

- (CGPoint)lngLatToScreenPosition:(TGGeoPoint)lngLat;

- (TGGeoPoint)screenPositionToLngLat:(CGPoint)screenPosition;

#pragma mark Map View animations - Position interface

- (void)animateToPosition:(TGGeoPoint)position withDuration:(float)seconds;

- (void)animateToPosition:(TGGeoPoint)position withDuration:(float)seconds withEaseType:(TGEaseType)easeType;

- (void)animateToZoomLevel:(float)zoomLevel withDuration:(float)seconds;

- (void)animateToZoomLevel:(float)zoomLevel withDuration:(float)seconds withEaseType:(TGEaseType)easeType;

- (void)animateToRotation:(float)radians withDuration:(float)seconds;

- (void)animateToRotation:(float)radians withDuration:(float)seconds withEaseType:(TGEaseType)easeType;

- (void)animateToTilt:(float)radians withDuration:(float)seconds;

- (void)animateToTilt:(float)radians withDuration:(float)seconds withEaseType:(TGEaseType)easeType;

NS_ASSUME_NONNULL_END

@end
