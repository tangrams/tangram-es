//
//  TGGeoPoint.h
//  TangramMap
//
//  Created by Karim Naaji on 10/27/16.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#pragma once

/// Structure holding a geographic coordinate (longitude and latitude)
struct TGGeoPoint {
    /// The geographic longitude
    double longitude;
    /// The geographic latitude
    double latitude;
};

typedef struct TGGeoPoint TGGeoPoint;

/**
 Helper to create a `TGGeoPoint` from a longitude and latitude

 @param lon the longitude coordinate
 @param lat the latitude coordinate
 @return a `TGGeoPoint` holding the longitude and latitude
 */
static inline TGGeoPoint TGGeoPointMake(double lon, double lat)
{
    TGGeoPoint p;
    p.latitude = lat;
    p.longitude = lon;
    return p;
}

