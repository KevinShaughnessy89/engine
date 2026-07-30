#include "core/PrecompiledHeader.hpp"
#define ENABLE_LOGGIN

#include "core/Module.hpp"
#include "common/log.hpp"

class ModuleManager {
private:
    std::unordered_map<std::string, Module*> modules;
    bool initializationStarted = false;
    
public:
    void registerModule(Module* module, std::string name) {
        modules[name] = module;
    }

    void startInitialization() {
        if (initializationStarted) return;
        
        for (auto& [name, module] : modules) {
            module->startInitialization();
        }
        initializationStarted = true;
    }

    std::shared_future<void> getReadyFuture(std::string name) {
        Module* target = modules[name];
        return target->getReadyFuture();
    }
    
    bool areAllModulesReady() const {
        return std::all_of(modules.begin(), modules.end(), 
                          [](const auto& module) { return module.second->isReady(); });
    }
    
    float getInitializationProgress() const {
        int readyCount = std::count_if(modules.begin(), modules.end(),
                                      [](const auto& module) { return module.second->isReady(); });
        return static_cast<float>(readyCount) / modules.size();
    }
};
