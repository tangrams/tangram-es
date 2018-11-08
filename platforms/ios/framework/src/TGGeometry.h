//
//  TGGeometry.h
//  tangram
//
//  Created by Matt Blair on 8/22/18.
//

#import <CoreLocation/CoreLocation.h>
#import <Foundation/Foundation.h>

/**
 A map area defined by bounding coordinates.
 */
struct TGCoordinateBounds {
    /** The minimum longitude and latitude of the area. */
    CLLocationCoordinate2D sw;
    /** The maximum longitude and latitude of the area. */
    CLLocationCoordinate2D ne;
};

/**
 Struct `TGCoordinateBounds`
 */
typedef struct TGCoordinateBounds TGCoordinateBounds;

/**
 Convert an angle in radians to degrees.
 */
NS_INLINE CLLocationDirection TGDegreesFromRadians(CGFloat radians) {
    return 180.0 / M_PI * radians;
}

/**
 Convert an angle in degrees to radians.
 */
NS_INLINE CGFloat TGRadiansFromDegrees(CLLocationDirection degrees) {
    return (CGFloat)(M_PI / 180.0 * degrees);
}
