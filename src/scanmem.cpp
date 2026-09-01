// #include "memseek/scanmem.hpp"

// #include <exception>
// #include <string>

// #include "memseek/cli/app.hpp"
// #include "memseek/cli/app_config.hpp"

// namespace scanmem {

// auto run(const cli::AppConfig& config) -> std::expected<int, std::string> {
//     try {
//         cli::Application app{config};
//         return app.run();
//     } catch (const std::exception& e) {
//         return std::unexpected(std::string{"Runtime error: "} + e.what());
//     }
// }

// }  // namespace scanmem
