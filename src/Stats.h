#pragma once

#include <optional>
#include <string>

// One row for the "Question Most Answered Correctly/Incorrectly" stat
// lines on the Name Entry screen.
struct QuestionAttemptSummary {
    std::wstring questionText;
    int count = 0; // number of correct (or incorrect) attempts recorded for it
};

// One row for the "Most Popular Dictionary" stat line.
struct DictionaryPopularitySummary {
    std::wstring dictionaryName;
    int runCount = 0;
};

// Records one answer attempt (correct or not) for `questionText`,
// persisting the updated tally to disk immediately. Best-effort: failures
// are silently ignored, matching MatchHistory's philosophy -- stats are a
// nice-to-have, never something that blocks gameplay.
void RecordQuestionAttempt(const std::wstring& questionText, bool wasCorrect);

// Records that `dictionaryName` was played as a round dictionary in a
// just-completed game (called once per round actually played, so a game
// that reuses the same dictionary for both rounds counts it twice).
void RecordDictionaryRun(const std::wstring& dictionaryName);

// Each returns std::nullopt if no relevant data has been recorded yet.
// Ties are broken by whichever was recorded first.
std::optional<QuestionAttemptSummary> GetMostAnsweredCorrectly();
std::optional<QuestionAttemptSummary> GetMostAnsweredIncorrectly();
std::optional<DictionaryPopularitySummary> GetMostPopularDictionary();
