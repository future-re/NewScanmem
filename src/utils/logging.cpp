#include "newscanmem/utils/logging.hpp"

namespace utils {
Logger::Logger(std::string name, const std::optional<std::string> filename,
               const LogLevel level)
    : m_name(std::move(name)), m_level(level) {
    if (filename) initFile(*filename);
}
Logger::Logger(Logger&& other) noexcept
    : m_name(std::move(other.m_name)),
      m_level(other.m_level),
      m_filestream(std::move(other.m_filestream)) {}
auto Logger::operator=(Logger&& other) noexcept -> Logger& {
    if (this != &other) {
        m_name = std::move(other.m_name);
        m_level = other.m_level;
        m_filestream = std::move(other.m_filestream);
    }
    return *this;
}
Logger::~Logger() {
    if (m_filestream.is_open()) m_filestream.close();
}
void Logger::init(const std::string& filename, const LogLevel level) {
    m_level = level;
    initFile(filename);
}
auto Logger::instance() -> Logger& {
    static Logger instance{"global"};
    return instance;
}
void Logger::initFile(const std::string& filename) {
    m_filestream.open(filename, std::ios::app);
    if (!m_filestream)
        std::cerr << "Failed to open log file: " << filename << std::endl;
}
auto Logger::levelToString(const LogLevel level) -> std::string {
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
}  // namespace utils
