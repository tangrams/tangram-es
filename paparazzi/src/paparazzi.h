#pragma once

#include <memory>
#include <string>

#include "tangram.h"
#include "headlessContext.h"

class Paparazzi {
public:
    Paparazzi();
    ~Paparazzi();

    void setSize(const int &_width, const int &_height, const float &_density);
    void setZoom(const float &_zoom);
    void setTilt(const float &_deg);
    void setRotation(const float &_deg);
    void setScene(const std::string &_url);
    void setSceneContent(const std::string &_yaml_content);
    void setPosition(const double &_lon, const double &_lat);
    void render(std::string& _image);
    bool update(int32_t _maxWaitTime = -1);

    void setApiKey(const std::string &_apiKey) { m_apiKey = _apiKey; }

protected:

    std::string m_scene;
    std::string m_apiKey;
    double m_lat;
    double m_lon;
    float m_zoom;
    float m_rotation;
    float m_tilt;
    int m_width;
    int m_height;

    std::unique_ptr<Tangram::Map> m_map;
    std::unique_ptr<Tangram::HeadlessContext> m_glContext;
};
