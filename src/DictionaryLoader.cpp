#include "DictionaryLoader.h"

#include <windows.h>
#include "FileUtil.h"
#include "Json.h"

namespace {

// Extracts the filename without directory or extension, e.g.
// "C:\dicts\Literature.json" -> "Literature".
std::wstring FileStem(const std::wstring& path) {
    size_t slash = path.find_last_of(L"/\\");
    std::wstring base = (slash == std::wstring::npos) ? path : path.substr(slash + 1);
    size_t dot = base.find_last_of(L'.');
    return (dot == std::wstring::npos) ? base : base.substr(0, dot);
}

bool GetNonEmptyString(const JsonValue* value, std::wstring& out) {
    if (!value || !value->IsString() || value->stringValue.empty()) {
        return false;
    }
    out = value->stringValue;
    return true;
}

bool LoadAndParseJson(const std::wstring& path, JsonValue& outRoot, std::wstring& outError) {
    std::wstring text;
    if (!ReadFileUtf8(path, text, outError)) {
        return false;
    }
    std::wstring jsonError;
    if (!ParseJson(text, outRoot, jsonError)) {
        outError = L"Invalid JSON (" + jsonError + L").";
        return false;
    }
    if (!outRoot.IsObject()) {
        outError = L"The root of the dictionary file must be a JSON object.";
        return false;
    }
    return true;
}

} // namespace

bool LoadRoundDictionary(const std::wstring& path, Board& outBoard, std::wstring& outError) {
    JsonValue root;
    if (!LoadAndParseJson(path, root, outError)) {
        return false;
    }

    const JsonValue* categoriesVal = root.Find(L"categories");
    if (!categoriesVal || !categoriesVal->IsArray()) {
        outError = L"Dictionary must have a \"categories\" array.";
        return false;
    }
    if (categoriesVal->arrayValue.size() != 8) {
        outError = L"Expected exactly 8 categories, found " +
                    std::to_wstring(categoriesVal->arrayValue.size()) + L".";
        return false;
    }

    Board board;

    // Optional top-level "difficulty" (0-10), shown in Settings and
    // recorded in match history. Left at Board's default (unrated) if the
    // file doesn't specify one.
    const JsonValue* difficultyVal = root.Find(L"difficulty");
    if (difficultyVal) {
        if (!difficultyVal->IsNumber() || difficultyVal->numberValue < 0 ||
            difficultyVal->numberValue > 10) {
            outError = L"\"difficulty\" must be a number between 0 and 10.";
            return false;
        }
        board.difficulty = static_cast<int>(difficultyVal->numberValue);
    }

    for (int col = 0; col < 8; ++col) {
        const JsonValue& catVal = categoriesVal->arrayValue[col];
        if (!catVal.IsObject()) {
            outError = L"Category " + std::to_wstring(col + 1) + L" is not a JSON object.";
            return false;
        }

        std::wstring name;
        if (!GetNonEmptyString(catVal.Find(L"name"), name)) {
            outError = L"Category " + std::to_wstring(col + 1) + L" is missing a non-empty \"name\".";
            return false;
        }

        const JsonValue* questionsVal = catVal.Find(L"questions");
        if (!questionsVal || !questionsVal->IsArray()) {
            outError = L"Category \"" + name + L"\" must have a \"questions\" array.";
            return false;
        }
        if (questionsVal->arrayValue.size() != 8) {
            outError = L"Category \"" + name + L"\" must have exactly 8 questions, found " +
                        std::to_wstring(questionsVal->arrayValue.size()) + L".";
            return false;
        }

        Category category;
        category.name = name;
        for (int row = 0; row < 8; ++row) {
            const JsonValue& qVal = questionsVal->arrayValue[row];
            if (!qVal.IsObject()) {
                outError = L"Category \"" + name + L"\", question " + std::to_wstring(row + 1) +
                            L" is not a JSON object.";
                return false;
            }

            std::wstring questionText, answerText;
            if (!GetNonEmptyString(qVal.Find(L"question"), questionText)) {
                outError = L"Category \"" + name + L"\", question " + std::to_wstring(row + 1) +
                            L" is missing a non-empty \"question\".";
                return false;
            }
            if (!GetNonEmptyString(qVal.Find(L"answer"), answerText)) {
                outError = L"Category \"" + name + L"\", question " + std::to_wstring(row + 1) +
                            L" is missing a non-empty \"answer\".";
                return false;
            }
            const JsonValue* valueVal = qVal.Find(L"value");
            if (!valueVal || !valueVal->IsNumber() || valueVal->numberValue <= 0) {
                outError = L"Category \"" + name + L"\", question " + std::to_wstring(row + 1) +
                            L" is missing a positive numeric \"value\".";
                return false;
            }

            Question q;
            q.text = std::move(questionText);
            q.answer = std::move(answerText);
            q.value = static_cast<int>(valueVal->numberValue);
            q.used = false;
            category.questions[row] = std::move(q);
        }
        board.categories[col] = std::move(category);
    }

    outBoard = std::move(board);
    return true;
}

bool LoadFinalJeopardyDictionary(const std::wstring& path, FinalJeopardyDictionary& outDictionary,
                                  std::wstring& outError) {
    JsonValue root;
    if (!LoadAndParseJson(path, root, outError)) {
        return false;
    }

    const JsonValue* cluesVal = root.Find(L"clues");
    if (!cluesVal || !cluesVal->IsArray() || cluesVal->arrayValue.empty()) {
        outError = L"Dictionary must have a non-empty \"clues\" array.";
        return false;
    }

    std::vector<FinalJeopardyClue> clues;
    clues.reserve(cluesVal->arrayValue.size());
    for (size_t i = 0; i < cluesVal->arrayValue.size(); ++i) {
        const JsonValue& clueVal = cluesVal->arrayValue[i];
        if (!clueVal.IsObject()) {
            outError = L"Clue " + std::to_wstring(i + 1) + L" is not a JSON object.";
            return false;
        }

        FinalJeopardyClue clue;
        if (!GetNonEmptyString(clueVal.Find(L"question"), clue.question)) {
            outError = L"Clue " + std::to_wstring(i + 1) + L" is missing a non-empty \"question\".";
            return false;
        }
        if (!GetNonEmptyString(clueVal.Find(L"answer"), clue.answer)) {
            outError = L"Clue " + std::to_wstring(i + 1) + L" is missing a non-empty \"answer\".";
            return false;
        }
        clues.push_back(std::move(clue));
    }

    outDictionary.category = FileStem(path);
    outDictionary.clues = std::move(clues);
    return true;
}

std::wstring DictionaryDisplayName(const std::wstring& path) {
    return path.empty() ? L"Standard" : FileStem(path);
}
