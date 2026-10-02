project(organicmaps_sailfish)

find_package(Qt5 COMPONENTS REQUIRED Gui Qml Quick)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SAILFISHAPP REQUIRED sailfishapp)

set(SRC
  app.cpp
  countries_model.cpp
  countries_model.hpp
  framework_access.hpp
  map_item.cpp
  map_item.hpp
  place_page.cpp
  place_page.hpp
  search_model.cpp
  search_model.hpp
  ${OMIM_ROOT}/qt/qt_common/qtoglcontext.cpp
  ${OMIM_ROOT}/qt/qt_common/qtoglcontext.hpp
  ${OMIM_ROOT}/qt/qt_common/qtoglcontextfactory.cpp
  ${OMIM_ROOT}/qt/qt_common/qtoglcontextfactory.hpp
)

add_library(${PROJECT_NAME}_lib SHARED ${SRC})
set_target_properties(${PROJECT_NAME}_lib PROPERTIES OUTPUT_NAME organicmaps AUTOMOC ON)
target_compile_options(${PROJECT_NAME}_lib PRIVATE ${OMIM_WARNING_FLAGS})
# Keep the bundled 3party libraries (FreeType, ICU, ...) from interposing the system copies Qt uses.
target_link_options(${PROJECT_NAME}_lib PRIVATE -Wl,--exclude-libs,ALL)
target_include_directories(${PROJECT_NAME}_lib PRIVATE ${SAILFISHAPP_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}_lib
  PRIVATE
    Boost::headers
    map
    location_service
    Qt5::Gui
    Qt5::Qml
    Qt5::Quick
    ${SAILFISHAPP_LIBRARIES}
)

add_executable(${PROJECT_NAME} main.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES
  OUTPUT_NAME organicmaps
  INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/share/organicmaps/lib"
)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_lib)

file(STRINGS "${OMIM_ROOT}/qt/runtime_resources.txt" RUNTIME_RESOURCES)
list(APPEND RUNTIME_RESOURCES icudt78l.dat)
foreach(resource ${RUNTIME_RESOURCES})
  if (IS_DIRECTORY "${OMIM_ROOT}/data/${resource}")
    install(DIRECTORY "${OMIM_ROOT}/data/${resource}" DESTINATION share/organicmaps/data)
  else()
    install(FILES "${OMIM_ROOT}/data/${resource}" DESTINATION share/organicmaps/data)
  endif()
endforeach()

# Read at runtime by platform/localization_sailfish.cpp for type names and UI strings.
install(FILES "${OMIM_ROOT}/data/strings/types_strings.txt" "${OMIM_ROOT}/data/strings/strings.txt"
        DESTINATION share/organicmaps/data/strings)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(TARGETS ${PROJECT_NAME}_lib LIBRARY DESTINATION share/organicmaps/lib)
install(DIRECTORY qml DESTINATION share/organicmaps)
install(DIRECTORY icons/layers icons/categories icons/placepage DESTINATION share/organicmaps/icons)
# The Wikipedia logo is a bitmap on Android too.
install(FILES "${OMIM_ROOT}/android/app/src/main/res/drawable-xxhdpi/ic_wiki.webp"
        DESTINATION share/organicmaps/icons/placepage)
install(FILES organicmaps.desktop DESTINATION share/applications)
foreach(size 86 108 128 172)
  install(FILES icons/${size}x${size}/organicmaps.png DESTINATION share/icons/hicolor/${size}x${size}/apps)
endforeach()
