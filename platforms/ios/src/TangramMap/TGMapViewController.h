//
//  TGMapViewController.h
//  TangramMap
//
//  Created by Matt Blair on 8/25/14.
//  Updated by Matt Smollinger on 7/29/16.
//  Updated by Karim Naaji on 2/15/17.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

@class TGMapViewController;

#import "TGMapData.h"
#import "TGGeoPoint.h"
#import "TGGeoPolygon.h"
#import "TGGeoPolyline.h"
#import "TGSceneUpdate.h"
#import "TGHttpHandler.h"
#import "TGLabelPickResult.h"
#import "TGMarker.h"
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
 All the screen positions in this interface are in _logical pixel_ or _drawing coordinate system_
 (based on a `UIKit` coordinate system); which is independent of the phone pixel density. Refer the
 <a href="https://developer.apple.com/library/content/documentation/2DDrawing/Conceptual/DrawingPrintingiOS/GraphicsDrawingOverview/GraphicsDrawingOverview.html">Apple documentation</a>
 _Coordinate Systems and Drawing in iOS_ for more informations.

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
 Whether the map view should handle a double tap gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 @return Whether the map view should proceed by handling this gesture behavior
 */
- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizeDoubleTapGesture:(CGPoint)location;

/**
 Whether the map view should handle a long press gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 @return Whether the map view should proceed by handling this gesture behavior
 */
- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizeLongPressGesture:(CGPoint)location;

/**
 Whether the map view should handle a pan gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param displacement the logical pixel displacement of the recognized gesture
 @return Whether the map view should proceed by handling this gesture behavior
 */
- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizePanGesture:(CGPoint)displacement;

/**
 Whether the map view should handle a pinch gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 @return Whether the map view should proceed by handling this gesture behavior
 */
- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizePinchGesture:(CGPoint)location;


/**
 Whether the map view should handle a rotation gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 @return Whether the map view should proceed by handling this gesture behavior
 */
- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizeRotationGesture:(CGPoint)location;

/**
 Whether the map view should handle a shove gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param displacement the logical pixel displacement of the recognized gesture
 @return Whether the map view should proceed by handling this gesture behavior
 */
- (BOOL)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer shouldRecognizeShoveGesture:(CGPoint)displacement;

/**
 Called when the map view just handled a single tap gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 */
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeSingleTapGesture:(CGPoint)location;

/**
 Called when the map view just handled a single double gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 */
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeDoubleTapGesture:(CGPoint)location;

/**
 Called when the map view just handled a long press gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 */
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeLongPressGesture:(CGPoint)location;

/**
 Called when the map view just handled a pan gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param displacement the logical pixel displacement of the recognized gesture
 */
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizePanGesture:(CGPoint)displacement;

/**
 Called when the map view just handled a pinch gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 */
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizePinchGesture:(CGPoint)location;

/**
 Called when the map view just handled a rotation gesture.

 @param recognizer the `UIGestureRecognizer` associated with the gesture
 @param location the logical pixel location of the recognized gesture
 */
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeRotationGesture:(CGPoint)location;

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
- (void)mapView:(nonnull TGMapViewController *)mapView didSelectFeature:(nullable TGFeatureProperties *)feature atScreenPosition:(CGPoint)position;
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

/**
 Called whenever `-[TGMapViewController captureScreenshot:] is called on the map view.

 @param mapView a pointer to the map view
 @param screenshot the image object representing the screenshot
 */
- (void)mapView:(nonnull TGMapViewController *)view didCaptureScreenshot:(nonnull UIImage *)screenshot;
@end

NS_ASSUME_NONNULL_BEGIN

/**
 `TGMapViewController` is a flexible and customizable map view managing the lifecycle of a
 <a href="https://developer.apple.com/reference/glkit/glkview">GLKView</a> component.
 This view provides default gesture handlers for tap, double tap, long press, pan,
 pinch, rotate and shove.

 The public interface provides dynamic map marker placement, change of camera view
 settings, and map description changes through scene updates.

 This view uses scenes descibed by the
  <a href="https://mapzen.com/documentation/tangram/">Tangram YAML scene syntax</a>
 allowing you to fully customize your map description using your own data.
 Some pre-made basemap styles can be found
 <a href="https://mapzen.com/documentation/cartography/styles/">here</a> using Mapzen
 data sources.

 To use basemap styles you can <a href="https://mapzen.com/developers/sign_in">sign up for
 an API key</a> and load it through your application:

 ```swift
 let sceneURL = "https://mapzen.com/carto/walkabout-style-more-labels/walkabout-style-more-labels.yaml";
 view.loadSceneFile(sceneURL, sceneUpdates: [ TGSceneUpdate(path: "sources.mapzen.url_params", value: "{ api_key: \(YOUR_API_KEY) }") ]);
 ```
 @note All the screen positions used in this inteface are in _logical pixel_ or _drawing coordinate
 system_ (based on a `UIKit` coordinate system); which is independent of the phone pixel density. Refer the
 <a href="https://developer.apple.com/library/content/documentation/2DDrawing/Conceptual/DrawingPrintingiOS/GraphicsDrawingOverview/GraphicsDrawingOverview.html">Apple documentation</a>
 _Coordinate Systems and Drawing in iOS_ for more informations.

 */
@interface TGMapViewController : GLKViewController <UIGestureRecognizerDelegate>

/**
 If continuous rendering is set to `true`, the map view will render continuously, otherwise,
 the map will render only when an event occurs on the map (gesture, animation, view update...).

 Some styles can be set to be `animated` and this default value will be set appropriately,
 see the <a href="https://mapzen.com/documentation/tangram/scene/#animated">
 style parameter</a> for more details.

 @note Any changes to this value will override the default induced value from the `animated`
 style parameter.
 Setting this parameter to true will negatively impact battery life if left set for extended periods
 of time. Before setting, make sure battery usage is not something critical to your application.
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
/// Access the markers added to this map view
@property (readonly, nonatomic) NSArray<TGMarker *>* markers;

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

/**
 Adds a named data layer to the map. See `TGMapData` for more details.

 @param name the name of the data layer.

 @return the map data, nil if the data source can't be initialized

 @note You cannot create more than one data source with the same name, otherwise the same
 object will be returned.
 */
- (nullable TGMapData *)addDataLayer:(NSString *)name;

/**
 Asks to capture a screenshot of the map view buffer.

 A delegate should be set to the map view and `-[TGMapViewDelegate didCaptureScreenshot:view:screenshot]`
 implemented to receive the screenshot image data.

 @param waitForViewComplete whether the view should await for all of the map tiles of the current
 view to complete building and rendering
 */
- (void)captureScreenshot:(BOOL)waitForViewComplete;

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

#pragma mark Scene loading - updates interface

/**
 Loads a scene file synchronously.

 If the scene file is set as a resource in your application bundle, make sure to resolve
 the path for this URL relative to your bundle (for example `Resources/scene.yaml`).

 If your scene is hosted remotely (any path starting with `https://` or `http://` is considered a remote scene file)
 Tangram will automatically load that remote scene file.

 @param path the scene path URL
 */
- (void)loadSceneFile:(NSString *)path;

/**
 Loads a scene file (similar to `-loadSceneFile:`), with a list of
 updates to be applied to the scene.

 @param path the scene path URL
 @param sceneUpdates a list of `TGSceneUpdate` to apply to the scene
 */
- (void)loadSceneFile:(NSString *)path sceneUpdates:(NSArray<TGSceneUpdate *> *)sceneUpdates;

/**
 Loads a scene file asynchronously, may call `-[TGMapViewDelegate mapView:didLoadSceneAsync:]`
 if a `TGMapViewDelegate` is set to the map view.

 @param path the scene path URL
 */
- (void)loadSceneFileAsync:(NSString *)path;

/**
 Loads a scene asynchronously (similar to `-loadSceneFileAsync:`), with a
 list of updates to be applied to the scene.

 @param path the scene path URL
 @param sceneUpdates a list of `TGSceneUpdate` to apply to the scene
 */
- (void)loadSceneFileAsync:(NSString *)path sceneUpdates:(NSArray<TGSceneUpdate *> *)sceneUpdates;

/**
 Queue a scene update.

 The path is a series of yaml keys separated by a '.' and the value is a string
 of yaml to replace the current value at the given path in the scene.

 @param componentPath the path of the scene component to update
 @value the value to assign to the YAML component selected with `componentPath`

 @note Scene updates must be applied with `-applySceneUpdates:`
 */
- (void)queueSceneUpdate:(NSString*)componentPath withValue:(NSString*)value;

/**
 Queue a list of scene updates.

 @param sceneUpdates a list of updates to apply to the scene, see `TGSceneUpdate` for more infos

 @note Scene updates must be applied with `-applySceneUpdates:`
 */
- (void)queueSceneUpdates:(NSArray<TGSceneUpdate *> *)sceneUpdates;

/**
 Apply scene updates queued with `-queueSceneUpdate*` methods.
 */
- (void)applySceneUpdates;

#pragma mark Feature picking interface

/**
 Set the radius in logical pixels to use when picking features on the map (default is `0.5`).

 This affects the way `-pick*` method will behave, the large this value is, the more selectable
 elements would get selected.

 @param logicalPixels the pixel radius in logical pixel size
 */
- (void)setPickRadius:(float)logicalPixels;

/**
 Selects a feature marked as <a href="https://mapzen.com/documentation/tangram/draw/#interactive">
 `interactive`</a> in the stylesheet.

 Returns the result in `[TGMapViewDelegate mapView:didSelectFeature:atScreenPosition:].

 @param screenPosition the logical pixels screen position used for the feature selection query

 @note to receive events you must implement set `TGMapViewDelegate` to the map view and implement
 `[TGMapViewDelegate mapView:didSelectFeature:atScreenPosition:]`.
 */
- (void)pickFeatureAt:(CGPoint)screenPosition;

/**
 Selects a label (elements under <a href="https://mapzen.com/documentation/tangram/Styles-Overview/#points">
 points style</a> or <a href="https://mapzen.com/documentation/tangram/Styles-Overview/#text">text styles</a>)
 marked as <a href="https://mapzen.com/documentation/tangram/draw/#interactive">
 `interactive`</a> in the stylesheet.

 Returns the result in `[TGMapViewDelegate mapView:didSelectLabel:atScreenPosition:].

 @param screenPosition the logical pixels screen position used for the feature selection query

 @note To receive events you must set a `TGMapViewDelegate` to the map view and implement
 `[TGMapViewDelegate mapView:didSelectLabel:atScreenPosition:]`.
 */
- (void)pickLabelAt:(CGPoint)screenPosition;

/**
 Selects a marker marked as <a href="https://mapzen.com/documentation/tangram/draw/#interactive">
 `interactive`</a> and returns the result in `[TGMapViewDelegate mapView:didSelectMarker:atScreenPosition:].

 To set a marker interactive, you must set it when styling it:

 ```swift
 TGMarker marker;
 marker.styling = "{ style: 'points', interactive : true,  color: 'white', size: [30px, 30px], order: 500 }"
 ```

 @param screenPosition the logical pixels screen position used for the feature selection query

 @note To receive events you must set a `TGMapViewDelegate` to the map view and implement
 `[TGMapViewDelegate mapView:didSelectMarker:atScreenPosition:]`.
 */
- (void)pickMarkerAt:(CGPoint)screenPosition;

#pragma mark Marker access

/**
 Removes all the markers added to the map view.
 */
- (void)markerRemoveAll;

#pragma mark Map View lifecycle

/**
 Requests the view to draw another frame

 @note This method should only be called if reimplemented in a class that inherits from `TGMapViewController`
 */
- (void)requestRender;

/**
 Draws a frame

 @note This method should only be called if reimplemented in a class that inherits from `TGMapViewController`
 */
- (void)renderOnce;

/**
 Requests the view to update

 @note This method should only be called if reimplemented in a class that inherits from `TGMapViewController`
 */
- (void)update;

#pragma mark Longitude/Latitude - Screen position conversions

/**
 Converts a longitude and latitude to a screen position

 @param lngLat the longitude and latitude of the geographic coordinate to convert

 @return the screen position, `(nil, nil)` if the value is incoherent or not visible on the screen
 */
- (CGPoint)lngLatToScreenPosition:(TGGeoPoint)lngLat;

/**
 Given coordinates in screen space (`x` right, `y` down), set the output longitude and
 latitude to the geographic location corresponding to that point.

 @param screenPosition the 2d screen position to convert

 @return the longitude and latitude, `(nil, nil)` if the point is not visible on the screen
 */
- (TGGeoPoint)screenPositionToLngLat:(CGPoint)screenPosition;

#pragma mark Map View animations - Position interface

/**
 Animate the map view to another geographic coordinate.

 @param position the map view geographic coordinate
 @param seconds the duration in seconds

 @note default ease type for this animation is set to cubic, see `TGEaseType` for more details.
 */
- (void)animateToPosition:(TGGeoPoint)position withDuration:(float)seconds;

/**
 Animate the map view to another geographic coordinate with a specific animation ease.

 @param position the map view geographic coordinate
 @param seconds the duration of the animation given in seconds
 @param easeType the ease type to use for the animation, see `TGEaseType` for more details.
 */
- (void)animateToPosition:(TGGeoPoint)position withDuration:(float)seconds withEaseType:(TGEaseType)easeType;

/**
 Animate the map view to another zoom.

 @param zoomLevel the map view zoom level
 @param seconds the duration of the animation given in seconds

 @note default ease type for this animation is set to cubic, see `TGEaseType` for more details.
 */
- (void)animateToZoomLevel:(float)zoomLevel withDuration:(float)seconds;

/**
 Animate the map view to another zoom level with a specific animation ease.

 @param zoomLevel the map view zoom level
 @param seconds the duration of the animation given in seconds
 @param easeType the ease type to use for the animation, see `TGEaseType` for more details.
 */
- (void)animateToZoomLevel:(float)zoomLevel withDuration:(float)seconds withEaseType:(TGEaseType)easeType;

/**
 Animate the map view to another rotation.

 @param radians the map view rotation in radians
 @param seconds the duration of the animation given in seconds

 @note default ease type for this animation is set to cubic, see `TGEaseType` for more details.
 */
- (void)animateToRotation:(float)radians withDuration:(float)seconds;

/**
 Animate the map view to another rotation with a specific animation ease.

 @param radians the map view rotation in radians
 @param seconds the duration of the animation given in seconds
 @param easeType the ease type to use for the animation, see `TGEaseType` for more details.
 */
- (void)animateToRotation:(float)radians withDuration:(float)seconds withEaseType:(TGEaseType)easeType;

/**
 Animate the map view to another tilt.

 @param radians the map view tilt in radians
 @param seconds the duration of the animation given in seconds

 @note default ease type for this animation is set to cubic, see `TGEaseType` for more details.
 */
- (void)animateToTilt:(float)radians withDuration:(float)seconds;

/**
 Animate the map view to another tilt with a specific animation ease.

 @param radians the map view tilt in radians
 @param seconds the duration of the animation given in seconds
 @param easeType the ease type to use for the animation, see `TGEaseType` for more details.
 */
- (void)animateToTilt:(float)radians withDuration:(float)seconds withEaseType:(TGEaseType)easeType;

NS_ASSUME_NONNULL_END

@end
