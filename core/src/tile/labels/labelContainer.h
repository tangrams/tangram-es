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
    bool addLabel(const TileID& _tileID, const std::string& _styleName, LabelTransform _transform, std::string _text, Label::Type _type);

    /* Clean all labels for a specific <tileID> */
    void removeLabels(const TileID& _tileID);

    void setFontContext(std::shared_ptr<FontContext> _ftContext) { m_ftContext = _ftContext; }

    /* Returns a const reference to a pointer of the font context */
    const std::shared_ptr<FontContext>& getFontContext() { return m_ftContext; }

    /* Returns a const list of labels for a <TileID> and a style name */
    const std::vector<std::shared_ptr<Label>>& getLabels(const std::string& _styleName, const TileID& _tileID);
    
    void updateOcclusions();

private:

    LabelContainer();
    // map of <Style>s containing all <Label>s by <TileID>s
    std::map<std::string, std::map<TileID, std::vector<std::shared_ptr<Label>>>> m_labels;

    // reference to the <FontContext>
    std::shared_ptr<FontContext> m_ftContext;

};
