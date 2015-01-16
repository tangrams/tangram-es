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
