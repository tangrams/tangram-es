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
#import "TGSceneUpdate.h"
#import "TGHttpHandler.h"
#import "TGLabelPickResult.h"
#import "TGMarkerPickResult.h"

typedef NS_ENUM(NSInteger, TGCameraType) {
    TGCameraTypePerspective = 0,
    TGCameraTypeIsometric,
    TGCameraTypeFlat,
};

typedef NS_ENUM(NSInteger, TGEaseType) {
    TGEaseTypeLinear = 0,
    TGEaseTypeCubic,
    TGEaseTypeQuint,
    TGEaseTypeSine,
};

typedef NS_ENUM(NSInteger, TGDebugFlag) {
    TGDebugFlagFreezeTiles = 0,  // While on, the set of tiles currently being drawn will not update to match the view
    TGDebugFlagProxyColors,      // Apply a color change to every other zoom level to visualize proxy tile behavior
    TGDebugFlagTileBounds,       // Draw tile boundaries
    TGDebugFlagTileInfos,        // Draw tile infos (tile coordinates)
    TGDebugFlagLabels,           // Draw label bounding boxes and collision grid
    TGDebugFlagTangramInfos,     // Draw tangram infos (framerate, debug log...)
    TGDebugFlagDrawAllLabels,    // Draw all labels (including labels being occluded)
    TGDebugFlagTangramStats,     // Draw tangram frame graph stats
    TGDebugFlagSelectionBuffer,  // Draw feature selection framebuffer
};


NS_ASSUME_NONNULL_BEGIN

@protocol TGRecognizerDelegate <NSObject>
@optional
- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizeSingleTapGesture:(CGPoint)location;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeSingleTapGesture:(CGPoint)location;

- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizeDoubleTapGesture:(CGPoint)location;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeDoubleTapGesture:(CGPoint)location;

- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizeLongPressGesture:(CGPoint)location;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeLongPressGesture:(CGPoint)location;

- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizePanGesture:(CGPoint)displacement;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizePanGesture:(CGPoint)displacement;

- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizePinchGesture:(CGPoint)location;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizePinchGesture:(CGPoint)location;

- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizeRotationGesture:(CGPoint)location;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeRotationGesture:(CGPoint)location;

- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizeShoveGesture:(CGPoint)displacement;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeShoveGesture:(CGPoint)displacement;
@end

NS_ASSUME_NONNULL_END

@protocol TGMapViewDelegate <NSObject>
@optional
- (void)mapView:(nonnull TGMapViewController *)mapView didLoadSceneAsync:(nonnull NSString *)scene;
- (void)mapView:(nonnull TGMapViewController *)mapView didSelectFeature:(nullable NSDictionary *)feature atScreenPosition:(CGPoint)position;
- (void)mapView:(nonnull TGMapViewController *)mapView didSelectLabel:(nullable TGLabelPickResult *)labelPickResult atScreenPosition:(CGPoint)position;
- (void)mapView:(nonnull TGMapViewController *)mapView didSelectMarker:(nullable TGMarkerPickResult *)markerPickResult atScreenPosition:(CGPoint)position;
- (void)mapViewDidCompleteLoading:(nonnull TGMapViewController *)mapView;
@end

NS_ASSUME_NONNULL_BEGIN

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
@property (strong, nonatomic) TGHttpHandler* httpHandler;

- (void)setDebugFlag:(TGDebugFlag)debugFlag value:(BOOL)on;

- (BOOL)getDebugFlag:(TGDebugFlag)debugFlag;

- (void)toggleDebugFlag:(TGDebugFlag)debugFlag;

#pragma mark Marker interface

- (void)markerRemoveAll;

- (TGMapMarkerId)markerAdd;

- (BOOL)markerSetStyling:(TGMapMarkerId)identifier styling:(NSString *)styling;

- (BOOL)markerSetPoint:(TGMapMarkerId)identifier coordinates:(TGGeoPoint)coordinate;

- (BOOL)markerSetPointEased:(TGMapMarkerId)identifier coordinates:(TGGeoPoint)coordinate duration:(float)duration easeType:(TGEaseType)ease;

- (BOOL)markerSetPolyline:(TGMapMarkerId)identifier polyline:(TGGeoPolyline *)polyline;

- (BOOL)markerSetPolygon:(TGMapMarkerId)identifier polygon:(TGGeoPolygon *)polygon;

- (BOOL)markerSetVisible:(TGMapMarkerId)identifier visible:(BOOL)visible;

- (BOOL)markerSetImage:(TGMapMarkerId)identifier image:(UIImage *)image;

- (BOOL)markerRemove:(TGMapMarkerId)marker;

#pragma mark Scene loading - updates interface

- (void)loadSceneFile:(NSString *)path;

- (void)loadSceneFile:(NSString *)path sceneUpdates:(NSArray<TGSceneUpdate *> *)sceneUpdates;

- (void)loadSceneFileAsync:(NSString *)path;

- (void)loadSceneFileAsync:(NSString *)path sceneUpdates:(NSArray<TGSceneUpdate *> *)sceneUpdates;

- (void)queueSceneUpdate:(NSString*)componentPath withValue:(NSString*)value;

- (void)queueSceneUpdates:(NSArray<TGSceneUpdate *> *)sceneUpdates;

- (void)applySceneUpdates;

#pragma mark Feature picking interface

- (void)pickFeatureAt:(CGPoint)screenPosition;

- (void)pickLabelAt:(CGPoint)screenPosition;

#pragma mark Map View lifecycle

- (void)requestRender;

- (void)renderOnce;

- (void)update;

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
