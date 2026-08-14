#pragma once

#include <windows.h>
#include "MatchHistory.h"

// Pure geometry constants for the fixed-size window and the 8x8 board.
// Shared by Renderer (drawing) and AppWindow (click hit-testing) so they
// never disagree about where a cell is.
namespace Layout {

constexpr int kClientWidth = 1280;
constexpr int kClientHeight = 820;

constexpr int kBoardMarginX = 20;
constexpr int kBoardTop = 110;      // leaves room above the board for name/score text
constexpr int kHeaderHeight = 70;   // category name row
constexpr int kCellWidth = (kClientWidth - 2 * kBoardMarginX) / 8;
constexpr int kCellHeight = 70;
constexpr int kCellGap = 2;

// Name-entry screen: prompt text stays above kNameEntryPromptBottom, the
// EDIT/Submit controls sit at kNameEntryEditTop. Shared between Renderer
// (draws the prompt) and AppWindow (positions the controls) so the prompt
// text can never grow into the input box.
constexpr int kNameEntryPromptBottom = 60;
constexpr int kNameEntryEditTop = 70;

// Name-entry screen match history table: sits below the input box, showing
// up to kMaxHistoryEntries past games (most recent first) plus a header
// row. Columns: Name | Dictionary 1 | Dictionary 2 | Final Jeopardy
// Dictionary | Difficulty | Final Score.
constexpr int kHistoryTableTop = 150;
constexpr int kHistoryRowHeight = 42;
constexpr int kHistoryTableLeft = kBoardMarginX;
constexpr int kHistoryTableWidth = kClientWidth - 2 * kBoardMarginX;
constexpr int kHistoryColNameWidth = 220;
constexpr int kHistoryColDict1Width = 250;
constexpr int kHistoryColDict2Width = 250;
constexpr int kHistoryColFinalWidth = 250;
constexpr int kHistoryColDifficultyWidth = 120;
constexpr int kHistoryColScoreWidth =
    kHistoryTableWidth - kHistoryColNameWidth - kHistoryColDict1Width -
    kHistoryColDict2Width - kHistoryColFinalWidth - kHistoryColDifficultyWidth;
constexpr int kHistoryColumnCount = 6;

// Rect for column `col` (0=Name, 1=Dict1, 2=Dict2, 3=Final, 4=Difficulty,
// 5=Score) at table row `row` (-1 = header row, 0-based data rows below).
inline RECT HistoryCellRect(int row, int col) {
    static const int widths[kHistoryColumnCount] = {
        kHistoryColNameWidth,     kHistoryColDict1Width, kHistoryColDict2Width,
        kHistoryColFinalWidth,    kHistoryColDifficultyWidth, kHistoryColScoreWidth,
    };
    int left = kHistoryTableLeft;
    for (int i = 0; i < col; ++i) {
        left += widths[i];
    }
    RECT r;
    r.left = left;
    r.top = kHistoryTableTop + (row + 1) * kHistoryRowHeight; // +1 for the header row at row=-1
    r.right = left + widths[col];
    r.bottom = r.top + kHistoryRowHeight - 2;
    return r;
}

// Name-entry screen aggregate stats section (Most Answered Correctly/
// Incorrectly, Most Popular Dictionary), shown below the match history
// table as three left-aligned lines.
constexpr int kStatsSectionTop = 630;
constexpr int kStatsRowHeight = 32;
constexpr int kStatsSectionLeft = kBoardMarginX;
constexpr int kStatsSectionWidth = kClientWidth - 2 * kBoardMarginX;

inline RECT StatsRowRect(int index) {
    RECT r;
    r.left = kStatsSectionLeft;
    r.top = kStatsSectionTop + index * kStatsRowHeight;
    r.right = r.left + kStatsSectionWidth;
    r.bottom = r.top + kStatsRowHeight - 2;
    return r;
}

// Title screen: "JEOPARDY!" sits in the top portion of the window; the menu
// buttons (New Game/Continue/Settings/Quit) stack vertically below it.
constexpr int kTitleTextBottom = 340;
constexpr int kTitleButtonWidth = 260;
constexpr int kTitleButtonHeight = 50;
constexpr int kTitleButtonGap = 20;
constexpr int kTitleButtonFirstTop = 420;
constexpr int kTitleButtonLeft = (kClientWidth - kTitleButtonWidth) / 2;

// Top of the Nth (0-indexed) title-screen menu button.
inline int TitleButtonTop(int index) {
    return kTitleButtonFirstTop + index * (kTitleButtonHeight + kTitleButtonGap);
}

// Game Over screen: title/name/score text occupies the top ~420px; the
// "Return to Main Menu" button sits below that. Wider than a title-screen
// button (same height) since its label is much longer than "New Game".
constexpr int kGameOverButtonWidth = 340;
constexpr int kGameOverButtonTop = 460;
constexpr int kGameOverButtonLeft = (kClientWidth - kGameOverButtonWidth) / 2;

// Round 2 intro screen: "Round 2!" announcement text occupies the top
// ~440px; the "Start Round 2" button sits below that.
constexpr int kRound2ButtonWidth = 300;
constexpr int kRound2ButtonTop = 460;
constexpr int kRound2ButtonLeft = (kClientWidth - kRound2ButtonWidth) / 2;

// Settings screen: title text, a "Display:" label, three stacked radio
// buttons, then a Back button near the bottom.
constexpr int kSettingsTitleBottom = 140;
constexpr int kSettingsLabelTop = 160;
constexpr int kSettingsLabelBottom = 210;
constexpr int kSettingsRadioWidth = 320;
constexpr int kSettingsRadioHeight = 36;
constexpr int kSettingsRadioGap = 14;
constexpr int kSettingsRadioFirstTop = 230;
constexpr int kSettingsRadioLeft = (kClientWidth - kSettingsRadioWidth) / 2;
constexpr int kSettingsBackButtonWidth = 160;
constexpr int kSettingsBackButtonHeight = 50;
constexpr int kSettingsBackButtonTop = 730;
constexpr int kSettingsBackButtonLeft = (kClientWidth - kSettingsBackButtonWidth) / 2;

// Top of the Nth (0-indexed) settings-screen radio button.
inline int SettingsRadioTop(int index) {
    return kSettingsRadioFirstTop + index * (kSettingsRadioHeight + kSettingsRadioGap);
}

// Settings screen, "Question Dictionaries" section: a header label, then one
// row per dictionary slot (Round 1 / Round 2 / Final Jeopardy). Each row has
// a two-line text block on the left (drawn by Renderer: path on top, status
// below) and two native buttons (Browse.../Use Standard) on the right,
// positioned by AppWindow.
constexpr int kSettingsDictSectionLabelTop = 380;
constexpr int kSettingsDictSectionLabelBottom = 420;
constexpr int kSettingsDictRowHeight = 74;
constexpr int kSettingsDictRowGap = 10;
constexpr int kSettingsDictFirstRowTop = 430;
constexpr int kSettingsDictTextLeft = kBoardMarginX;
constexpr int kSettingsDictTextWidth = 760;
constexpr int kSettingsDictButtonLeft = 800;
constexpr int kSettingsDictButtonWidth = 140;
constexpr int kSettingsDictButtonHeight = 32;
constexpr int kSettingsDictButtonGap = 10;

// Top of the Nth (0-indexed: 0=Round1, 1=Round2, 2=FinalJeopardy) dictionary row.
inline int SettingsDictRowTop(int index) {
    return kSettingsDictFirstRowTop + index * (kSettingsDictRowHeight + kSettingsDictRowGap);
}

// Text block rect (path + status, drawn by Renderer) for dictionary row `index`.
inline RECT SettingsDictTextRect(int index) {
    RECT r;
    r.left = kSettingsDictTextLeft;
    r.top = SettingsDictRowTop(index);
    r.right = r.left + kSettingsDictTextWidth;
    r.bottom = r.top + kSettingsDictRowHeight;
    return r;
}

// Vertically-centered top for the Browse/Use Standard buttons on dictionary row `index`.
inline int SettingsDictButtonTop(int index) {
    return SettingsDictRowTop(index) + (kSettingsDictRowHeight - kSettingsDictButtonHeight) / 2;
}

// Rect of the category header cell for column `col` (0..7).
inline RECT CategoryHeaderRect(int col) {
    RECT r;
    r.left = kBoardMarginX + col * kCellWidth;
    r.top = kBoardTop;
    r.right = r.left + kCellWidth - kCellGap;
    r.bottom = r.top + kHeaderHeight - kCellGap;
    return r;
}

// Rect of the question cell at `row` (0..7, increasing dollar value) in
// column `col` (0..7).
inline RECT CellRect(int row, int col) {
    RECT r;
    r.left = kBoardMarginX + col * kCellWidth;
    r.top = kBoardTop + kHeaderHeight + row * kCellHeight;
    r.right = r.left + kCellWidth - kCellGap;
    r.bottom = r.top + kCellHeight - kCellGap;
    return r;
}

// Hit-tests a client-area point against the 8x8 question grid. Returns true
// and sets outRow/outCol if the point falls inside a question cell.
inline bool HitTestCell(POINT pt, int& outRow, int& outCol) {
    for (int col = 0; col < 8; ++col) {
        for (int row = 0; row < 8; ++row) {
            RECT r = CellRect(row, col);
            if (PtInRect(&r, pt)) {
                outRow = row;
                outCol = col;
                return true;
            }
        }
    }
    return false;
}

} // namespace Layout
