#include "Renderer.h"

#include <cstdio>
#include <cstdlib>

#include "Fonts.h"
#include "Layout.h"

namespace {

void FillBackground(HDC hdc, const RECT& client) {
    HBRUSH bg = CreateSolidBrush(Renderer::kBackgroundColor);
    FillRect(hdc, &client, bg);
    DeleteObject(bg);
}

HFONT CreateUiFont(int pointSize, bool bold, const wchar_t* fontName = kGameFontName) {
    return CreateFontW(
        -pointSize, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, fontName);
}

// Note: DT_VCENTER only takes effect together with DT_SINGLELINE, so it has
// no effect on this word-wrapped text. Vertical centering is instead done
// manually: measure the wrapped text's height via DT_CALCRECT, then offset
// the draw rect so that height is centered in `rect`.
void DrawCenteredText(HDC hdc, const RECT& rect, const std::wstring& text,
                       int pointSize, COLORREF color, bool bold = false,
                       const wchar_t* fontName = kGameFontName) {
    HFONT font = CreateUiFont(pointSize, bold, fontName);
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);

    RECT measured = {rect.left, 0, rect.right, 0};
    DrawTextW(hdc, text.c_str(), -1, &measured, DT_CENTER | DT_WORDBREAK | DT_CALCRECT);
    int textHeight = measured.bottom - measured.top;
    int rectHeight = rect.bottom - rect.top;

    RECT drawRect = rect;
    drawRect.top += (rectHeight - textHeight) / 2;
    DrawTextW(hdc, text.c_str(), -1, &drawRect, DT_CENTER | DT_WORDBREAK);

    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

// Like DrawCenteredText, but picks the largest font size in
// [minPointSize, maxPointSize] whose word-wrapped text still fits inside
// `rect` (minus `padding` on each side), so the text grows to fill boxes of
// varying size (board cells) instead of using one fixed size everywhere.
void DrawAutoFitText(HDC hdc, const RECT& rect, const std::wstring& text,
                      int minPointSize, int maxPointSize, COLORREF color,
                      bool bold, int padding, const wchar_t* fontName = kGameFontName) {
    RECT inner = rect;
    InflateRect(&inner, -padding, -padding);
    int innerWidth = inner.right - inner.left;
    int innerHeight = inner.bottom - inner.top;
    if (innerWidth <= 0 || innerHeight <= 0) {
        return;
    }

    int chosenSize = minPointSize;
    for (int pointSize = maxPointSize; pointSize >= minPointSize; --pointSize) {
        HFONT font = CreateUiFont(pointSize, bold, fontName);
        HFONT oldFont = (HFONT)SelectObject(hdc, font);

        RECT measured = {0, 0, innerWidth, 0};
        DrawTextW(hdc, text.c_str(), -1, &measured, DT_CENTER | DT_WORDBREAK | DT_CALCRECT);

        SelectObject(hdc, oldFont);
        DeleteObject(font);

        if (measured.bottom - measured.top <= innerHeight) {
            chosenSize = pointSize;
            break;
        }
    }

    DrawCenteredText(hdc, inner, text, chosenSize, color, bold, fontName);
}

// Left-aligned, single-line (long text is ellipsized, not wrapped) --
// distinct from DrawCenteredText, used for file paths/status lines where
// centering or wrapping would read oddly.
void DrawLeftAlignedText(HDC hdc, const RECT& rect, const std::wstring& text,
                          int pointSize, COLORREF color, bool bold = false) {
    HFONT font = CreateUiFont(pointSize, bold);
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);

    RECT textRect = rect;
    DrawTextW(hdc, text.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

void DrawTitleScreen(HDC hdc, const RECT& client, const GameState&) {
    RECT titleRect = client;
    titleRect.top = 40;
    titleRect.bottom = Layout::kTitleTextBottom;
    DrawAutoFitText(hdc, titleRect, L"JEOPARDY!", 60, 140, RGB(255, 215, 0), true, 20,
                     kTitleFontName);
    // The New Game/Continue/Settings/Quit buttons are native controls
    // positioned by AppWindow below the title text.
}

void DrawSettingsScreen(HDC hdc, const RECT& client, const GameState& state) {
    RECT titleRect = client;
    titleRect.bottom = Layout::kSettingsTitleBottom;
    DrawCenteredText(hdc, titleRect, L"Settings", 44, RGB(255, 215, 0), true);

    RECT labelRect = client;
    labelRect.top = Layout::kSettingsLabelTop;
    labelRect.bottom = Layout::kSettingsLabelBottom;
    DrawCenteredText(hdc, labelRect, L"Display:", 24, RGB(255, 255, 255), true);
    // The Windowed/Borderless Windowed/Full Screen radio buttons are native
    // controls positioned by AppWindow below this.

    RECT dictHeaderRect = client;
    dictHeaderRect.top = Layout::kSettingsDictSectionLabelTop;
    dictHeaderRect.bottom = Layout::kSettingsDictSectionLabelBottom;
    DrawCenteredText(hdc, dictHeaderRect, L"Question Dictionaries:", 24, RGB(255, 255, 255), true);

    auto drawDictRow = [&](int index, const wchar_t* label, const DictionaryStatus& status) {
        RECT rowRect = Layout::SettingsDictTextRect(index);
        int half = (rowRect.bottom - rowRect.top) / 2;

        RECT pathRect = rowRect;
        pathRect.bottom = rowRect.top + half;
        std::wstring pathLine =
            std::wstring(label) + L": " + (status.path.empty() ? L"(Standard)" : status.path);
        DrawLeftAlignedText(hdc, pathRect, pathLine, 16, RGB(255, 255, 255), true);

        RECT statusRect = rowRect;
        statusRect.top = rowRect.top + half;
        std::wstring statusLine = (status.isValid ? L"OK: " : L"Error: ") + status.message;
        COLORREF statusColor = status.isValid ? RGB(120, 220, 120) : RGB(230, 90, 90);
        DrawLeftAlignedText(hdc, statusRect, statusLine, 14, statusColor);
    };

    drawDictRow(0, L"Round 1", state.Round1DictionaryStatus());
    drawDictRow(1, L"Round 2", state.Round2DictionaryStatus());
    drawDictRow(2, L"Final Jeopardy", state.FinalJeopardyDictionaryStatus());
    // The Browse.../Use Standard buttons for each row are native controls
    // positioned by AppWindow to the right of this text.
}

// Draws one match-history cell: fills its background, then left-aligns
// `text` inside with a small padding inset.
void DrawHistoryCell(HDC hdc, RECT rect, const std::wstring& text, COLORREF bgColor,
                      COLORREF textColor, int pointSize, bool bold = false) {
    HBRUSH brush = CreateSolidBrush(bgColor);
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);

    RECT textRect = rect;
    textRect.left += 10;
    textRect.right -= 6;
    DrawLeftAlignedText(hdc, textRect, text, pointSize, textColor, bold);
}

std::wstring FormatOneDecimal(double value) {
    wchar_t buf[32];
    swprintf_s(buf, L"%.1f", value);
    return buf;
}

void DrawHistoryTable(HDC hdc, const GameState& state) {
    static const wchar_t* kHeaders[Layout::kHistoryColumnCount] = {
        L"Name",        L"Dictionary 1", L"Dictionary 2", L"Final Jeopardy Dictionary",
        L"Difficulty",  L"Final Score",
    };
    // Left-aligned with the same inset as DrawHistoryCell below, so header
    // text lines up with the row data underneath it instead of a centered
    // header sitting over left-aligned rows.
    for (int col = 0; col < Layout::kHistoryColumnCount; ++col) {
        DrawHistoryCell(hdc, Layout::HistoryCellRect(-1, col), kHeaders[col], RGB(20, 20, 110),
                         RGB(255, 215, 0), 14, true);
    }

    const std::vector<MatchRecord>& history = state.RecentMatchHistory();
    if (history.empty()) {
        RECT emptyRect = Layout::HistoryCellRect(0, 0);
        emptyRect.right = Layout::HistoryCellRect(0, Layout::kHistoryColumnCount - 1).right;
        DrawCenteredText(hdc, emptyRect, L"No games played yet.", 15, RGB(180, 180, 180));
        return;
    }

    size_t rowCount = history.size() < kMaxHistoryEntries ? history.size() : kMaxHistoryEntries;
    for (size_t row = 0; row < rowCount; ++row) {
        const MatchRecord& record = history[row];
        COLORREF bgColor = (row % 2 == 0) ? RGB(15, 15, 75) : RGB(10, 10, 60);

        DrawHistoryCell(hdc, Layout::HistoryCellRect(static_cast<int>(row), 0), record.playerName,
                         bgColor, RGB(255, 255, 255), 15, true);
        DrawHistoryCell(hdc, Layout::HistoryCellRect(static_cast<int>(row), 1),
                         record.round1DictionaryName, bgColor, RGB(220, 220, 220), 14);
        DrawHistoryCell(hdc, Layout::HistoryCellRect(static_cast<int>(row), 2),
                         record.round2DictionaryName, bgColor, RGB(220, 220, 220), 14);
        DrawHistoryCell(hdc, Layout::HistoryCellRect(static_cast<int>(row), 3),
                         record.finalJeopardyDictionaryName, bgColor, RGB(220, 220, 220), 14);
        DrawHistoryCell(hdc, Layout::HistoryCellRect(static_cast<int>(row), 4),
                         FormatOneDecimal(record.difficulty) + L"/10", bgColor,
                         RGB(220, 220, 220), 14);
        DrawHistoryCell(hdc, Layout::HistoryCellRect(static_cast<int>(row), 5),
                         L"$" + std::to_wstring(record.finalScore), bgColor, RGB(255, 215, 0), 15,
                         true);
    }
}

void DrawStatsSection(HDC hdc, const GameState& state) {
    auto drawStatLine = [&](int index, const std::wstring& text) {
        DrawLeftAlignedText(hdc, Layout::StatsRowRect(index), text, 16, RGB(220, 220, 220));
    };

    const auto& mostCorrect = state.MostAnsweredCorrectly();
    drawStatLine(0, mostCorrect
                         ? L"Question Most Answered Correctly: \"" + mostCorrect->questionText +
                               L"\" (" + std::to_wstring(mostCorrect->count) +
                               L" successful attempts)"
                         : L"Question Most Answered Correctly: (no data yet)");

    const auto& mostIncorrect = state.MostAnsweredIncorrectly();
    drawStatLine(1, mostIncorrect
                         ? L"Question Most Answered Incorrectly: \"" +
                               mostIncorrect->questionText + L"\" (" +
                               std::to_wstring(mostIncorrect->count) + L" failed attempts)"
                         : L"Question Most Answered Incorrectly: (no data yet)");

    const auto& mostPopular = state.MostPopularDictionary();
    drawStatLine(2, mostPopular
                         ? L"Most Popular Dictionary: " + mostPopular->dictionaryName + L" (" +
                               std::to_wstring(mostPopular->runCount) + L" runs)"
                         : L"Most Popular Dictionary: (no data yet)");
}

void DrawNameEntryScreen(HDC hdc, const RECT& client, const GameState& state) {
    RECT promptRect = client;
    promptRect.bottom = Layout::kNameEntryPromptBottom;
    // Auto-fit (rather than a fixed size) so the prompt can never grow tall
    // enough to overlap the EDIT control positioned just below it.
    DrawAutoFitText(hdc, promptRect, L"Enter your name:", 16, 32, RGB(255, 255, 255), true, 4);
    // The actual EDIT + BUTTON controls are positioned on top of this
    // screen by AppWindow.

    DrawHistoryTable(hdc, state);
    DrawStatsSection(hdc, state);
}

void DrawBoardScreen(HDC hdc, const RECT& client, const GameState& state) {
    // Top banner: round + player name + score.
    RECT banner = client;
    banner.bottom = Layout::kBoardTop;
    std::wstring bannerText = L"Round " + std::to_wstring(state.CurrentRound()) + L"    " +
                               state.PlayerName() + L"    Score: $" +
                               std::to_wstring(state.Score());
    DrawCenteredText(hdc, banner, bannerText, 28, RGB(255, 255, 255), true);

    const Board& board = state.GetBoard();

    // Category headers. Font size grows to fill each header box, since
    // category name length varies ("Music" vs. "Food & Drink").
    for (int col = 0; col < 8; ++col) {
        RECT r = Layout::CategoryHeaderRect(col);
        HBRUSH headerBrush = CreateSolidBrush(RGB(20, 20, 110));
        FillRect(hdc, &r, headerBrush);
        DeleteObject(headerBrush);
        DrawAutoFitText(hdc, r, board.categories[col].name, 10, 22, RGB(255, 215, 0), true, 6);
    }

    // Question value cells. Font size grows to fill each cell box.
    for (int col = 0; col < 8; ++col) {
        for (int row = 0; row < 8; ++row) {
            RECT r = Layout::CellRect(row, col);
            const Question& q = board.categories[col].questions[row];

            HBRUSH cellBrush = CreateSolidBrush(
                q.used ? RGB(10, 10, 60) : RGB(20, 20, 110));
            FillRect(hdc, &r, cellBrush);
            DeleteObject(cellBrush);

            if (!q.used) {
                std::wstring valueText = L"$" + std::to_wstring(q.value);
                DrawAutoFitText(hdc, r, valueText, 14, 32, RGB(255, 215, 0), true, 6);
            }
        }
    }
}

void DrawQuestionScreen(HDC hdc, const RECT& client, const GameState& state) {
    const Question& q = state.SelectedQuestion();
    const std::wstring& categoryName = state.GetBoard().categories[state.SelectedCol()].name;
    // A Daily Double is played for the player's wager, not the cell's face
    // value -- show whichever one actually determines the score swing.
    int stakes = q.isDailyDouble ? state.CurrentWager() : q.value;

    RECT header = client;
    header.bottom = Layout::kBoardTop;
    DrawCenteredText(hdc, header, categoryName + L" - $" + std::to_wstring(stakes),
                      24, RGB(255, 215, 0), true);

    RECT questionArea = client;
    questionArea.top = Layout::kBoardTop + 20;
    questionArea.bottom = client.bottom - 140;
    DrawCenteredText(hdc, questionArea, q.text, 32, RGB(255, 255, 255));
    // The EDIT + BUTTON controls for the answer are positioned by AppWindow
    // below this text.
}

void DrawWagerScreen(HDC hdc, const RECT& client, const GameState& state) {
    const std::wstring& categoryName = state.GetBoard().categories[state.SelectedCol()].name;

    RECT header = client;
    header.bottom = Layout::kBoardTop;
    DrawCenteredText(hdc, header, categoryName, 24, RGB(255, 215, 0), true);

    RECT titleRect = client;
    titleRect.top = Layout::kBoardTop + 20;
    titleRect.bottom = 260;
    DrawAutoFitText(hdc, titleRect, L"DAILY DOUBLE!", 50, 90, RGB(255, 215, 0), true, 20,
                     kTitleFontName);

    RECT promptRect = client;
    promptRect.top = 280;
    promptRect.bottom = 340;
    DrawCenteredText(hdc, promptRect,
                      L"Enter your wager: $" + std::to_wstring(state.MinDailyDoubleWager()) +
                          L" - $" + std::to_wstring(state.MaxDailyDoubleWager()),
                      22, RGB(255, 255, 255));
    // The EDIT + BUTTON controls for the wager are positioned by AppWindow
    // below this text.
}

void DrawResultScreen(HDC hdc, const RECT& client, const GameState& state) {
    const Question& q = state.SelectedQuestion();
    bool correct = state.LastAnswerCorrect();
    COLORREF verdictColor = correct ? RGB(80, 220, 80) : RGB(220, 80, 80);

    RECT verdictRect = client;
    verdictRect.bottom = 140;
    DrawCenteredText(hdc, verdictRect, correct ? L"Correct!" : L"Incorrect.", 40, verdictColor, true);

    int delta = state.LastScoreDelta();
    std::wstring deltaText = (delta >= 0 ? L"+$" : L"-$") + std::to_wstring(std::abs(delta));
    RECT deltaRect = client;
    deltaRect.top = 140;
    deltaRect.bottom = 230;
    DrawCenteredText(hdc, deltaRect, deltaText, 32, verdictColor, true);

    // A partial-match answer ("Mozart" for "Wolfgang Amadeus Mozart") still
    // counts as correct, but the full canonical answer is called out in
    // gold so the player can see exactly what was accepted for.
    bool partial = state.LastAnswerWasPartialMatch();
    RECT answerRect = client;
    answerRect.top = 230;
    answerRect.bottom = 330;
    std::wstring answerLabel = partial ? L"Full Answer: " : L"Answer: ";
    COLORREF answerColor = partial ? RGB(255, 215, 0) : RGB(255, 255, 255);
    DrawCenteredText(hdc, answerRect, answerLabel + q.answer, 26, answerColor, partial);

    RECT scoreRect = client;
    scoreRect.top = 330;
    scoreRect.bottom = 410;
    DrawCenteredText(hdc, scoreRect, L"Total Score: $" + std::to_wstring(state.Score()),
                      26, RGB(255, 215, 0), true);

    RECT hintRect = client;
    hintRect.top = client.bottom - 80;
    DrawCenteredText(hdc, hintRect, L"Click or press Enter to continue", 18, RGB(200, 200, 200));
}

void DrawGameOverScreen(HDC hdc, const RECT& client, const GameState& state) {
    RECT titleRect = client;
    titleRect.bottom = 200;
    DrawCenteredText(hdc, titleRect, L"Game Over", 48, RGB(255, 215, 0), true);

    RECT nameRect = client;
    nameRect.top = 200;
    nameRect.bottom = 320;
    DrawCenteredText(hdc, nameRect, state.PlayerName(), 30, RGB(255, 255, 255));

    RECT scoreRect = client;
    scoreRect.top = 320;
    scoreRect.bottom = 420;
    DrawCenteredText(hdc, scoreRect, L"Final Score: $" + std::to_wstring(state.Score()),
                      36, RGB(255, 215, 0), true);
    // The "Return to Main Menu" button is a native control positioned by
    // AppWindow below this.
}

void DrawRound2IntroScreen(HDC hdc, const RECT& client, const GameState& state) {
    RECT titleRect = client;
    titleRect.top = 120;
    titleRect.bottom = 280;
    DrawAutoFitText(hdc, titleRect, L"Round 2!", 50, 100, RGB(255, 215, 0), true, 20,
                     kTitleFontName);

    RECT subRect = client;
    subRect.top = 300;
    subRect.bottom = 360;
    DrawCenteredText(hdc, subRect, L"All values are doubled.", 22, RGB(255, 255, 255));

    RECT scoreRect = client;
    scoreRect.top = 380;
    scoreRect.bottom = 440;
    DrawCenteredText(hdc, scoreRect, L"Current Score: $" + std::to_wstring(state.Score()),
                      26, RGB(255, 215, 0), true);
    // The "Start Round 2" button is a native control positioned by
    // AppWindow below this.
}

} // namespace

namespace Renderer {

void DrawFrame(HDC hdc, const RECT& client, const GameState& state) {
    FillBackground(hdc, client);

    switch (state.CurrentState()) {
        case AppState::Title:
            DrawTitleScreen(hdc, client, state);
            break;
        case AppState::Settings:
            DrawSettingsScreen(hdc, client, state);
            break;
        case AppState::NameEntry:
            DrawNameEntryScreen(hdc, client, state);
            break;
        case AppState::Board:
            DrawBoardScreen(hdc, client, state);
            break;
        case AppState::Wager:
            DrawWagerScreen(hdc, client, state);
            break;
        case AppState::Question:
            DrawQuestionScreen(hdc, client, state);
            break;
        case AppState::Result:
            DrawResultScreen(hdc, client, state);
            break;
        case AppState::Round2Intro:
            DrawRound2IntroScreen(hdc, client, state);
            break;
        case AppState::GameOver:
            DrawGameOverScreen(hdc, client, state);
            break;
    }
}

} // namespace Renderer
