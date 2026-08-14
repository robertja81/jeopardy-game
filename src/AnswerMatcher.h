#pragma once

#include <string>

// The single seam for answer-checking logic.
enum class AnswerMatch {
    Incorrect,
    // Matches the canonical answer once case, punctuation, and extra
    // whitespace differences are ignored -- e.g. "Guns N' Roses" vs.
    // "Guns 'N Roses", or "T-Rex" vs. "T Rex".
    Exact,
    // Not identical, but the given answer is a contiguous, whole-word
    // piece of the canonical answer (or vice versa) -- e.g. "Mozart" for
    // "Wolfgang Amadeus Mozart", or "Urals" for "Ural Mountains" (plain
    // plural/singular differences are tolerated too). Also covers spelled-
    // out vs. digit numbers zero through twenty ("Six" vs. "6"), and a
    // small typo in one word ("Wilt Chamberlin" for "Wilt Chamberlain").
    // Counts as correct, but the UI should show the full canonical answer
    // for confirmation.
    Partial,
};

// Judges `given` against `canonical`. Both Exact and Partial count as a
// correct answer for scoring purposes (see GameState::SubmitAnswer).
AnswerMatch MatchAnswer(const std::wstring& given, const std::wstring& canonical);
