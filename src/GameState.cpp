#include "GameState.h"

#include "AnswerMatcher.h"

GameState::GameState() : board_(LoadHardcodedBoard()) {
    round1Status_.message = L"Using standard dictionary (8 categories, 64 questions).";
    round2Status_.message = L"Using standard dictionary (8 categories, 64 questions).";
    finalStatus_.message = L"Using standard Final Jeopardy dictionary. Not used in gameplay yet.";
    RefreshHistoryAndStats();
}

void GameState::RefreshHistoryAndStats() {
    matchHistory_ = LoadMatchHistory();
    mostAnsweredCorrectly_ = GetMostAnsweredCorrectly();
    mostAnsweredIncorrectly_ = GetMostAnsweredIncorrectly();
    mostPopularDictionary_ = GetMostPopularDictionary();
}

void GameState::StartNewGame() {
    if (state_ != AppState::Title) {
        return;
    }
    // Apply whichever Round 1 dictionary is currently valid. An invalid
    // custom dictionary silently falls back to the standard one -- Settings
    // shows the error, so the game is never blocked by a bad file.
    board_ = (!round1Status_.path.empty() && round1Status_.isValid) ? pendingRound1Board_
                                                                     : LoadHardcodedBoard();
    AssignRandomDailyDoubles(board_);
    // Reload from disk so a game that just finished (Continue() records it
    // on GameOver) shows up immediately if the player is starting another.
    RefreshHistoryAndStats();
    score_ = 0;
    currentRound_ = 1;
    selectedRow_ = -1;
    selectedCol_ = -1;
    currentWager_ = 0;
    answeredCount_ = 0;
    lastAnswerCorrect_ = false;
    lastAnswerWasPartialMatch_ = false;
    lastScoreDelta_ = 0;
    state_ = AppState::NameEntry;
}

void GameState::OpenSettings() {
    if (state_ != AppState::Title) {
        return;
    }
    state_ = AppState::Settings;
}

void GameState::CloseSettings() {
    if (state_ != AppState::Settings) {
        return;
    }
    state_ = AppState::Title;
}

void GameState::SetDictionaryPath(DictionarySlot slot, std::wstring path) {
    DictionaryStatus status;
    status.path = path;

    if (slot == DictionarySlot::FinalJeopardy) {
        if (path.empty()) {
            status.isValid = true;
            status.message = L"Using standard Final Jeopardy dictionary. Not used in gameplay yet.";
        } else {
            FinalJeopardyDictionary parsed;
            std::wstring error;
            if (LoadFinalJeopardyDictionary(path, parsed, error)) {
                status.isValid = true;
                status.message = L"OK: " + std::to_wstring(parsed.clues.size()) +
                                  L" clue(s), category \"" + parsed.category +
                                  L"\". Not used in gameplay yet.";
                pendingFinalDictionary_ = std::move(parsed);
            } else {
                status.isValid = false;
                status.message = error + L" Not used in gameplay yet.";
            }
        }
        finalStatus_ = std::move(status);
        return;
    }

    bool isRound1 = (slot == DictionarySlot::Round1);
    Board* pendingBoard = isRound1 ? &pendingRound1Board_ : &pendingRound2Board_;

    if (path.empty()) {
        status.isValid = true;
        status.difficulty = kStandardDictionaryDifficulty;
        status.message = L"Using standard dictionary (8 categories, 64 questions). Difficulty: " +
                          std::to_wstring(status.difficulty) + L"/10.";
    } else {
        Board parsed;
        std::wstring error;
        if (LoadRoundDictionary(path, parsed, error)) {
            status.isValid = true;
            status.difficulty = parsed.difficulty;
            status.message = L"OK: 8 categories, 64 questions loaded. Difficulty: " +
                              std::to_wstring(status.difficulty) + L"/10.";
            *pendingBoard = std::move(parsed);
        } else {
            status.isValid = false;
            status.difficulty = kStandardDictionaryDifficulty; // gameplay falls back to standard
            status.message = error + L" Standard dictionary will be used instead.";
        }
    }

    if (isRound1) {
        round1Status_ = std::move(status);
    } else {
        round2Status_ = std::move(status);
    }
}

void GameState::SetPlayerName(std::wstring name) {
    if (state_ != AppState::NameEntry) {
        return;
    }
    playerName_ = std::move(name);
    state_ = AppState::Board;
}

void GameState::SelectCell(int row, int col) {
    if (state_ != AppState::Board) {
        return;
    }
    if (row < 0 || row > 7 || col < 0 || col > 7) {
        return;
    }
    const Question& q = board_.categories[col].questions[row];
    if (q.used) {
        return;
    }
    selectedRow_ = row;
    selectedCol_ = col;
    currentWager_ = 0;
    state_ = q.isDailyDouble ? AppState::Wager : AppState::Question;
}

void GameState::SubmitWager(int amount) {
    if (state_ != AppState::Wager) {
        return;
    }
    if (amount < MinDailyDoubleWager() || amount > MaxDailyDoubleWager()) {
        return;
    }
    currentWager_ = amount;
    state_ = AppState::Question;
}

void GameState::SubmitAnswer(std::wstring text) {
    if (state_ != AppState::Question) {
        return;
    }
    const Question& q = SelectedQuestion();
    AnswerMatch match = MatchAnswer(text, q.answer);
    lastAnswerCorrect_ = (match != AnswerMatch::Incorrect);
    lastAnswerWasPartialMatch_ = (match == AnswerMatch::Partial);
    int stakes = q.isDailyDouble ? currentWager_ : q.value;
    lastScoreDelta_ = lastAnswerCorrect_ ? stakes : -stakes;
    score_ += lastScoreDelta_;
    RecordQuestionAttempt(q.text, lastAnswerCorrect_);
    state_ = AppState::Result;
}

void GameState::Continue() {
    if (state_ != AppState::Result) {
        return;
    }
    board_.categories[selectedCol_].questions[selectedRow_].used = true;
    ++answeredCount_;
    selectedRow_ = -1;
    selectedCol_ = -1;

    if (answeredCount_ < 64) {
        state_ = AppState::Board;
        return;
    }
    if (currentRound_ == 1) {
        state_ = AppState::Round2Intro;
        return;
    }

    // Round 2 just finished -- the whole game is over.
    MatchRecord record;
    record.playerName = playerName_;
    record.round1DictionaryName = DictionaryDisplayName(round1Status_.path);
    record.round2DictionaryName = DictionaryDisplayName(round2Status_.path);
    record.finalJeopardyDictionaryName = DictionaryDisplayName(finalStatus_.path);
    record.finalScore = score_;
    record.difficulty = (round1Status_.difficulty + round2Status_.difficulty) / 2.0;
    AppendMatchHistory(record);
    RecordDictionaryRun(record.round1DictionaryName);
    RecordDictionaryRun(record.round2DictionaryName);
    state_ = AppState::GameOver;
}

void GameState::StartRound2() {
    if (state_ != AppState::Round2Intro) {
        return;
    }
    // Apply whichever Round 2 dictionary is currently valid (falling back
    // to the standard one, same policy as Round 1), then double every
    // value regardless of source -- Round 2 always means Double Jeopardy
    // stakes, so a dictionary authored for Round 1 (e.g. one of the
    // themed $100-$800 files) works fine here too without extra setup.
    Board round2Board = (!round2Status_.path.empty() && round2Status_.isValid)
                             ? pendingRound2Board_
                             : LoadHardcodedBoard();
    for (Category& category : round2Board.categories) {
        for (Question& question : category.questions) {
            question.value *= 2;
        }
    }
    AssignRandomDailyDoubles(round2Board);
    board_ = std::move(round2Board);
    currentRound_ = 2;
    answeredCount_ = 0;
    selectedRow_ = -1;
    selectedCol_ = -1;
    currentWager_ = 0;
    state_ = AppState::Board;
}

void GameState::ReturnToMainMenu() {
    if (state_ != AppState::GameOver) {
        return;
    }
    state_ = AppState::Title;
}

void GameState::QuitToMainMenu() {
    if (state_ != AppState::Board && state_ != AppState::Wager && state_ != AppState::Question &&
        state_ != AppState::Result && state_ != AppState::Round2Intro) {
        return;
    }
    state_ = AppState::Title;
}
