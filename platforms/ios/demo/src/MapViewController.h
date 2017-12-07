//
//  MapViewController.h
//
//  Created by Karim Naaji on 10/12/16.
//  Copyright © 2016 Karim Naaji. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <TangramMap/TangramMap.h>

@interface MapViewControllerDelegate : NSObject <TGMapViewDelegate>

- (void)mapView:(nonnull TGMapViewController *)mapView didLoadScene:(int)sceneID withError:(nullable NSError *)sceneError;
- (void)mapViewDidCompleteLoading:(nonnull TGMapViewController *)mapView;
- (void)mapView:(nonnull TGMapViewController *)mapView didSelectFeature:(nullable NSDictionary *)feature atScreenPosition:(CGPoint)position;
- (void)mapView:(nonnull TGMapViewController *)mapView didSelectLabel:(nullable TGLabelPickResult *)labelPickResult atScreenPosition:(CGPoint)position;
- (void)mapView:(nonnull TGMapViewController *)mapView didSelectMarker:(nullable TGMarkerPickResult *)markerPickResult atScreenPosition:(TGGeoPoint)position;
- (void)mapView:(nonnull TGMapViewController *)view didCaptureScreenshot:(nonnull UIImage *)screenshot;

@end

@interface MapViewControllerRecognizerDelegate : NSObject <TGRecognizerDelegate>

- (void)mapView:(nonnull TGMapViewController *)view recognizer:(nonnull UIGestureRecognizer *)recognizer didRecognizeSingleTapGesture:(CGPoint)location;
- (void)mapView:(nonnull TGMapViewController *)view recognizer:(nonnull UIGestureRecognizer *)recognizer didRecognizeLongPressGesture:(CGPoint)location;

@end

@interface MapViewController : TGMapViewController

@end

