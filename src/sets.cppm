module;

#include <string>
#include <vector>

export module sets;

constexpr auto size_t_cmp = [](const size_t& i1, const size_t& i2) -> int {
    auto result = i1 <=> i2;
    if (result == std::strong_ordering::less) {
        return -1;
    } else if (result == std::strong_ordering::greater) {
        return 1;
    } else {
        return 0;
    }
};

[[deprecated(
    "This interface is deprecated. Please use std::vector's capacity "
    "management "
    "instead C-style array.")]]
constexpr auto inc_arr_sz =
    [](size_t** valarr, size_t* arr_maxsz, size_t maxsz) -> bool {
    if (*arr_maxsz > maxsz / 2) {
        *arr_maxsz = maxsz;
    } else {
        *arr_maxsz *= 2;
    }

    // 使用 std::realloc 进行内存重新分配
    void* valarr_tmpptr = std::realloc(*valarr, *arr_maxsz * sizeof(size_t));
    if (valarr_tmpptr == nullptr) {
        return false;  // 分配失败
    }

    *valarr = static_cast<size_t*>(valarr_tmpptr);  // 转换为正确的类型
    return true;
};

bool parse_uintset(std::string_view lptr, std::vector<size_t>& set,
                   size_t maxSZ) {

    
    return false;
}
