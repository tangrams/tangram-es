#include <prime_server/prime_server.hpp>
#include <prime_server/http_protocol.hpp>
using namespace prime_server;

#include <prime_server/logging.hpp>

#include <functional>
#include <string>
#include <csignal>
#include <regex>
#include <math.h>

#include "paparazzi.h"

#define MAX_WAITING_TIME 100.0

const headers_t::value_type CORS{"Access-Control-Allow-Origin", "*"};
const headers_t::value_type PNG_MIME{"Content-type", "image/png"};
const headers_t::value_type TXT_MIME{"Content-type", "text/plain;charset=utf-8"};

const std::regex TILE_REGEX("\\/(\\d*)\\/(\\d*)\\/(\\d*)\\.png");

struct tile_s {
    uint32_t x;
    uint32_t y;
    uint32_t z;
};

static double radians_to_degrees(double radians) {
    return radians * 180 / M_PI;
}

static double degrees_to_radians(double degrees) {
    return degrees * M_PI / 180;
}

// http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
// TODO make output into point
void tile_to_lnglat(tile_s *tile, double *out_lng_deg, double *out_lat_deg) {
    double n = pow(2, tile->z);
    double lng_deg = tile->x / n * 360.0 - 180.0;
    double lat_rad = atan(sinh(M_PI * (1 - 2 * tile->y / n)));
    double lat_deg = radians_to_degrees(lat_rad);
    *out_lng_deg = lng_deg;
    *out_lat_deg = lat_deg;
}

// http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
// make input point
void lnglat_to_tile(double lng_deg, double lat_deg, int zoom, tile_s *out) {
    double lat_rad = degrees_to_radians(lat_deg);
    double n = pow(2.0, zoom);
    out->x = (lng_deg + 180.0) / 360.0 * n;
    out->y = (1.0 - log(tan(lat_rad) + (1 / cos(lat_rad))) / M_PI) / 2.0 * n;
    out->z = zoom;
}

worker_t::result_t workFn(Paparazzi& _map, const std::list<zmq::message_t>& job, void* request_info){
    // false means this is going back to the client, there is no next stage of the pipeline
    worker_t::result_t result{false, {}, ""};

    // This type differs per protocol hence the void* fun
    auto& info = *static_cast<http_request_info_t*>(request_info);

    // Try to generate a response
    http_response_t response;
    try {
        // double start_call = getTime();

        logging::INFO(std::string("Handle request: ") + static_cast<const char*>(job.front().data()));
        //TODO:
        //   - actually use/validate the request parameters
        auto request = http_request_t::from_string(static_cast<const char*>(job.front().data()),
                                                   job.front().size());

        if (request.path == "/check") {
            // ELB check
          response = http_response_t(200, "OK", "OK", headers_t{CORS, TXT_MIME}, "HTTP/1.1");
          response.from_info(info);
          result.messages.emplace_back(response.to_string());
          return result;
        }

        //  SCENE
        //  ---------------------
        auto scene_itr = request.query.find("scene");
        if (scene_itr == request.query.cend() || scene_itr->second.size() == 0) {
            // If there is NO SCENE QUERY value
            if (request.body.empty())
                // if there is not POST body content return error...
                throw std::runtime_error("Missing scene parameter");

            // ... otherwise load content
            _map.setSceneContent(request.body);

            // The size of the custom scene is unique enough
            result.heart_beat = std::to_string(request.body.size());
        } else {
            // If there IS a SCENE QUERRY value load it
            _map.setScene(scene_itr->second.front());

            result.heart_beat = scene_itr->second.front();
        }

        bool size_and_pos = true;
        float pixel_density = 1.0f;

        //  SIZE
        auto width_itr = request.query.find("width");
        if (width_itr == request.query.cend() || width_itr->second.size() == 0)
            size_and_pos = false;
        auto height_itr = request.query.find("height");
        if (height_itr == request.query.cend() || height_itr->second.size() == 0)
            size_and_pos = false;
        auto density_itr = request.query.find("density");
        if (density_itr != request.query.cend() && density_itr->second.size() > 0)
            pixel_density = std::max(1.f,std::stof(density_itr->second.front()));

        //  POSITION
        auto lat_itr = request.query.find("lat");
        if (lat_itr == request.query.cend() || lat_itr->second.size() == 0)
            size_and_pos = false;
        auto lon_itr = request.query.find("lon");
        if (lon_itr == request.query.cend() || lon_itr->second.size() == 0)
            size_and_pos = false;
        auto zoom_itr = request.query.find("zoom");
        if (zoom_itr == request.query.cend() || zoom_itr->second.size() == 0)
            size_and_pos = false;

        std::smatch match;

        if (size_and_pos) {
            // Set Map and OpenGL context size
            _map.setSize(std::stoi(width_itr->second.front()),
                    std::stoi(height_itr->second.front()),
                    pixel_density);
            _map.setPosition(std::stod(lon_itr->second.front()),
                        std::stod(lat_itr->second.front()));
            _map.setZoom(std::stof(zoom_itr->second.front()));

        } else if (std::regex_search(request.path, match, TILE_REGEX) && match.size() == 4) {
            _map.setSize(256, 256, pixel_density);

            int tile_coord[3] = {0,0,0};
            for (int i = 0; i < 3; i++) {
                std::istringstream cur(match.str(i+1));
                cur >> tile_coord[i];
            }
            tile_s tile;
            tile.z = tile_coord[0];
            _map.setZoom(tile.z);

            tile.x = tile_coord[1];
            tile.y = tile_coord[2];

            double n = pow(2, tile.z);
            double lng_deg = (tile.x + 0.5) / n * 360.0 - 180.0;
            double lat_rad = atan(sinh(M_PI * (1 - 2 * (tile.y + 0.5) / n)));
            double lat_deg = radians_to_degrees(lat_rad);

            _map.setPosition(lng_deg, lat_deg);
        } else {
            throw std::runtime_error("Missing parameters to construct image");
        }

        //  OPTIONAL tilt and rotation
        //  ---------------------
        auto tilt_itr = request.query.find("tilt");
        if (tilt_itr != request.query.cend() && tilt_itr->second.size() != 0) {
            // If TILT QUERRY is provided assigned ...
            _map.setTilt(std::stof(tilt_itr->second.front()));
        }
        else {
            // othewise use default (0.)
            _map.setTilt(0.0f);
        }

        auto rotation_itr = request.query.find("rotation");
        if (rotation_itr != request.query.cend() && rotation_itr->second.size() != 0) {
            // If ROTATION QUERRY is provided assigned ...
            _map.setRotation(std::stof(rotation_itr->second.front()));
        }
        else {
            // othewise use default (0.)
            _map.setRotation(0.0f);
        }

        if (!_map.update(MAX_WAITING_TIME)) {
            throw std::runtime_error("Image creation timeout");
        }

        std::string image;

        _map.render(image);

        response = http_response_t(200, "OK", image, headers_t{CORS, PNG_MIME}, "HTTP/1.1");
    }
    catch(const std::exception& e) {
        response = http_response_t(400, "Bad Request", e.what(), headers_t{CORS}, "HTTP/1.1");
    }

    response.from_info(info);
    result.messages.emplace_back(response.to_string());

    return result;
}

void cleanupFn(Paparazzi& _map) {}

int main(int argc, char* argv[]) {
    //we need the location of the proxy and the loopback
    if(argc < 3) {
        logging::ERROR("Usage: " + std::string(argv[0]) +
                       " [tcp|ipc]://upstream_endpoint[:tcp_port] [tcp|ipc]://loopback_endpoint[:tcp_port]");
        return EXIT_FAILURE;
    }

    auto apiKey = getenv("MAPZEN_API_KEY");
    if (!apiKey) {
        logging::ERROR("MAPZEN_API_KEY environment variable is not set");
        return EXIT_FAILURE;
    }

    // Gets requests from the http server
    auto upstream_endpoint = std::string(argv[1]);
    // or returns just location information back to the server
    auto loopback_endpoint = std::string(argv[2]);

    zmq::context_t context;
    Paparazzi paparazzi{};

    paparazzi.setApiKey(apiKey);

    // Listen for requests
    worker_t worker(context,
                    upstream_endpoint,
                    "ipc:///dev/null", // downstream_proxy_endpoint
                    loopback_endpoint, // result_endpoint
                    "ipc:///dev/null", // interrupt_endpoint
                    std::bind(&workFn,
                              std::ref(paparazzi),
                              std::placeholders::_1,
                              std::placeholders::_2),
                    std::bind(&cleanupFn,
                              std::ref(paparazzi)));
    worker.work();

    // terminate on SIGINT
    std::signal(SIGINT, [](int s){ exit(1); });

    return EXIT_SUCCESS;
}
