#include "InputValidator.h"

#include <fstream>
#include <system_error>

InputCheckResult InputValidator::check(const std::filesystem::path& path) {
    if (path.empty()) {
        return {false, {}, "路径为空"};
    }

    std::filesystem::path absPath;
    try {
        absPath = std::filesystem::absolute(path).lexically_normal();
    } catch (const std::filesystem::filesystem_error& e) {
        return {false, {}, std::string("无法解析路径：") + e.what()};
    }

    std::error_code ec;

    if (!std::filesystem::exists(absPath, ec) || ec) {
        return {false, {}, "文件不存在：" + absPath.u8string()};
    }

    if (!std::filesystem::is_regular_file(absPath, ec) || ec) {
        return {false, {}, "路径不是普通文件：" + absPath.u8string()};
    }

    auto size = std::filesystem::file_size(absPath, ec);
    if (ec) {
        return {false, {}, "无法获取文件大小：" + absPath.u8string()};
    }
    if (size == 0) {
        return {false, {}, "文件为空：" + absPath.u8string()};
    }

    std::ifstream f(absPath);
    if (!f.is_open()) {
        return {false, {}, "文件无法读取：" + absPath.u8string()};
    }

    return {true, absPath, {}};
}
