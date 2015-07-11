#pragma once

#include "label.h"
#include "textLabel.h"
#include "spriteLabel.h"
#include "util/tileID.h"
//#include "style/textStyle.h"

#include <memory>
#include <mutex>
#include <vector>

class FontContext;
class MapTile;
class View;
class TextBatch;

struct LabelUnit {

private:
    std::weak_ptr<Label> m_label;

public:
    std::unique_ptr<TileID> m_tileID;
    std::string m_styleName;

    LabelUnit(std::shared_ptr<Label>& _label, std::unique_ptr<TileID>& _tileID, const std::string& _styleName) :
        m_label(std::move(_label)), m_tileID(std::move(_tileID)), m_styleName(_styleName) {}

    LabelUnit(LabelUnit&& _other) : m_label(std::move(_other.m_label)), m_tileID(std::move(_other.m_tileID)), m_styleName(_other.m_styleName) {}

    LabelUnit& operator=(LabelUnit&& _other) {
        m_label = std::move(_other.m_label);
        m_tileID = std::move(_other.m_tileID);
        m_styleName = std::move(_other.m_styleName);
        return *this;
    }

    // Could return a null pointer
    std::shared_ptr<Label> getWeakLabel() { return m_label.lock(); }
};


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
    std::shared_ptr<Label> addTextLabel(MapTile& _tile, TextBatch& _buffer, const std::string& _styleName, Label::Transform _transform, std::string _text, Label::Type _type);

    /*
     * Creates a sprite slabel for and associate it with the current processed <MapTile> TileID for a specific syle name
     * Returns the created labe
     */
    std::shared_ptr<Label> addSpriteLabel(MapTile& _tile, const std::string& _styleName, Label::Transform _transform, const glm::vec2& _size,
                                          const glm::vec2& _offset, size_t _bufferOffset);

    void setFontContext(std::shared_ptr<FontContext> _ftContext) { m_ftContext = _ftContext; }

    /* Returns a const reference to a pointer of the font context */
    std::shared_ptr<FontContext>& getFontContext() { return m_ftContext; }

    void updateOcclusions();

    void setView(std::shared_ptr<View> _view) { m_view = _view; }

    void setScreenSize(int _width, int _height) { m_screenSize = glm::vec2(_width, _height); }

    void drawDebug();

private:

    void addLabel(MapTile& _tile, const std::string& _styleName, std::shared_ptr<Label> _label);

    int LODDiscardFunc(float _maxZoom, float _zoom);

    Labels();
    std::vector<LabelUnit> m_labelUnits;
    std::vector<LabelUnit> m_pendingLabelUnits;

    // reference to the <FontContext>
    std::shared_ptr<FontContext> m_ftContext;

    std::mutex m_mutex;

    glm::vec2 m_screenSize;
    std::shared_ptr<View> m_view;
    float m_currentZoom;

};
