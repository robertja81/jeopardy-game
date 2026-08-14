#pragma once

#include <string>
#include <utility>
#include <vector>

// Minimal JSON parser -- just enough to read dictionary files (objects,
// arrays, strings, numbers, bool/null). No external dependency; this is a
// hobby project and the schema we need is small.
enum class JsonType { Null, Bool, Number, String, Array, Object };

struct JsonValue {
    JsonType type = JsonType::Null;
    bool boolValue = false;
    double numberValue = 0;
    std::wstring stringValue;
    std::vector<JsonValue> arrayValue;
    std::vector<std::pair<std::wstring, JsonValue>> objectValue; // insertion order, linear lookup

    bool IsObject() const { return type == JsonType::Object; }
    bool IsArray() const { return type == JsonType::Array; }
    bool IsString() const { return type == JsonType::String; }
    bool IsNumber() const { return type == JsonType::Number; }

    // Returns nullptr if this isn't an object or `key` isn't present.
    const JsonValue* Find(const std::wstring& key) const;
};

// Parses `text` as a single JSON value. On success returns true and fills
// `outValue`; on failure returns false and fills `outError` with a
// human-readable (if terse) message.
bool ParseJson(const std::wstring& text, JsonValue& outValue, std::wstring& outError);

// Escapes `s` for embedding inside a JSON string literal (quotes,
// backslashes, control characters). Does not add the surrounding quotes --
// callers writing JSON by hand (MatchHistory, Stats) wrap the result
// themselves.
std::wstring EscapeJsonString(const std::wstring& s);
