#pragma once

#include <string>

enum class PatchError {
    Ok = 0,
    InvalidSn,
    SourceOpenFailed,
    DestOpenFailed,
    FileTooShort,
    SeekFailed,
    WriteFailed,
    CopyFailed
};

struct PatchResult {
    PatchError error;
    std::string message;
};

PatchResult patch_binary_with_sn(
    const std::string& source_path,
    const std::string& dest_path,
    const std::string& uppercase_sn
);
