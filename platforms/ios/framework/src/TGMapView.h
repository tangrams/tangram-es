//
//  TGMapView.h
//  TangramMap
//
//  Created by Matt Blair on 7/9/18.
//

#import <CoreLocation/CoreLocation.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "TGExport.h"
#import "TGGeometry.h"
#import "TGMapData.h"
#import "TGTypes.h"

NS_ASSUME_NONNULL_BEGIN

@class TGCameraPosition;
@class TGLabelPickResult;
@class TGMapData;
@class TGMarker;
@class TGMarkerPickResult;
@class TGSceneUpdate;

@protocol TGMapViewDelegate;
@protocol TGRecognizerDelegate;
@protocol TGURLHandler;

/**
 `TGMapView` is a flexible and customizable map view managing the lifecycle of an OpenGL ES map. This view provides
 gesture handlers for tap, double-tap, long press, pan, pinch, rotate, and shove gestures.

 The public interface provides dynamic map marker placement, change of camera view settings, and map description changes
 through scene updates.

 This view uses scene files described by the <a href="https://tangrams.readthedocs.io/en/latest/Overviews/Scene-File/">
 Tangram scene format</a> allowing you to fully customize your map using your own data.
 Some pre-made basemap styles can be found <a href="https://www.nextzen.org/">here</a> using Nextzen data sources.

 To use basemap styles you can <a href="https://developers.nextzen.org/"> sign up for an API key</a> and load it
 through your application:

 ```swift
 let sceneURL = URL("https://www.nextzen.org/carto/bubble-wrap-style/9/bubble-wrap-style.zip");
 let sceneUpdates = [ TGSceneUpdate(path: "global.sdk_api_key", value: "YOUR_API_KEY") ];
 view.loadScene(from: sceneURL, with: sceneUpdates);
 ```
 @note All the screen positions used in this inteface are in _logical pixel_ or _drawing coordinate system_ (based on a
 `UIKit` coordinate system), which is independent of the phone pixel density. Refer to the
 <a href="https://developer.apple.com/library/content/documentation/2DDrawing/Conceptual/DrawingPrintingiOS/GraphicsDrawingOverview/GraphicsDrawingOverview.html">Apple documentation</a>
 regarding _Coordinate Systems and Drawing in iOS_ for more information.
 */
TG_EXPORT
@interface TGMapView : UIView <UIGestureRecognizerDelegate>

#pragma mark Initialize the View

/**
 Initializes and returns a new map view with no scene loaded.

 To load a map scene use one of:

 `-[TGMapView loadSceneFromURL:withUpdates:]`

 `-[TGMapView loadSceneAsyncFromURL:withUpdates:]`

 `-[TGMapView loadSceneFromYAML:relativeToURL:withUpdates:]`

 `-[TGMapView loadSceneAsyncFromYAML:relativeToUrl:withUpdates:]`

 @param frame The view frame.
 @return An initialized map view.
 */
- (instancetype)initWithFrame:(CGRect)frame;

/**
 Initializes and returns a new map view with no scene loaded.

 To load a map scene use one of:

 `-[TGMapView loadSceneFromURL:withUpdates:]`

 `-[TGMapView loadSceneAsyncFromURL:withUpdates:]`

 `-[TGMapView loadSceneFromYAML:relativeToURL:withUpdates:]`

 `-[TGMapView loadSceneAsyncFromYAML:relativeToUrl:withUpdates:]`

 @param frame The view frame.
 @param urlHandler A `TGURLHandler` for customizing URL request behavior.
 @return An initialized map view.
 */
- (instancetype)initWithFrame:(CGRect)frame urlHandler:(id<TGURLHandler>)urlHandler;

#pragma mark Loading Scenes

/**
 Load a scene file synchronously from a URL with a list of updates.

 If an error occurs while applying updates the new scene will not be applied. See `TGSceneUpdate` for details.

 @param url An http(s) or file URL for the scene file.
 @param updates A list of `TGSceneUpdate` to apply to the scene, or `nil`.
 @return The integer ID for the new scene or -1 if the scene cannot be loaded.
 */
- (int)loadSceneFromURL:(NSURL *)url withUpdates:(nullable NSArray<TGSceneUpdate *> *)updates;

/**
 Load a scene file asynchronously from a URL with a list of updates.

 Calls `-[TGMapViewDelegate mapView:didLoadScene:withError:]` on the `mapViewDelegate` when it completes.

 If an error occurs while applying updates the new scene will not be applied. See `TGSceneUpdate` for details.

 @param url An http(s) or file URL for the scene file.
 @param updates A list of `TGSceneUpdate` to apply to the scene, or `nil`.
 @return The integer ID for the new scene or -1 if the scene cannot be loaded.
 */
- (int)loadSceneAsyncFromURL:(NSURL *)url withUpdates:(nullable NSArray<TGSceneUpdate *> *)updates;

/**
 Load a scene synchronously from string with a list of updates.

 Calls `-[TGMapViewDelegate mapView:didLoadScene:withError:]` on the `mapViewDelegate` when it completes.

 If an error occurs while applying updates the new scene will not be applied. See `TGSceneUpdate` for details.

 @param yaml YAML scene string.
 @param url The base URL used to resolve relative URLs in the scene.
 @param updates A list of `TGSceneUpdate` to apply to the scene, or `nil`.
 @return The integer ID for the new scene or -1 if the scene cannot be loaded.
 */
- (int)loadSceneFromYAML:(NSString *)yaml
           relativeToURL:(NSURL *)url
             withUpdates:(nullable NSArray<TGSceneUpdate *> *)updates;

/**
 Load a scene asynchronously from string with a list of updates.

 Calls `-[TGMapViewDelegate mapView:didLoadScene:withError:]` on the `mapViewDelegate` when it completes.

 If an error occurs while applying updates the new scene will not be applied. See `TGSceneUpdate` for details.

 @param yaml YAML scene string.
 @param url The base URL used to resolve relative URLs in the scene.
 @param updates A list of `TGSceneUpdate` to apply to the scene, or `nil`.
 @return The integer ID for the new scene or -1 if the scene cannot be loaded.
 */
- (int)loadSceneAsyncFromYAML:(NSString *)yaml
                relativeToURL:(NSURL *)url
                  withUpdates:(nullable NSArray<TGSceneUpdate *> *)updates;

#pragma mark Delegates

/**
 The gesture recognizer delegate
 May be `nil`. See `TGRecognizerDelegate` for more details.
 */
@property (weak, nonatomic, nullable) id<TGRecognizerDelegate> gestureDelegate;

/**
 The map view delegate
 May be `nil`. See `TGMapViewDelegate` for more details.
 */
@property (weak, nonatomic, nullable) id<TGMapViewDelegate> mapViewDelegate;

#pragma mark Changing the Map Viewport

/**
 The current position and orientation of the map camera.

 Setting this property will move the map immediately. To animate the movement use
 `-[TGMapView setCameraPosition:withDuration:easeType:callback:]` or
 `-[TGMapView flyToCameraPosition:withDuration:callback:]`
 */
@property (copy, nonatomic) TGCameraPosition *cameraPosition;

/**
 The minimum zoom level for the map view.

 The default minimum zoom is 0. Values less than the default will be clamped. Assigning a value greater than the current
 maximum zoom will set the maximum zoom to this value.
 */
@property (assign, nonatomic) CGFloat minimumZoomLevel;

/**
 The maximum zoom level for the map view.

 The default maximum zoom is 20.5. Values greater than the default will be clamped. Assigning a value less than the
 current minimum zoom will set the minimum zoom to this value.
 */
@property (assign, nonatomic) CGFloat maximumZoomLevel;

/**
 Assign a `TGCameraType` to the view camera
 */
@property (nonatomic) TGCameraType cameraType;

/**
 Move the map camera to a new position with an easing animation.

 @param cameraPosition The new camera position
 @param duration The animation duration in seconds
 @param easeType The type of easing animation
 @param callback A callback to execute when the animation completes
 */
- (void)setCameraPosition:(TGCameraPosition *)cameraPosition
             withDuration:(NSTimeInterval)duration
                 easeType:(TGEaseType)easeType
                 callback:(nullable void (^)(BOOL canceled))callback;

/**
 Move the map camera to a new position with an animation that pans and zooms in a smooth arc.

 The animation duration is calculated based on the distance to the new camera position.

 @param cameraPosition The new camera position
 @param callback A callback to execute when the animation completes
 */
- (void)flyToCameraPosition:(TGCameraPosition *)cameraPosition
                   callback:(nullable void (^)(BOOL canceled))callback;

/**
 Move the map camera to a new position with an animation that pans and zooms in a smooth arc.

 @param cameraPosition The new camera position
 @param callback A callback to execute when the animation completes
 */
- (void)flyToCameraPosition:(TGCameraPosition *)cameraPosition
               withDuration:(NSTimeInterval)duration
                   callback:(nullable void (^)(BOOL canceled))callback;

/**
 Get a camera position that encloses the given bounds with at least the given amount of padding on each side.

 @param bounds The map bounds to enclose
 @param padding The minimum distance to keep between the bounds and the edges of the view
 */
- (TGCameraPosition *)cameraThatFitsBounds:(TGCoordinateBounds)bounds withPadding:(UIEdgeInsets)padding;

/**
 The longitude and latitude of the center of the map view
 */
@property (assign, nonatomic) CLLocationCoordinate2D position;

/**
 The zoom level of the map view
 */
@property (assign, nonatomic) CGFloat zoom;

/**
 The orientation of to the map view clockwise from true North
 */
@property (assign, nonatomic) CLLocationDirection bearing;

/**
 The tilt angle of the map view in degrees away from straight down
 */
@property (assign, nonatomic) CGFloat pitch;

#pragma mark Coordinate Conversions

/**
 Convert a longitude and latitude to a view position.

 @param coordinate The geographic coordinate to convert
 @return The view position of the input coordinate
 */
- (CGPoint)viewPositionFromCoordinate:(CLLocationCoordinate2D)coordinate;

/**
 Convert a position in view coordinates into the longitude and latitude of the corresponding geographic location.

 @param viewPosition the position in view coordinates to convert
 @return the longitude and latitude
 */
- (CLLocationCoordinate2D)coordinateFromViewPosition:(CGPoint)viewPosition;

#pragma mark Markers

/**
 Remove all the Markers from the map.
 */
- (void)markerRemoveAll;

/**
 Create a marker and add it to the map.

 @return The new Marker.

 @note The new Marker will not be usable until you set its styling, geometry, or image depending on your use case.
 */
- (TGMarker*)markerAdd;

/**
 Remove a Marker from the map.

 @param marker The Marker to remove.

 @note Do not use a Marker after removing it from the map.
 */
- (void)markerRemove:(TGMarker*)marker;

/**
 Access the Markers added to the map.
 */
@property (readonly, nonatomic) NSArray<TGMarker *>* markers;

#pragma mark File Handling

/**
 Assign the resource root for this map view.

 Scene file URLs will be resolved relative to this URL.

 @note By default the resource root is the main bundle resource URL. Using the default resource root: `scene.yaml` is
 resolved to `file://<main bundle path>/Resources/scene.yaml`, `/path/scene.yaml` is resolved to
 `file:///path/scene.yaml`, and `https://my.host/scene.yaml` is resolved to itself.
 */
@property (strong, nonatomic) NSURL* resourceRoot;

#pragma mark Rendering Behavior

/**
 Request the view to draw another frame.

 Typically there is no need to call this. The map view re-draws automatically when needed.
 */
- (void)requestRender;

/**
 The rate you want the map view to re-draw its contents.

 The default value is 60 frames per second.
 */
@property (assign, nonatomic) NSInteger preferredFramesPerSecond;

/**
 The CADisplayLink used by the renderer. Exposed mostly for the ability to pause/unpause rendering without having to initiate
 a scene update, as animations defined within the stylesheet force a continuous rendering regardless of the `continuous`
 setting defined in code.
 
 @note This property is manipulated by the rendering subsystem depending on lifecycle events coming from the OS as well as
 certain events within Tangram itself. As such, do not expect your manipulation of individual settings (such as `paused`) to
 be long lived. An example of this is setting displayLink.paused to YES, then backgrounding and foregrounding the app. Tangram
 will respond to the OS lifecycle events and pause/unpause the map rendering, respectively, thereby overriding your setting.
 This is intended functionality and will not be changed.
 
 @note This property, even though it is marked NS_ASSUME_NONNULL at the top of the header, *will be nil* until the view is
 added to some kind of view hierarchy (window or subview). In a future PR we should clean up this assumption, but until then,
 you have been warned :)
 */
@property (strong, nonatomic, readonly) CADisplayLink *displayLink;

/**
 If `continuous` is `YES`, the map view will re-draw continuously. Otherwise, the map will re-draw only when an event
 changes the map view.

 Scenes can be configured as `animated` (see the <a href="https://mapzen.com/documentation/tangram/scene/#animated">
 animated property documentation</a> for details). When a scene is loaded this property is set to match the animated
 value from the scene.

 @note Changing this property will override the inferred value from the scene. Enabling continuous rendering can
 significantly increase the energy usage of an application.
 */
@property (assign, nonatomic) BOOL continuous;

#pragma mark Gesture Recognizers

/**
 Replaces the tap gesture recognizer used by the map view and adds it to the UIView.
 */
@property (strong, nonatomic) UITapGestureRecognizer* tapGestureRecognizer;

/**
 Replaces the double tap gesture recognizer used by the map view and adds it to the UIView.
 */
@property (strong, nonatomic) UITapGestureRecognizer* doubleTapGestureRecognizer;

/**
 Replaces the pan gesture recognizer used by the map view and adds it to the UIView.
 */
@property (strong, nonatomic) UIPanGestureRecognizer* panGestureRecognizer;

/**
 Replaces the pinch gesture recognizer used by the map view and adds it to the UIView.
 */
@property (strong, nonatomic) UIPinchGestureRecognizer* pinchGestureRecognizer;

/**
 Replaces the rotation gesture recognizer used by the map view and adds it to the UIView.
 */
@property (strong, nonatomic) UIRotationGestureRecognizer* rotationGestureRecognizer;

/**
 Replaces the shove gesture recognizer used by the map view and adds it to the UIView.
 */
@property (strong, nonatomic) UIPanGestureRecognizer* shoveGestureRecognizer;

/**
 Replaces the long press gesture recognizer used by the map view and adds it to the UIView.
 */
@property (strong, nonatomic) UILongPressGestureRecognizer* longPressGestureRecognizer;

#pragma mark Map region change state notifiers

/**
 Client can use these to trigger explicit `TGMapRegionChangeState` changes
 Notifies that client is going to begin map region animation
 */
- (void)notifyGestureDidBegin;

/**
 Client can use these to trigger explicit `TGMapRegionChangeState` changes
 Notifies that client is animating map region
 */
- (void)notifyGestureIsChanging;

/**
 Client can use these to trigger explicit `TGMapRegionChangeState` changes
 Notifies that client is stopping animating map region
 */
- (void)notifyGestureDidEnd;

#pragma mark Data Layers

/**
 Adds a named data layer to the map.

 @param name The name of the data layer.
 @param generateCentroid If YES, a point feature will be added at the centroid of every polygon feature. This can be
 useful for labeling.
 @return The map data, or `nil` if the data source can't be initialized.

 @note You cannot create more than one data source with the same name. If you call this with a name that is already in
 use, the previously returned object will be returned again.
 */
- (nullable TGMapData *)addDataLayer:(NSString *)name generateCentroid:(BOOL)generateCentroid;

#pragma mark Screenshots

/**
 Capture a screenshot of the map view.

 The captured screenshot will be delivered to the `mapViewDelegate` by the `mapView:didCaptureScreenshot:` method. The
 delegate must implement this method to receive the screenshot.

 @param waitForViewComplete If YES, the view will wait for all parts of the map in the current view to finish loading
 before taking the screenshot.
 */
- (void)captureScreenshot:(BOOL)waitForViewComplete;

#pragma mark Picking Map Objects

/**
 Set the radius in logical pixels to use when picking features on the map (default is `0.5`).

 The `-pick*` methods will retrieve all `interactive` map objects from a circular area with this radius around the pick
 location. Setting a larger radius can help ensure that desired features are retrieved from an imprecise touch input.

 @param pixels The pick radius in logical pixels.
 */
- (void)setPickRadius:(CGFloat)pixels;

/**
 Select a visible feature marked as `interactive` from the map view.

 The pick result will be delivered to the `mapViewDelegate` by `mapView:didSelectFeature:atScreenPosition:`.

 The scene file determines which features can be picked. See the
 <a href="https://mapzen.com/documentation/tangram/draw/#interactive">interactive property documentation</a> for
 details.

 @param viewPosition The position in the view to pick from, in logical pixels.
 */
- (void)pickFeatureAt:(CGPoint)viewPosition;

/**
 Select a label marked as `interactive` from the map view.

 The pick result will be delivered to the `mapViewDelegate` by `mapView:didSelectFeature:atScreenPosition:`.

 The scene file determines which features can be picked. See the
 <a href="https://mapzen.com/documentation/tangram/draw/#interactive"> interactive property documentation</a> for
 details.

 @param viewPosition The position in the view to pick from, in logical pixels.
 */
- (void)pickLabelAt:(CGPoint)viewPosition;

/**
 Select a Marker marked as `interactive` from the map view.

 The pick result will be delivered to the `mapViewDelegate` by `mapView:didSelectFeature:atScreenPosition:`.

 To pick a marker you must set the `interactive` property when styling it:
 ```swift
 TGMarker marker;
 marker.styling = "{ style: 'points', interactive : true,  color: 'white', size: [30px, 30px], order: 500 }"
 ```

 @param viewPosition The position in the view to pick from, in logical pixels.
 */
- (void)pickMarkerAt:(CGPoint)viewPosition;

#pragma mark Memory Management

/**
 Reduce memory usage by freeing currently unused resources.
 */
- (void)didReceiveMemoryWarning;

#pragma mark Debugging

/**
 Set a `TGDebugFlag` on the map view.

 @param debugFlag The debug flag to set.
 @param on Whether the flag is on or off.
 */
- (void)setDebugFlag:(TGDebugFlag)debugFlag value:(BOOL)on;

/**
 Get the status of a `TGDebugFlag`.

 @param debugFlag The debug flag to get the status of.
 @return Whether the flag is currently on or off for the map view.
 */
- (BOOL)getDebugFlag:(TGDebugFlag)debugFlag;

/**
 Invert the state of a `TGDebugFlag`.

 @param debugFlag The debug flag to toggle the state of.
 */
- (void)toggleDebugFlag:(TGDebugFlag)debugFlag;

@end // interface TGMapView

NS_ASSUME_NONNULL_END
