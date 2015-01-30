#pragma once

#include "label.h"
#include "util/tileID.h"
#include "text/fontContext.h"
#include <memory>
#include <vector>
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
    std::shared_ptr<Label> addLabel(const std::string& _styleName, LabelTransform _transform, std::string _text);

    /* Clean all labels for a specific <tileID> */
    void removeLabels(const TileID& _tileID);

    void setFontContext(std::shared_ptr<FontContext> _ftContext) { m_ftContext = _ftContext; }

    // FUTURE : 
    // QuadTree structure used to iterate through labels

    /* Returns a const reference to a pointer of the font context */
    const std::shared_ptr<FontContext>& getFontContext() { return m_ftContext; }

    /* Returns a const list of labels for a <TileID> and a style name */
    const std::vector<std::shared_ptr<Label>>& getLabels(const std::string& _styleName, const TileID& _tileID);

    /*
     * A pointer to the tile being currently processed, e.g. the tile which data is being added to 
     * nullptr if no tile is being processed
     */
    MapTile* processedTile;

private:

    LabelContainer();
    std::map<std::string, std::map<TileID, std::vector<std::shared_ptr<Label>>>> m_labels;
    std::shared_ptr<FontContext> m_ftContext;

};
