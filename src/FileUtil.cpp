#include "FileUtil.h"

#include <shlobj.h>
#include <windows.h>
#include <vector>

namespace {
constexpr LONGLONG kMaxTextFileBytes = 10 * 1024 * 1024; // sanity limit, not a real constraint
} // namespace

bool ReadFileUtf8(const std::wstring& path, std::wstring& outText, std::wstring& outError) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        outError = L"Could not open the file. Check the path and try again.";
        return false;
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(file, &size) || size.QuadPart < 0 || size.QuadPart > kMaxTextFileBytes) {
        CloseHandle(file);
        outError = L"File is missing, empty, or unreasonably large.";
        return false;
    }

    std::vector<char> buffer(static_cast<size_t>(size.QuadPart));
    DWORD bytesRead = 0;
    BOOL ok = buffer.empty()
                  ? TRUE
                  : ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr);
    CloseHandle(file);
    if (!ok || (!buffer.empty() && bytesRead != buffer.size())) {
        outError = L"Failed to read the file.";
        return false;
    }

    size_t offset = 0;
    if (buffer.size() >= 3 && static_cast<unsigned char>(buffer[0]) == 0xEF &&
        static_cast<unsigned char>(buffer[1]) == 0xBB && static_cast<unsigned char>(buffer[2]) == 0xBF) {
        offset = 3; // skip UTF-8 BOM
    }

    int remaining = static_cast<int>(buffer.size() - offset);
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, buffer.data() + offset, remaining, nullptr, 0);
    if (wideLen <= 0 && remaining > 0) {
        outError = L"File is not valid UTF-8 text.";
        return false;
    }
    outText.resize(wideLen);
    if (wideLen > 0) {
        MultiByteToWideChar(CP_UTF8, 0, buffer.data() + offset, remaining, &outText[0], wideLen);
    }
    return true;
}

bool WriteFileUtf8(const std::wstring& path, const std::wstring& text, std::wstring& outError) {
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                                       nullptr, 0, nullptr, nullptr);
    std::vector<char> buffer(utf8Len);
    if (utf8Len > 0) {
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), buffer.data(),
                             utf8Len, nullptr, nullptr);
    }

    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        outError = L"Could not create or open the file for writing.";
        return false;
    }

    DWORD bytesWritten = 0;
    BOOL ok = buffer.empty()
                  ? TRUE
                  : WriteFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesWritten, nullptr);
    CloseHandle(file);
    if (!ok || (!buffer.empty() && bytesWritten != buffer.size())) {
        outError = L"Failed to write the file.";
        return false;
    }
    return true;
}

std::wstring GetAppDataFilePath(const std::wstring& fileName) {
    PWSTR appDataPath = nullptr;
    std::wstring result;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appDataPath))) {
        result = std::wstring(appDataPath) + L"\\JeopardyGame";
        CreateDirectoryW(result.c_str(), nullptr); // ignore failure/already-exists
        result += L"\\" + fileName;
    }
    if (appDataPath) {
        CoTaskMemFree(appDataPath);
    }
    return result;
}
