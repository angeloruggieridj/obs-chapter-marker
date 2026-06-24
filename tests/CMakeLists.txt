add_executable(chapter-marker-tests
  test_main.cpp
  test_smoke.cpp
  test_chaptername.cpp
  test_recordingclock.cpp
  test_timeformat.cpp
  test_timecode.cpp
  test_export.cpp
  test_version.cpp
)
target_link_libraries(chapter-marker-tests PRIVATE chapter-marker-core)
target_include_directories(chapter-marker-tests PRIVATE ${CMAKE_SOURCE_DIR}/third_party)
add_test(NAME core COMMAND chapter-marker-tests)
