#pragma once

#include "label.h"
#include "util/tileID.h"
#include "text/fontContext.h"
#include "isect2d.h"
#include <memory>
#include <vector>
#include <set>
#include <map>

class MapTile;

struct LabelUnit {
    
private:
    std::weak_ptr<Label> m_label;
    
public:
    std::unique_ptr<TileID> m_tileID;
    std::string m_styleName;
    
    LabelUnit(std::shared_ptr<Label>& _label, std::unique_ptr<TileID>& _tileID, const std::string& _styleName) : m_label(std::move(_label)), m_tileID(std::move(_tileID)), m_styleName(_styleName) {}
    
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

class LabelContainer {

public:

    static std::shared_ptr<LabelContainer> GetInstance() {
        static std::shared_ptr<LabelContainer> instance(new LabelContainer());
        return instance;
    }

    virtual ~LabelContainer();

    /*
     * Creates a label for and associate it with the current processed <MapTile> TileID for a specific syle name
     * Returns nullptr if no text buffer are currently used by the FontContext
     */
    bool addLabel(MapTile& _tile, const std::string& _styleName, LabelTransform _transform, std::string _text, Label::Type _type);

    void setFontContext(std::shared_ptr<FontContext> _ftContext) { m_ftContext = _ftContext; }

    /* Returns a const reference to a pointer of the font context */
    const std::shared_ptr<FontContext>& getFontContext() { return m_ftContext; }

    void updateOcclusions();
    
    void setViewProjectionMatrix(glm::mat4 _viewProjection) { m_viewProjection = _viewProjection; }
    
    void setScreenSize(int _width, int _height) { m_screenSize = glm::vec2(_width, _height); }

private:

    LabelContainer();
    std::vector<LabelUnit> m_labelUnits;
    std::vector<LabelUnit> m_pendingLabelUnits;

    // reference to the <FontContext>
    std::shared_ptr<FontContext> m_ftContext;
    
    std::mutex m_mutex;
    
    glm::mat4 m_viewProjection;
    glm::vec2 m_screenSize;

};
