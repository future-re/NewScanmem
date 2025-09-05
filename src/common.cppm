module;

#include <sys/types.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

export module common;

/* From `include/linux/compiler.h`, in the linux kernel:
 * Offers a simple interface to the expect builtin */
#ifdef __GNUC__
template <typename T>
constexpr auto likely(T xval) -> bool {
    return __builtin_expect(!!(xval), 1);
}
template <typename T>
constexpr auto unlikely(T xval) -> bool {
    return __builtin_expect(!!(xval), 0);
}
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

/* Use the best `getenv()` implementation we have */
#if HAVE_SECURE_GETENV
#define UTIL_GETENV secure_getenv
#else
#define UTIL_GETENV getenv
#endif

/* states returned by check_process() */
enum class Pstate {
    PROC_RUNNING,
    PROC_ERR, /* error during detection */
    PROC_DEAD,
    PROC_ZOMBIE
};

/*
 * 在 /proc 中检查进程是否存在并正在运行，同时能检测僵尸进程。
 *
 * 要求：Linux 内核且已挂载 /proc。
 */
static auto checkProcess(pid_t pid) -> Pstate {
    if (pid <= 0) {
        return Pstate::PROC_ERR;
    }

    const std::string PATH = "/proc/" + std::to_string(pid) + "/status";
    if (!std::filesystem::exists(PATH)) {
        return Pstate::PROC_DEAD;
    }

    std::ifstream ifs(PATH);
    if (!ifs.is_open()) {
        return Pstate::PROC_ERR;
    }

    std::string line;
    constexpr std::string_view PREFIX = "State:\t";
    const std::size_t PFXLEN = PREFIX.size();  // 7

    while (std::getline(ifs, line)) {
        if (line.size() <= PFXLEN) {
            continue;
        }
        if (line.starts_with(PREFIX)) {
            char status = line[PFXLEN];  // 状态字符在 prefix 之后
            if (status == 'Z' || status == 'X') {
                return Pstate::PROC_ZOMBIE;
            }
            if (status >= 'A' && status <= 'Z') {
                return Pstate::PROC_RUNNING;
            }
            return Pstate::PROC_ERR;
        }
    }
    return Pstate::PROC_ERR;
}

/* Function declarations */
auto smProcessIsDead(pid_t pid) -> bool {
    return (checkProcess(pid) != Pstate::PROC_RUNNING);
}