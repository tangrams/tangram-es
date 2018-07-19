//
//  TGCameraPosition.h
//  tangram
//
//  Created by Matt Blair on 7/18/18.
//

#import "TGExport.h"
#import <CoreGraphics/CoreGraphics.h>
#import <CoreLocation/CoreLocation.h>

/**
 A camera position defining a viewpoint for a map.
 */
TG_EXPORT
@interface TGCameraPosition : NSObject <NSCopying>

- (instancetype)initWithCenter:(CLLocationCoordinate2D)center
                          zoom:(CGFloat)zoom
                       bearing:(CLLocationDirection)bearing
                         pitch:(CGFloat)pitch;

/**
 The coordinate at the center of the map view.
 */
@property (assign, nonatomic) CLLocationCoordinate2D center;

/**
 The zoom level of the map view. Lower values show more area.
 */
@property (assign, nonatomic) CGFloat zoom;

/**
 The orientation of the map view in degrees clockwise from North.
 */
@property (assign, nonatomic) CLLocationDirection bearing;

/**
 The tilt of the map view in degrees away from straight down.
 */
@property (assign, nonatomic) CGFloat pitch;

@end // interface TGCameraPosition
