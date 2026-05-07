#include "BinaryPatcher.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>

namespace {
// 使用 0 基偏移 23，对应用户描述的“第 24 位(1 基)”
constexpr std::streamoff kWriteOffset = 23;
constexpr std::size_t kSnLength = 8;

bool is_valid_upper_sn(const std::string& sn) {
    if (sn.size() != kSnLength) {
        return false;
    }

    for (char ch : sn) {
        if (!std::isalnum(static_cast<unsigned char>(ch))) {
            return false;
        }
        if (std::isalpha(static_cast<unsigned char>(ch)) &&
            !std::isupper(static_cast<unsigned char>(ch))) {
            return false;
        }
    }
    return true;
}
}  // namespace

PatchResult patch_binary_with_sn(
    const std::string& source_path,
    const std::string& dest_path,
    const std::string& uppercase_sn
) {
    if (!is_valid_upper_sn(uppercase_sn)) {
        return {PatchError::InvalidSn, "SN 必须为 8 位大写字母数字"};
    }

    std::ifstream src(source_path, std::ios::binary);
    if (!src) {
        return {PatchError::SourceOpenFailed, "无法打开源文件"};
    }

    std::error_code ec;
    const auto parent = std::filesystem::path(dest_path).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            return {PatchError::DestOpenFailed, "无法创建目标目录"};
        }
    }

    std::ofstream dst(dest_path, std::ios::binary | std::ios::trunc);
    if (!dst) {
        return {PatchError::DestOpenFailed, "无法创建目标文件"};
    }

    dst << src.rdbuf();
    if (!dst.good()) {
        return {PatchError::CopyFailed, "复制源文件到目标文件失败"};
    }

    dst.flush();
    dst.close();
    src.close();

    ec.clear();
    const auto file_size = std::filesystem::file_size(dest_path, ec);
    if (ec) {
        return {PatchError::CopyFailed, "无法读取目标文件大小"};
    }

    if (file_size < static_cast<std::uintmax_t>(kWriteOffset + static_cast<std::streamoff>(kSnLength))) {
        return {PatchError::FileTooShort, "目标文件长度不足，无法从第 24 位开始写入 8 字节"};
    }

    std::fstream out(dest_path, std::ios::binary | std::ios::in | std::ios::out);
    if (!out) {
        return {PatchError::DestOpenFailed, "无法以读写模式打开目标文件"};
    }

    out.seekp(kWriteOffset, std::ios::beg);
    if (!out.good()) {
        return {PatchError::SeekFailed, "定位到第 24 位失败"};
    }

    out.write(uppercase_sn.data(), static_cast<std::streamsize>(uppercase_sn.size()));
    if (!out.good()) {
        return {PatchError::WriteFailed, "写入 SN ASCII 数据失败"};
    }

    out.flush();
    if (!out.good()) {
        return {PatchError::WriteFailed, "刷新目标文件失败"};
    }

    return {PatchError::Ok, "修改完成"};
}
