//
//  TGMapViewController.h
//  TangramiOS
//
//  Created by Matt Blair on 8/25/14.
//  Updated by Matt Smollinger on 7/29/16.
//  Copyright (c) 2016 Mapzen. All rights reserved.
//

#include "tangram.h"

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

typedef NS_ENUM(NSInteger, TangramCameraType) {
  TangramCameraTypePerspective = 0,
  TangramCameraTypeIsometric,
  TangramCameraTypeFlat
};

typedef NS_ENUM(NSInteger, TangramEaseType){
    TangramEaseTypeLinear = 0,
    TangramEaseTypeCubic,
    TangramEaseTypeQuint,
    TangramEaseTypeSine
};

typedef struct {
    double longitude;
    double latitude;
} TangramGeoPoint;

NS_ASSUME_NONNULL_BEGIN

@protocol TangramGestureRecognizerDelegate <NSObject>
@optional
- (void)recognizer:(UIGestureRecognizer *)recognizer didRecognizeSingleTap:(CGPoint)location;
- (void)recognizer:(UIGestureRecognizer *)recognizer didRecognizeDoubleTap:(CGPoint)location;
- (void)recognizer:(UIGestureRecognizer *)recognizer didRecognizePanGesture:(CGPoint)location;
- (void)recognizer:(UIGestureRecognizer *)recognizer didRecognizePinchGesture:(CGPoint)location;
- (void)recognizer:(UIGestureRecognizer *)recognizer didRecognizeRotationGesture:(CGPoint)location;
- (void)recognizer:(UIGestureRecognizer *)recognizer didRecognizeShoveGesture:(CGPoint)location;
@end

NS_ASSUME_NONNULL_END

struct TileID;
@interface TGMapViewController : GLKViewController <UIGestureRecognizerDelegate>

@property (assign, nonatomic, nullable) Tangram::Map* map;
@property (assign, nonatomic) BOOL continuous;
@property (weak, nonatomic, nullable) id<TangramGestureRecognizerDelegate> gestureDelegate;

// The following are computed property. They return sensible defaults when the above .map property is nil
@property (assign, nonatomic) TangramCameraType cameraType;
@property (assign, nonatomic) TangramGeoPoint position;
@property (assign, nonatomic) float zoom;
@property (assign, nonatomic) float rotation;
@property (assign, nonatomic) float tilt;

- (void)renderOnce;

- (void)animateToPosition:(TangramGeoPoint)position withDuration:(float)duration;

- (void)animateToPosition:(TangramGeoPoint)position withDuration:(float)duration withEaseType:(TangramEaseType)easeType;

- (void)animateToZoomLevel:(float)zoomLevel withDuration:(float)duration;

- (void)animateToZoomLevel:(float)zoomLevel withDuration:(float)duration withEaseType:(TangramEaseType)easeType;

- (void)animateRotation:(float)rotation withDuration:(float)duration;

- (void)animateRotation:(float)rotation withDuration:(float)duration withEaseType:(TangramEaseType)easeType;

- (void)animateCameraTilt:(float)radians withDuration:(float)duration;

- (void)animateCameraTilt:(float)radians withDuration:(float)duration withEaseType:(TangramEaseType)easeType;

@end
