#pragma once

#include <windows.h>
#include "GameState.h"

// How the main window is presented. Windowed/BorderlessWindowed always size
// the window to exactly the fixed game canvas (Layout::kClientWidth x
// kClientHeight); FullScreen sizes the window to the monitor and letterboxes
// that same fixed canvas in the middle (see AppWindow::LayoutControls).
enum class DisplayMode {
    Windowed,
    BorderlessWindowed,
    FullScreen,
};

// The only file that touches Win32 window/message APIs. Owns the HWND, the
// one reusable EDIT+BUTTON control pair (used for both name entry and
// answer entry), the title/settings screen controls, and the GameState
// instance. Translates window messages into GameState calls, and repaints
// via Renderer.
class AppWindow {
public:
    // Registers the window class (once) and creates the window. Returns
    // true on success.
    bool Create(HINSTANCE hInstance, int nCmdShow);

    HWND Handle() const { return hwnd_; }

private:
    static constexpr int kSubmitButtonId = 102;
    static constexpr int kNewGameButtonId = 103;
    static constexpr int kContinueButtonId = 104;
    static constexpr int kSettingsButtonId = 105;
    static constexpr int kQuitButtonId = 106;
    static constexpr int kRadioWindowedId = 107;
    static constexpr int kRadioBorderlessId = 108;
    static constexpr int kRadioFullScreenId = 109;
    static constexpr int kSettingsBackButtonId = 110;
    static constexpr int kRound1BrowseButtonId = 111;
    static constexpr int kRound1ResetButtonId = 112;
    static constexpr int kRound2BrowseButtonId = 113;
    static constexpr int kRound2ResetButtonId = 114;
    static constexpr int kFinalBrowseButtonId = 115;
    static constexpr int kFinalResetButtonId = 116;
    static constexpr int kReturnToMenuButtonId = 117;
    static constexpr int kStartRound2ButtonId = 118;

    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Subclass proc on editControl_: pressing Enter while the edit box has
    // focus fires the same WM_COMMAND the Submit button would, instead of
    // requiring a mouse click. Registered via SetWindowSubclass in OnCreate.
    static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam,
                                              LPARAM lParam, UINT_PTR subclassId,
                                              DWORD_PTR refData);

    void OnCreate(HWND hwnd);
    void OnPaint(HWND hwnd);
    void OnLButtonDown(int x, int y);
    void OnSubmit();
    void OnNewGame();
    void OnOpenSettings();
    void OnSettingsBack();
    void OnReturnToMainMenu();
    void OnStartRound2();
    void OnDisplayModeSelected(DisplayMode mode);
    // Opens a native "select a file" dialog; on a successful pick, hands the
    // chosen path to GameState::SetDictionaryPath for validation. Does
    // nothing if the user cancels.
    void OnBrowseDictionary(DictionarySlot slot);
    // Resets a dictionary slot back to the standard/default dictionary.
    void OnResetDictionary(DictionarySlot slot);
    // Esc during active gameplay (Board/Question/Result/Round2Intro) shows
    // a blocking Yes/No confirmation; Yes abandons the current game and
    // returns to Title. A no-op in any other screen.
    void OnEscapePressed();

    // Resizes/repositions the window itself for `mode`, then repositions
    // every child control to match (see LayoutControls). No-op if `mode`
    // is already the current mode.
    void ApplyDisplayMode(DisplayMode mode);

    // Single source of truth for every child control's position: recomputes
    // canvasOrigin_ (where the fixed-size game canvas sits within the
    // current, possibly larger, client area) and moves every control
    // relative to it. Called after creation, on every state change, and
    // whenever the window is resized/repositioned by ApplyDisplayMode.
    void LayoutControls();

    // Shows/hides the EDIT+BUTTON pair, the title-screen menu, and the
    // settings-screen controls for the current AppState, and clears any
    // previously entered text.
    void SyncInputControlsToState();

    HWND hwnd_ = nullptr;
    HWND editControl_ = nullptr;
    HWND submitButton_ = nullptr;

    // Title-screen menu. Continue is a placeholder (created WS_DISABLED)
    // until save/load exists.
    HWND newGameButton_ = nullptr;
    HWND continueButton_ = nullptr;
    HWND settingsButton_ = nullptr;
    HWND quitButton_ = nullptr;

    // Settings screen.
    HWND radioWindowed_ = nullptr;
    HWND radioBorderless_ = nullptr;
    HWND radioFullScreen_ = nullptr;
    HWND settingsBackButton_ = nullptr;
    HWND round1BrowseButton_ = nullptr;
    HWND round1ResetButton_ = nullptr;
    HWND round2BrowseButton_ = nullptr;
    HWND round2ResetButton_ = nullptr;
    HWND finalBrowseButton_ = nullptr;
    HWND finalResetButton_ = nullptr;

    // Game Over screen.
    HWND returnToMenuButton_ = nullptr;

    // Round 2 intro screen.
    HWND startRound2Button_ = nullptr;

    DisplayMode displayMode_ = DisplayMode::Windowed;
    // Top-left of the fixed-size game canvas within the actual client area.
    // (0,0) except in FullScreen mode, where the client area is larger than
    // the canvas and this centers it (letterboxing).
    POINT canvasOrigin_ = {0, 0};

    GameState gameState_;

    static constexpr int kEditControlId = 101;
};
