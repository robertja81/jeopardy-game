#pragma once

#include <array>
#include <string>

// A single cell on the board: one question/answer pair worth a dollar value.
struct Question {
    std::wstring text;
    std::wstring answer;
    int value = 0;
    bool used = false;
    // Secret until selected: the board never renders this. See
    // AssignRandomDailyDoubles.
    bool isDailyDouble = false;
};

// One column of the board: a category name plus its 8 questions (row 0..7,
// increasing dollar value).
struct Category {
    std::wstring name;
    std::array<Question, 8> questions;
};

// The full 8x8 board: 8 categories, each with 8 questions.
struct Board {
    std::array<Category, 8> categories;
    // Overall difficulty rating for this dictionary, 0-10. Defaults to a
    // neutral "unrated" middle value; LoadHardcodedBoard() overrides this
    // to kStandardDictionaryDifficulty, and a custom dictionary's optional
    // "difficulty" field (see DictionaryLoader) overrides it further.
    int difficulty = 5;
};

// The standard/built-in dictionary is deliberately rated as very easy, so
// custom dictionaries have a fixed, known baseline to compare against.
constexpr int kStandardDictionaryDifficulty = 1;

// Phase 1/2 data source: hardcoded placeholder categories/questions/answers.
// Later, LoadBoardFromJson(path) can return the same Board shape so nothing
// downstream (GameState, Renderer, AppWindow) needs to change.
Board LoadHardcodedBoard();

// Randomly picks 2 of the board's 64 questions and marks them as Daily
// Doubles, clearing the flag on every other question first (so it's safe
// to call on a board that was already used elsewhere). Called once per
// round, right after that round's board is loaded, so the assignment is
// fresh and unpredictable each game.
void AssignRandomDailyDoubles(Board& board);
