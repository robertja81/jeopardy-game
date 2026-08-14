#pragma once

// Shared UI font family, used by both Renderer (in-game text) and AppWindow
// (native EDIT/BUTTON controls) so the whole app matches consistently.
// "ITC Korinna" is the closest match to the on-air Jeopardy! board/clue font.
// If it isn't installed, GDI falls back to a default UI font automatically.
constexpr wchar_t kGameFontName[] = L"ITC Korinna";

// The "JEOPARDY!" title-screen logo specifically uses the show's actual
// logo face, not the board font.
constexpr wchar_t kTitleFontName[] = L"Swiss911 XCm BT";
