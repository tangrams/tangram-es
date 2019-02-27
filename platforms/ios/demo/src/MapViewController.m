//
//  MapViewController.m
//
//  Created by Karim Naaji on 10/12/16.
//  Copyright © 2016 Karim Naaji. All rights reserved.
//

#import "MapViewController.h"
#import <CoreLocation/CoreLocation.h>

static const NSUInteger MAX_TAPPED_LOCATIONS = 8;

@interface MapViewController ()  <CLLocationManagerDelegate> {
    CLLocationCoordinate2D _tappedLocations[MAX_TAPPED_LOCATIONS];
    NSUInteger _tappedLocationCount;
}

@property (assign, nonatomic) TGMarker* markerPolygon;
@property (strong, nonatomic) TGMapData* mapData;
@property (strong, nonatomic) CLLocationManager *locationManager;
@property (strong, nonatomic) TGMarker* locationTrackingMarker;

- (void)addAlert:(NSString *)message withTitle:(NSString *)title;

@end

@implementation MapViewController

#pragma mark TGMapView Delegate

- (void)mapView:(TGMapView *)view didCaptureScreenshot:(UIImage *)screenshot
{
    NSLog(@"Did capture screenshot");
}

- (void)mapViewRegionIsChanging:(TGMapView *)mapView
{
    NSLog(@"Region Is Changing");
}

- (void)mapView:(TGMapView *)mapView regionWillChangeAnimated:(BOOL)animated
{
    NSLog(@"Region Will Change animated: %d", animated);
}

- (void)mapView:(TGMapView *)mapView regionDidChangeAnimated:(BOOL)animated
{
    NSLog(@"Region Did Change animated: %d", animated);
}

- (void)mapViewDidCompleteLoading:(TGMapView *)mapView
{
    NSLog(@"Did complete view");
    // [mapView captureScreenshot:YES];
}

- (void)mapView:(TGMapView *)mapView didLoadScene:(int)sceneID withError:(nullable NSError *)sceneError
{
    if (sceneError) {
        NSLog(@"Scene Ready with error %@", sceneError);
        return;
    }

    // Add a client data source, named 'mz_route_line_transit'
    self.mapData = [mapView addDataLayer:@"mz_route_line_transit" generateCentroid:NO];
}

- (void)mapView:(TGMapView *)mapView didSelectMarker:(TGMarkerPickResult *)markerPickResult atScreenPosition:(CGPoint)position;
{
    if (!markerPickResult) {
        return;
    }

    NSString* message = [NSString stringWithFormat:@"Marker %f %f",
        markerPickResult.marker.point.latitude,
        markerPickResult.marker.point.longitude];

    [self addAlert:message withTitle:@"Marker pick callback"];
}

- (void)mapView:(TGMapView *)mapView didSelectLabel:(TGLabelPickResult *)labelPickResult atScreenPosition:(CGPoint)position
{
    if (!labelPickResult) { return; }

    NSLog(@"Picked label:");

    for (NSString* key in [labelPickResult properties]) {
        NSLog(@"\t%@ -- %@", key, [[labelPickResult properties] objectForKey:key]);

        if ([key isEqualToString:@"name"]) {
            [self addAlert:[[labelPickResult properties] objectForKey:key] withTitle:@"Label selection callback"];
        }
    }
}

- (void)mapView:(TGMapView *)mapView didSelectFeature:(NSDictionary *)feature atScreenPosition:(CGPoint)position
{
    if (!feature) { return; }

    NSLog(@"Picked features:");

    for (id key in feature) {
        NSLog(@"\t%@ -- %@", key, [feature objectForKey:key]);

        if ([key isEqualToString:@"name"]) {
            [self addAlert:[[feature objectForKey:key] objectForKey:key] withTitle:@"Feature selection callback"];
        }
    }
}

#pragma mark Gesture Delegate

- (void)mapView:(TGMapView *)view recognizer:(UIGestureRecognizer *)recognizer didRecognizeSingleTapGesture:(CGPoint)location
{
    NSLog(@"Did tap at %f %f", location.x, location.y);

    CLLocationCoordinate2D coordinates = [view coordinateFromViewPosition:location];

    if (_tappedLocationCount < MAX_TAPPED_LOCATIONS) {
        _tappedLocations[_tappedLocationCount++] = coordinates;
    }

    // Add polyline feature
    {
        TGFeatureProperties* properties = @{ @"type" : @"line", @"color" : @"#D2655F" };

        if (_tappedLocationCount > 1) {
            TGGeoPolyline* polyline = [[TGGeoPolyline alloc] initWithCoordinates:_tappedLocations count:_tappedLocationCount];
            TGMapFeature *feature = [TGMapFeature mapFeatureWithPolyline:polyline properties:properties];
            [_mapData setFeatures:@[feature]];
        }
    }

    // Add polygon marker
    {
        if (!_markerPolygon) {
            _markerPolygon = [view markerAdd];
            _markerPolygon.stylingString = @"{ style: 'polygons', color: 'blue', order: 500 }";
        }
        if (_tappedLocationCount > 2) {
            TGGeoPolyline *ring = [[TGGeoPolyline alloc] initWithCoordinates:_tappedLocations count:_tappedLocationCount];
            TGGeoPolygon *polygon = [[TGGeoPolygon alloc] initWithRings:[NSArray arrayWithObject:ring]];
            _markerPolygon.polygon = polygon;
        }

    }

    // Add point marker
    {
        TGMarker* markerPoint = [view markerAdd];
        markerPoint.stylingString = @"{ style: 'points', color: 'white', size: [25px, 25px], collide: false }";
        markerPoint.point = coordinates;
    }

    // Request feature picking
    [view pickFeatureAt:location];
    [view pickLabelAt:location];
    // [view pickMarkerAt:location];

    TGCameraPosition* camera = [view cameraPosition];
    camera.center = CLLocationCoordinate2DMake(coordinates.latitude, coordinates.longitude);
    [view setCameraPosition:camera withDuration:0.5 easeType:TGEaseTypeCubic callback: ^(BOOL canceled){
        NSLog(@"Animation completed %d", !canceled);
    }];
}

- (void)mapView:(TGMapView *)mapView recognizer:(UIGestureRecognizer *)recognizer didRecognizeLongPressGesture:(CGPoint)location
{
    NSLog(@"Did long press at %f %f", location.x, location.y);
}

- (void)addAlert:(NSString *)message withTitle:(NSString *)title
{
    UIAlertController *alert = [[UIAlertController alloc] init];

    alert.title = title;
    alert.message = message;

    UIAlertAction* okAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleCancel handler:nil];
    [alert addAction:okAction];

    [self presentViewController:alert animated:YES completion:nil];
}

#pragma mark View Controller

- (void)viewWillAppear:(BOOL)animated
{
    NSString* apiKey = [[[NSBundle mainBundle] infoDictionary] valueForKey:@"NEXTZEN_API_KEY"];
    if ([apiKey length] == 0) {
        apiKey = [[[NSProcessInfo processInfo] environment] valueForKeyPath:@"NEXTZEN_API_KEY"];
    }
    NSAssert(apiKey, @"Please provide a valid API key by setting the environment variable NEXTZEN_API_KEY at build time");

    NSMutableArray<TGSceneUpdate *>* updates = [[NSMutableArray alloc]init];
    [updates addObject:[[TGSceneUpdate alloc]initWithPath:@"global.sdk_api_key" value:apiKey]];

    TGMapView *mapView = (TGMapView *)self.view;

    [mapView loadSceneAsyncFromURL:[NSURL URLWithString:@"https://www.nextzen.org/carto/bubble-wrap-style/9/bubble-wrap-style.zip"] withUpdates:updates];

    //Location tracking marker setup
    TGMarker* markerPoint = [mapView markerAdd];
    markerPoint.stylingString = @"{ style: 'points', color: 'white', size: [25px, 25px], collide: false }";
    CLLocationCoordinate2D newYork;
    newYork.longitude = -74.00976419448854;
    newYork.latitude = 40.70532700869127;
    markerPoint.point = newYork;
    self.locationTrackingMarker = markerPoint;

    TGCameraPosition *camera = [[TGCameraPosition alloc] initWithCenter:newYork zoom:15 bearing:0 pitch:0];
    [mapView setCameraPosition:camera];
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    TGMapView *mapView = (TGMapView *)self.view;
    mapView.mapViewDelegate = self;
    mapView.gestureDelegate = self;
    self.locationManager = [[CLLocationManager alloc] init];
    self.locationManager.delegate = self;
    // Enable for Location Tracking
//     [self.locationManager requestAlwaysAuthorization];
}

- (void)beginBackgroundLocationTracking {
    self.locationManager.activityType = CLActivityTypeOther;
    self.locationManager.desiredAccuracy = kCLLocationAccuracyBest;
    self.locationManager.pausesLocationUpdatesAutomatically = NO;
    self.locationManager.allowsBackgroundLocationUpdates = YES;
    [self.locationManager startUpdatingLocation];
    [self.locationManager startUpdatingHeading];
}

#pragma mark - Location Manager Delegate

- (void)locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray<CLLocation *> *)locations {
    NSLog(@"Locations came in - %@", locations);
    CLLocationCoordinate2D point = CLLocationCoordinate2DMake(locations[0].coordinate.longitude, locations[0].coordinate.latitude);
    [self.locationTrackingMarker pointEased:point seconds:1.0 easeType:TGEaseTypeCubic];
}

- (void)locationManager:(CLLocationManager *)manager didChangeAuthorizationStatus:(CLAuthorizationStatus)status {
    switch (status) {
        case kCLAuthorizationStatusAuthorizedAlways:
            // Enable to turn on background location tracking - be aware we don't shut this off ever, so the sample app will consume battery life if allowed to run in the background.
//             [self beginBackgroundLocationTracking];
            break;
        default:
            break;
    }
}

@end
