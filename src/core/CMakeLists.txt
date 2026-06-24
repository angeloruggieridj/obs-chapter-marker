add_library(chapter-marker-core STATIC
  ChapterName.cpp
  RecordingClock.cpp
  TimeFormat.cpp
  Timecode.cpp
  Export.cpp
  Version.cpp
)
target_include_directories(chapter-marker-core
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC ${CMAKE_SOURCE_DIR}/third_party
)
target_compile_features(chapter-marker-core PUBLIC cxx_std_17)

# The core is linked into the shared plugin module; on Linux a static lib must
# be position-independent to be linked into a .so.
set_target_properties(chapter-marker-core PROPERTIES POSITION_INDEPENDENT_CODE ON)
