
#include <aspl/Plugin.hpp>

namespace GoXLR {
    class GoXLRManager {
        public: 
            GoXLRManager(std::shared_ptr<aspl::Plugin> plugin);
            ~GoXLRManager();

            GoXLRManager(const GoXLRManager&) = delete;
            GoXLRManager& operator=(const GoXLRManager&) = delete;

            void locate_goxlr();

        private:
            std::shared_ptr<aspl::Plugin> plugin;
    };
}