#include "glfwApp.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stl.h"
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <atomic>
#include "gl.h"

#ifndef BUILD_NUM_STRING
#define BUILD_NUM_STRING ""
#endif

namespace Tangram {

namespace GlfwApp {

// Forward-declare our callback functions.
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorMoveCallback(GLFWwindow* window, double x, double y);
void scrollCallback(GLFWwindow* window, double scrollx, double scrolly);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void dropCallback(GLFWwindow* window, int count, const char** paths);
void framebufferResizeCallback(GLFWwindow* window, int fWidth, int fHeight);

// Forward-declare GUI functions.
void showDebugFlagsGUI();
void showViewportGUI();
void showSceneGUI();

constexpr double double_tap_time = 0.5; // seconds
constexpr double scroll_span_multiplier = 0.05; // scaling for zoom and rotation
constexpr double scroll_distance_multiplier = 5.0; // scaling for shove
constexpr double single_tap_time = 0.25; //seconds (to avoid a long press being considered as a tap)

std::string sceneFile = "scene.yaml";
std::string sceneYaml;
std::string apiKey;

std::string markerStylingPath = "layers.touch.point.draw.icons";
std::string polylineStyle = "{ style: lines, interactive: true, color: red, width: 20px, order: 5000 }";


GLFWwindow* main_window = nullptr;
Tangram::Map* map = nullptr;
int width = 800;
int height = 600;
float density = 1.0;
float pixel_scale = 1.0;
bool recreate_context = false;

bool was_panning = false;
double last_time_released = -double_tap_time; // First click should never trigger a double tap
double last_time_pressed = 0.0;
double last_time_moved = 0.0;
double last_x_down = 0.0;
double last_y_down = 0.0;
double last_x_velocity = 0.0;
double last_y_velocity = 0.0;

bool wireframe_mode = false;
bool show_gui = true;

Tangram::MarkerID marker = 0;
Tangram::MarkerID poiMarker = 0;
Tangram::MarkerID polyline = 0;

std::vector<SceneUpdate> sceneUpdates;

void loadSceneFile(bool setPosition = false, std::vector<SceneUpdate> updates = {}) {

    for (auto& update : updates) {
        bool found = false;
        for (auto& prev : sceneUpdates) {
            if (update.path == prev.path) {
                prev = update;
                found = true;
                break;
            }
        }
        if (!found) { sceneUpdates.push_back(update); }
    }

    if (!sceneYaml.empty()) {
        map->loadSceneYamlAsync(sceneYaml, sceneFile, setPosition, sceneUpdates);
    } else {
        map->loadSceneAsync(sceneFile, setPosition, sceneUpdates);
    }
}

void parseArgs(int argc, char* argv[]) {
    // Load file from command line, if given.
    int argi = 0;
    while (++argi < argc) {
        if (strcmp(argv[argi - 1], "-f") == 0) {
            sceneFile = std::string(argv[argi]);
            LOG("File from command line: %s\n", argv[argi]);
            break;
        }
        if (strcmp(argv[argi - 1], "-s") == 0) {

            if (argi+1 < argc) {
                sceneYaml = std::string(argv[argi]);
                sceneFile = std::string(argv[argi+1]);
                LOG("Yaml from command line: %s, resource path: %s\n",
                    sceneYaml.c_str(), sceneFile.c_str());
            } else {
                LOG("-s options requires YAML string and resource path");
                exit(1);
            }
            break;
        }
    }
}

void setScene(const std::string& _path, const std::string& _yaml) {
    sceneFile = _path;
    sceneYaml = _yaml;
}

void create(std::unique_ptr<Platform> p, int w, int h) {

    width = w;
    height = h;

    if (!glfwInit()) {
        assert(false);
        return;
    }

    char* nextzenApiKeyEnvVar = getenv("NEXTZEN_API_KEY");
    if (nextzenApiKeyEnvVar && strlen(nextzenApiKeyEnvVar) > 0) {
        apiKey = nextzenApiKeyEnvVar;
    } else {
        LOGW("No API key found!\n\nNextzen data sources require an API key. "
             "Sign up for a key at https://developers.nextzen.org/about.html and then set it from the command line with: "
             "\n\n\texport NEXTZEN_API_KEY=YOUR_KEY_HERE"
             "\n\nOr, if using an IDE on macOS, with: "
             "\n\n\tlaunchctl setenv NEXTZEN_API_KEY YOUR_API_KEY\n");
    }

    if (!apiKey.empty()) {
        sceneUpdates.push_back(SceneUpdate("global.sdk_api_key", apiKey));
    }

    // Setup tangram
    if (!map) {
        map = new Tangram::Map(std::move(p));
    }

    // Build a version string for the window title.
    char versionString[256] = { 0 };
    std::snprintf(versionString, sizeof(versionString), "Tangram ES %d.%d.%d " BUILD_NUM_STRING,
        TANGRAM_VERSION_MAJOR, TANGRAM_VERSION_MINOR, TANGRAM_VERSION_PATCH);

    const char* glsl_version = "#version 120";

    // Create a windowed mode window and its OpenGL context
    glfwWindowHint(GLFW_SAMPLES, 2);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    if (!main_window) {
        main_window = glfwCreateWindow(width, height, versionString, NULL, NULL);
    }
    if (!main_window) {
        glfwTerminate();
    }

    // Make the main_window's context current
    glfwMakeContextCurrent(main_window);
    glfwSwapInterval(1); // Enable vsync

    // Set input callbacks
    glfwSetFramebufferSizeCallback(main_window, framebufferResizeCallback);
    glfwSetMouseButtonCallback(main_window, mouseButtonCallback);
    glfwSetCursorPosCallback(main_window, cursorMoveCallback);
    glfwSetScrollCallback(main_window, scrollCallback);
    glfwSetKeyCallback(main_window, keyCallback);
    glfwSetDropCallback(main_window, dropCallback);

    glfwSetCharCallback(main_window, ImGui_ImplGlfw_CharCallback);

    // Setup graphics
    map->setupGL();
    int fWidth = 0, fHeight = 0;
    glfwGetFramebufferSize(main_window, &fWidth, &fHeight);
    framebufferResizeCallback(main_window, fWidth, fHeight);

    // Setup ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    ImGui_ImplGlfw_InitForOpenGL(main_window, false);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup style
    ImGui::StyleColorsDark();

}

void run() {

    loadSceneFile(true);

    double lastTime = glfwGetTime();

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(main_window)) {

        if(show_gui) {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Create ImGui interface.
            // ImGui::ShowDemoWindow();
            showSceneGUI();
            showViewportGUI();
            showDebugFlagsGUI();
        }
        double currentTime = glfwGetTime();
        double delta = currentTime - lastTime;
        lastTime = currentTime;

        // Render
        map->update(delta);
        const bool wireframe = wireframe_mode;
        if(wireframe) {
            glPolygonMode(GL_FRONT, GL_LINE);
            glPolygonMode(GL_BACK, GL_LINE);
        }
        map->render();
        if(wireframe) {
            glPolygonMode(GL_FRONT, GL_FILL);
            glPolygonMode(GL_BACK, GL_FILL);
        }

        if (show_gui) {
            // Render ImGui interface.
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        // Swap front and back buffers
        glfwSwapBuffers(main_window);

        // Poll for and process events
        if (map->getPlatform().isContinuousRendering()) {
            glfwPollEvents();
        } else {
            glfwWaitEvents();
        }
    }
}

void stop(int) {
    if (!glfwWindowShouldClose(main_window)) {
        logMsg("shutdown\n");
        glfwSetWindowShouldClose(main_window, 1);
        glfwPostEmptyEvent();
    } else {
        logMsg("killed!\n");
        exit(1);
    }
}

void destroy() {
    if (map) {
        delete map;
        map = nullptr;
    }
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    if (main_window) {
        glfwDestroyWindow(main_window);
        main_window = nullptr;
    }
    glfwTerminate();
}

template<typename T>
static constexpr T clamp(T val, T min, T max) {
    return val > max ? max : val < min ? min : val;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {

    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse) {
        return; // Imgui is handling this event.
    }

    if (button != GLFW_MOUSE_BUTTON_1) {
        return; // This event is for a mouse button that we don't care about
    }

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    x *= density;
    y *= density;
    double time = glfwGetTime();

    if (was_panning && action == GLFW_RELEASE) {
        was_panning = false;
        auto vx = clamp(last_x_velocity, -2000.0, 2000.0);
        auto vy = clamp(last_y_velocity, -2000.0, 2000.0);
        map->handleFlingGesture(x, y, vx, vy);
        return; // Clicks with movement don't count as taps, so stop here
    }

    if (action == GLFW_PRESS) {
        map->handlePanGesture(0.0f, 0.0f, 0.0f, 0.0f);
        last_x_down = x;
        last_y_down = y;
        last_time_pressed = time;
        return;
    }

    if ((time - last_time_released) < double_tap_time) {
        // Double tap recognized
        const float duration = 0.5f;
        Tangram::LngLat tapped, current;
        map->screenPositionToLngLat(x, y, &tapped.longitude, &tapped.latitude);
        map->getPosition(current.longitude, current.latitude);
        auto pos = map->getCameraPosition();
        pos.zoom += 1.f;
        pos.longitude = 0.5 * (tapped.longitude + current.longitude);
        pos.latitude = 0.5 * (tapped.latitude + current.latitude);

        map->setCameraPositionEased(pos, duration, EaseType::quint);
    } else if ((time - last_time_pressed) < single_tap_time) {
        // Single tap recognized
        Tangram::LngLat p;
        map->screenPositionToLngLat(x, y, &p.longitude, &p.latitude);
        double xx, yy;
        map->lngLatToScreenPosition(p.longitude, p.latitude, &xx, &yy);

        logMsg("------\n");
        logMsg("LngLat: %f, %f\n", p.longitude, p.latitude);
        logMsg("Clicked:  %f, %f\n", x, y);
        logMsg("Remapped: %f, %f\n", xx, yy);

        map->getPlatform().requestRender();
    }

    last_time_released = time;

}

void cursorMoveCallback(GLFWwindow* window, double x, double y) {

    if (ImGui::GetIO().WantCaptureMouse) {
        return; // Imgui is handling this event.
    }

    x *= density;
    y *= density;

    int action = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);
    double time = glfwGetTime();

    if (action == GLFW_PRESS) {

        if (was_panning) {
            map->handlePanGesture(last_x_down, last_y_down, x, y);
        }

        was_panning = true;
        last_x_velocity = (x - last_x_down) / (time - last_time_moved);
        last_y_velocity = (y - last_y_down) / (time - last_time_moved);
        last_x_down = x;
        last_y_down = y;

    }

    last_time_moved = time;

}

void scrollCallback(GLFWwindow* window, double scrollx, double scrolly) {

    ImGui_ImplGlfw_ScrollCallback(window, scrollx, scrolly);
    if (ImGui::GetIO().WantCaptureMouse) {
        return; // Imgui is handling this event.
    }

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    x *= density;
    y *= density;

    bool rotating = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
    bool shoving = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

    if (shoving) {
        map->handleShoveGesture(scroll_distance_multiplier * scrolly);
    } else if (rotating) {
        map->handleRotateGesture(x, y, scroll_span_multiplier * scrolly);
    } else {
        map->handlePinchGesture(x, y, 1.0 + scroll_span_multiplier * scrolly, 0.f);
    }

}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    if (ImGui::GetIO().WantCaptureKeyboard) {
        return; // Imgui is handling this event.
    }

    CameraPosition camera = map->getCameraPosition();

    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_D:
                show_gui = !show_gui;
                break;
            case GLFW_KEY_BACKSPACE:
                recreate_context = true;
                break;
            case GLFW_KEY_R:
                loadSceneFile();
                break;
            case GLFW_KEY_Z: {
                auto pos = map->getCameraPosition();
                pos.zoom += 1.f;
                map->setCameraPositionEased(pos, 1.5f);
                break;
            }
            case GLFW_KEY_N: {
                auto pos = map->getCameraPosition();
                pos.rotation = 0.f;
                map->setCameraPositionEased(pos, 1.f);
                break;
            }
            case GLFW_KEY_S:
                if (pixel_scale == 1.0) {
                    pixel_scale = 2.0;
                } else if (pixel_scale == 2.0) {
                    pixel_scale = 0.75;
                } else {
                    pixel_scale = 1.0;
                }
                map->setPixelScale(pixel_scale);
                break;
            case GLFW_KEY_P:
                loadSceneFile(false, {SceneUpdate{"cameras", "{ main_camera: { type: perspective } }"}});
                break;
            case GLFW_KEY_I:
                loadSceneFile(false, {SceneUpdate{"cameras", "{ main_camera: { type: isometric } }"}});
                break;
            case GLFW_KEY_M:
                map->loadSceneYamlAsync("{ scene: { background: { color: red } } }", std::string(""));
                break;
            case GLFW_KEY_G:
                static bool geoJSON = false;
                if (!geoJSON) {
                    loadSceneFile(false,
                                  { SceneUpdate{"sources.osm.type", "GeoJSON"},
                                    SceneUpdate{"sources.osm.url", "https://tile.mapzen.com/mapzen/vector/v1/all/{z}/{x}/{y}.json"}});
                } else {
                    loadSceneFile(false,
                                  { SceneUpdate{"sources.osm.type", "MVT"},
                                    SceneUpdate{"sources.osm.url", "https://tile.mapzen.com/mapzen/vector/v1/all/{z}/{x}/{y}.mvt"}});
                }
                geoJSON = !geoJSON;
                break;
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, true);
                break;
            case GLFW_KEY_F1:
                map->setPosition(-74.00976419448854, 40.70532700869127);
                map->setZoom(16);
                break;
            case GLFW_KEY_F2:
                map->setPosition(8.82, 53.08);
                map->setZoom(14);
                break;
            case GLFW_KEY_F3:
                camera.longitude = 8.82;
                camera.latitude = 53.08;
                camera.zoom = 16.f;
                map->flyTo(camera, -1.f, 2.0);
                break;
            case GLFW_KEY_F4:
                camera.longitude = 8.82;
                camera.latitude = 53.08;
                camera.zoom = 10.f;
                map->flyTo(camera, -1.f, 2.5);
                break;
            case GLFW_KEY_F5:
                camera.longitude = -74.00976419448854;
                camera.latitude = 40.70532700869127;
                camera.zoom = 16.f;
                map->flyTo(camera, 4.);
                break;
            case GLFW_KEY_F6:
                camera.longitude = -122.41;
                camera.latitude = 37.7749;
                camera.zoom = 16.f;
                map->flyTo(camera, -1.f, 4.0);
                break;
            case GLFW_KEY_F7:
                camera.longitude = 139.839478;
                camera.latitude = 35.652832;
                camera.zoom = 16.f;
                map->flyTo(camera, -1.f, 1.0);
                break;
            case GLFW_KEY_W:
                map->onMemoryWarning();
                break;
        default:
                break;
        }
    }
}

void dropCallback(GLFWwindow* window, int count, const char** paths) {

    sceneFile = "file://" + std::string(paths[0]);
    sceneYaml.clear();

    loadSceneFile();
}

void framebufferResizeCallback(GLFWwindow* window, int fWidth, int fHeight) {

    int wWidth = 0, wHeight = 0;
    glfwGetWindowSize(main_window, &wWidth, &wHeight);
    float new_density = (float)fWidth / (float)wWidth;
    if (new_density != density) {
        recreate_context = true;
        density = new_density;
    }
    map->setPixelScale(density);
    map->resize(fWidth, fHeight);
}

void showSceneGUI() {
    if (ImGui::CollapsingHeader("Scene")) {
        if (ImGui::InputText("Scene URL", &sceneFile, ImGuiInputTextFlags_EnterReturnsTrue)) {
            loadSceneFile();
        }
        if (ImGui::InputText("API key", &apiKey, ImGuiInputTextFlags_EnterReturnsTrue)) {
            loadSceneFile();
        }
        if (ImGui::Button("Reload Scene")) {
            loadSceneFile();
        }
    }
}

void showViewportGUI() {
    if (ImGui::CollapsingHeader("Viewport")) {
        CameraPosition camera = map->getCameraPosition();
        float lngLatZoom[3] = {static_cast<float>(camera.longitude), static_cast<float>(camera.latitude), camera.zoom};
        if (ImGui::InputFloat3("Lng/Lat/Zoom", lngLatZoom, "%.5f", ImGuiInputTextFlags_EnterReturnsTrue)) {
            camera.longitude = lngLatZoom[0];
            camera.latitude = lngLatZoom[1];
            camera.zoom = lngLatZoom[2];
            map->setCameraPosition(camera);
        }
        if (ImGui::SliderAngle("Tilt", &camera.tilt, 0.f, 90.f)) {
            map->setCameraPosition(camera);
        }
        if (ImGui::SliderAngle("Rotation", &camera.rotation, 0.f, 360.f)) {
            map->setCameraPosition(camera);
        }
    }
}

void showDebugFlagsGUI() {
    if (ImGui::CollapsingHeader("Debug Flags")) {
        bool flag;
        flag = getDebugFlag(DebugFlags::freeze_tiles);
        if (ImGui::Checkbox("Freeze Tiles", &flag)) {
            setDebugFlag(DebugFlags::freeze_tiles, flag);
        }
        flag = getDebugFlag(DebugFlags::proxy_colors);
        if (ImGui::Checkbox("Recolor Proxy Tiles", &flag)) {
            setDebugFlag(DebugFlags::proxy_colors, flag);
        }
        flag = getDebugFlag(DebugFlags::tile_bounds);
        if (ImGui::Checkbox("Show Tile Bounds", &flag)) {
            setDebugFlag(DebugFlags::tile_bounds, flag);
        }
        flag = getDebugFlag(DebugFlags::tile_infos);
        if (ImGui::Checkbox("Show Tile Info", &flag)) {
            setDebugFlag(DebugFlags::tile_infos, flag);
        }
        flag = getDebugFlag(DebugFlags::labels);
        if (ImGui::Checkbox("Show Label Debug Info", &flag)) {
            setDebugFlag(DebugFlags::labels, flag);
        }
        flag = getDebugFlag(DebugFlags::tangram_infos);
        if (ImGui::Checkbox("Show Map Info", &flag)) {
            setDebugFlag(DebugFlags::tangram_infos, flag);
        }
        flag = getDebugFlag(DebugFlags::draw_all_labels);
        if (ImGui::Checkbox("Show All Labels", &flag)) {
            setDebugFlag(DebugFlags::draw_all_labels, flag);
        }
        flag = getDebugFlag(DebugFlags::tangram_stats);
        if (ImGui::Checkbox("Show Frame Stats", &flag)) {
            setDebugFlag(DebugFlags::tangram_stats, flag);
        }
        flag = getDebugFlag(DebugFlags::selection_buffer);
        if (ImGui::Checkbox("Show Selection Buffer", &flag)) {
            setDebugFlag(DebugFlags::selection_buffer, flag);
        }
        ImGui::Checkbox("Wireframe Mode", &wireframe_mode);
    }
}

} // namespace GlfwApp

} // namespace Tangram
