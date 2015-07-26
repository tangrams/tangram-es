#pragma once

#include "label.h"
#include "textLabel.h"
#include "spriteLabel.h"
#include "tile/tileID.h"

#include <memory>
#include <mutex>
#include <vector>
#include <map>

namespace Tangram {

class FontContext;
class Tile;
class View;
class Style;


/*
 * Singleton class containing all labels
 */

class Labels {

public:

    static std::shared_ptr<Labels> GetInstance() {
        static std::shared_ptr<Labels> instance(new Labels());
        return instance;
    }

    virtual ~Labels();

    /*
     * Creates a text slabel for and associate it with the current processed <MapTile> TileID for a specific syle name
     * Returns the created label
     */
    std::shared_ptr<Label> addTextLabel(Tile& _tile, TextBuffer& _buffer, const std::string& _styleName,
                                        Label::Transform _transform, std::string _text, Label::Type _type);

    /*
     * Creates a sprite slabel for and associate it with the current processed <MapTile> TileID for a specific syle name
     * Returns the created labe
     */
    std::shared_ptr<Label> addSpriteLabel(Tile& _tile, const std::string& _styleName, Label::Transform _transform,
                                          const glm::vec2& _size, size_t _bufferOffset);

    void setFontContext(std::shared_ptr<FontContext> _ftContext) { m_ftContext = _ftContext; }

    /* Returns a const reference to a pointer of the font context */
    const std::shared_ptr<FontContext>& getFontContext() { return m_ftContext; }


    void setView(std::shared_ptr<View> _view) { m_view = _view; }

    void drawDebug();

    void update(float _dt, const std::vector<std::unique_ptr<Style>>& _styles, const std::map<TileID, std::shared_ptr<Tile>>& _tiles);

    void lazyRenderRequest();

    bool needUpdate() { return Label::s_needUpdate; }

private:

    void updateOcclusions();

    void addLabel(Tile& _tile, const std::string& _styleName, std::shared_ptr<Label> _label);

    int LODDiscardFunc(float _maxZoom, float _zoom);

    Labels();
    std::vector<std::weak_ptr<Label>> m_labels;
    std::vector<std::weak_ptr<Label>> m_pendingLabels;

    // reference to the <FontContext>
    std::shared_ptr<FontContext> m_ftContext;

    std::mutex m_mutex;

    std::shared_ptr<View> m_view;
    float m_currentZoom;

};

}
