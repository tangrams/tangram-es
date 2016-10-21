//
//  MapViewController.m
//
//  Created by Karim Naaji on 10/12/16.
//  Copyright © 2016 Karim Naaji. All rights reserved.
//

#import "MapViewController.h"

@implementation MapViewControllerDelegate

- (void)mapView:(TGMapViewController *)mapView didLoadSceneAsync:(NSString *)scene
{
    NSLog(@"Did load scene async %@", scene);

    TGGeoPoint newYork;
    newYork.longitude = -74.00976419448854;
    newYork.latitude = 40.70532700869127;

    [mapView setZoom:16];
    [mapView setPosition:newYork];
}

- (void)mapView:(TGMapViewController*)mapView didSelectFeatures:(NSDictionary *)features atScreenPosition:(CGPoint)position
{
    NSLog(@"Picked features:");

    for (id key in features) {
        NSLog(@"\t%@ -- %@", key, [features objectForKey:key]);
    }
}

@end

@implementation MapViewControllerRecognizerDelegate

- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeSingleTap:(CGPoint)location
{
    NSLog(@"Did tap at %f %f", location.x, location.y);
    [view pickFeaturesAt:location];
}

@end

@interface MapViewController ()

@end

@implementation MapViewController

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.mapViewDelegate = [[MapViewControllerDelegate alloc] init];
    self.gestureDelegate = [[MapViewControllerRecognizerDelegate alloc] init];

    [super loadSceneFileAsync:@"https://tangrams.github.io/walkabout-style/walkabout-style.yaml"];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
