//
//  MapViewController.h
//
//  Created by Karim Naaji on 10/12/16.
//  Copyright © 2016 Karim Naaji. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <TangramMap/TangramMap.h>

@interface MapViewControllerDelegate : NSObject <TGMapViewDelegate>

- (void)mapView:(TGMapViewController *)mapView didLoadSceneAsync:(NSString *)scene;
- (void)mapView:(TGMapViewController*)mapView didSelectFeature:(NSDictionary *)feature atScreenPosition:(CGPoint)position;
- (void)mapViewDidCompleteLoading:(TGMapViewController *)mapView;

@end

@interface MapViewControllerRecognizerDelegate : NSObject <TGRecognizerDelegate>

- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeSingleTapGesture:(CGPoint)location;
- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeLongPressGesture:(CGPoint)location;

@end

@interface MapViewController : TGMapViewController

@end

