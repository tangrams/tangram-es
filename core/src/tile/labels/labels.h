#pragma once

#include "label.h"
#include "util/tileID.h"

#include <memory>
#include <mutex>
#include <vector>

class FontContext;
class MapTile;
class View;

struct LabelUnit {

private:
    std::weak_ptr<Label> m_label;

public:
    TileID m_tileID;
    std::string m_styleName;

    LabelUnit(std::shared_ptr<Label>& _label, const TileID& _tileID, std::string _styleName)
        : m_label(_label),
          m_tileID(_tileID),
          m_styleName(std::move(_styleName)) {}

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
     * Creates a label for and associate it with the current processed <MapTile> TileID for a specific syle name
     * Returns true if label was created
     */
    bool addLabel(MapTile& _tile, const std::string& _styleName, Label::Transform _transform, std::string _text, Label::Type _type);

    void setFontContext(std::shared_ptr<FontContext> _ftContext) { m_ftContext = _ftContext; }

    /* Returns a const reference to a pointer of the font context */
    const std::shared_ptr<FontContext>& getFontContext() { return m_ftContext; }

    void updateOcclusions();

    void setView(std::shared_ptr<View> _view) { m_view = _view; }

    void setScreenSize(int _width, int _height) { m_screenSize = glm::vec2(_width, _height); }

    void drawDebug();

private:

    int LODDiscardFunc(float _maxZoom, float _zoom);

    Labels();
    std::vector<std::unique_ptr<LabelUnit>> m_labelUnits;
    std::vector<std::unique_ptr<LabelUnit>> m_pendingLabelUnits;

    // reference to the <FontContext>
    std::shared_ptr<FontContext> m_ftContext;

    std::mutex m_mutex;

    glm::vec2 m_screenSize;
    std::shared_ptr<View> m_view;
    float m_currentZoom;

};
