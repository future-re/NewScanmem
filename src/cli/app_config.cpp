#include "newscanmem/cli/app_config.hpp"

namespace cli {
auto AppConfig::isValid() const noexcept -> bool { return targetPid >= 0; }
}  // namespace cli
