#pragma once
#include "core/PrecompiledHeader.hpp"

#define ENABLE_LOGGING
#include <future>

#include "common/log.hpp"

class Module {
private:
    std::promise<void> readyPromise;
    std::shared_future<void> readyFuture;
    std::future<void> initializationTask;

protected:
    // Derived classes implement their specific initialization
    virtual void doInitialization() = 0;
    
    // Called when initialization is complete
    void markReady() {
        LOG("MARK READY CALLED");
        readyPromise.set_value();
    }

public:
    Module() 
        : readyFuture(readyPromise.get_future().share()) {}
    
    virtual ~Module() = default;
        
    void startInitialization() {
        initializationTask = std::async(std::launch::async, [this]() {
            try {
                doInitialization();
                markReady();
            } catch (const std::exception& e) {
                std::cerr << "Exception in " << typeid(*this).name() << ": " << e.what() << "\n";
                readyPromise.set_exception(std::current_exception());
            } catch (...) {
                std::cerr << "Unknown exception in " << typeid(*this).name() << "\n";
                readyPromise.set_exception(std::current_exception());
            }
        });
    }

    
    // For other modules to wait on
    std::shared_future<void> getReadyFuture() const {
        return readyFuture;
    }
    
    // Convenience methods
    void waitForReady() { readyFuture.wait(); }
    bool isReady() const {
        return readyFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }
};
