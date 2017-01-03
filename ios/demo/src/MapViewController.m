//
//  MapViewController.m
//
//  Created by Karim Naaji on 10/12/16.
//  Copyright © 2016 Karim Naaji. All rights reserved.
//

#import "MapViewController.h"

@interface MapViewController ()

@property (assign, nonatomic) TGMapMarkerId polyline;
@property (assign, nonatomic) TGMapMarkerId polygon;

@end

@implementation MapViewControllerDelegate

- (void)mapViewDidCompleteLoading:(TGMapViewController *)mapView
{
    NSLog(@"Did complete view");
}

- (void)mapView:(TGMapViewController *)mapView didLoadSceneAsync:(NSString *)scene
{
    NSLog(@"Did load scene async %@", scene);

    TGGeoPoint newYork;
    newYork.longitude = -74.00976419448854;
    newYork.latitude = 40.70532700869127;

    TGGeoPoint cairo;
    cairo.longitude = 30.00;
    cairo.latitude = 31.25;

    [mapView setZoom:15];
    [mapView setPosition:newYork];
}

- (void)mapView:(TGMapViewController *)mapView didSelectMarker:(TGMarkerPickResult *)markerPickResult atScreenPosition:(TGGeoPoint)position;
{
    if (!markerPickResult) {
        return;
    }

    NSString* message = [NSString stringWithFormat:@"Marker %d", markerPickResult.identifier];

    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Marker pick callback"
                                                    message:message
                                                   delegate:nil
                                          cancelButtonTitle:@"OK"
                                          otherButtonTitles:nil];
    [alert show];
}

- (void)mapView:(TGMapViewController *)mapView didSelectLabel:(TGLabelPickResult *)labelPickResult atScreenPosition:(CGPoint)position
{
    if (!labelPickResult) { return; }

    NSLog(@"Picked label:");

    for (NSString* key in [labelPickResult properties]) {
        NSLog(@"\t%@ -- %@", key, [[labelPickResult properties] objectForKey:key]);

        if ([key isEqualToString:@"name"]) {
             UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Label selection callback"
                                                             message:[[labelPickResult properties] objectForKey:key]
                                                            delegate:nil
                                                   cancelButtonTitle:@"OK"
                                                   otherButtonTitles:nil];
             [alert show];
        }
    }
}

- (void)mapView:(TGMapViewController *)mapView didSelectFeature:(NSDictionary *)feature atScreenPosition:(CGPoint)position
{
    // Not feature selected
    if (!feature) { return; }

    NSLog(@"Picked features:");

    for (id key in feature) {
        NSLog(@"\t%@ -- %@", key, [feature objectForKey:key]);

        if ([key isEqualToString:@"name"]) {
             UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Feature selection callback"
                                                             message:[feature objectForKey:key]
                                                            delegate:nil
                                                   cancelButtonTitle:@"OK"
                                                   otherButtonTitles:nil];
             [alert show];
        }
    }
}

@end

@implementation MapViewControllerRecognizerDelegate

- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeSingleTapGesture:(CGPoint)location
{
    NSLog(@"Did tap at %f %f", location.x, location.y);

    MapViewController* vc = (MapViewController *)view;

    TGGeoPoint coordinate = [vc screenPositionToLngLat:location];

    // Add polyline marker
    {
        if (!vc.polyline) {
            vc.polyline = [vc markerAdd];
            [vc markerSetStyling:vc.polyline styling:@"{ style: 'lines', color: 'red', width: 20px, order: 500 }"];
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
            [vc markerSetStyling:vc.polygon styling:@" { style: 'polygons', color: 'blue', order: 500 } "];
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
        [vc markerSetStyling:mid styling:@"{ style: 'points', color: 'white', size: [25px, 25px], order:500, collide: false }"];
        [vc markerSetPoint:mid coordinates:coordinate];
    }

    // Request feature picking
    [vc pickFeatureAt:location];
    [vc pickLabelAt:location];
}

- (void)mapView:(TGMapViewController *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeLongPressGesture:(CGPoint)location
{
    NSLog(@"Did long press at %f %f", location.x, location.y);
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
