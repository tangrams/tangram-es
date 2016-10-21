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
- (void)mapView:(TGMapViewController*)mapView didSelectFeatures:(NSDictionary *)features atScreenPosition:(CGPoint)position;

@end

@interface MapViewController : TGMapViewController

@end

