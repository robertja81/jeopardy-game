#include "AnswerMatcher.h"

#include <algorithm>
#include <cwctype>
#include <utility>
#include <vector>

namespace {

// Lowercases, and treats punctuation as either a word boundary or as
// nothing, then collapses whitespace to single spaces. Apostrophes/quote
// marks are dropped outright (not treated as a boundary) so contractions
// normalize the same regardless of where the apostrophe sits -- "Guns N'
// Roses" and "Guns 'N Roses" both become "guns n roses". Everything else
// non-alphanumeric (spaces, hyphens, periods, commas, ...) becomes a word
// boundary, so "T-Rex" and "T Rex" both become "t rex".
std::wstring Normalize(const std::wstring& s) {
    std::wstring result;
    result.reserve(s.size());
    bool lastWasSpace = true; // true so leading separators produce no leading space
    for (wchar_t c : s) {
        // Straight apostrophe (U+0027) or curly single quotes (U+2018/U+2019,
        // written as escapes so this survives regardless of source-file
        // encoding) are removed outright, not treated as a boundary.
        if (std::iswalnum(c)) {
            result += static_cast<wchar_t>(std::towlower(c));
            lastWasSpace = false;
        } else if (c == L'\'' || c == L'‘' || c == L'’') {
            continue;
        } else if (!lastWasSpace) {
            result += L' ';
            lastWasSpace = true;
        }
    }
    while (!result.empty() && result.back() == L' ') {
        result.pop_back();
    }
    return result;
}

// True if `needle` appears as a contiguous run of whole words inside
// `haystack` (both already Normalize()'d). Padding both with boundary
// spaces means a plain substring search only matches at word boundaries --
// e.g. "art" does not match inside "mozart".
bool IsWholeWordSubsequence(const std::wstring& needle, const std::wstring& haystack) {
    if (needle.empty()) {
        return false;
    }
    std::wstring paddedNeedle = L" " + needle + L" ";
    std::wstring paddedHaystack = L" " + haystack + L" ";
    return paddedHaystack.find(paddedNeedle) != std::wstring::npos;
}

// Below this length, a given answer is too short/generic to safely accept
// as a partial match (e.g. a stray single letter shouldn't auto-match).
constexpr size_t kMinPartialMatchLength = 3;

// Applies `wordTransform` to every space-separated word in an already-
// Normalize()'d string, preserving single-space separators.
template <typename WordTransform>
std::wstring TransformWords(const std::wstring& normalized, WordTransform wordTransform) {
    std::wstring result;
    size_t start = 0;
    while (start <= normalized.size()) {
        size_t end = normalized.find(L' ', start);
        if (end == std::wstring::npos) {
            end = normalized.size();
        }
        if (!result.empty()) {
            result += L' ';
        }
        result += wordTransform(normalized.substr(start, end - start));
        if (end == normalized.size()) {
            break;
        }
        start = end + 1;
    }
    return result;
}

// Strips one trailing 's' from a single word, if that leaves at least 2
// characters -- "urals" -> "ural", "mountains" -> "mountain", but a short
// word like "us" is left alone. Deliberately simple: this is meant to
// smooth over plain plural/singular mismatches, not do real stemming.
std::wstring SingularizeWord(const std::wstring& word) {
    if (word.size() > 2 && word.back() == L's') {
        return word.substr(0, word.size() - 1);
    }
    return word;
}

// Small number-word -> digit table (zero..twenty), covering the range
// actual trivia answers use ("Six" strings, "Eighteen" holes, "Five"
// players) -- not a general number-word parser.
std::wstring DigitizeWord(const std::wstring& word) {
    static const std::pair<const wchar_t*, const wchar_t*> kNumberWords[] = {
        {L"zero", L"0"},      {L"one", L"1"},      {L"two", L"2"},
        {L"three", L"3"},     {L"four", L"4"},     {L"five", L"5"},
        {L"six", L"6"},       {L"seven", L"7"},    {L"eight", L"8"},
        {L"nine", L"9"},      {L"ten", L"10"},     {L"eleven", L"11"},
        {L"twelve", L"12"},   {L"thirteen", L"13"}, {L"fourteen", L"14"},
        {L"fifteen", L"15"},  {L"sixteen", L"16"}, {L"seventeen", L"17"},
        {L"eighteen", L"18"}, {L"nineteen", L"19"}, {L"twenty", L"20"},
    };
    for (const auto& entry : kNumberWords) {
        if (word == entry.first) {
            return entry.second;
        }
    }
    return word;
}

// Splits an already-Normalize()'d string into its space-separated words.
std::vector<std::wstring> SplitWords(const std::wstring& normalized) {
    std::vector<std::wstring> words;
    size_t start = 0;
    while (start <= normalized.size()) {
        size_t end = normalized.find(L' ', start);
        if (end == std::wstring::npos) {
            end = normalized.size();
        }
        if (end > start) {
            words.push_back(normalized.substr(start, end - start));
        }
        if (end == normalized.size()) {
            break;
        }
        start = end + 1;
    }
    return words;
}

// Levenshtein edit distance (insertions/deletions/substitutions) between
// two words.
int EditDistance(const std::wstring& a, const std::wstring& b) {
    size_t n = a.size(), m = b.size();
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1));
    for (size_t i = 0; i <= n; ++i) dp[i][0] = static_cast<int>(i);
    for (size_t j = 0; j <= m; ++j) dp[0][j] = static_cast<int>(j);
    for (size_t i = 1; i <= n; ++i) {
        for (size_t j = 1; j <= m; ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost});
        }
    }
    return dp[n][m];
}

// How many single-character edits (insert/delete/substitute) a word of
// this length is allowed to be off by and still count as a typo rather
// than a different word. Short words require an exact match -- a 1-off
// tolerance on a 3-letter word accepts almost anything.
int MaxTypoDistanceForWordLength(size_t length) {
    if (length <= 3) return 0;
    if (length <= 6) return 1;
    return 2;
}

// Two words are "close enough" if identical, or within
// MaxTypoDistanceForWordLength of each other -- e.g. "Chamberlin" (10) vs.
// "Chamberlain" (11), edit distance 1, allowed 2 for an 11-letter word.
bool WordsAreCloseEnough(const std::wstring& a, const std::wstring& b) {
    if (a == b) {
        return true;
    }
    size_t maxLen = std::max(a.size(), b.size());
    int allowed = MaxTypoDistanceForWordLength(maxLen);
    if (allowed == 0) {
        return false;
    }
    size_t lenDiff = (a.size() > b.size()) ? (a.size() - b.size()) : (b.size() - a.size());
    if (static_cast<int>(lenDiff) > allowed) {
        return false; // cheap reject before running the O(n*m) comparison below
    }
    return EditDistance(a, b) <= allowed;
}

// True if `needleWords` appears as a contiguous run within `haystackWords`,
// comparing each word pair with WordsAreCloseEnough instead of requiring
// exact equality -- so one typo'd word in a multi-word answer doesn't sink
// the whole match.
bool IsFuzzyWholeWordSubsequence(const std::vector<std::wstring>& needleWords,
                                  const std::vector<std::wstring>& haystackWords) {
    if (needleWords.empty() || needleWords.size() > haystackWords.size()) {
        return false;
    }
    for (size_t start = 0; start + needleWords.size() <= haystackWords.size(); ++start) {
        bool allMatch = true;
        for (size_t i = 0; i < needleWords.size(); ++i) {
            if (!WordsAreCloseEnough(needleWords[i], haystackWords[start + i])) {
                allMatch = false;
                break;
            }
        }
        if (allMatch) {
            return true;
        }
    }
    return false;
}

} // namespace

AnswerMatch MatchAnswer(const std::wstring& given, const std::wstring& canonical) {
    std::wstring normalizedGiven = Normalize(given);
    std::wstring normalizedCanonical = Normalize(canonical);

    if (normalizedGiven.empty()) {
        return AnswerMatch::Incorrect;
    }
    if (normalizedGiven == normalizedCanonical) {
        return AnswerMatch::Exact;
    }

    // Number-word vs. digit equivalence ("Six" vs. "6") is checked before
    // the short-answer length gate below -- unlike a generic short
    // substring, digits 0-20 are a small closed set, so a short given
    // answer can't falsely match here.
    std::wstring digitGiven = TransformWords(normalizedGiven, DigitizeWord);
    std::wstring digitCanonical = TransformWords(normalizedCanonical, DigitizeWord);
    if (digitGiven == digitCanonical) {
        return AnswerMatch::Partial;
    }

    if (normalizedGiven.size() < kMinPartialMatchLength) {
        return AnswerMatch::Incorrect;
    }
    // Either the player gave a whole-word piece of the full answer
    // ("Mozart" for "Wolfgang Amadeus Mozart") or extra context around it
    // ("the great Mozart" for "Mozart").
    if (IsWholeWordSubsequence(normalizedGiven, normalizedCanonical) ||
        IsWholeWordSubsequence(normalizedCanonical, normalizedGiven)) {
        return AnswerMatch::Partial;
    }
    // Retry the whole-word checks after applying plural stripping to the
    // (already digit-normalized) forms, so combinations like "Urals" for
    // "Ural Mountains" resolve too, without corrupting the exact-match
    // check above (a genuinely different word, e.g. "Andes" vs. "Ares",
    // won't happen to reduce to the same thing).
    std::wstring canonicalizedGiven = TransformWords(digitGiven, SingularizeWord);
    std::wstring canonicalizedCanonical = TransformWords(digitCanonical, SingularizeWord);
    if (canonicalizedGiven == canonicalizedCanonical ||
        IsWholeWordSubsequence(canonicalizedGiven, canonicalizedCanonical) ||
        IsWholeWordSubsequence(canonicalizedCanonical, canonicalizedGiven)) {
        return AnswerMatch::Partial;
    }
    // Last resort: tolerate a small typo in one word of the (already
    // digit/plural-normalized) answer -- e.g. "Wilt Chamberlin" for "Wilt
    // Chamberlain". Word-level edit distance rather than a fixed prefix
    // length so it scales with how long/distinctive the word is.
    std::vector<std::wstring> givenWords = SplitWords(canonicalizedGiven);
    std::vector<std::wstring> canonicalWords = SplitWords(canonicalizedCanonical);
    if (IsFuzzyWholeWordSubsequence(givenWords, canonicalWords) ||
        IsFuzzyWholeWordSubsequence(canonicalWords, givenWords)) {
        return AnswerMatch::Partial;
    }
    return AnswerMatch::Incorrect;
}
