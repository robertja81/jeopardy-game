#include <windows.h>
#include "AppWindow.h"

// AppWindow subclasses its EDIT control with SetWindowSubclass/DefSubclassProc
// (comctl32 v6 APIs). Without this manifest dependency, the OS may bind to
// the legacy comctl32.dll (v5.82) that lacks those exports, and the app
// would fail to launch.
#pragma comment(linker,                                                         \
    "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' " \
    "version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' " \
    "language='*'\"")

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    AppWindow appWindow;
    if (!appWindow.Create(hInstance, nCmdShow)) {
        return 1;
    }

    // Enter-to-submit is handled by subclassing the EDIT control directly
    // (see AppWindow::EditSubclassProc), so the message loop stays plain.
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
