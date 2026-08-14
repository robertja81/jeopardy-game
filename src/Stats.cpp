#include "Stats.h"

#include <vector>
#include "FileUtil.h"
#include "Json.h"

namespace {

struct QuestionStatEntry {
    std::wstring text;
    int correct = 0;
    int incorrect = 0;
};

struct DictionaryStatEntry {
    std::wstring name;
    int runs = 0;
};

struct StatsData {
    std::vector<QuestionStatEntry> questions;
    std::vector<DictionaryStatEntry> dictionaries;
};

std::wstring GetStringField(const JsonValue& obj, const wchar_t* key) {
    const JsonValue* v = obj.Find(key);
    return (v && v->IsString()) ? v->stringValue : std::wstring();
}

int GetIntField(const JsonValue& obj, const wchar_t* key) {
    const JsonValue* v = obj.Find(key);
    return (v && v->IsNumber()) ? static_cast<int>(v->numberValue) : 0;
}

StatsData LoadStats() {
    StatsData data;

    std::wstring path = GetAppDataFilePath(L"stats.json");
    if (path.empty()) {
        return data;
    }

    std::wstring text, error;
    if (!ReadFileUtf8(path, text, error)) {
        return data; // no stats file yet, or unreadable -- fine, just empty
    }

    JsonValue root;
    std::wstring jsonError;
    if (!ParseJson(text, root, jsonError) || !root.IsObject()) {
        return data; // corrupt file -- don't propagate an error, just start fresh
    }

    const JsonValue* questionsVal = root.Find(L"questions");
    if (questionsVal && questionsVal->IsArray()) {
        for (const JsonValue& entry : questionsVal->arrayValue) {
            if (!entry.IsObject()) {
                continue;
            }
            QuestionStatEntry q;
            q.text = GetStringField(entry, L"text");
            q.correct = GetIntField(entry, L"correct");
            q.incorrect = GetIntField(entry, L"incorrect");
            if (!q.text.empty()) {
                data.questions.push_back(std::move(q));
            }
        }
    }

    const JsonValue* dictionariesVal = root.Find(L"dictionaries");
    if (dictionariesVal && dictionariesVal->IsArray()) {
        for (const JsonValue& entry : dictionariesVal->arrayValue) {
            if (!entry.IsObject()) {
                continue;
            }
            DictionaryStatEntry d;
            d.name = GetStringField(entry, L"name");
            d.runs = GetIntField(entry, L"runs");
            if (!d.name.empty()) {
                data.dictionaries.push_back(std::move(d));
            }
        }
    }

    return data;
}

void SaveStats(const StatsData& data) {
    std::wstring path = GetAppDataFilePath(L"stats.json");
    if (path.empty()) {
        return;
    }

    std::wstring json = L"{\n  \"questions\": [\n";
    for (size_t i = 0; i < data.questions.size(); ++i) {
        const QuestionStatEntry& q = data.questions[i];
        json += L"    {\"text\":\"" + EscapeJsonString(q.text) + L"\",\"correct\":" +
                std::to_wstring(q.correct) + L",\"incorrect\":" + std::to_wstring(q.incorrect) +
                L"}";
        json += (i + 1 < data.questions.size()) ? L",\n" : L"\n";
    }
    json += L"  ],\n  \"dictionaries\": [\n";
    for (size_t i = 0; i < data.dictionaries.size(); ++i) {
        const DictionaryStatEntry& d = data.dictionaries[i];
        json += L"    {\"name\":\"" + EscapeJsonString(d.name) + L"\",\"runs\":" +
                std::to_wstring(d.runs) + L"}";
        json += (i + 1 < data.dictionaries.size()) ? L",\n" : L"\n";
    }
    json += L"  ]\n}\n";

    std::wstring error;
    WriteFileUtf8(path, json, error); // best-effort; a failed save shouldn't block the game
}

} // namespace

void RecordQuestionAttempt(const std::wstring& questionText, bool wasCorrect) {
    if (questionText.empty()) {
        return;
    }
    StatsData data = LoadStats();
    QuestionStatEntry* existing = nullptr;
    for (QuestionStatEntry& q : data.questions) {
        if (q.text == questionText) {
            existing = &q;
            break;
        }
    }
    if (!existing) {
        data.questions.push_back({questionText, 0, 0});
        existing = &data.questions.back();
    }
    if (wasCorrect) {
        ++existing->correct;
    } else {
        ++existing->incorrect;
    }
    SaveStats(data);
}

void RecordDictionaryRun(const std::wstring& dictionaryName) {
    if (dictionaryName.empty()) {
        return;
    }
    StatsData data = LoadStats();
    DictionaryStatEntry* existing = nullptr;
    for (DictionaryStatEntry& d : data.dictionaries) {
        if (d.name == dictionaryName) {
            existing = &d;
            break;
        }
    }
    if (!existing) {
        data.dictionaries.push_back({dictionaryName, 0});
        existing = &data.dictionaries.back();
    }
    ++existing->runs;
    SaveStats(data);
}

std::optional<QuestionAttemptSummary> GetMostAnsweredCorrectly() {
    StatsData data = LoadStats();
    const QuestionStatEntry* best = nullptr;
    for (const QuestionStatEntry& q : data.questions) {
        if (q.correct > 0 && (!best || q.correct > best->correct)) {
            best = &q;
        }
    }
    if (!best) {
        return std::nullopt;
    }
    return QuestionAttemptSummary{best->text, best->correct};
}

std::optional<QuestionAttemptSummary> GetMostAnsweredIncorrectly() {
    StatsData data = LoadStats();
    const QuestionStatEntry* worst = nullptr;
    for (const QuestionStatEntry& q : data.questions) {
        if (q.incorrect > 0 && (!worst || q.incorrect > worst->incorrect)) {
            worst = &q;
        }
    }
    if (!worst) {
        return std::nullopt;
    }
    return QuestionAttemptSummary{worst->text, worst->incorrect};
}

std::optional<DictionaryPopularitySummary> GetMostPopularDictionary() {
    StatsData data = LoadStats();
    const DictionaryStatEntry* best = nullptr;
    for (const DictionaryStatEntry& d : data.dictionaries) {
        if (!best || d.runs > best->runs) {
            best = &d;
        }
    }
    if (!best) {
        return std::nullopt;
    }
    return DictionaryPopularitySummary{best->name, best->runs};
}
