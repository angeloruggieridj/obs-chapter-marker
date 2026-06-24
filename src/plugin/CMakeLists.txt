find_package(libobs QUIET)
if(NOT libobs_FOUND)
  find_package(LibObs REQUIRED)
endif()
find_package(obs-frontend-api REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

set(CMAKE_AUTOMOC ON)

add_library(obs-chapter-marker MODULE
  plugin-main.cpp
  ChapterMarkerDock.cpp
)

target_link_libraries(obs-chapter-marker PRIVATE
  chapter-marker-core
  OBS::libobs
  OBS::obs-frontend-api
  Qt6::Core
  Qt6::Widgets
)

target_compile_definitions(obs-chapter-marker PRIVATE OCM_VERSION="${PROJECT_VERSION}")

target_include_directories(obs-chapter-marker PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src/core
)

if(WIN32)
  set(OBS_PLUGIN_DESTINATION "obs-plugins/64bit")
  set(OBS_DATA_DESTINATION   "data/obs-plugins/obs-chapter-marker")
elseif(APPLE)
  set(OBS_PLUGIN_DESTINATION "obs-plugins")
  set(OBS_DATA_DESTINATION   "obs-plugins/obs-chapter-marker")
else()
  include(GNUInstallDirs)
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Install path prefix." FORCE)
  endif()
  set(OBS_PLUGIN_DESTINATION "${CMAKE_INSTALL_LIBDIR}/obs-plugins")
  set(OBS_DATA_DESTINATION   "${CMAKE_INSTALL_DATAROOTDIR}/obs/obs-plugins/obs-chapter-marker")
endif()

install(TARGETS obs-chapter-marker LIBRARY DESTINATION "${OBS_PLUGIN_DESTINATION}")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/data/" DESTINATION "${OBS_DATA_DESTINATION}")
