//
//  TGMapViewDelegate.h
//  tangram
//
//  Created by Karim Naaji on 10/17/16.
//
//

@class TGMapViewController;

@protocol TGMapViewDelegate <NSObject>

@optional

- (void)mapView:(TGMapViewController*)mapView didLoadSceneAsync:(NSString*)scene;

- (void)mapView:(TGMapViewController*)mapView didSelectFeatures:(NSDictionary*)features atScreenPosition:(CGPoint)position;

@end
