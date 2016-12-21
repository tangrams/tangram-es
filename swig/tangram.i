
%module tangram
%{
#define SWIG_FILE_WITH_INIT
//#define SWIG_PYTHON_EXTRA_NATIVE_CONTAINERS

#include "platform_linux.h"
#include "debug/textDisplay.h"
#include "log.h"
#include "data/propertyItem.h"
#include "data/properties.h"
#include "tile/tileID.h"
#include "util/mapProjection.h"
#include "util/inputHandler.h"
#include "view/view.h"
#include "gl/renderState.h"
#include "gl/framebuffer.h"
#include "gl/primitives.h"
#include "gl/hardware.h"
#include "scene/scene.h"
#include "scene/sceneLoader.h"
#include "tile/tileWorker.h"
#include "tile/tileManager.h"
#include "debug/frameInfo.h"
#include "marker/marker.h"
#include "marker/markerManager.h"
#include "labels/labels.h"
%}

%include "typemaps.i"
%include "std_common.i"
%include "std_string.i"
%include "std_shared_ptr.i"

typedef int GLsizei;
typedef int GLint;

/// Python style names
%rename("%(undercase)s", %$isfunction) "";
%rename("%(undercase)s", %$isvariable) "";
%rename("%(uppercase)s", %$isenumitem) "";
//%rename("%(uppercase)s", %$isenum) "";
%rename(EASE) EaseType;
%rename(CAMERA) CameraType;

/// Tangram::Map API
%include "tangram_api.i"


%shared_ptr(Tangram::Tile);
%shared_ptr(Tangram::Scene);

%ignore Tangram::RenderState::jobQueue;
%ignore Tangram::RenderState::attributeBindings;
%include "gl/renderState.h"

/// No default constructors needed yet
%ignore Tangram::TileID::TileID;
%ignore Tangram::Tile::Tile;
%ignore Tangram::Style::Style;
%ignore Tangram::DataSource::DataSource;
%ignore Tangram::Label::Label;


namespace Tangram {
struct BoundingBox {};
class DataSource {};
class Label {};

struct TileID {
    TileID(int32_t _x, int32_t _y, int32_t _z);
};

class Tile {
public:
    void update(float dt, const Tangram::View& view);
};
}

%include "util/mapProjection.h"
%include "view/view.h"

namespace Tangram {
class Scene {
public:
    std::vector<std::shared_ptr<DataSource>>& dataSources();
    //const std::vector<std::unique_ptr<Style>>& styles();
};
class Style {
public:
    void onBeginUpdate();
    void onBeginFrame(RenderState& rs);
    void onBeginDrawFrame(RenderState& rs, const View& _view, Scene& _scene);
    void onEndDrawFrame();
    void draw(RenderState& rs, const Tile& _tile);
};
}

%include "basic_vector.i"
BASIC_VECTOR(TileVector, std::vector<std::shared_ptr<Tangram::Tile>>);
BASIC_VECTOR(StyleVector, std::vector<Tangram::Style*>);

%extend Tangram::Scene {
    bool isLoading() {
        return $self->pendingTextures || $self->pendingFonts;
    }

    // TODO wrapper for std::vector<std::unique_ptr<...>>
    std::vector<std::unique_ptr<Tangram::Style>>& style_set() {
        return $self->styles();
    }
    std::vector<Tangram::Style*> styles() {
        const auto& s = $self->styles();
        std::vector<Tangram::Style*> out;
        out.reserve(s.size());
        for (auto& i : s) out.push_back(i.get());
        return out;
    }
}

%ignore Tangram::TileWorker::enqueue;
%include "tile/tileWorker.h"

namespace Tangram {
class TileManager {
public:

    TileManager(TileWorker& _tileWorker);

    void setDataSources(const std::vector<std::shared_ptr<DataSource>>& _sources);
    void updateTileSets(const ViewState& _view, const std::set<TileID>& _visibleTiles);
    const std::vector<std::shared_ptr<Tile>>& getVisibleTiles();
    void clearTileSets();
    void clearTileSet(int32_t _sourceId);
    bool hasTileSetChanged();
    bool hasLoadingTiles();
};
}
%extend Tangram::TileManager {
    Tangram::TileCache& getTileCache() {
        return *($self->getTileCache());
    }
}

%include "util/inputHandler.h"
%include "marker/markerManager.h"
%include "labels/labels.h"


/// Global helper methods
%{
std::shared_ptr<Tangram::Scene> load_scene(const char* scenePath) {
    auto scene = std::make_shared<Tangram::Scene>(scenePath);
    Tangram::SceneLoader::loadScene(scene);
    return scene;
}

void init_globals() {
    Tangram::Hardware::loadExtensions();
    Tangram::Hardware::loadCapabilities();
    Tangram::Hardware::printAvailableExtensions();
}

void teardown_globals() {
    Tangram::TextDisplay::Instance().deinit();
    Tangram::Primitives::deinit();
}

void frame_buffer_apply(Tangram::RenderState& rs, Tangram::View& view, Tangram::Scene& scene) {
    using namespace Tangram;
    Primitives::setResolution(rs, view.getWidth(), view.getHeight());

    glm::vec2 viewport(view.getWidth(), view.getHeight());
    FrameBuffer::apply(rs, rs.defaultFrameBuffer(),
                       viewport, scene.background().asIVec4());
}

void frame_info_draw(Tangram::RenderState& rs, Tangram::View& view, Tangram::TileManager& tileManager) {
    Tangram::FrameInfo::draw(rs, view, tileManager);
}

void poke_network_queue() {
    processNetworkQueue();
}

void drain_network_queue() {
    finishUrlRequests();
}
%}

std::shared_ptr<Tangram::Scene> load_scene(const char* scenePath);
void init_globals();
void teardown_globals();
void frame_buffer_apply(Tangram::RenderState& rs, Tangram::View& view, Tangram::Scene& scene);
void frame_info_draw(Tangram::RenderState& rs, Tangram::View& view, Tangram::TileManager& tileManager);
void poke_network_queue();
void drain_network_queue();

