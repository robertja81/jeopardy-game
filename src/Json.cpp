#include "Json.h"

#include <cstdio>
#include <cstdlib>
#include <cwctype>

namespace {

struct Parser {
    const wchar_t* p;
    const wchar_t* end;

    void SkipWhitespace() {
        while (p < end && (*p == L' ' || *p == L'\t' || *p == L'\n' || *p == L'\r')) {
            ++p;
        }
    }

    bool AtEnd() const { return p >= end; }

    bool ParseLiteral(const wchar_t* lit, size_t len) {
        if (static_cast<size_t>(end - p) < len) {
            return false;
        }
        for (size_t i = 0; i < len; ++i) {
            if (p[i] != lit[i]) {
                return false;
            }
        }
        p += len;
        return true;
    }

    bool ParseString(std::wstring& out, std::wstring& err) {
        if (AtEnd() || *p != L'"') {
            err = L"expected string";
            return false;
        }
        ++p;
        out.clear();
        while (true) {
            if (AtEnd()) {
                err = L"unterminated string";
                return false;
            }
            wchar_t c = *p++;
            if (c == L'"') {
                break;
            }
            if (c != L'\\') {
                out += c;
                continue;
            }
            if (AtEnd()) {
                err = L"unterminated escape sequence";
                return false;
            }
            wchar_t esc = *p++;
            switch (esc) {
                case L'"': out += L'"'; break;
                case L'\\': out += L'\\'; break;
                case L'/': out += L'/'; break;
                case L'n': out += L'\n'; break;
                case L't': out += L'\t'; break;
                case L'r': out += L'\r'; break;
                case L'b': out += L'\b'; break;
                case L'f': out += L'\f'; break;
                case L'u': {
                    if (end - p < 4) {
                        err = L"bad \\u escape";
                        return false;
                    }
                    wchar_t code = 0;
                    for (int i = 0; i < 4; ++i) {
                        wchar_t h = p[i];
                        code = static_cast<wchar_t>(code << 4);
                        if (h >= L'0' && h <= L'9') {
                            code = static_cast<wchar_t>(code | (h - L'0'));
                        } else if (h >= L'a' && h <= L'f') {
                            code = static_cast<wchar_t>(code | (h - L'a' + 10));
                        } else if (h >= L'A' && h <= L'F') {
                            code = static_cast<wchar_t>(code | (h - L'A' + 10));
                        } else {
                            err = L"bad \\u escape";
                            return false;
                        }
                    }
                    p += 4;
                    out += code;
                    break;
                }
                default:
                    err = L"unrecognized escape sequence";
                    return false;
            }
        }
        return true;
    }

    bool ParseNumber(JsonValue& out, std::wstring& err) {
        const wchar_t* start = p;
        if (!AtEnd() && *p == L'-') {
            ++p;
        }
        if (AtEnd() || !iswdigit(*p)) {
            err = L"invalid number";
            return false;
        }
        while (!AtEnd() && iswdigit(*p)) {
            ++p;
        }
        if (!AtEnd() && *p == L'.') {
            ++p;
            if (AtEnd() || !iswdigit(*p)) {
                err = L"invalid number";
                return false;
            }
            while (!AtEnd() && iswdigit(*p)) {
                ++p;
            }
        }
        if (!AtEnd() && (*p == L'e' || *p == L'E')) {
            ++p;
            if (!AtEnd() && (*p == L'+' || *p == L'-')) {
                ++p;
            }
            if (AtEnd() || !iswdigit(*p)) {
                err = L"invalid number";
                return false;
            }
            while (!AtEnd() && iswdigit(*p)) {
                ++p;
            }
        }
        out.type = JsonType::Number;
        out.numberValue = std::wcstod(std::wstring(start, p).c_str(), nullptr);
        return true;
    }

    bool ParseArray(JsonValue& out, std::wstring& err) {
        ++p; // consume '['
        out.type = JsonType::Array;
        SkipWhitespace();
        if (!AtEnd() && *p == L']') {
            ++p;
            return true;
        }
        while (true) {
            SkipWhitespace();
            JsonValue element;
            if (!ParseValue(element, err)) {
                return false;
            }
            out.arrayValue.push_back(std::move(element));
            SkipWhitespace();
            if (AtEnd()) {
                err = L"unterminated array";
                return false;
            }
            if (*p == L',') {
                ++p;
                continue;
            }
            if (*p == L']') {
                ++p;
                break;
            }
            err = L"expected ',' or ']' in array";
            return false;
        }
        return true;
    }

    bool ParseObject(JsonValue& out, std::wstring& err) {
        ++p; // consume '{'
        out.type = JsonType::Object;
        SkipWhitespace();
        if (!AtEnd() && *p == L'}') {
            ++p;
            return true;
        }
        while (true) {
            SkipWhitespace();
            std::wstring key;
            if (!ParseString(key, err)) {
                return false;
            }
            SkipWhitespace();
            if (AtEnd() || *p != L':') {
                err = L"expected ':' in object";
                return false;
            }
            ++p;
            SkipWhitespace();
            JsonValue value;
            if (!ParseValue(value, err)) {
                return false;
            }
            out.objectValue.emplace_back(std::move(key), std::move(value));
            SkipWhitespace();
            if (AtEnd()) {
                err = L"unterminated object";
                return false;
            }
            if (*p == L',') {
                ++p;
                continue;
            }
            if (*p == L'}') {
                ++p;
                break;
            }
            err = L"expected ',' or '}' in object";
            return false;
        }
        return true;
    }

    bool ParseValue(JsonValue& out, std::wstring& err) {
        SkipWhitespace();
        if (AtEnd()) {
            err = L"unexpected end of input";
            return false;
        }
        wchar_t c = *p;
        if (c == L'{') return ParseObject(out, err);
        if (c == L'[') return ParseArray(out, err);
        if (c == L'"') {
            out.type = JsonType::String;
            return ParseString(out.stringValue, err);
        }
        if (c == L't') {
            if (ParseLiteral(L"true", 4)) {
                out.type = JsonType::Bool;
                out.boolValue = true;
                return true;
            }
            err = L"invalid literal";
            return false;
        }
        if (c == L'f') {
            if (ParseLiteral(L"false", 5)) {
                out.type = JsonType::Bool;
                out.boolValue = false;
                return true;
            }
            err = L"invalid literal";
            return false;
        }
        if (c == L'n') {
            if (ParseLiteral(L"null", 4)) {
                out.type = JsonType::Null;
                return true;
            }
            err = L"invalid literal";
            return false;
        }
        if (c == L'-' || iswdigit(c)) {
            return ParseNumber(out, err);
        }
        err = L"unexpected character";
        return false;
    }
};

} // namespace

const JsonValue* JsonValue::Find(const std::wstring& key) const {
    if (type != JsonType::Object) {
        return nullptr;
    }
    for (const auto& kv : objectValue) {
        if (kv.first == key) {
            return &kv.second;
        }
    }
    return nullptr;
}

bool ParseJson(const std::wstring& text, JsonValue& outValue, std::wstring& outError) {
    Parser parser{text.c_str(), text.c_str() + text.size()};
    if (!parser.ParseValue(outValue, outError)) {
        return false;
    }
    parser.SkipWhitespace();
    if (!parser.AtEnd()) {
        outError = L"trailing content after JSON value";
        return false;
    }
    return true;
}

std::wstring EscapeJsonString(const std::wstring& s) {
    std::wstring out;
    out.reserve(s.size() + 8);
    for (wchar_t c : s) {
        switch (c) {
            case L'"': out += L"\\\""; break;
            case L'\\': out += L"\\\\"; break;
            case L'\n': out += L"\\n"; break;
            case L'\r': out += L"\\r"; break;
            case L'\t': out += L"\\t"; break;
            default:
                if (c < 0x20) {
                    wchar_t buf[8];
                    swprintf_s(buf, L"\\u%04x", static_cast<unsigned int>(c));
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}
