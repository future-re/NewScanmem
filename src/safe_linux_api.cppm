module;

#include <memory>

export module safe_linux_api;

using FilePtr = std::unique_ptr<FILE, int (*)(FILE*)>;

