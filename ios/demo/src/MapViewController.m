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
}

- (void)mapView:(TGMapViewController*)mapView didSelectFeatures:(NSDictionary *)features atScreenPosition:(CGPoint)position
{
    NSLog(@"Picked features:");

    for (id key in features) {
        NSLog(@"\t%@ -- %@", key, [features objectForKey:key]);
    }
}

@end

@interface MapViewController ()

@end

@implementation MapViewController

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.mapViewDelegate = [[MapViewControllerDelegate alloc] init];

    [super loadSceneFileAsync:@"https://tangrams.github.io/walkabout-style/walkabout-style.yaml"];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
