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

/**
 Dictionary of feature properties keyed by their property name
 */
typedef NSDictionary<NSString *, NSString *> TGFeatureProperties;

TG_EXPORT
@interface TGMapFeature : NSObject

- (instancetype)initWithProperties:(TGFeatureProperties *)properties;

@property(readonly) TGFeatureProperties *properties;

@end

TG_EXPORT
@interface TGMapPointFeature : TGMapFeature

- (instancetype)initWithPoint:(CLLocationCoordinate2D)point properties:(TGFeatureProperties *)properties;

@property(nonatomic, readonly) CLLocationCoordinate2D point;

@end

TG_EXPORT
@interface TGMapPolylineFeature : TGMapFeature

- (instancetype)initWithPolyline:(TGGeoPolyline *)polyline properties:(TGFeatureProperties *)properties;

@property(readonly) TGGeoPolyline *polyline;

@end

TG_EXPORT
@interface TGMapPolygonFeature : TGMapFeature

- (instancetype)initWithPolygon:(TGGeoPolygon *)polygon properties:(TGFeatureProperties *)properties;

@property(readonly) TGGeoPolygon *polygon;

@end

NS_ASSUME_NONNULL_END
