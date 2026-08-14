cmake_minimum_required(VERSION 3.20)
project(JeopardyGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Statically link the MSVC C++ runtime so the built .exe runs on a machine
# without the Visual C++ Redistributable installed -- important once this
# is distributed rather than just run on this dev machine.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(JeopardyGame WIN32
    src/main.cpp
    src/AppWindow.cpp
    src/GameState.cpp
    src/GameData.cpp
    src/AnswerMatcher.cpp
    src/Renderer.cpp
    src/Json.cpp
    src/DictionaryLoader.cpp
    src/FileUtil.cpp
    src/MatchHistory.cpp
    src/Stats.cpp
)

target_compile_definitions(JeopardyGame PRIVATE UNICODE _UNICODE)
target_link_libraries(JeopardyGame PRIVATE user32.lib gdi32.lib comctl32.lib comdlg32.lib)
