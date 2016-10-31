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
#import "TGMapViewDelegate.h"

typedef NS_ENUM(NSInteger, TGCameraType) {
    TGCameraTypePerspective = 0,
    TGCameraTypeIsometric,
    TGCameraTypeFlat
};

typedef NS_ENUM(NSInteger, TGEaseType){
    TGEaseTypeLinear = 0,
    TGEaseTypeCubic,
    TGEaseTypeQuint,
    TGEaseTypeSine
};

typedef struct {
    double longitude;
    double latitude;
} TGGeoPoint;

NS_ASSUME_NONNULL_BEGIN

@protocol TGRecognizerDelegate <NSObject>
@optional
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeSingleTap:(CGPoint)location;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeDoubleTap:(CGPoint)location;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizePanGesture:(CGPoint)location;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizePinchGesture:(CGPoint)location;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeRotationGesture:(CGPoint)location;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeShoveGesture:(CGPoint)location;
@end

NS_ASSUME_NONNULL_END

struct TileID;
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

- (void)renderOnce;

NS_ASSUME_NONNULL_BEGIN

- (void)loadSceneFile:(NSString*)path;

- (void)loadSceneFileAsync:(NSString*)path;

- (void)queueSceneUpdate:(NSString*)componentPath withValue:(NSString*)value;

NS_ASSUME_NONNULL_END

- (void)pickFeaturesAt:(CGPoint)screenPosition;

- (void)requestRender;

- (CGPoint)lngLatToScreenPosition:(TGGeoPoint)lngLat;

- (TGGeoPoint)screenPositionToLngLat:(CGPoint)screenPosition;

- (void)animateToPosition:(TGGeoPoint)position withDuration:(float)seconds;

- (void)animateToPosition:(TGGeoPoint)position withDuration:(float)seconds withEaseType:(TGEaseType)easeType;

- (void)animateToZoomLevel:(float)zoomLevel withDuration:(float)seconds;

- (void)animateToZoomLevel:(float)zoomLevel withDuration:(float)seconds withEaseType:(TGEaseType)easeType;

- (void)animateToRotation:(float)radians withDuration:(float)seconds;

- (void)animateToRotation:(float)radians withDuration:(float)seconds withEaseType:(TGEaseType)easeType;

- (void)animateToTilt:(float)radians withDuration:(float)seconds;

- (void)animateToTilt:(float)radians withDuration:(float)seconds withEaseType:(TGEaseType)easeType;

@end
