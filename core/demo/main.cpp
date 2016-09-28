#include <memory>


#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>
#include <signal.h>
#include <string>
#include <string.h>
#include <fstream>
#include <sstream>
#include <thread>

#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <tangram-core.h>
#include <stdarg.h>  // For va_start, etc.
#include <memory>    // For std::unique_ptr
#include "urlGet.h"

// Forward declaration
void init_main_window(bool recreate);

std::string sceneFile = "scene.yaml";

GLFWwindow* main_window = nullptr;
tangram_map_t map = nullptr;
int width = 800;
int height = 600;
bool recreate_context;
float pixel_scale = 1.0;

// Input handling
// ==============

const double double_tap_time = 0.5; // seconds
const double scroll_span_multiplier = 0.05; // scaling for zoom and rotation
const double scroll_distance_multiplier = 5.0; // scaling for shove
const double single_tap_time = 0.25; //seconds (to avoid a long press being considered as a tap)

bool was_panning = false;
double last_time_released = -double_tap_time; // First click should never trigger a double tap
double last_time_pressed = 0.0;
double last_time_moved = 0.0;
double last_x_down = 0.0;
double last_y_down = 0.0;
double last_x_velocity = 0.0;
double last_y_velocity = 0.0;
bool scene_editing_mode = false;

tangram_data_source_t data_source;
TangramLngLat last_point = { 0, 0 };

template<typename T>
static constexpr T clamp(T val, T min, T max) {
    return val > max ? max : val < min ? min : val;
}
const char* GeoJSONLineTemplate = R"json(
{
  "type": "Feature",
  "geometry": {
    "type": "LineString",
    "coordinates": [
       %s
    ]
  },
  "properties": {
    %s
  }
}
)json";

const char* GeoJSONPointTemplate = R"json(
{
  "type": "Feature",
  "geometry": {
    "type": "Point",
    "coordinates": [%lf, %lf]
  },
  "properties": {
    %s
  }
}
)json";

std::string string_format(const char* fmt_str, ...) {
  int n = strlen(fmt_str) + 1; /* Reserve two times as much as the length of the fmt_str */
  std::string formatted;
  va_list ap;
  va_start(ap, fmt_str);
  int sz = vsnprintf(NULL, 0, fmt_str, ap);
  va_end(ap);
  do {
    if (sz >= 0) {
      n = sz + 1;
    } else {
      n = n << 1;
    }
    formatted.resize(n); /* Wrap the plain char array into the unique_ptr */
    va_start(ap, fmt_str);
    sz = vsnprintf(&formatted.front(), n, fmt_str, ap);
    va_end(ap);
  } while (sz < 0 || sz > n);
  formatted.resize(sz);
  return std::move(formatted);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

    if (button != GLFW_MOUSE_BUTTON_1) {
        return; // This event is for a mouse button that we don't care about
    }

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    double time = glfwGetTime();

    if (was_panning) {
        was_panning = false;
        auto vx = clamp(last_x_velocity, -2000.0, 2000.0);
        auto vy = clamp(last_y_velocity, -2000.0, 2000.0);
        tangramHandleFlingGesture(map, x, y, vx, vy);
        return; // Clicks with movement don't count as taps, so stop here
    }

    if (action == GLFW_PRESS) {
        tangramHandlePanGesture(map, 0.0f, 0.0f, 0.0f, 0.0f);
        last_x_down = x;
        last_y_down = y;
        last_time_pressed = time;
        return;
    }

    if ((mods & GLFW_MOD_CONTROL) && (action == GLFW_RELEASE)) {
      TangramLngLat p;
      tangramScreenPositionToLngLat(map, x, y, &p.longitude, &p.latitude);
      tangramLogMsg("pick feature\n");

      tangramPickFeaturesAt(map, x, y, [](TangramTouchItemArray picks) {
          tangramLogMsg("picked %d features\n", picks.size);
          for (size_t i = 0; i < picks.size; ++i) {
              TangramTouchItem touchItem = ((TangramTouchItem*)picks.data)[i];
              TangramString properties = touchItem.properties;
              std::string propertiesJSON((char*)properties.data, properties.size);
              tangramLogMsg(" - %f\t [%f,%f] %s\n", touchItem.distance, touchItem.position[0], touchItem.position[1], propertiesJSON.c_str());
          }
      });
    }
    if ((mods & GLFW_MOD_SHIFT) && (action == GLFW_RELEASE)) {
        TangramLngLat p1;
        tangramScreenPositionToLngLat(map, x, y, &p1.longitude, &p1.latitude);

        if (!(last_point == TangramLngLat{ 0, 0 })) {
          TangramLngLat p2 = last_point;

          tangramLogMsg("add line %f %f - %f %f\n",
            p1.longitude, p1.latitude,
            p2.longitude, p2.latitude);
          std::string lineCoordinates = string_format("[%lf,%lf],\n[%lf, %lf]", p1.longitude, p1.latitude, p2.longitude, p2.latitude);
          std::string lineJSON = string_format(GeoJSONLineTemplate, lineCoordinates.c_str(), R"quote("type":"line")quote");
          tangramDataSourceAddGeoJSON(data_source, lineJSON.data(), lineJSON.length());

          std::string pointJSON = string_format(GeoJSONPointTemplate, p2.longitude, p2.latitude, R"quote("type":"point")quote");
          tangramDataSourceAddGeoJSON(data_source, pointJSON.data(), pointJSON.length());
        }
        last_point = p1;

        // This updates the tiles (maybe we need a recalcTiles())
        glfwPostEmptyEvent();
    }
}

void cursor_pos_callback(GLFWwindow* window, double x, double y) {

    int action = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);
    double time = glfwGetTime();

    if (action == GLFW_PRESS) {

        if (was_panning) {
            tangramHandlePanGesture(map, last_x_down, last_y_down, x, y);
        }

        was_panning = true;
        last_x_velocity = (x - last_x_down) / (time - last_time_moved);
        last_y_velocity = (y - last_y_down) / (time - last_time_moved);
        last_x_down = x;
        last_y_down = y;
    }

    last_time_moved = time;

}

void scroll_callback(GLFWwindow* window, double scrollx, double scrolly) {

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    bool rotating = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
    bool shoving = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

    if (shoving) {
        tangramHandleShoveGesture(map, scroll_distance_multiplier * scrolly);
    } else if (rotating) {
        tangramHandleRotateGesture(map, x, y, scroll_span_multiplier * scrolly);
    } else {
        tangramHandlePinchGesture(map, x, y, 1.0 + scroll_span_multiplier * scrolly, 0.f);
    }

}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_1:
                tangramToggleDebugFlag(TangramDebugFlagFreezeTiles);
                break;
            case GLFW_KEY_2:
                tangramToggleDebugFlag(TangramDebugFlagProxyColors);
                break;
            case GLFW_KEY_3:
                tangramToggleDebugFlag(TangramDebugFlagTileBounds);
                break;
            case GLFW_KEY_4:
                tangramToggleDebugFlag(TangramDebugFlagTileInfos);
                break;
            case GLFW_KEY_5:
                tangramToggleDebugFlag(TangramDebugFlagLabels);
                break;
            case GLFW_KEY_6:
                tangramToggleDebugFlag(TangramDebugFlagDrawAllLabels);
                break;
            case GLFW_KEY_7:
                tangramToggleDebugFlag(TangramDebugFlagTangramInfos);
                break;
            case GLFW_KEY_8:
                tangramToggleDebugFlag(TangramDebugFlagTangramStats);
                break;
            case GLFW_KEY_R:
                tangramLoadSceneAsync(map, sceneFile.c_str(), true, nullptr, nullptr);
                break;
            case GLFW_KEY_E:
                if (scene_editing_mode) {
                    scene_editing_mode = false;
                    tangramSetContinuousRendering(false);
                    glfwSwapInterval(0);
                } else {
                    scene_editing_mode = true;
                    tangramSetContinuousRendering(true);
                    glfwSwapInterval(1);
                }
                tangramLoadSceneAsync(map, sceneFile.c_str(), true, nullptr, nullptr);
                break;
            case GLFW_KEY_BACKSPACE:
                recreate_context = true;
                break;
            case GLFW_KEY_N:
                tangramSetRotationEased(map, tangramGetRotation(map) + 3.14/8, 1.f, TangramEaseQuint);
                break;
            case GLFW_KEY_S:
                if (pixel_scale == 1.0) {
                    pixel_scale = 2.0;
                } else if (pixel_scale == 2.0) {
                    pixel_scale = 0.75;
                } else {
                    pixel_scale = 1.0;
                }
                tangramLoadSceneAsync(map, sceneFile.c_str(), true, nullptr, nullptr);
                tangramSetPixelScale(map, pixel_scale);

                break;
            case GLFW_KEY_P:
                tangramQueueSceneUpdate(map, "cameras", "{ main_camera: { type: perspective } }");
                tangramApplySceneUpdates(map);
                break;
            case GLFW_KEY_I:
                tangramQueueSceneUpdate(map, "cameras", "{ main_camera: { type: isometric } }");
                tangramApplySceneUpdates(map);
                break;
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(main_window, true);
                break;
            default:
                break;
        }
    }
}

void drop_callback(GLFWwindow* window, int count, const char** paths) {
    sceneFile = std::string(paths[0]);
    tangramLoadSceneAsync(map, sceneFile.c_str(), true, nullptr, nullptr);
}

// Window handling
// ===============

void window_size_callback(GLFWwindow* window, int width, int height) {
    tangramResize(map, width, height);
}

void init_main_window(bool recreate) {

    // Setup tangram
    if (!map) {
        map = tangramMapCreate();
        tangramLoadSceneAsync(map, sceneFile.c_str(), true, [](void*)->void {
            tangramLogMsg("Load finished\n");
        }, nullptr);
    }

    if (!recreate) {
        // Destroy old window
        if (main_window != nullptr) {
            glfwDestroyWindow(main_window);
        }

        // Create a windowed mode window and its OpenGL context
        glfwWindowHint(GLFW_SAMPLES, 2);
        main_window = glfwCreateWindow(width, height, "Tangram ES", NULL, NULL);
        if (!main_window) {
            glfwTerminate();
        }

        // Make the main_window's context current
        glfwMakeContextCurrent(main_window);

        // Set input callbacks
        glfwSetWindowSizeCallback(main_window, window_size_callback);
        glfwSetMouseButtonCallback(main_window, mouse_button_callback);
        glfwSetCursorPosCallback(main_window, cursor_pos_callback);
        glfwSetScrollCallback(main_window, scroll_callback);
        glfwSetKeyCallback(main_window, key_callback);
        glfwSetDropCallback(main_window, drop_callback);
    }

    // Setup graphics
    tangramSetupGL(map);
    tangramResize(map, width, height);

    data_source = tangramDataSourceCreate("touch", "", 18);
    tangramAddDataSource(map, data_source);
}

// Main program
// ============

int main(int argc, char* argv[]) {
    std::thread fetcherThread(tangamUrlFetcherRunner);
    // Initialize cURL
    tangramRegisterUrlFetcher(3, tangamUrlFetcherDefault, tangamUrlCancelerDefault);

    static bool keepRunning = true;

    // Give it a chance to shutdown cleanly on CTRL-C
    signal(SIGINT, [](int) {
            if (keepRunning) {
                tangramLogMsg("shutdown\n");
                keepRunning = false;
                glfwPostEmptyEvent();
            } else {
                tangramLogMsg("killed!\n");
                exit(1);
            }});

    int argi = 0;
    while (++argi < argc) {
        if (strcmp(argv[argi - 1], "-f") == 0) {
            sceneFile = std::string(argv[argi]);
            tangramLogMsg("File from command line: %s\n", argv[argi]);
            break;
        }
    }

    // Initialize the windowing library
    if (!glfwInit()) {
        return -1;
    }
#define USING_OPENGL_ES 0
#if USING_OPENGL_ES
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#else
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
#endif
    tangramInit(glfwPostEmptyEvent);

    struct stat sb {0};
    auto last_mod = sb.st_mtime;

    init_main_window(false);

    double lastTime = glfwGetTime();

    tangramSetContinuousRendering(false);
    glfwSwapInterval(0);

    recreate_context = false;

    // Loop until the user closes the window
    while (keepRunning && !glfwWindowShouldClose(main_window)) {

        double currentTime = glfwGetTime();
        double delta = currentTime - lastTime;
        lastTime = currentTime;

        // Render
        tangramUpdate(map, delta);
        tangramRender(map);

        // Swap front and back buffers
        glfwSwapBuffers(main_window);

        // Poll for and process events
        if (tangramIsContinuousRendering()) {
            glfwPollEvents();
        } else {
            glfwWaitEvents();
        }

        if (recreate_context) {
            tangramLogMsg("recreate context\n");
             // Simulate GL context loss
            init_main_window(true);
            recreate_context = false;
        }

        if (scene_editing_mode) {
            //if (stat(sceneFile.c_str(), &sb) == 0) {
                if (last_mod != sb.st_mtime) {
                    tangramLoadSceneAsync(map, sceneFile.c_str(), true, nullptr, nullptr);
                    last_mod = sb.st_mtime;
                }
            //}
        }
    }

    if (map) {
        tangramMapDestroy(map);
        map = nullptr;
    }
    fetcherThread.join();

    glfwTerminate();
    return 0;
}
