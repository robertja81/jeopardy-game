#pragma once

#include <string>

// Small shared UTF-8 text file helpers, used by both DictionaryLoader
// (reading user-selected dictionary files) and MatchHistory (reading/
// writing the persisted match history).

// Reads `path` as UTF-8 text (BOM tolerated) into a wide string. Returns
// false and fills outError with a short reason on failure.
bool ReadFileUtf8(const std::wstring& path, std::wstring& outText, std::wstring& outError);

// Writes `text` to `path` as UTF-8 (no BOM), overwriting any existing file.
// Returns false and fills outError with a short reason on failure.
bool WriteFileUtf8(const std::wstring& path, const std::wstring& text, std::wstring& outError);

// Returns "%APPDATA%\JeopardyGame\<fileName>", creating the JeopardyGame
// folder the first time it's needed. Returns an empty string if the app
// data folder can't be resolved (extremely unlikely) -- callers treat that
// as "persistence unavailable" rather than failing.
std::wstring GetAppDataFilePath(const std::wstring& fileName);
