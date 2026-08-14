#pragma once

#include <windows.h>
#include "GameState.h"

// Read-only drawing functions: never mutate GameState, only read from it.
// AppWindow is responsible for positioning/showing the EDIT+BUTTON controls
// that overlay these screens; Renderer only draws static content.
namespace Renderer {

// Classic Jeopardy navy. Exposed so AppWindow can paint the letterbox area
// around the fixed-size canvas in Full Screen mode with a matching color.
constexpr COLORREF kBackgroundColor = RGB(10, 10, 60);

// Draws the entire current screen (based on state.CurrentState()) into hdc,
// which the caller has already set up as an off-screen, double-buffered DC
// sized to `client`.
void DrawFrame(HDC hdc, const RECT& client, const GameState& state);

// Draws the studio splash/intro screen (pixel-art skunk mascot + animated
// "SkunkWorks Studios" wordmark) into hdc, sized to `client`. Independent
// of GameState -- this plays before any game state exists, purely driven
// by `elapsedMs` (milliseconds since the intro started) for its animation.
void DrawIntroScreen(HDC hdc, const RECT& client, int elapsedMs);

} // namespace Renderer
