#include <limits>
#include "geom.h"

int signValue(float _n) {
    if( _n > 0 ) return 1;
    else if(_n < 0) return -1;
    else return 0;
}

void wrapRad(double& _angle){
    if (_angle < -PI) _angle += PI*2.;
    if (_angle > PI) _angle -= PI*2.;
}

float mapValue(const float& _value, const float& _inputMin, const float& _inputMax, const float& _outputMin, const float& _outputMax, bool _clamp ) {
    if (fabs(_inputMin - _inputMax) < std::numeric_limits<float>::epsilon()){
        return _outputMin;
    } else {
        float outVal = ((_value - _inputMin) / (_inputMax - _inputMin) * (_outputMax - _outputMin) + _outputMin);

        if( _clamp ){
            if(_outputMax < _outputMin){
                if( outVal < _outputMax )outVal = _outputMax;
                else if( outVal > _outputMin )outVal = _outputMin;
            }else{
                if( outVal > _outputMax )outVal = _outputMax;
                else if( outVal < _outputMin )outVal = _outputMin;
            }
        }
        return outVal;
    }
}

float lerp(const float& _start, const float& _stop, float const& _amt) {
    if(_start!=_stop){
        return _start + (_stop-_start) * _amt;
    } else {
        return _stop;
    }
}

void setLength(glm::vec3& _vec, const float _length ) {
    float l = sqrtf(_vec.x*_vec.x + _vec.y*_vec.y + _vec.z*_vec.z);
    if (l > 0) {
        _vec.x = (_vec.x/l)*_length;
        _vec.y = (_vec.y/l)*_length;
        _vec.z = (_vec.z/l)*_length;
    }
}

glm::vec3 getWithLength(const glm::vec3& _vec, float _length) {
    float l = (float)sqrt(_vec.x*_vec.x + _vec.y*_vec.y + _vec.z*_vec.z);
    if( l > 0 )
        return glm::vec3( (_vec.x/l)*_length, (_vec.y/l)*_length, (_vec.z/l)*_length );
    else
        return glm::vec3();
}

bool isPowerOf2(unsigned int _val) {
    return _val > 0 && (_val & (_val - 1)) == 0;
}

float angleBetweenPoints(const glm::vec2& _p1, const glm::vec2& _p2) {
    glm::vec2 p1p2 = _p2 - _p1;
    p1p2 = glm::normalize(p1p2);
    return (float) atan2(p1p2.x, -p1p2.y);
}

glm::vec4 worldToClipSpace(const glm::mat4& _mvp, const glm::vec4& _worldPosition) {
    return _mvp * _worldPosition;
}

glm::vec2 clipToScreenSpace(const glm::vec4& _clipCoords, const glm::vec2& _screenSize) {
    glm::vec2 halfScreen = glm::vec2(_screenSize * 0.5f);
    
    glm::vec4 ndc = _clipCoords / _clipCoords.w;
    
    // from normalized device coordinates to screen space coordinate system
    // top-left screen axis, y pointing down
    
    return glm::vec2((ndc.x + 1) * halfScreen.x, (1 - ndc.y) * halfScreen.y);
}

glm::vec2 worldToScreenSpace(const glm::mat4& _mvp, const glm::vec4& _worldPosition, const glm::vec2& _screenSize) {
    return clipToScreenSpace(worldToClipSpace(_mvp, _worldPosition), _screenSize);
}

