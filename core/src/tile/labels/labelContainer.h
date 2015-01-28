#pragma once

#include "label.h"
#include "text/fontContext.h"
#include <memory>
#include <set>

class LabelContainer {

public:
    
    static std::shared_ptr<LabelContainer> GetInstance() {
        static std::shared_ptr<LabelContainer> instance(new LabelContainer());
        return instance;
    }

    virtual ~LabelContainer();
    std::shared_ptr<Label> addLabel(LabelTransform _transform, std::string _text);
    void setFontContext(std::shared_ptr<FontContext> _ftContext) { m_ftContext = _ftContext; }

    // FUTURE : 
    // QuadTree structure used to iterate through labels

    const std::shared_ptr<FontContext>& getFontContext() { return m_ftContext; }

private:
    
    LabelContainer();
    std::set<std::shared_ptr<Label>> m_labels;
    std::shared_ptr<FontContext> m_ftContext;

};
