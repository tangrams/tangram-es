//
// Created by Matt Blair on 2019-03-07.
//

#pragma once

namespace Tangram {

class OnFrameListener {

public:

    virtual ~OnFrameListener() = default;

    virtual void onAdded() = 0;

    virtual void onFrame() = 0;

    virtual void onRemoved() = 0;
};

} // namespace Tangram
