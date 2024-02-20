#include "goxlr_manager.hpp"

namespace GoXLR {
    GoXLRManager::GoXLRManager(std::shared_ptr<aspl::Plugin> plugin) : plugin(plugin) {
        assert(plugin);

        plugin->GetContext()->Tracer->Message("Starting GoXLR Manager..");
    }
    GoXLRManager::~GoXLRManager() {
        // NOOP, destructor..
    }

    void GoXLRManager::locate_goxlr() {
        plugin->GetContext()->Tracer->Message("Looking for GoXLR Devices");
    }
}