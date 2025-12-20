module;
#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
export module utils.logging;
export namespace utils {

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

class Logger {
   public:
    Logger(const Logger&) = delete;
    auto operator=(const Logger&) -> Logger& = delete;
    Logger(Logger&&) = delete;
    auto operator=(Logger&&) -> Logger& = delete;

    static auto instance() -> Logger& {
        static Logger instance;
        return instance;
    }

    void init(const std::string& filename, LogLevel level = LogLevel::INFO) {
        m_level = level;
        if (!filename.empty()) {
            m_filestream.open(filename, std::ios::app);
            if (!m_filestream.is_open()) {
                std::cerr << "Failed to open log file: " << filename
                          << std::endl;
            }
        }
    }

    template <typename... Args>
    void log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
        if (level < m_level) {
            return;
        }

        auto now = std::chrono::system_clock::now();
        std::string message = std::format(
            "[{:%Y-%m-%d %H:%M:%S}] [{}] {}", now, levelToString(level),
            std::format(fmt, std::forward<Args>(args)...));

        std::scoped_lock lock(m_mutex);
        std::cout << message << std::endl;
        if (m_filestream.is_open()) {
            m_filestream << message << std::endl;
        }
    }

    template <typename... Args>
    static void debug(std::format_string<Args...> fmt, Args&&... args) {
        instance().log(LogLevel::DEBUG, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void info(std::format_string<Args...> fmt, Args&&... args) {
        instance().log(LogLevel::INFO, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warn(std::format_string<Args...> fmt, Args&&... args) {
        instance().log(LogLevel::WARN, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void error(std::format_string<Args...> fmt, Args&&... args) {
        instance().log(LogLevel::ERROR, fmt, std::forward<Args>(args)...);
    }

   private:
    Logger() = default;
    ~Logger() {
        if (m_filestream.is_open()) {
            m_filestream.close();
        }
    }

    static auto levelToString(LogLevel level) -> std::string {
        switch (level) {
            case LogLevel::DEBUG:
                return "DEBUG";
            case LogLevel::INFO:
                return "INFO";
            case LogLevel::WARN:
                return "WARN";
            case LogLevel::ERROR:
                return "ERROR";
        }
        return "UNKNOWN";
    }

    LogLevel m_level = LogLevel::INFO;
    std::ofstream m_filestream;
    std::mutex m_mutex;
};

}  // namespace utils
