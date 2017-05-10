#include "paparazzi.h"

#define AA_SCALE 1.0
#define IMAGE_DEPTH 4

#if PLATFORM_LINUX
#include "platform_linux.h"
#elif PLATFORM_OSX
#include "platform_osx.h"
#endif

#include "log.h"
#include "gl/texture.h"

#include "hash-library/md5.h"

#include <functional>
#include <csignal>
#include <fstream>
#include <sstream>
#include <sys/time.h>
#include <unistd.h>
#include "glm/trigonometric.hpp"

#include "stb_image_write.h"


using namespace Tangram;

unsigned long long timeStart;

double getTime() {
    static struct timeval tv;

    gettimeofday(&tv, NULL);
    return ((unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000) * 0.001;
}

Paparazzi::Paparazzi()
  : m_scene(""),
    m_lat(0.0),
    m_lon(0.0),
    m_zoom(0.0f),
    m_rotation(0.0f),
    m_tilt(0.0),
    m_width(0),
    m_height(0) {

    m_glContext = std::make_unique<HeadlessContext>();
    if (!m_glContext->init()) {
        throw std::runtime_error("Could not initialize GL context");
    }
    m_glContext->resize(1, 1);
    if (!m_glContext->makeCurrent()) {
        throw std::runtime_error("Could not activate GL context");
    }

#if PLATFORM_LINUX
    UrlClient::Options urlClientOptions;
    urlClientOptions.numberOfThreads = 10;
    auto platform = std::make_shared<LinuxPlatform>(urlClientOptions);
#elif PLATFORM_OSX
    auto platform = std::make_shared<OSXPlatform>();
#endif

    timeStart = getTime();

    m_map = std::unique_ptr<Tangram::Map>(new Tangram::Map(platform));

    setScene("scene.yaml");

    m_map->setupGL();
    m_map->setPixelScale(AA_SCALE);

    //m_map->resize(m_width*AA_SCALE, m_height*AA_SCALE);
    //m_glContext->setScale(AA_SCALE);
    //setSize(m_width, m_height, 1.0);

    Tangram::setDebugFlag(DebugFlags::tile_bounds, true);

    logMsg("Done Init!\n");

}

Paparazzi::~Paparazzi() {
}

void Paparazzi::setSize (const int &_width, const int &_height, const float &_density) {
    if (_density*_width == m_width && _density*_height == m_height &&
        _density*AA_SCALE == m_map->getPixelScale()) { return; }

    m_width = _width*_density;
    m_height = _height*_density;

    // Setup the size of the image
    if (_density*AA_SCALE != m_map->getPixelScale()) {
        m_map->setPixelScale(_density*AA_SCALE);
    }
    m_map->resize(m_width*AA_SCALE, m_height*AA_SCALE);

    m_glContext->resize(m_width, m_height);
}

void Paparazzi::setZoom(const float &_zoom) {
    if (_zoom == m_zoom) { return; }
    m_zoom = _zoom;
    m_map->setZoom(_zoom);
}

void Paparazzi::setTilt(const float &_deg) {
    if (_deg == m_tilt) { return; }
    m_tilt = _deg;
    m_map->setTilt(glm::radians(m_tilt));
}
void Paparazzi::setRotation(const float &_deg) {
    if (_deg == m_rotation) { return; }

    m_rotation = _deg;
    m_map->setRotation(glm::radians(m_rotation));
}

void Paparazzi::setPosition(const double &_lon, const double &_lat) {
    if (_lon == m_lon && _lat == m_lat) { return; }

    m_lon = _lon;
    m_lat = _lat;
    m_map->setPosition(m_lon, m_lat);
}

void Paparazzi::setScene(const std::string &_url) {
    if (_url == m_scene) { return; }
    m_scene = _url;
    m_map->loadScene(m_scene.c_str(), false,
                     {SceneUpdate("global.sdk_mapzen_api_key", m_apiKey)});
}

void Paparazzi::setSceneContent(const std::string &_yaml_content) {
    MD5 md5;
    std::string md5_scene =  md5(_yaml_content);

    if (md5_scene == m_scene) { return; }
    m_scene = md5_scene;

    // TODO:
    //    - This is waiting for LoadSceneConfig to be implemented in Tangram::Map
    //      Once that's done there is no need to save the file.
    std::string name = "cache/"+md5_scene+".yaml";
    std::ofstream out(name.c_str());
    out << _yaml_content.c_str();
    out.close();

    m_map->loadScene(name.c_str(), false, {SceneUpdate("global.sdk_mapzen_api_key", m_apiKey)});
}

bool Paparazzi::update(int32_t _maxWaitTime) {
    double startTime = getTime();
    float delta = 0.0;

    while (_maxWaitTime < 0 || delta < _maxWaitTime) {

        bool bFinish = m_map->update(10.);
        delta = float(getTime() - startTime);

        if (bFinish) {
            logMsg("Update: Finish!\n");
            return true;
        }
        usleep(10000);
    }
    logMsg("Update: Done waiting...\n");
    return false;
}

void Paparazzi::render(std::string& _image) {
    m_glContext->makeCurrent();

    m_map->render();

    GL::finish();

    stbi_write_png_to_func([](void *context, void *data, int size) {
            static_cast<std::string*>(context)->append(static_cast<const char*>(data), size);
        },
        &_image,
        m_glContext->width(),
        m_glContext->height(),
        IMAGE_DEPTH,
        m_glContext->buffer(),
        m_glContext->width() * IMAGE_DEPTH);
}
