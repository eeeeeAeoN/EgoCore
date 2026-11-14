#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <imagehlp.h>
#pragma comment(lib, "imagehlp.lib")

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace LargeAddressPatcher {

enum class Status {
    MissingOrInvalid,
    NeedsPatch,
    AlreadyPatched
};

namespace detail {

inline bool ReadPeInfo(
    const std::filesystem::path& exePath,
    LONG& outLfNew,
    IMAGE_NT_HEADERS32& outNt,
    uint64_t& outFileSize) {
    std::error_code ec;
    if (!std::filesystem::exists(exePath, ec) || !std::filesystem::is_regular_file(exePath, ec)) {
        return false;
    }

    outFileSize = std::filesystem::file_size(exePath, ec);
    if (ec || outFileSize < sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS32)) {
        return false;
    }

    std::ifstream in(exePath, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }

    IMAGE_DOS_HEADER dos{};
    in.read(reinterpret_cast<char*>(&dos), sizeof(dos));
    if (!in || dos.e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }

    if (dos.e_lfanew < static_cast<LONG>(sizeof(IMAGE_DOS_HEADER)) ||
        static_cast<uint64_t>(dos.e_lfanew) + sizeof(IMAGE_NT_HEADERS32) > outFileSize) {
        return false;
    }

    in.seekg(dos.e_lfanew, std::ios::beg);
    in.read(reinterpret_cast<char*>(&outNt), sizeof(outNt));
    if (!in || outNt.Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }

    if (outNt.FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        outNt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        return false;
    }

    outLfNew = dos.e_lfanew;
    return true;
}

} // namespace detail

inline Status CheckStatus(const std::filesystem::path& exePath) {
    LONG lfanew = 0;
    IMAGE_NT_HEADERS32 nt{};
    uint64_t fileSize = 0;
    if (!detail::ReadPeInfo(exePath, lfanew, nt, fileSize)) {
        return Status::MissingOrInvalid;
    }
    return (nt.FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) != 0
        ? Status::AlreadyPatched
        : Status::NeedsPatch;
}

inline bool ApplyPatch(
    const std::filesystem::path& exePath,
    bool createBackup = true,
    std::string* outMessage = nullptr) {
    LONG lfanew = 0;
    IMAGE_NT_HEADERS32 nt{};
    uint64_t fileSize = 0;
    if (!detail::ReadPeInfo(exePath, lfanew, nt, fileSize)) {
        if (outMessage) *outMessage = "Missing or invalid 32-bit Fable.exe PE header.";
        return false;
    }

    if ((nt.FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) != 0) {
        if (outMessage) *outMessage = "Fable.exe is already 4GB (Large Address Aware) patched.";
        return true;
    }

    if (createBackup) {
        std::filesystem::path bakPath = exePath;
        bakPath += ".bak";
        std::error_code ec;
        if (!std::filesystem::exists(bakPath, ec)) {
            std::filesystem::copy_file(exePath, bakPath, std::filesystem::copy_options::skip_existing, ec);
        }
    }

    std::fstream io(exePath, std::ios::binary | std::ios::in | std::ios::out);
    if (!io.is_open()) {
        if (outMessage) *outMessage = "Could not open Fable.exe for writing (close Fable.exe or check permissions).";
        return false;
    }

    nt.FileHeader.Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
    const std::streamoff charOffset =
        static_cast<std::streamoff>(lfanew) +
        static_cast<std::streamoff>(offsetof(IMAGE_NT_HEADERS32, FileHeader)) +
        static_cast<std::streamoff>(offsetof(IMAGE_FILE_HEADER, Characteristics));

    io.seekp(charOffset, std::ios::beg);
    io.write(reinterpret_cast<const char*>(&nt.FileHeader.Characteristics), sizeof(WORD));
    if (!io) {
        if (outMessage) *outMessage = "Failed writing Characteristics flag to Fable.exe.";
        return false;
    }

    if (nt.OptionalHeader.CheckSum != 0) {
        io.flush();
        io.seekg(0, std::ios::beg);
        std::vector<uint8_t> imageBytes(static_cast<size_t>(fileSize));
        io.read(reinterpret_cast<char*>(imageBytes.data()), static_cast<std::streamsize>(fileSize));
        if (io) {
            DWORD headerSum = 0;
            DWORD newCheckSum = 0;
            if (CheckSumMappedFile(imageBytes.data(), static_cast<DWORD>(imageBytes.size()), &headerSum, &newCheckSum) != nullptr) {
                const std::streamoff csumOffset =
                    static_cast<std::streamoff>(lfanew) +
                    static_cast<std::streamoff>(offsetof(IMAGE_NT_HEADERS32, OptionalHeader)) +
                    static_cast<std::streamoff>(offsetof(IMAGE_OPTIONAL_HEADER32, CheckSum));
                io.seekp(csumOffset, std::ios::beg);
                io.write(reinterpret_cast<const char*>(&newCheckSum), sizeof(DWORD));
            }
        }
    }

    io.flush();
    if (outMessage) *outMessage = "Patched Fable.exe with 4GB Large Address Aware flag (backup: Fable.exe.bak).";
    return true;
}

inline Status CheckFableFolder(const std::string& gameRoot) {
    if (gameRoot.empty()) return Status::MissingOrInvalid;
    return CheckStatus(std::filesystem::path(gameRoot) / "Fable.exe");
}

inline bool EnsureFableFolder4GB(const std::string& gameRoot, std::string* outMessage = nullptr) {
    if (gameRoot.empty()) {
        if (outMessage) *outMessage = "No game folder configured.";
        return false;
    }
    const std::filesystem::path exePath = std::filesystem::path(gameRoot) / "Fable.exe";
    const Status st = CheckStatus(exePath);
    if (st == Status::AlreadyPatched) {
        if (outMessage) *outMessage = "Fable.exe is already 4GB (Large Address Aware) patched.";
        return true;
    }
    if (st == Status::MissingOrInvalid) {
        if (outMessage) *outMessage = "Fable.exe not found or invalid in selected folder.";
        return false;
    }
    return ApplyPatch(exePath, true, outMessage);
}

} // namespace LargeAddressPatcher
