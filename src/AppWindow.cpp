#include "AppWindow.h"

#include <commctrl.h>
#include <commdlg.h>
#include <windowsx.h>
#include <cwctype>
#include "Fonts.h"
#include "Layout.h"
#include "Renderer.h"

namespace {
constexpr wchar_t kWindowClassName[] = L"JeopardyGameWindowClass";

// Style for the normal, bordered window (used by DisplayMode::Windowed).
DWORD WindowedStyle() {
    return WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
}

// Parses a Daily Double wager typed into the edit box: digits only, with
// optional surrounding whitespace and an optional leading '$' (so "$500",
// " 500 ", and "500" all work). Returns false (leaving outValue untouched)
// for anything else, including empty/negative/non-numeric input --
// GameState::SubmitWager range-checks the result regardless.
bool TryParseWagerAmount(const std::wstring& text, int& outValue) {
    size_t start = text.find_first_not_of(L" \t");
    if (start == std::wstring::npos) {
        return false;
    }
    size_t end = text.find_last_not_of(L" \t");
    std::wstring trimmed = text.substr(start, end - start + 1);

    if (!trimmed.empty() && trimmed[0] == L'$') {
        trimmed.erase(0, 1);
    }
    if (trimmed.empty()) {
        return false;
    }
    for (wchar_t c : trimmed) {
        if (!std::iswdigit(c)) {
            return false;
        }
    }
    try {
        outValue = std::stoi(trimmed);
    } catch (...) {
        return false;
    }
    return true;
}
}

bool AppWindow::Create(HINSTANCE hInstance, int nCmdShow) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &AppWindow::StaticWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr; // we paint the whole client area ourselves
    wc.lpszClassName = kWindowClassName;

    if (!RegisterClassExW(&wc)) {
        return false;
    }

    DWORD style = WindowedStyle();
    RECT windowRect = {0, 0, Layout::kClientWidth, Layout::kClientHeight};
    AdjustWindowRect(&windowRect, style, FALSE);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    hwnd_ = CreateWindowExW(
        0, kWindowClassName, L"Jeopardy", style,
        (screenWidth - windowWidth) / 2, (screenHeight - windowHeight) / 2,
        windowWidth, windowHeight,
        nullptr, nullptr, hInstance, this);

    if (!hwnd_) {
        return false;
    }

    ShowWindow(hwnd_, nCmdShow);
    UpdateWindow(hwnd_);
    return true;
}

LRESULT CALLBACK AppWindow::StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AppWindow* self = nullptr;

    if (msg == WM_NCCREATE) {
        auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<AppWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<AppWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self) {
        return self->WndProc(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT AppWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            OnCreate(hwnd);
            return 0;
        case WM_PAINT:
            OnPaint(hwnd);
            return 0;
        case WM_ERASEBKGND:
            return 1; // avoid flicker; WM_PAINT repaints the whole client area
        case WM_LBUTTONDOWN:
            OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_KEYDOWN:
            // Reached when the main window itself has keyboard focus, i.e.
            // whenever the EDIT control is hidden (Board/Result/GameOver) --
            // e.g. the Result screen's "press Enter to continue". When the
            // EDIT control is visible and focused, its own subclass handles
            // these same keys directly and this case is never hit for them.
            if (wParam == VK_RETURN) {
                OnSubmit();
            } else if (wParam == VK_ESCAPE) {
                OnEscapePressed();
            }
            return 0;
        case WM_COMMAND:
            if (HIWORD(wParam) != BN_CLICKED) {
                return 0;
            }
            switch (LOWORD(wParam)) {
                case kSubmitButtonId:
                    OnSubmit();
                    break;
                case kNewGameButtonId:
                    OnNewGame();
                    break;
                case kQuitButtonId:
                    PostMessageW(hwnd_, WM_CLOSE, 0, 0);
                    break;
                case kSettingsButtonId:
                    OnOpenSettings();
                    break;
                case kSettingsBackButtonId:
                    OnSettingsBack();
                    break;
                case kRadioWindowedId:
                    OnDisplayModeSelected(DisplayMode::Windowed);
                    break;
                case kRadioBorderlessId:
                    OnDisplayModeSelected(DisplayMode::BorderlessWindowed);
                    break;
                case kRadioFullScreenId:
                    OnDisplayModeSelected(DisplayMode::FullScreen);
                    break;
                case kRound1BrowseButtonId:
                    OnBrowseDictionary(DictionarySlot::Round1);
                    break;
                case kRound1ResetButtonId:
                    OnResetDictionary(DictionarySlot::Round1);
                    break;
                case kRound2BrowseButtonId:
                    OnBrowseDictionary(DictionarySlot::Round2);
                    break;
                case kRound2ResetButtonId:
                    OnResetDictionary(DictionarySlot::Round2);
                    break;
                case kFinalBrowseButtonId:
                    OnBrowseDictionary(DictionarySlot::FinalJeopardy);
                    break;
                case kFinalResetButtonId:
                    OnResetDictionary(DictionarySlot::FinalJeopardy);
                    break;
                case kReturnToMenuButtonId:
                    OnReturnToMainMenu();
                    break;
                case kStartRound2ButtonId:
                    OnStartRound2();
                    break;
                // Continue is created WS_DISABLED (placeholder), so
                // BN_CLICKED never fires for it.
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void AppWindow::OnCreate(HWND hwnd) {
    // hwnd_ isn't assigned until CreateWindowExW returns in Create(), but
    // WM_CREATE (and everything below, which needs a valid hwnd_ for
    // GetClientRect/MoveWindow) fires *during* that call. Set it now.
    hwnd_ = hwnd;
    HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));

    // All controls are created at a placeholder position/size; LayoutControls
    // (called at the end of this function, via SyncInputControlsToState)
    // is the single source of truth that positions them for real.
    editControl_ = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | ES_AUTOHSCROLL,
        0, 0, 400, 28,
        hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kEditControlId)),
        hInstance, nullptr);

    submitButton_ = CreateWindowExW(
        0, L"BUTTON", L"Submit",
        WS_CHILD | BS_PUSHBUTTON,
        0, 0, 100, 32,
        hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSubmitButtonId)),
        hInstance, nullptr);

    HFONT controlFont = CreateFontW(
        -18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, kGameFontName);
    SendMessageW(editControl_, WM_SETFONT, reinterpret_cast<WPARAM>(controlFont), TRUE);
    SendMessageW(submitButton_, WM_SETFONT, reinterpret_cast<WPARAM>(controlFont), TRUE);

    // Title-screen menu buttons. Continue is a placeholder (no feature
    // behind it yet) so it's created disabled/greyed-out.
    auto makeButton = [&](const wchar_t* text, int id, DWORD extraStyle) {
        HWND button = CreateWindowExW(
            0, L"BUTTON", text, WS_CHILD | BS_PUSHBUTTON | extraStyle,
            0, 0, 100, 32,
            hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInstance, nullptr);
        SendMessageW(button, WM_SETFONT, reinterpret_cast<WPARAM>(controlFont), TRUE);
        return button;
    };
    newGameButton_ = makeButton(L"New Game", kNewGameButtonId, 0);
    continueButton_ = makeButton(L"Continue", kContinueButtonId, WS_DISABLED);
    settingsButton_ = makeButton(L"Settings", kSettingsButtonId, 0);
    quitButton_ = makeButton(L"Quit", kQuitButtonId, 0);

    // Settings-screen display-mode radio buttons (grouped so Windows treats
    // them as one mutually-exclusive set) and its Back button.
    auto makeRadio = [&](const wchar_t* text, int id, DWORD extraStyle) {
        HWND radio = CreateWindowExW(
            0, L"BUTTON", text, WS_CHILD | BS_AUTORADIOBUTTON | extraStyle,
            0, 0, 100, 32,
            hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInstance, nullptr);
        SendMessageW(radio, WM_SETFONT, reinterpret_cast<WPARAM>(controlFont), TRUE);
        return radio;
    };
    radioWindowed_ = makeRadio(L"Windowed", kRadioWindowedId, WS_GROUP);
    radioBorderless_ = makeRadio(L"Borderless Windowed", kRadioBorderlessId, 0);
    radioFullScreen_ = makeRadio(L"Full Screen", kRadioFullScreenId, 0);
    SendMessageW(radioWindowed_, BM_SETCHECK, BST_CHECKED, 0); // matches displayMode_'s default
    settingsBackButton_ = makeButton(L"Back", kSettingsBackButtonId, 0);

    // Question-dictionary Browse.../Use Standard buttons (one pair per slot).
    round1BrowseButton_ = makeButton(L"Browse...", kRound1BrowseButtonId, 0);
    round1ResetButton_ = makeButton(L"Use Standard", kRound1ResetButtonId, 0);
    round2BrowseButton_ = makeButton(L"Browse...", kRound2BrowseButtonId, 0);
    round2ResetButton_ = makeButton(L"Use Standard", kRound2ResetButtonId, 0);
    finalBrowseButton_ = makeButton(L"Browse...", kFinalBrowseButtonId, 0);
    finalResetButton_ = makeButton(L"Use Standard", kFinalResetButtonId, 0);

    returnToMenuButton_ = makeButton(L"Return to Main Menu", kReturnToMenuButtonId, 0);

    startRound2Button_ = makeButton(L"Start Round 2", kStartRound2ButtonId, 0);

    // Make Enter (while the edit box has focus) trigger the same WM_COMMAND
    // as clicking Submit.
    SetWindowSubclass(editControl_, &AppWindow::EditSubclassProc, 0,
                       reinterpret_cast<DWORD_PTR>(this));

    SyncInputControlsToState();
}

LRESULT CALLBACK AppWindow::EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam,
                                              LPARAM lParam, UINT_PTR /*subclassId*/,
                                              DWORD_PTR refData) {
    if (msg == WM_KEYDOWN) {
        AppWindow* self = reinterpret_cast<AppWindow*>(refData);
        if (wParam == VK_RETURN) {
            SendMessageW(self->hwnd_, WM_COMMAND, MAKEWPARAM(kSubmitButtonId, BN_CLICKED),
                         reinterpret_cast<LPARAM>(self->submitButton_));
            return 0; // swallow the Enter keystroke (no beep, no newline attempt)
        }
        if (wParam == VK_ESCAPE) {
            // The edit box has focus during NameEntry/Question; OnEscapePressed
            // itself no-ops outside active gameplay, so this is safe either way.
            self->OnEscapePressed();
            return 0;
        }
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void AppWindow::OnPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT client;
    GetClientRect(hwnd, &client);

    // The client area can be larger than the fixed-size game canvas (Full
    // Screen mode letterboxes it), so fill the whole thing with the
    // background color first.
    HBRUSH bg = CreateSolidBrush(Renderer::kBackgroundColor);
    FillRect(hdc, &client, bg);
    DeleteObject(bg);

    // Double buffering: the game always renders into a fixed-size canvas
    // bitmap, then that bitmap is blitted at canvasOrigin_ in one shot, to
    // avoid flicker from painting shapes/text directly.
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, Layout::kClientWidth, Layout::kClientHeight);
    HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(memDC, memBitmap));

    RECT canvasRect = {0, 0, Layout::kClientWidth, Layout::kClientHeight};
    Renderer::DrawFrame(memDC, canvasRect, gameState_);

    BitBlt(hdc, canvasOrigin_.x, canvasOrigin_.y, Layout::kClientWidth, Layout::kClientHeight,
           memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);

    EndPaint(hwnd, &ps);
}

void AppWindow::OnLButtonDown(int x, int y) {
    // Convert window-client coordinates to the fixed-canvas coordinate
    // space that GameState/Layout operate in (see canvasOrigin_).
    int canvasX = x - canvasOrigin_.x;
    int canvasY = y - canvasOrigin_.y;

    AppState state = gameState_.CurrentState();
    POINT pt = {canvasX, canvasY};

    if (state == AppState::Board) {
        int row = -1, col = -1;
        if (Layout::HitTestCell(pt, row, col)) {
            gameState_.SelectCell(row, col);
            SyncInputControlsToState();
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
    } else if (state == AppState::Result) {
        // Anywhere on the result screen advances back to the board.
        gameState_.Continue();
        SyncInputControlsToState();
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void AppWindow::OnSubmit() {
    wchar_t buffer[256];
    GetWindowTextW(editControl_, buffer, ARRAYSIZE(buffer));
    std::wstring text(buffer);

    switch (gameState_.CurrentState()) {
        case AppState::NameEntry:
            if (!text.empty()) {
                gameState_.SetPlayerName(text);
                SyncInputControlsToState();
                InvalidateRect(hwnd_, nullptr, FALSE);
            }
            break;
        case AppState::Wager: {
            int amount;
            if (TryParseWagerAmount(text, amount)) {
                gameState_.SubmitWager(amount);
                // SubmitWager no-ops (stays in Wager) if amount was out of
                // range; only sync/clear the box once it's actually been
                // accepted, so a rejected wager stays visible to correct --
                // same spirit as leaving an empty name untouched below.
                if (gameState_.CurrentState() != AppState::Wager) {
                    SyncInputControlsToState();
                    InvalidateRect(hwnd_, nullptr, FALSE);
                }
            }
            break;
        }
        case AppState::Question:
            gameState_.SubmitAnswer(text);
            SyncInputControlsToState();
            InvalidateRect(hwnd_, nullptr, FALSE);
            break;
        case AppState::Result:
            // Enter also advances past the result screen (mirrors the
            // click-to-continue handling in OnLButtonDown).
            gameState_.Continue();
            SyncInputControlsToState();
            InvalidateRect(hwnd_, nullptr, FALSE);
            break;
        default:
            break;
    }
}

void AppWindow::OnNewGame() {
    gameState_.StartNewGame();
    SyncInputControlsToState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void AppWindow::OnOpenSettings() {
    gameState_.OpenSettings();
    SyncInputControlsToState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void AppWindow::OnSettingsBack() {
    gameState_.CloseSettings();
    SyncInputControlsToState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void AppWindow::OnReturnToMainMenu() {
    gameState_.ReturnToMainMenu();
    SyncInputControlsToState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void AppWindow::OnStartRound2() {
    gameState_.StartRound2();
    SyncInputControlsToState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void AppWindow::OnBrowseDictionary(DictionarySlot slot) {
    wchar_t fileBuffer[MAX_PATH] = L"";

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = L"JSON Dictionary Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = ARRAYSIZE(fileBuffer);
    ofn.lpstrTitle = L"Select a Question Dictionary";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameW(&ofn)) {
        return; // user cancelled; leave the current setting untouched
    }
    gameState_.SetDictionaryPath(slot, fileBuffer);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void AppWindow::OnResetDictionary(DictionarySlot slot) {
    gameState_.SetDictionaryPath(slot, L"");
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void AppWindow::OnEscapePressed() {
    AppState state = gameState_.CurrentState();
    bool inGameplay = (state == AppState::Board || state == AppState::Wager ||
                        state == AppState::Question || state == AppState::Result ||
                        state == AppState::Round2Intro);
    if (!inGameplay) {
        return;
    }

    // Blocking by design -- this is a deliberate "are you sure" interrupt,
    // and a native message box is the simplest correct way to get one.
    int choice = MessageBoxW(hwnd_,
                              L"Quit to the main menu? Your current game progress will be lost.",
                              L"Quit to Main Menu?", MB_YESNO | MB_ICONQUESTION);
    if (choice != IDYES) {
        return;
    }
    gameState_.QuitToMainMenu();
    SyncInputControlsToState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void AppWindow::OnDisplayModeSelected(DisplayMode mode) {
    if (mode == displayMode_) {
        return;
    }
    displayMode_ = mode;
    ApplyDisplayMode(mode);
}

void AppWindow::ApplyDisplayMode(DisplayMode mode) {
    HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo = {};
    monitorInfo.cbSize = sizeof(monitorInfo);
    GetMonitorInfoW(monitor, &monitorInfo);
    int monitorLeft = monitorInfo.rcMonitor.left;
    int monitorTop = monitorInfo.rcMonitor.top;
    int monitorWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
    int monitorHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

    DWORD style;
    int x, y, cx, cy;

    switch (mode) {
        case DisplayMode::Windowed: {
            style = WindowedStyle();
            RECT rect = {0, 0, Layout::kClientWidth, Layout::kClientHeight};
            AdjustWindowRect(&rect, style, FALSE);
            cx = rect.right - rect.left;
            cy = rect.bottom - rect.top;
            x = monitorLeft + (monitorWidth - cx) / 2;
            y = monitorTop + (monitorHeight - cy) / 2;
            break;
        }
        case DisplayMode::BorderlessWindowed: {
            style = WS_POPUP;
            cx = Layout::kClientWidth;
            cy = Layout::kClientHeight;
            x = monitorLeft + (monitorWidth - cx) / 2;
            y = monitorTop + (monitorHeight - cy) / 2;
            break;
        }
        case DisplayMode::FullScreen:
        default: {
            style = WS_POPUP;
            x = monitorLeft;
            y = monitorTop;
            cx = monitorWidth;
            cy = monitorHeight;
            break;
        }
    }

    SetWindowLongPtrW(hwnd_, GWL_STYLE, style);
    SetWindowPos(hwnd_, HWND_TOP, x, y, cx, cy, SWP_FRAMECHANGED);
    ShowWindow(hwnd_, SW_SHOW);

    LayoutControls();
    InvalidateRect(hwnd_, nullptr, TRUE);
}

void AppWindow::LayoutControls() {
    RECT client = {0, 0, Layout::kClientWidth, Layout::kClientHeight};
    if (!hwnd_ || !GetClientRect(hwnd_, &client)) {
        return; // hwnd_ not ready yet; a later call (e.g. from Create()) will lay out for real
    }
    canvasOrigin_.x = ((client.right - client.left) - Layout::kClientWidth) / 2;
    canvasOrigin_.y = ((client.bottom - client.top) - Layout::kClientHeight) / 2;
    int ox = canvasOrigin_.x;
    int oy = canvasOrigin_.y;

    auto place = [&](HWND hwnd, int left, int top, int width, int height) {
        MoveWindow(hwnd, left + ox, top + oy, width, height, TRUE);
    };

    // Title-screen menu.
    place(newGameButton_, Layout::kTitleButtonLeft, Layout::TitleButtonTop(0),
          Layout::kTitleButtonWidth, Layout::kTitleButtonHeight);
    place(continueButton_, Layout::kTitleButtonLeft, Layout::TitleButtonTop(1),
          Layout::kTitleButtonWidth, Layout::kTitleButtonHeight);
    place(settingsButton_, Layout::kTitleButtonLeft, Layout::TitleButtonTop(2),
          Layout::kTitleButtonWidth, Layout::kTitleButtonHeight);
    place(quitButton_, Layout::kTitleButtonLeft, Layout::TitleButtonTop(3),
          Layout::kTitleButtonWidth, Layout::kTitleButtonHeight);

    // Settings screen.
    place(radioWindowed_, Layout::kSettingsRadioLeft, Layout::SettingsRadioTop(0),
          Layout::kSettingsRadioWidth, Layout::kSettingsRadioHeight);
    place(radioBorderless_, Layout::kSettingsRadioLeft, Layout::SettingsRadioTop(1),
          Layout::kSettingsRadioWidth, Layout::kSettingsRadioHeight);
    place(radioFullScreen_, Layout::kSettingsRadioLeft, Layout::SettingsRadioTop(2),
          Layout::kSettingsRadioWidth, Layout::kSettingsRadioHeight);
    place(settingsBackButton_, Layout::kSettingsBackButtonLeft, Layout::kSettingsBackButtonTop,
          Layout::kSettingsBackButtonWidth, Layout::kSettingsBackButtonHeight);

    // Question-dictionary rows: Browse.../Use Standard buttons, side by side,
    // to the right of the path/status text Renderer draws for each row.
    auto placeDictButtons = [&](int rowIndex, HWND browseButton, HWND resetButton) {
        int top = Layout::SettingsDictButtonTop(rowIndex);
        place(browseButton, Layout::kSettingsDictButtonLeft, top,
              Layout::kSettingsDictButtonWidth, Layout::kSettingsDictButtonHeight);
        place(resetButton,
              Layout::kSettingsDictButtonLeft + Layout::kSettingsDictButtonWidth +
                  Layout::kSettingsDictButtonGap,
              top, Layout::kSettingsDictButtonWidth, Layout::kSettingsDictButtonHeight);
    };
    placeDictButtons(0, round1BrowseButton_, round1ResetButton_);
    placeDictButtons(1, round2BrowseButton_, round2ResetButton_);
    placeDictButtons(2, finalBrowseButton_, finalResetButton_);

    // Game Over screen.
    place(returnToMenuButton_, Layout::kGameOverButtonLeft, Layout::kGameOverButtonTop,
          Layout::kGameOverButtonWidth, Layout::kTitleButtonHeight);

    // Round 2 intro screen.
    place(startRound2Button_, Layout::kRound2ButtonLeft, Layout::kRound2ButtonTop,
          Layout::kRound2ButtonWidth, Layout::kTitleButtonHeight);

    // Name entry / answer entry (position depends on which screen it's on).
    int editWidth = 400;
    int editHeight = 28;
    int editLeft = (Layout::kClientWidth - editWidth) / 2 - 60;
    int editTop = (gameState_.CurrentState() == AppState::NameEntry)
                      ? Layout::kNameEntryEditTop
                      : (Layout::kClientHeight - 120);
    place(editControl_, editLeft, editTop, editWidth, editHeight);
    place(submitButton_, editLeft + editWidth + 10, editTop - 2, 100, editHeight + 4);
}

void AppWindow::SyncInputControlsToState() {
    LayoutControls();

    AppState state = gameState_.CurrentState();

    bool showTitleButtons = (state == AppState::Title);
    ShowWindow(newGameButton_, showTitleButtons ? SW_SHOW : SW_HIDE);
    ShowWindow(continueButton_, showTitleButtons ? SW_SHOW : SW_HIDE);
    ShowWindow(settingsButton_, showTitleButtons ? SW_SHOW : SW_HIDE);
    ShowWindow(quitButton_, showTitleButtons ? SW_SHOW : SW_HIDE);

    bool showSettingsControls = (state == AppState::Settings);
    ShowWindow(radioWindowed_, showSettingsControls ? SW_SHOW : SW_HIDE);
    ShowWindow(radioBorderless_, showSettingsControls ? SW_SHOW : SW_HIDE);
    ShowWindow(radioFullScreen_, showSettingsControls ? SW_SHOW : SW_HIDE);
    ShowWindow(settingsBackButton_, showSettingsControls ? SW_SHOW : SW_HIDE);
    ShowWindow(round1BrowseButton_, showSettingsControls ? SW_SHOW : SW_HIDE);
    ShowWindow(round1ResetButton_, showSettingsControls ? SW_SHOW : SW_HIDE);
    ShowWindow(round2BrowseButton_, showSettingsControls ? SW_SHOW : SW_HIDE);
    ShowWindow(round2ResetButton_, showSettingsControls ? SW_SHOW : SW_HIDE);
    ShowWindow(finalBrowseButton_, showSettingsControls ? SW_SHOW : SW_HIDE);
    ShowWindow(finalResetButton_, showSettingsControls ? SW_SHOW : SW_HIDE);

    ShowWindow(returnToMenuButton_, state == AppState::GameOver ? SW_SHOW : SW_HIDE);
    ShowWindow(startRound2Button_, state == AppState::Round2Intro ? SW_SHOW : SW_HIDE);

    bool needsTextInput = (state == AppState::NameEntry || state == AppState::Wager ||
                            state == AppState::Question);
    ShowWindow(editControl_, needsTextInput ? SW_SHOW : SW_HIDE);
    ShowWindow(submitButton_, needsTextInput ? SW_SHOW : SW_HIDE);
    if (needsTextInput) {
        SetWindowTextW(editControl_, L"");
        SetFocus(editControl_);
    }
}
