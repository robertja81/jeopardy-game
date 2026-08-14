#pragma once

#include <string>
#include <vector>

// One completed game, as shown in the Name Entry screen's history table.
// The three dictionary fields are display names (see
// DictionaryDisplayName in DictionaryLoader.h) -- "Standard" or a filename
// stem -- not full paths.
struct MatchRecord {
    std::wstring playerName;
    std::wstring round1DictionaryName;
    std::wstring round2DictionaryName;
    std::wstring finalJeopardyDictionaryName;
    int finalScore = 0;
    // Average of the Round 1 and Round 2 dictionaries' difficulty ratings
    // (0-10; the standard dictionary is 1) at the time this game was
    // played. A snapshot, not a live lookup -- editing a dictionary's
    // rating later doesn't retroactively change past history rows.
    double difficulty = 1.0;
};

// Most match-history entries kept/shown, oldest dropped first.
constexpr size_t kMaxHistoryEntries = 10;

// Loads recent match history from disk, most-recent-first. Returns an
// empty list if none has been recorded yet, or if the file can't be
// read/parsed -- history is a nice-to-have, never something that blocks
// the game.
std::vector<MatchRecord> LoadMatchHistory();

// Adds `record` as the newest entry in the on-disk history (creating the
// file/directory the first time) and trims it to kMaxHistoryEntries.
// Best-effort: failures are silently ignored, since losing history should
// never crash or block gameplay.
void AppendMatchHistory(const MatchRecord& record);
