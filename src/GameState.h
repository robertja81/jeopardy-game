#pragma once

#include "DictionaryLoader.h"
#include "GameData.h"
#include "MatchHistory.h"
#include "Stats.h"

// Which screen the app is currently showing / what input it expects.
enum class AppState {
    Title,
    Settings,
    NameEntry,
    Board,
    Wager,
    Question,
    Result,
    Round2Intro,
    GameOver,
};

// Which dictionary slot a Settings action applies to. Round1 and Round2
// both feed gameplay; FinalJeopardy is validated and stored for the Final
// Jeopardy feature, which doesn't exist yet.
enum class DictionarySlot { Round1, Round2, FinalJeopardy };

// Validation result for one dictionary slot, as shown in Settings.
struct DictionaryStatus {
    std::wstring path;    // empty means "use the standard/default dictionary"
    bool isValid = true;
    std::wstring message;  // human-readable status or error, shown in Settings
    int difficulty = kStandardDictionaryDifficulty; // 0-10, matches the loaded Board's rating
};

// Owns all mutable game data. This class has no knowledge of Win32 or GDI --
// it is pure game logic/state so it stays easy to reason about (and reuse)
// as rendering and input handling evolve independently.
class GameState {
public:
    GameState();

    AppState CurrentState() const { return state_; }
    const std::wstring& PlayerName() const { return playerName_; }
    int Score() const { return score_; }
    const Board& GetBoard() const { return board_; }
    // 1 or 2. Shown on the board banner; Round 2's values are always
    // double whatever the underlying dictionary specifies (see
    // StartRound2), matching the real show's Double Jeopardy round.
    int CurrentRound() const { return currentRound_; }

    // Valid only while CurrentState() is Wager, Question, or Result.
    int SelectedRow() const { return selectedRow_; }
    int SelectedCol() const { return selectedCol_; }
    const Question& SelectedQuestion() const {
        return board_.categories[selectedCol_].questions[selectedRow_];
    }

    // Valid only while CurrentState() is Wager. The minimum a Daily Double
    // wager may be is always 0; the maximum is the player's current score,
    // or -- if that's zero or negative -- the square's original face
    // value, so a player "in the hole" can still wager up to what the
    // square would normally have been worth.
    int MinDailyDoubleWager() const { return 0; }
    int MaxDailyDoubleWager() const {
        return (score_ > 0) ? score_ : SelectedQuestion().value;
    }
    // Valid once a wager has been submitted (Question/Result states, for
    // the currently-selected Daily Double question).
    int CurrentWager() const { return currentWager_; }

    bool LastAnswerCorrect() const { return lastAnswerCorrect_; }
    // True when the last answer was accepted via lenient/partial matching
    // (e.g. "Mozart" for "Wolfgang Amadeus Mozart") rather than matching
    // the canonical answer outright -- the Result screen uses this to
    // highlight the full answer for confirmation.
    bool LastAnswerWasPartialMatch() const { return lastAnswerWasPartialMatch_; }
    // Signed amount the last answer added (+) or subtracted (-) from score_.
    int LastScoreDelta() const { return lastScoreDelta_; }

    const DictionaryStatus& Round1DictionaryStatus() const { return round1Status_; }
    const DictionaryStatus& Round2DictionaryStatus() const { return round2Status_; }
    const DictionaryStatus& FinalJeopardyDictionaryStatus() const { return finalStatus_; }

    // Recent completed games (most-recent-first), shown on the Name Entry
    // screen. Refreshed from disk at the start of every game (see
    // StartNewGame), so a game that just finished appears immediately.
    const std::vector<MatchRecord>& RecentMatchHistory() const { return matchHistory_; }

    // Aggregate trivia stats shown alongside the match history table.
    // std::nullopt until there's at least one recorded data point.
    // Refreshed at the same time as RecentMatchHistory().
    const std::optional<QuestionAttemptSummary>& MostAnsweredCorrectly() const {
        return mostAnsweredCorrectly_;
    }
    const std::optional<QuestionAttemptSummary>& MostAnsweredIncorrectly() const {
        return mostAnsweredIncorrectly_;
    }
    const std::optional<DictionaryPopularitySummary>& MostPopularDictionary() const {
        return mostPopularDictionary_;
    }

    // Title -> NameEntry. Applies the currently-validated Round 1 dictionary
    // (or the standard one, if none is set / the custom one is invalid) as
    // this game's board.
    void StartNewGame();

    // Title -> Settings
    void OpenSettings();

    // Settings -> Title
    void CloseSettings();

    // Validates `path` for `slot` and records the result (see
    // *DictionaryStatus above); an empty path resets that slot to the
    // standard/default dictionary. Safe to call at any time -- it never
    // touches the board of a game already in progress. Round 1's and Round
    // 2's validated dictionaries take effect at the next StartNewGame() /
    // StartRound2() respectively.
    void SetDictionaryPath(DictionarySlot slot, std::wstring path);

    // NameEntry -> Board
    void SetPlayerName(std::wstring name);

    // Board -> Question, or Board -> Wager if the selected cell is a
    // (secret, until now) Daily Double. No-op if the cell is already used
    // or we're not on the board screen.
    void SelectCell(int row, int col);

    // Wager -> Question. No-op if amount is outside
    // [MinDailyDoubleWager(), MaxDailyDoubleWager()] -- AppWindow validates
    // the typed text first, but this stays defensive regardless of caller.
    void SubmitWager(int amount);

    // Question -> Result. Scores immediately: adds the stakes (the wager,
    // for a Daily Double; the cell's face value otherwise) on a correct
    // answer, subtracts them on an incorrect one.
    void SubmitAnswer(std::wstring text);

    // Result -> Board (marks the cell used), -> Round2Intro (Round 1
    // finished), or -> GameOver (Round 2 finished) once all 64 cells on
    // the active board have been answered.
    void Continue();

    // Round2Intro -> Board. Loads the Round 2 dictionary (or the standard
    // one, if none is set / the custom one is invalid) with every value
    // doubled, and resets used/answered tracking for the new board -- the
    // running score carries over unchanged.
    void StartRound2();

    // GameOver -> Title
    void ReturnToMainMenu();

    // Board/Wager/Question/Result/Round2Intro -> Title. Abandons the game
    // in progress without recording it anywhere -- only a completed game
    // (reaching GameOver) counts toward match history or stats.
    void QuitToMainMenu();

private:
    // Reloads matchHistory_ and the three aggregate stats accessors from
    // disk. Called from the constructor and from StartNewGame() (so a game
    // that just finished shows up immediately).
    void RefreshHistoryAndStats();

    AppState state_ = AppState::Title;
    std::wstring playerName_;
    int score_ = 0;
    Board board_;
    int currentRound_ = 1;

    int selectedRow_ = -1;
    int selectedCol_ = -1;
    int currentWager_ = 0;
    int answeredCount_ = 0;
    bool lastAnswerCorrect_ = false;
    bool lastAnswerWasPartialMatch_ = false;
    int lastScoreDelta_ = 0;

    DictionaryStatus round1Status_;
    DictionaryStatus round2Status_;
    DictionaryStatus finalStatus_;
    // Last successfully validated custom board/dictionary for each slot.
    // Round1's is applied by StartNewGame(); Round2's by StartRound2();
    // Final is stored for a feature that doesn't exist yet.
    Board pendingRound1Board_;
    Board pendingRound2Board_;
    FinalJeopardyDictionary pendingFinalDictionary_;

    std::vector<MatchRecord> matchHistory_;
    std::optional<QuestionAttemptSummary> mostAnsweredCorrectly_;
    std::optional<QuestionAttemptSummary> mostAnsweredIncorrectly_;
    std::optional<DictionaryPopularitySummary> mostPopularDictionary_;
};
