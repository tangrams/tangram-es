#include "debugFlags.h"

#include <bitset>

namespace Tangram {

static std::bitset<9> g_flags = 0;

void setDebugFlag(DebugFlags _flag, bool _on) {
    g_flags.set(_flag, _on);
    // m_view->setZoom(m_view->getZoom()); // Force the view to refresh
}

bool getDebugFlag(DebugFlags _flag) {
    return g_flags.test(_flag);
}

void toggleDebugFlag(DebugFlags _flag) {
    g_flags.flip(_flag);
    // m_view->setZoom(m_view->getZoom()); // Force the view to refresh

    // Rebuild tiles for debug modes that needs it
    // if (_flag == DebugFlags::proxy_colors
    //  || _flag == DebugFlags::draw_all_labels
    //  || _flag == DebugFlags::tile_bounds
    //  || _flag == DebugFlags::tile_infos) {
    //     if (m_tileManager) {
    //         std::lock_guard<std::mutex> lock(m_tilesMutex);
    //         m_tileManager->clearTileSets();
    //     }
    // }
}
}
