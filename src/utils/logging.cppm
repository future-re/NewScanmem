/**
 * @file logging.cppm
 * @brief Logger with dependency injection support
 */

module;

#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>

export module utils.logging;

export namespace utils {

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

/**
 * @class Logger
 * @brief Thread-safe logger with file and console output
 */
class Logger {
   public:
    explicit Logger(std::string name,
                    std::optional<std::string> filename = std::nullopt,
                    LogLevel level = LogLevel::INFO)
        : m_name(std::move(name)), m_level(level) {
        if (filename) {
            initFile(*filename);
        }
    }

    // Non-copyable
    Logger(const Logger&) = delete;
    auto operator=(const Logger&) -> Logger& = delete;
    
    // Movable - must handle mutex
    Logger(Logger&& other) noexcept
        : m_name(std::move(other.m_name)),
          m_level(other.m_level),
          m_filestream(std::move(other.m_filestream)) {}
    
    auto operator=(Logger&& other) noexcept -> Logger& {
        if (this != &other) {
            m_name = std::move(other.m_name);
            m_level = other.m_level;
            m_filestream = std::move(other.m_filestream);
        }
        return *this;
    }

    ~Logger() {
        if (m_filestream.is_open()) {
            m_filestream.close();
        }
    }

    void init(const std::string& filename, LogLevel level = LogLevel::INFO) {
        m_level = level;
        initFile(filename);
    }

    template <typename... Args>
    void log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
        if (level < m_level) {
            return;
        }

        auto now = std::chrono::system_clock::now();
        std::string message = std::format(
            "[{:%Y-%m-%d %H:%M:%S}] [{}] {}", now,
            levelToString(level),
            std::format(fmt, std::forward<Args>(args)...));

        std::scoped_lock lock(m_mutex);
        std::cout << message << std::endl;
        if (m_filestream.is_open()) {
            m_filestream << message << std::endl;
        }
    }

    template <typename... Args>
    void debug(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::DEBUG, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::INFO, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::WARN, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::ERROR, fmt, std::forward<Args>(args)...);
    }

    // Global singleton accessor
    static auto instance() -> Logger& {
        static Logger s_instance{"global"};
        return s_instance;
    }

    // Static convenience methods (backward compatible)
    template <typename... Args>
    static void s_debug(std::format_string<Args...> fmt, Args&&... args) {
        instance().debug(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void s_info(std::format_string<Args...> fmt, Args&&... args) {
        instance().info(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void s_warn(std::format_string<Args...> fmt, Args&&... args) {
        instance().warn(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void s_error(std::format_string<Args...> fmt, Args&&... args) {
        instance().error(fmt, std::forward<Args>(args)...);
    }

   private:
    void initFile(const std::string& filename) {
        m_filestream.open(filename, std::ios::app);
        if (!m_filestream.is_open()) {
            std::cerr << "Failed to open log file: " << filename << std::endl;
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

    std::string m_name;
    LogLevel m_level{LogLevel::INFO};
    std::ofstream m_filestream;
    std::mutex m_mutex;
};

}  // namespace utils
