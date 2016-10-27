//
//  MapViewController.m
//
//  Created by Karim Naaji on 10/12/16.
//  Copyright © 2016 Karim Naaji. All rights reserved.
//

#import "MapViewController.h"

@interface MapViewController ()

@property (assign, atomic) TGMapMarkerId polyline;
@property (assign, atomic) TGMapMarkerId polygon;

@end

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

- (void)mapView:(TGMapViewController *)mapView didSelectFeatures:(NSDictionary *)features atScreenPosition:(CGPoint)position
{

    // Not feature selected
    if (features.count == 0) {

        // Convert the 2d screen position to the lat lon
        TGGeoPoint latlon = [mapView screenPositionToLngLat:position];

        // Set the map position
        [mapView setPosition:latlon];
        return;
    }

    NSLog(@"Picked features:");

    for (id key in features) {
        NSLog(@"\t%@ -- %@", key, [features objectForKey:key]);

        if ([key isEqualToString:@"name"]) {
             UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Selection callback"
                                                             message:[features objectForKey:key]
                                                            delegate:nil
                                                   cancelButtonTitle:@"OK"
                                                   otherButtonTitles:nil];
             [alert show];
             [alert release];
        }
    }
}

@end

@implementation MapViewControllerRecognizerDelegate

- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeSingleTap:(CGPoint)location
{
    NSLog(@"Did tap at %f %f", location.x, location.y);

    MapViewController* vc = (MapViewController *)view;

    TGGeoPoint coordinate = [vc screenPositionToLngLat:location];

    // Add polyline marker
    {
        if (!vc.polyline) {
            vc.polyline = [vc markerAdd];
            [vc markerSetStyling:vc.polyline styling:@"{ style: 'lines', color: 'red', width: 20px, order: 5000 }"];
        }

        static TGGeoPolyline* line = nil;
        if (!line) { line = [[TGGeoPolyline alloc] init]; }

        if ([line count] > 0) {
            [line addPoint:coordinate];
            [vc markerSetPolyline:vc.polyline polyline:line];
        } else {
            [line addPoint:coordinate];
        }
    }

    // Add polygon marker
    {
        if (!vc.polygon) {
            vc.polygon = [vc markerAdd];
            [vc markerSetStyling:vc.polygon styling:@" { style: 'polygons', color: 'blue', order: 5000 } "];
        }

        static TGGeoPolygon* polygon = nil;
        if (!polygon) { polygon = [[TGGeoPolygon alloc] init]; }

        if ([polygon count] == 0) {
            [polygon startPath:coordinate withSize:5];
        } else if ([polygon count] % 5 == 0) {
            [vc markerSetPolygon:vc.polygon polygon:polygon];
            [polygon removeAll];
            [polygon startPath:coordinate withSize:5];
        } else {
            [polygon addPoint:coordinate];
        }
    }

    // Add point marker
    {
        TGMapMarkerId mid = [vc markerAdd];
        [vc markerSetStyling:mid styling:@"{ style: 'points', color: 'white', size: [25px, 25px], order:5000, collide: false }"];
        [vc markerSetPoint:mid coordinates:coordinate];
    }

    // Request feature picking
    [vc pickFeaturesAt:location];
}

@end

@implementation MapViewController

- (void)viewWillAppear:(BOOL)animated
{
    [super loadSceneFileAsync:@"https://tangrams.github.io/walkabout-style/walkabout-style.yaml"];
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.mapViewDelegate = [[MapViewControllerDelegate alloc] init];
    self.gestureDelegate = [[MapViewControllerRecognizerDelegate alloc] init];

    self.polyline = 0;
    self.polygon = 0;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
}

@end
