//
//  MapViewController.h
//
//  Created by Karim Naaji on 10/12/16.
//  Copyright © 2016 Karim Naaji. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "TangramMap/TangramMap.h"

@interface MapViewController : UIViewController <TGMapViewDelegate, TGRecognizerDelegate>

- (void)mapView:(nonnull TGMapView *)mapView didLoadScene:(int)sceneID withError:(nullable NSError *)sceneError;
- (void)mapViewDidCompleteLoading:(nonnull TGMapView *)mapView;
- (void)mapView:(nonnull TGMapView *)mapView didSelectFeature:(nullable NSDictionary *)feature atScreenPosition:(CGPoint)position;
- (void)mapView:(nonnull TGMapView *)mapView didSelectLabel:(nullable TGLabelPickResult *)labelPickResult atScreenPosition:(CGPoint)position;
- (void)mapView:(nonnull TGMapView *)mapView didSelectMarker:(nullable TGMarkerPickResult *)markerPickResult atScreenPosition:(CGPoint)position;
- (void)mapView:(nonnull TGMapView *)view didCaptureScreenshot:(nonnull UIImage *)screenshot;

- (void)mapView:(nonnull TGMapView *)view recognizer:(nonnull UIGestureRecognizer *)recognizer didRecognizeSingleTapGesture:(CGPoint)location;
- (void)mapView:(nonnull TGMapView *)view recognizer:(nonnull UIGestureRecognizer *)recognizer didRecognizeLongPressGesture:(CGPoint)location;

@end

