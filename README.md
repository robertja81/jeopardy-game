# Jeopardy Game

A native Win32 desktop Jeopardy game for Windows, written in C++. Built for personal/hobby use — host your own two-round game with Daily Doubles, custom question dictionaries, and match history tracking.

![Platform](https://img.shields.io/badge/platform-Windows-blue) ![Language](https://img.shields.io/badge/language-C%2B%2B17-orange)

## Features

- Classic two-round board: 8 categories × 8 clues per round, Round 2 values automatically doubled
- 2 secret Daily Doubles per round with custom wagering (wager up to your score, or the square's face value if you're in the red)
- Lenient answer matching: tolerates punctuation differences, plurals, spelled-out numbers, and small typos, while still requiring the right answer
- Fully custom question dictionaries loaded from JSON — swap in your own categories/questions per round via Settings, independently for Round 1, Round 2, and Final Jeopardy
- 15 ready-made dictionaries included (see [`dictionaries/`](dictionaries/)): Video Games, World History, Art & Artists, Classic Literature, Science & Technology, Movies & TV, Geography, Mythology, Music, Sports, plus 5 Final Jeopardy sets
- Per-dictionary difficulty ratings (0–10)
- Match history and aggregate trivia stats (most-answered questions, most-played dictionary), persisted across sessions
- Windowed / Borderless Windowed / Full Screen display modes
- Esc-to-quit confirmation during gameplay

## Installing

Download the latest installer from the [Releases](../../releases) page and run it. No other dependencies required — the app statically links its runtime.

## Building from source

Requires Visual Studio 2022 (or the MSVC Build Tools) and CMake (both ship with VS2022).

```powershell
# From a "Developer PowerShell for VS 2022":
git clone https://github.com/robertja81/jeopardy-game.git
cd jeopardy-game
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
.\build\JeopardyGame.exe
```

## Custom dictionaries

Question dictionaries are plain JSON files, loaded via Settings → Question Dictionaries. Two formats:

**Round dictionary** (Round 1 / Round 2) — exactly 8 categories, each with exactly 8 questions:

```json
{
  "difficulty": 5,
  "categories": [
    {
      "name": "Category Name",
      "questions": [
        { "question": "Clue text", "answer": "Correct answer", "value": 100 }
      ]
    }
  ]
}
```

`difficulty` is optional (0–10; omit for unrated). Round 2 always doubles whatever `value` is specified, so the same file works for either round.

**Final Jeopardy dictionary** — any number of clues; the category is taken from the file name, not stored in the file, so multiple clues can share a category without spoiling which one gets picked:

```json
{
  "clues": [
    { "question": "Clue text", "answer": "Correct answer" }
  ]
}
```

See [`dictionaries/`](dictionaries/) and [`data/round1_standard.json`](data/round1_standard.json) for complete examples.

## Project structure

```
src/            Game source (see file-level comments for architecture notes)
data/           Built-in dictionary template
dictionaries/   15 ready-made custom dictionaries (Round/ and FinalJeopardy/)
installer/      Inno Setup script used to build the Windows installer
CMakeLists.txt
```

## License

Personal hobby project. No license file yet — treat as all-rights-reserved unless the repo owner says otherwise.
