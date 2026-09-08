#pragma once
#include <string>
#include <functional>
#include <mutex>

namespace module1 {

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setLogCallback(std::function<void(const std::string&)> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        callback_ = callback;
    }

    static void log(const std::string& message) {
        getInstance().logInternal(message);
    }

private:
    Logger() = default;
    
    void logInternal(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (callback_) {
            callback_(message);
        }
    }

    std::function<void(const std::string&)> callback_;
    std::mutex mutex_;
};

} // namespace module1
