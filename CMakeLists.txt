cmake_minimum_required(VERSION 3.21)

project(MicVolumeFix VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT WIN32)
  message(FATAL_ERROR "Mic Volume Fix requires Windows (Core Audio / WASAPI).")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

if(NOT Qt6_FOUND)
  message(FATAL_ERROR
    "Qt 6 Widgets not found.\n"
    "Install Qt 6 (MSVC 2022 64-bit) via https://www.qt.io/download-qt-installer\n"
    "Then set CMAKE_PREFIX_PATH to the kit folder, e.g. C:/Qt/6.8.1/msvc2022_64")
endif()

include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

set(MVL_SOURCES
  src/main.cpp
  src/audio/AudioEngine.cpp
  src/config/LockConfig.cpp
  src/config/SessionStore.cpp
  src/guardian/GuardianWorker.cpp
  src/win/StartupRegistry.cpp
  src/gui/EnumeratorWorker.cpp
  src/gui/MainWindow.cpp
)

add_executable(MicVolumeFix WIN32 ${MVL_SOURCES})

target_include_directories(MicVolumeFix PRIVATE src)

target_compile_definitions(MicVolumeFix PRIVATE
  UNICODE
  _UNICODE
  WIN32_LEAN_AND_MEAN
  NOMINMAX
  QT_NO_FOREACH
)

target_link_libraries(MicVolumeFix PRIVATE
  Qt6::Widgets
  nlohmann_json::nlohmann_json
  ole32
  uuid
  propsys
  shell32
)

if(MSVC)
  target_compile_options(MicVolumeFix PRIVATE /W4 /permissive-)
  target_compile_options(MicVolumeFix PRIVATE $<$<CONFIG:Release>:/O2>)
  target_link_options(MicVolumeFix PRIVATE $<$<CONFIG:Release>:/INCREMENTAL:NO>)
endif()

install(TARGETS MicVolumeFix RUNTIME DESTINATION bin)
