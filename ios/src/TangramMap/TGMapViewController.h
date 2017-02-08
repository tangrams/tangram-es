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

/**
 Description for a <a href="https://mapzen.com/documentation/tangram/Cameras-Overview/#camera-types">
 Tangram camera type</a>.
 */
typedef NS_ENUM(NSInteger, TGCameraType) {
    /// Type for a <a href="https://mapzen.com/documentation/tangram/Cameras-Overview/#perspective-camera">perspective camera</a>
    TGCameraTypePerspective = 0,
    /// Type for an <a href="https://mapzen.com/documentation/tangram/Cameras-Overview/#isometric-camera">isometric camera</a>
    TGCameraTypeIsometric,
    /// Type for a <a href="https://mapzen.com/documentation/tangram/Cameras-Overview/#flat-camera">flat camera</a>
    TGCameraTypeFlat,
};

/**
 These defines describe an ease type function to be used for camera or other transition animation.
 The function is being used to interpolate between the start and end position of the animation.
 */
typedef NS_ENUM(NSInteger, TGEaseType) {
    /// Linear ease type `f(t) = t`
    TGEaseTypeLinear = 0,
    /// Cube ease type `f(t) = (-2 * t + 3) * t * t`
    TGEaseTypeCubic,
    /// Quint ease type `f(t) = (6 * t * t - 15 * t + 10) * t * t * t`
    TGEaseTypeQuint,
    /// Sine ease type `f(t) = 0.5 - 0.5 * cos(PI * t)`
    TGEaseTypeSine,
};

/// Debug flags to render additional information about various map components
typedef NS_ENUM(NSInteger, TGDebugFlag) {
    /// While on, the set of tiles currently being drawn will not update to match the view
    TGDebugFlagFreezeTiles = 0,
    /// Apply a color change to every other zoom level to visualize proxy tile behavior
    TGDebugFlagProxyColors,
    /// Draw tile boundaries
    TGDebugFlagTileBounds,
    /// Draw tile infos (tile coordinates)
    TGDebugFlagTileInfos,
    /// Draw label bounding boxes and collision grid
    TGDebugFlagLabels,
    /// Draw tangram infos (framerate, debug log...)
    TGDebugFlagTangramInfos,
    /// Draw all labels (including labels being occluded)
    TGDebugFlagDrawAllLabels,
    /// Draw tangram frame graph stats
    TGDebugFlagTangramStats,
    /// Draw feature selection framebuffer
    TGDebugFlagSelectionBuffer,
};


NS_ASSUME_NONNULL_BEGIN

/**
 A gesture recognizer delegate that can be implemented to receive gesture events from the map view.
 The map view will first check whether a gestureDelegate is available, then check whether it responds
 to any `shouldRecognize*` method:

    - If the `TGRecognizerDelegate` responds to `shouldRecognize`, the map view calls it before proceeding with
    its default implementation based on the resturn status of `shouldRecognize`.

    - If the `TGRecognizerDelegate` doesn't respond to `shouldRecognize*`, the map view will proceed with
    calling default gesture handling code.

 Finally, the map view checks whether the `TGRecognizerDelegate` implements `didRecognize*` and call this
 method if so.

 @note Those methods are all **optional**.
 */
@protocol TGRecognizerDelegate <NSObject>
@optional
/**
 Whether the map view should handle a single tap gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 @return Whether the map view should proceed by handling this gesture behavior
 */
- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizeSingleTapGesture:(CGPoint)location;
/**
 Called when the map view just handled a single tap gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 */
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeSingleTapGesture:(CGPoint)location;
/**
 Whether the map view should handle a double tap gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 @return Whether the map view should proceed by handling this gesture behavior
 */
- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizeDoubleTapGesture:(CGPoint)location;
/**
 Called when the map view just handled a single double gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 */
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeDoubleTapGesture:(CGPoint)location;
/**
 Whether the map view should handle a long press gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 @return Whether the map view should proceed by handling this gesture behavior
 */
- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizeLongPressGesture:(CGPoint)location;
/**
 Called when the map view just handled a long press gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 */
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeLongPressGesture:(CGPoint)location;
/**
 Whether the map view should handle a pan gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param displacement the logical pixel displacement of the recognized gesture
 @return Whether the map view should proceed by handling this gesture behavior
 */
- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizePanGesture:(CGPoint)displacement;
/**
 Called when the map view just handled a pan gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param displacement the logical pixel displacement of the recognized gesture
 */
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizePanGesture:(CGPoint)displacement;
/**
 Whether the map view should handle a pinch gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 @return Whether the map view should proceed by handling this gesture behavior
 */
- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizePinchGesture:(CGPoint)location;
/**
 Called when the map view just handled a pinch gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 */
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizePinchGesture:(CGPoint)location;
/**
 Whether the map view should handle a rotation gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 @return Whether the map view should proceed by handling this gesture behavior
 */
- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizeRotationGesture:(CGPoint)location;
/**
 Called when the map view just handled a rotation gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 */
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeRotationGesture:(CGPoint)location;
/**
 Whether the map view should handle a shove gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param displacement the logical pixel displacement of the recognized gesture
 @return Whether the map view should proceed by handling this gesture behavior
 */
- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizeShoveGesture:(CGPoint)displacement;
/**
 Called when the map view just handled a shove gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param displacement the logical pixel displacement of the recognized gesture
 */
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeShoveGesture:(CGPoint)displacement;
@end

NS_ASSUME_NONNULL_END

/**
 A map view delegate that can be implemented to receive miscellaneous map event.

 @note All of those methods are called from main thread, those methods are all **optional**.
 */
@protocol TGMapViewDelegate <NSObject>
@optional
/**
 Called after a `-[TGMapViewController loadSceneFileAsync:]` or
 `-[TGMapViewController loadSceneFileAsync:sceneUpdates:]` is completed.

 @param mapView a pointer to the map view
 @param scene the path to the scene that has been loaded
 */
- (void)mapView:(nonnull TGMapViewController *)mapView didLoadSceneAsync:(nonnull NSString *)scene;
/**
 Always called after the method `-[TGMapViewController pickFeatureAt:]` is called on the map view.

 @param mapView a pointer to the map view
 @param feature a dictionnary containing the data contained by the selected feature, may be `nil`
 @param position the screen position at which the feature has been selected

 @note If no feature have been selected, `feature` will be `nil`, and `position` `(0, 0)`.
 */
- (void)mapView:(nonnull TGMapViewController *)mapView didSelectFeature:(nullable NSDictionary *)feature atScreenPosition:(CGPoint)position;
 /**
 Always called after the method `-[TGMapViewController pickLabelAt:]` is called on the map view.

 @param mapView a pointer to the map view
 @param labelPickResult the label pick result object containing information about the label being picked, may be `nil`
 @param position the screen position at which the feature has been selected

 @note If no feature have been selected, `feature` will be `nil`, and `position` `(0, 0)`.
 */
- (void)mapView:(nonnull TGMapViewController *)mapView didSelectLabel:(nullable TGLabelPickResult *)labelPickResult atScreenPosition:(CGPoint)position;
/**
 Always called after the method `-[TGMapViewController pickMarkerAt:]` is called on the map view.

 @param mapView a pointer to the map view
 @param markerPickResult the marker pick result object containing information about the marker being picked, may be `nil`
 @param position the screen position at which the feature has been selected

 @note If no feature have been selected, `feature` will be `nil`, and `position` `(0, 0)`.
 */
- (void)mapView:(nonnull TGMapViewController *)mapView didSelectMarker:(nullable TGMarkerPickResult *)markerPickResult atScreenPosition:(CGPoint)position;
/**
 Called whenever the view did complete loading and building of all of the visible tiles of the current view.

 @param mapView a pointer to the map view
 */
- (void)mapViewDidCompleteLoading:(nonnull TGMapViewController *)mapView;
@end

NS_ASSUME_NONNULL_BEGIN

/**
 `TGMapViewController` is a flexible and customizable map view, allowing you to embed
 it inside a <a href="https://developer.apple.com/reference/glkit/glkview">GLKView</a> component.
 This view provides default gesture handlers for tap, double tap, long press, pan,
 pinch, rotate and shove.

 The public interface provides dynamic map marker placement, change of camera view
 settings, and map description changes through scene updates.

 This view uses scenes descibed by the
  <a href="https://mapzen.com/documentation/tangram/">Tangram YAML scene syntax</a>
 allowing you to fully customize your map description using your own data.
 Some already made basemap styles can be found
 <a href="https://mapzen.com/documentation/cartography/styles/">here</a> using Mapzen
 data sources.

 To use basemap styles you can <a href="https://mapzen.com/developers/sign_in">sign up for
 an API key</a> and load it through your application:

 ```swift
 let sceneURL = "https://mapzen.com/carto/walkabout-style-more-labels/walkabout-style-more-labels.yaml";
 view.loadSceneFile(sceneURL, sceneUpdates: [ TGSceneUpdate(path: "sources.mapzen.url_params", value: "{ api_key: \(YOUR_API_KEY) }") ]);
 ```
 */
@interface TGMapViewController : GLKViewController <UIGestureRecognizerDelegate>

/**
 If continuous rendering is set to `true`, the map view will render continuously, otherwise,
 the map will render only when necessary.
 Some styles can be set to be `animated` and this default value will be set appropriately,
 see the <a href="https://mapzen.com/documentation/tangram/scene/#animated">
 style parameter</a> for more details.

 @note Any changes to this value will override the default induced value from the `animated`
 style parameter.
 Before making changes to this value, make sure battery usage is not something critical
 for your application.
 */
@property (assign, nonatomic) BOOL continuous;

#pragma mark Delegates

/// Assign a gesture recognizer delegate, may be `nil`, see `TGRecognizerDelegate` for more details
@property (weak, nonatomic, nullable) id<TGRecognizerDelegate> gestureDelegate;
/// Assign a map viw delegate, may be `nil`, see `TGMapViewDelegate` for more details
@property (weak, nonatomic, nullable) id<TGMapViewDelegate> mapViewDelegate;

#pragma mark Camera/View modifiers

/* The following are computed property. They return sensible defaults when the above .map property is nil */

/// Assign a `TGCameraType` to the view camera
@property (assign, nonatomic) TGCameraType cameraType;
/// Assign a longitude and latitude to the map view camera
@property (assign, nonatomic) TGGeoPoint position;
/// Assign a floating point zoom to the map view camera
@property (assign, nonatomic) float zoom;
/// Assign a rotation angle in radians to the map view camera
@property (assign, nonatomic) float rotation;
/// Assign a tilt angle in radians to the map view camera
@property (assign, nonatomic) float tilt;

/**
 Assign `TGHttpHandler` for network request management.
 A http handler with a default configuration is already provided to the map view.

 Must be non-`nil`.

 @note Assigning a http handler is optional and should only be done if you want to change
 any network related configuration (cache path and network cache size, both memory and disk,
 or be notified when a network request completes).
 More informations on the default provided configuration can be found on the description
 of `TGHttpHandler`.
 */
@property (strong, nonatomic) TGHttpHandler* httpHandler;

#pragma mark Debug toggles

/**
 Set a `TGDebugFlag` to the map view

 @param debugFlag the debug flag to set
 @param value whether the flag is on or off
 */
- (void)setDebugFlag:(TGDebugFlag)debugFlag value:(BOOL)on;

/**
 Get the status of a specific `TGDebugFLag`

 @param debugFlag the debug flag to get the status of
 @return whether the flag is currently on or off for the map view
 */
- (BOOL)getDebugFlag:(TGDebugFlag)debugFlag;

/**
 Changes the status of a debug flag.
 If a debug flag is on, this will turn it off and vice-versa.

 @param debugFlag the debug flag to toggle the status of
 */
- (void)toggleDebugFlag:(TGDebugFlag)debugFlag;

#pragma mark Marker interface

/**
 Removes all the markers added to the map view.
 */
- (void)markerRemoveAll;

/**
 Creates a marker identifier which can be used to dynamically add points and polylines
 to the map.

 Example on how to add a point to the map for a location `coordinate`:

 ```objc
 TGMapMarkerId identifier = [view markerAdd];
 [view markerSetStyling:identifier styling:@"{ style: 'points', color: 'white', size: [25px, 25px], order:500 }"];
 [view markerSetPoint:identifier coordinates:coordinate];
 ```

 @return a marker identifier
 @note The marker should be appropriately styled using `-[TGMapViewController markerSetStyling:styling:]`
 and a type must be set (point, polyline, polygon) through `markerSetPoint*`, `markerSetPolyline` or `markerSetPolygon`
 for it to be visible.
 */
- (TGMapMarkerId)markerAdd;

/**
 Sets the styling for a marker identifier.

 See the more detailed scene <a href="https://mapzen.com/documentation/tangram/Styles-Overview/">documentation</a>
 to get more styling informations.

 @param identifier the marker identifier created with `-[TGMapViewController markerAdd]`
 @return `YES` if this operation was successful, `NO` otherwise
 */
- (BOOL)markerSetStyling:(TGMapMarkerId)identifier styling:(NSString *)styling;

/**
 Sets a marker styled with a <a href="https://mapzen.com/documentation/tangram/Styles-Overview/#points">point style</a>
 to be at the location `coordinate`.

 @param identifier the marker identifier created with `-[TGMapViewController markerAdd]`
 @param the longitude and latitude where the marker will be placed
 @return `YES` if this operation was successful, `NO` otherwise
 */
- (BOOL)markerSetPoint:(TGMapMarkerId)identifier coordinates:(TGGeoPoint)coordinate;

/**
 Similar to `-[TGMapViewController markerSetPoint:coordinates:]` except that the point will transition to the coordinate
 with a transition of time `duration` and with an ease type function of type `ease` (See `TGEaseType`) from its previous
 coordinate, if point hasn't been set any coordinate yet, this method will act as
 `-[TGMapViewController markerSetPoint:coordinates:]`.

 @param identifier the marker identifier created with `-[TGMapViewController markerAdd]`
 @param the longitude and latitude where the marker will be placed
 @param duration the duration in milliseconds of the animated transition
 @param ease the ease function to be used between animation timestep
 @return `YES` if this operation was successful, `NO` otherwise
 */
- (BOOL)markerSetPointEased:(TGMapMarkerId)identifier coordinates:(TGGeoPoint)coordinate duration:(float)duration easeType:(TGEaseType)ease;

/**

 @param identifier the marker identifier created with `-[TGMapViewController markerAdd]`
 @oaram polyline
 @return `YES` if this operation was successful, `NO` otherwise
 */
- (BOOL)markerSetPolyline:(TGMapMarkerId)identifier polyline:(TGGeoPolyline *)polyline;

/**

 @param identifier the marker identifier created with `-[TGMapViewController markerAdd]`
 @param polygon
 @return `YES` if this operation was successful, `NO` otherwise
 */
- (BOOL)markerSetPolygon:(TGMapMarkerId)identifier polygon:(TGGeoPolygon *)polygon;

/**

 @param identifier the marker identifier created with `-[TGMapViewController markerAdd]`
 @param whether the marker is visible
 @return `YES` if this operation was successful, `NO` otherwise
 */
- (BOOL)markerSetVisible:(TGMapMarkerId)identifier visible:(BOOL)visible;

/**

 @param identifier the marker identifier created with `-[TGMapViewController markerAdd]`
 @param the `UIImage` that will be used to be displayed for the marker
 @return `YES` if this operation was successful, `NO` otherwise

 @note An image marker must be styled with a
 <a href="https://mapzen.com/documentation/tangram/Styles-Overview/#points">point style</a>.
 */
- (BOOL)markerSetImage:(TGMapMarkerId)identifier image:(UIImage *)image;

/**
 Removes a marker for a specific identifier.

 @param identifier the marker identifier created with `-[TGMapViewController markerAdd]`
 @return `YES` if removal was succesful, `NO` otherwise
 */
- (BOOL)markerRemove:(TGMapMarkerId)identifier;

#pragma mark Scene loading - updates interface

- (void)loadSceneFile:(NSString *)path;

- (void)loadSceneFile:(NSString *)path sceneUpdates:(NSArray<TGSceneUpdate *> *)sceneUpdates;

- (void)loadSceneFileAsync:(NSString *)path;

- (void)loadSceneFileAsync:(NSString *)path sceneUpdates:(NSArray<TGSceneUpdate *> *)sceneUpdates;

- (void)queueSceneUpdate:(NSString*)componentPath withValue:(NSString*)value;

- (void)queueSceneUpdates:(NSArray<TGSceneUpdate *> *)sceneUpdates;

- (void)applySceneUpdates;

#pragma mark Feature picking interface

- (void)setPickRadius:(float)logicalPixels;

- (void)pickFeatureAt:(CGPoint)screenPosition;

- (void)pickLabelAt:(CGPoint)screenPosition;

- (void)pickMarkerAt:(CGPoint)screenPosition;

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
