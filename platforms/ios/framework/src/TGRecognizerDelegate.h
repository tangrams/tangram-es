//
//  TGRecognizerDelegate.h
//  TangramMap
//
//  Created by Matt Blair on 7/25/18.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class TGMapView;

/**
 The `TGGestureDelegate` protocol can be implemented to receive gesture events from the map view. The map view will
 first check whether a gestureDelegate is set, then check whether it responds to any `shouldRecognize*` method:

 - If the delegate responds to `shouldRecognize*`, the map view only performs its default handling of the gesture if
 `shouldRecognize*` returns `YES`.

 - If the delegate doesn't respond to `shouldRecognize*`, the map view performs its default handling of the gesture.

 Finally, if the delegate implements `didRecognize*` then the map view calls this method after the gesture is handled.

 @note These methods are all **optional**. All the screen positions in this interface are in _logical pixels_ or
 _drawing coordinate system_ (based on a `UIKit` coordinate system) which is independent of the phone pixel density.
 Refer to the
 <a href="https://developer.apple.com/library/content/documentation/2DDrawing/Conceptual/DrawingPrintingiOS/GraphicsDrawingOverview/GraphicsDrawingOverview.html">
 Apple documentation</a> regarding _Coordinate Systems and Drawing in iOS_ for more informations.
 */
@protocol TGRecognizerDelegate <NSObject>
@optional

/**
 Whether the map view should handle a single tap gesture.

 @param view The map view instance.
 @param recognizer The `UIGestureRecognizer` that recognized the gesture.
 @param location The location of the recognized gesture in the view.
 @return `YES` if the map view should handle this gesture, otherwise `NO`.
 */
- (BOOL)mapView:(TGMapView *)view
     recognizer:(UIGestureRecognizer *)recognizer
shouldRecognizeSingleTapGesture:(CGPoint)location;

/**
 Whether the map view should handle a double tap gesture.

 @param view The map view instance.
 @param recognizer The `UIGestureRecognizer` that recognized the gesture.
 @param location The location of the recognized gesture in the view.
 @return `YES` if the map view should handle this gesture, otherwise `NO`.
 */
- (BOOL)mapView:(TGMapView *)view
     recognizer:(UIGestureRecognizer *)recognizer
shouldRecognizeDoubleTapGesture:(CGPoint)location;

/**
 Whether the map view should handle a long press gesture.

 @param view The map view instance.
 @param recognizer The `UIGestureRecognizer` that recognized the gesture.
 @param location The location of the recognized gesture in the view.
 @return `YES` if the map view should handle this gesture, otherwise `NO`.
 */
- (BOOL)mapView:(TGMapView *)view
     recognizer:(UIGestureRecognizer *)recognizer
shouldRecognizeLongPressGesture:(CGPoint)location;

/**
 Whether the map view should handle a pan gesture.

 @param view The map view instance.
 @param recognizer The `UIGestureRecognizer` that recognized the gesture.
 @param displacement The displacement of the recognized gesture in the view.
 @return `YES` if the map view should handle this gesture, otherwise `NO`.
 */
- (BOOL)mapView:(TGMapView *)view
     recognizer:(UIGestureRecognizer *)recognizer
shouldRecognizePanGesture:(CGPoint)displacement;

/**
 Whether the map view should handle a pinch gesture.

 @param view The map view instance.
 @param recognizer The `UIGestureRecognizer` that recognized the gesture.
 @param location The position of the recognized gesture in the view.
 @return `YES` if the map view should handle this gesture, otherwise `NO`.
 */
- (BOOL)mapView:(TGMapView *)view
     recognizer:(UIGestureRecognizer *)recognizer
shouldRecognizePinchGesture:(CGPoint)location;

/**
 Whether the map view should handle a rotation gesture.

 @param view The map view instance.
 @param recognizer The `UIGestureRecognizer` that recognized the gesture.
 @param location The position of the recognized gesture in the view.
 @return `YES` if the map view should handle this gesture, otherwise `NO`.
 */
- (BOOL)mapView:(TGMapView *)view
     recognizer:(UIGestureRecognizer *)recognizer
shouldRecognizeRotationGesture:(CGPoint)location;

/**
 Whether the map view should handle a shove gesture.

 @param view The map view instance.
 @param recognizer The `UIGestureRecognizer` that recognized the gesture.
 @param displacement The displacement of the recognized gesture in the view.
 @return `YES` if the map view should handle this gesture, otherwise `NO`.
 */
- (BOOL)mapView:(TGMapView *)view
     recognizer:(UIGestureRecognizer *)recognizer
shouldRecognizeShoveGesture:(CGPoint)displacement;

/**
 If implemented, the returned value will be the focus for the rotation gesture.

 @param view The map view instance.
 @param recognizer The `UIGestureRecognizer` that recognized the gesture.
 @return The screen position the rotation gesture should focus to.
 */
- (CGPoint)rotationFocus:(TGMapView *)view recognizer:(UIGestureRecognizer *)recognizer;

/**
 If implemented, the returned value will be the focus for the pinch gesture.

 @param view The map view instance.
 @param recognizer The `UIGestureRecognizer` that recognized the gesture.
 @return The screen position the pinch gesture should focus to.
 */
- (CGPoint)pinchFocus:(TGMapView *)view recognizer:(UIGestureRecognizer *)recognizer;

/**
 Called when the map view just handled a single tap gesture.

 @param view The map view instance.
 @param recognizer The `UIGestureRecognizer` the recognized the gesture.
 @param location The position of the recognized gesture in the view.
 */
- (void)mapView:(TGMapView *)view
     recognizer:(UIGestureRecognizer *)recognizer
didRecognizeSingleTapGesture:(CGPoint)location;

/**
 Called when the map view just handled a single double tap gesture.

 @param view The map view instance.
 @param recognizer The `UIGestureRecognizer` the recognized the gesture.
 @param location The position of the recognized gesture in the view.
 */
- (void)mapView:(TGMapView *)view
     recognizer:(UIGestureRecognizer *)recognizer
didRecognizeDoubleTapGesture:(CGPoint)location;

/**
 Called when the map view just handled a long press gesture.

 @param view The map view instance.
 @param recognizer The `UIGestureRecognizer` the recognized the gesture.
 @param location The position of the recognized gesture in the view.
 */
- (void)mapView:(TGMapView *)view
     recognizer:(UIGestureRecognizer *)recognizer
didRecognizeLongPressGesture:(CGPoint)location;

/**
 Called when the map view just handled a pan gesture.

 @param view The map view instance.
 @param recognizer The `UIGestureRecognizer` the recognized the gesture.
 @param displacement The displacement of the recognized gesture in the view.
 */
- (void)mapView:(TGMapView *)view
     recognizer:(UIGestureRecognizer *)recognizer
didRecognizePanGesture:(CGPoint)displacement;

/**
 Called when the map view just handled a pinch gesture.

 @param view The map view instance.
 @param recognizer The `UIGestureRecognizer` the recognized the gesture.
 @param location The position of the recognized gesture in the view.
 */
- (void)mapView:(TGMapView *)view
     recognizer:(UIGestureRecognizer *)recognizer
didRecognizePinchGesture:(CGPoint)location;

/**
 Called when the map view just handled a rotation gesture.

 @param view The map view instance.
 @param recognizer The `UIGestureRecognizer` the recognized the gesture.
 @param location The position of the recognized gesture in the view.
 */
- (void)mapView:(TGMapView *)view
     recognizer:(UIGestureRecognizer *)recognizer
didRecognizeRotationGesture:(CGPoint)location;

/**
 Called when the map view just handled a shove gesture.

 @param view The map view instance.
 @param recognizer The `UIGestureRecognizer` the recognized the gesture.
 @param displacement The displacement of the recognized gesture in the view.
 */
- (void)mapView:(TGMapView *)view
     recognizer:(UIGestureRecognizer *)recognizer
didRecognizeShoveGesture:(CGPoint)displacement;

@end // protocol TGGestureDelegate
