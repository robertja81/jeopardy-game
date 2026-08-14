#pragma once

#include <string>
#include <vector>
#include "GameData.h"

// A single Final Jeopardy clue.
struct FinalJeopardyClue {
    std::wstring question;
    std::wstring answer;
};

// A Final Jeopardy dictionary: unlike a round board, this is intentionally
// loose -- any number (>=1) of question/answer clues, all sharing one
// category. The category is derived from the file's name (not stored in the
// file itself), so that a single file can hold several clues for that
// category and the game can randomly pick one without the category name
// giving away which clue was chosen.
struct FinalJeopardyDictionary {
    std::wstring category;
    std::vector<FinalJeopardyClue> clues;
};

// Loads and validates a round-board dictionary (used for Round 1 and Round
// 2): a JSON object with a "categories" array of exactly 8 entries, each
// with a non-empty "name" and a "questions" array of exactly 8 entries, each
// with a non-empty "question", non-empty "answer", and positive numeric
// "value". An optional top-level "difficulty" (a number 0-10) sets
// outBoard.difficulty; if omitted, Board's default (unrated) is kept. On
// success returns true and fills outBoard; on failure returns false and
// fills outError with a message suitable for display in Settings.
bool LoadRoundDictionary(const std::wstring& path, Board& outBoard, std::wstring& outError);

// Loads and validates a Final Jeopardy dictionary: a JSON object with a
// non-empty "clues" array, each entry having a non-empty "question" and
// "answer". On success returns true and fills outDictionary (category taken
// from the file name); on failure returns false and fills outError.
bool LoadFinalJeopardyDictionary(const std::wstring& path, FinalJeopardyDictionary& outDictionary,
                                  std::wstring& outError);

// Returns a short, human-readable name for a dictionary path, suitable for
// UI display (e.g. the match history table): "Standard" for an empty path
// (the built-in default), or the file's name without directory/extension
// otherwise (e.g. "C:\Jeopardy\Dictionaries\Round\VideoGames.json" ->
// "VideoGames").
std::wstring DictionaryDisplayName(const std::wstring& path);
