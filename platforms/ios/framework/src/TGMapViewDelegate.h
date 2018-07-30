//
//  TGMapViewDelegate.h
//  TangramMap
//
//  Created by Matt Blair on 7/25/18.
//

#import "TGMapData.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class TGLabelPickResult;
@class TGMapView;
@class TGMarkerPickResult;

/**
 A map view delegate can receive various map events.

 @note All of these methods are called from main thread, these methods are all **optional**.
 */
@protocol TGMapViewDelegate <NSObject>
@optional

/**
 Receive the result from `-[TGMapView pickFeatureAt:]`.

 @param mapView The map view instance.
 @param feature A dictionary of properties of the picked feature or `nil` if no feature was found.
 @param position The view position where the feature was picked.
 */
- (void)mapView:(nonnull TGMapView *)mapView
didSelectFeature:(nullable TGFeatureProperties *)feature
atScreenPosition:(CGPoint)position;

/**
 Receive the result from `-[TGMapView pickLabelAt:]`.

 @param mapView The map view instance.
 @param labelPickResult A result object with information about the picked label or `nil` if no label was found.
 @param position The view position where the label was picked.
 */
- (void)mapView:(nonnull TGMapView *)mapView
 didSelectLabel:(nullable TGLabelPickResult *)labelPickResult
atScreenPosition:(CGPoint)position;

/**
 Receive the result from `-[TGMapView pickMarkerAt:]`.

 @param mapView The map view instance.
 @param markerPickResult A result object with information about the picked marker or `nil` if no marker was found.
 @param position The view position where the marker was picked.
 */
- (void)mapView:(nonnull TGMapView *)mapView
didSelectMarker:(nullable TGMarkerPickResult *)markerPickResult
atScreenPosition:(CGPoint)position;

/**
 Receive the result from `-[TGMapView captureScreenshot:]`.

 @param mapView The map view instance.
 @param screenshot The image object representing the screenshot.
 */
- (void)mapView:(nonnull TGMapView *)mapView
didCaptureScreenshot:(nonnull UIImage *)screenshot;

/**
 Called after a scene has been loaded or updated.

 See:

 `-[TGMapView loadSceneAsyncFromURL:withUpdates:]`

 `-[TGMapView loadSceneFromYAML:relativeToURL:withUpdates:]`

 `-[TGMapView loadSceneAsyncFromYAML:relativeToURL:withUpdates:]`

 `-[TGMapView updateSceneAsync:]`

 @param mapView The map view instance.
 @param sceneID The ID of the scene that was loaded or updated.
 @param sceneError Any error encountered while loading or updating the scene.
 */
- (void)mapView:(nonnull TGMapView *)mapView didLoadScene:(int)sceneID withError:(nullable NSError *)sceneError;

/**
 Called immediately before the displayed map region moves.

 @param mapView The map view instance.
 @param animated If YES the move will be animated over time, if NO the move will happen immediately.
 */
- (void)mapView:(nonnull TGMapView *)mapView regionWillChangeAnimated:(BOOL)animated;

/**
 Called repeatedly when the displayed map region is moving due to an animation or a gesture.

 @param mapView The map view instance.
 */
- (void)mapViewRegionIsChanging:(nonnull TGMapView *)mapView;

/**
 Called after the displayed map region moves.

 @param mapView The map view instance.
 @param animated If YES the move was animated over time, if NO the move happened immediately.
 */
- (void)mapView:(nonnull TGMapView *)mapView regionDidChangeAnimated:(BOOL)animated;

/**
 Called after the view completes loading all content in the current view.

 @param mapView The map view instance.
 */
- (void)mapViewDidCompleteLoading:(nonnull TGMapView *)mapView;

@end // protocol TGMapViewDelegate
