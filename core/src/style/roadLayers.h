#pragma once

/* This function and the related variables serve to map the 'sort_key' property of road features into a smaller range
 * so that it can be used for geometry layering using the z-buffer. The sort_key property ranges between MIN_ROAD_SORT_KEY
 * and MAX_ROAD_SORT_KEY. 'reduceSortKey' is a one-to-one mapping of sort_key that preserves ordering and maintains an integer 
 * distance between successive values.
 */

constexpr float reduceSortKey(float _key) {
    
    return 30.f * (((int)_key % 10000) / 1000) +
           10.f * (((int)_key % 1000) / 100) +
           1.f  * ((int)_key % 10);
    
}

// These values are a function of the server code that generates sort_key
const int MIN_ROAD_SORT_KEY = -3109;
const int MAX_ROAD_SORT_KEY = 3100;

// The minimum 'reduceSortKey' output
constexpr float MIN_ROAD_LAYER = reduceSortKey(MIN_ROAD_SORT_KEY);

// The maximum 'reduceSortKey' output
constexpr float MAX_ROAD_LAYER = reduceSortKey(MAX_ROAD_SORT_KEY);

// The total number of possible road layers
constexpr float ROAD_LAYER_OFFSET = MAX_ROAD_LAYER - MIN_ROAD_LAYER;

// Maps _key from the range [MIN_ROAD_SORT_KEY, MAX_ROAD_SORT_KEY] to the range [0, ROAD_LAYER_OFFSET]
constexpr float sortKeyToLayer(float _key) {
    return reduceSortKey(_key) - MIN_ROAD_LAYER;
}
