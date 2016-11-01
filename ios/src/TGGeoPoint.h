//
//  TGGeoPoint.h
//  tangram
//
//  Created by Karim Naaji on 10/27/16.
//
//

#ifndef TGGeoPoint_h
#define TGGeoPoint_h

struct TGGeoPoint {
    double longitude;
    double latitude;
};

typedef struct TGGeoPoint TGGeoPoint;

static inline TGGeoPoint TGGeoPointMake(double lat, double lon)
{
    TGGeoPoint p;
    p.latitude = lat;
    p.longitude = lon;
    return p;
}

#endif /* TGGeoPoint_h */
