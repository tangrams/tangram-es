//
//  TGMapFeature.h
//  TangramMap
//
//  Created by Matt Blair on 2/21/19.
//

#import <CoreLocation/CoreLocation.h>
#import <Foundation/Foundation.h>
#import "TGExport.h"
#import "TGGeoPolygon.h"
#import "TGGeoPolyline.h"

NS_ASSUME_NONNULL_BEGIN

/// Dictionary of feature properties keyed by their property name.
typedef NSDictionary<NSString *, NSString *> TGFeatureProperties;

/**
 A map object defined by a geometry and a set of properties.

 Map features can be assigned to a `TGMapData` to be rendered in the current scene.

 Create a map feature with either a point, polyline, or polygon geometry using the factory methods. Access the geometry
 using the method for that geometry type - the other methods will return `nil`.
 */
TG_EXPORT
@interface TGMapFeature : NSObject

#pragma mark Creating Map Features

/**
 Creates a map feature with a point geometry.

 @param point The point defining the feature geometry.
 @param properties The feature properties.
 @return An initialized map feature object.
 */
+ (instancetype)mapFeatureWithPoint:(CLLocationCoordinate2D)point properties:(TGFeatureProperties *)properties;

/**
 Creates a map feature with a polyline geometry.

 @param polyline The polyline defining the feature geometry.
 @param properties The feature properties.
 @return An initialized map feature object.
 */
+ (instancetype)mapFeatureWithPolyline:(TGGeoPolyline *)polyline properties:(TGFeatureProperties *)properties;

/**
 Creates a map feature with a polygon geometry.

 @param polygon The polygon defining the feature geometry.
 @param properties The feature properties.
 @return An initialized map feature object.
 */
+ (instancetype)mapFeatureWithPolygon:(TGGeoPolygon *)polygon properties:(TGFeatureProperties *)properties;

#pragma mark Accessing Map Feature Geometry

/// If this feature has a point geometry then this returns that point, otherwise returns `nil`.
- (nullable CLLocationCoordinate2D *)point;

/// If this feature has a polyline geometry then this returns that polyline, otherwise returns `nil`.
- (nullable TGGeoPolyline *)polyline;

/// If this feature has a polygon geometry then this returns that polygon, otherwise returns `nil`.
- (nullable TGGeoPolygon *)polygon;

#pragma mark Accessing Map Feature Properties

/**
 A set of key-value pairs defining the properties of this map feature.

 These properties are used to determine if and how the feature will be rendered in the map scene, according the the
 <a href="https://tangrams.readthedocs.io/en/latest/Overviews/Scene-File/">Scene File</a>.
 */
@property(readonly) TGFeatureProperties *properties;

@end

NS_ASSUME_NONNULL_END
