#include "MatchHistory.h"

#include <cstdio>
#include "FileUtil.h"
#include "Json.h"

namespace {

std::wstring FormatOneDecimal(double value) {
    wchar_t buf[32];
    swprintf_s(buf, L"%.1f", value);
    return buf;
}

std::wstring GetStringField(const JsonValue& obj, const wchar_t* key) {
    const JsonValue* v = obj.Find(key);
    return (v && v->IsString()) ? v->stringValue : std::wstring();
}

} // namespace

std::vector<MatchRecord> LoadMatchHistory() {
    std::vector<MatchRecord> result;

    std::wstring path = GetAppDataFilePath(L"history.json");
    if (path.empty()) {
        return result;
    }

    std::wstring text, error;
    if (!ReadFileUtf8(path, text, error)) {
        return result; // no history file yet, or unreadable -- fine, just empty
    }

    JsonValue root;
    std::wstring jsonError;
    if (!ParseJson(text, root, jsonError) || !root.IsArray()) {
        return result; // corrupt file -- don't propagate an error, just start fresh
    }

    for (const JsonValue& entry : root.arrayValue) {
        if (!entry.IsObject()) {
            continue;
        }
        MatchRecord record;
        record.playerName = GetStringField(entry, L"playerName");
        record.round1DictionaryName = GetStringField(entry, L"round1");
        record.round2DictionaryName = GetStringField(entry, L"round2");
        record.finalJeopardyDictionaryName = GetStringField(entry, L"final");
        const JsonValue* scoreVal = entry.Find(L"score");
        record.finalScore = (scoreVal && scoreVal->IsNumber())
                                 ? static_cast<int>(scoreVal->numberValue)
                                 : 0;
        const JsonValue* difficultyVal = entry.Find(L"difficulty");
        record.difficulty = (difficultyVal && difficultyVal->IsNumber())
                                 ? difficultyVal->numberValue
                                 : 1.0; // older history rows predate this field
        result.push_back(std::move(record));
    }
    return result;
}

void AppendMatchHistory(const MatchRecord& record) {
    std::vector<MatchRecord> history = LoadMatchHistory();
    history.insert(history.begin(), record); // most-recent-first
    if (history.size() > kMaxHistoryEntries) {
        history.resize(kMaxHistoryEntries);
    }

    std::wstring json = L"[\n";
    for (size_t i = 0; i < history.size(); ++i) {
        const MatchRecord& r = history[i];
        json += L"  {\"playerName\":\"" + EscapeJsonString(r.playerName) +
                L"\",\"round1\":\"" + EscapeJsonString(r.round1DictionaryName) +
                L"\",\"round2\":\"" + EscapeJsonString(r.round2DictionaryName) +
                L"\",\"final\":\"" + EscapeJsonString(r.finalJeopardyDictionaryName) +
                L"\",\"score\":" + std::to_wstring(r.finalScore) +
                L",\"difficulty\":" + FormatOneDecimal(r.difficulty) + L"}";
        json += (i + 1 < history.size()) ? L",\n" : L"\n";
    }
    json += L"]\n";

    std::wstring path = GetAppDataFilePath(L"history.json");
    if (path.empty()) {
        return;
    }
    std::wstring error;
    WriteFileUtf8(path, json, error); // best-effort; a failed save shouldn't block the game
}
