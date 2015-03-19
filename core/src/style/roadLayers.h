#pragma once

constexpr float reduceSortKey(float _key) {
    
    return 30.f * (((int)_key % 10000) / 1000) +
           10.f * (((int)_key % 1000) / 100) +
           1.f  * ((int)_key % 10);
    
}

const int MIN_ROAD_SORT_KEY = -3109;
const int MAX_ROAD_SORT_KEY = 3100;

constexpr float MIN_ROAD_LAYER = reduceSortKey(MIN_ROAD_SORT_KEY);
constexpr float MAX_ROAD_LAYER = reduceSortKey(MAX_ROAD_SORT_KEY);
constexpr float ROAD_LAYER_OFFSET = MAX_ROAD_LAYER - MIN_ROAD_LAYER;
