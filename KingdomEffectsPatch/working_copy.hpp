#pragma once

#include <windows.h>
#include <bcrypt.h>

#include <array>
#include <stdexcept>
#include <string>

namespace working_copy {

struct Handle {
    HANDLE value;
    explicit Handle(HANDLE handle) : value(handle) {}
    ~Handle() { if (valid()) CloseHandle(value); }
    bool valid() const { return value && value != INVALID_HANDLE_VALUE; }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
};

inline void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

inline std::wstring process_path(HANDLE process) {
    std::wstring path(32768, L'\0');
    DWORD size = static_cast<DWORD>(path.size());
    require(QueryFullProcessImageNameW(process, 0, path.data(), &size) != FALSE,
            "Cannot read the running process path.");
    path.resize(size);
    return path;
}

inline BY_HANDLE_FILE_INFORMATION identify(HANDLE file) {
    BY_HANDLE_FILE_INFORMATION info{};
    require(GetFileInformationByHandle(file, &info) != FALSE, "Cannot identify the executable.");
    return info;
}

inline bool same_file(const BY_HANDLE_FILE_INFORMATION& a, const BY_HANDLE_FILE_INFORMATION& b) {
    return a.dwVolumeSerialNumber == b.dwVolumeSerialNumber &&
           a.nFileIndexHigh == b.nFileIndexHigh && a.nFileIndexLow == b.nFileIndexLow;
}

inline void verify_executable(HANDLE file) {
    DWORD size = 0;
    IMAGE_DOS_HEADER dos{};
    require(ReadFile(file, &dos, sizeof dos, &size, nullptr) && size == sizeof dos &&
            dos.e_magic == IMAGE_DOS_SIGNATURE && dos.e_lfanew > 0 && dos.e_lfanew < 4096,
            "Invalid DOS header.");
    require(SetFilePointer(file, dos.e_lfanew, nullptr, FILE_BEGIN) != INVALID_SET_FILE_POINTER,
            "Cannot seek to the PE header.");
    IMAGE_NT_HEADERS32 pe{};
    require(ReadFile(file, &pe, sizeof pe, &size, nullptr) && size == sizeof pe &&
            pe.Signature == IMAGE_NT_SIGNATURE && pe.FileHeader.Machine == IMAGE_FILE_MACHINE_I386 &&
            pe.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC, "Expected an x86 PE executable.");
    require(SetFilePointer(file, 0, nullptr, FILE_BEGIN) == 0, "Cannot rewind the executable.");
    BCRYPT_ALG_HANDLE algorithm{};
    require(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr,
                                        BCRYPT_HASH_REUSABLE_FLAG) >= 0, "Cannot open SHA-256.");
    BCRYPT_HASH_HANDLE hash{};
    bool ok = BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) >= 0;
    std::array<UCHAR, 16384> buffer{};
    do {
        ok = ok && ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &size, nullptr);
        if (ok && size) ok = BCryptHashData(hash, buffer.data(), size, 0) >= 0;
    } while (ok && size);
    std::array<UCHAR, 32> digest{};
    ok = ok && BCryptFinishHash(hash, digest.data(), static_cast<DWORD>(digest.size()), 0) >= 0;
    if (hash) BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    constexpr std::array<UCHAR, 32> expected{
        0x56,0xe8,0xd5,0x63,0x32,0x0b,0x7d,0x6c,0x0c,0xf6,0x8a,0xe1,0x1c,0xdf,0x2c,0x0a,
        0xa9,0x71,0xdb,0xff,0x44,0xbb,0x29,0x66,0x51,0xb6,0x30,0x81,0x5c,0x54,0x88,0x77
    };
    require(ok && digest == expected, "Executable hash differs from Dominion 3315.");
}

}
