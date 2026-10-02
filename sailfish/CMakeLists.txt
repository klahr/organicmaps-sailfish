project(organicmaps_sailfish)

find_package(Qt5 COMPONENTS REQUIRED Gui Network Qml Quick Sensors)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SAILFISHAPP REQUIRED sailfishapp)

set(SRC
  app.cpp
  app_info.cpp
  app_info.hpp
  app_settings.cpp
  app_settings.hpp
  bookmarks_model.cpp
  bookmarks_model.hpp
  countries_model.cpp
  countries_model.hpp
  framework_access.hpp
  map_item.cpp
  map_item.hpp
  place_page.cpp
  place_page.hpp
  routing.cpp
  routing.hpp
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
    Qt5::Network
    Qt5::Qml
    Qt5::Quick
    Qt5::Sensors
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
install(DIRECTORY icons/layers icons/categories icons/placepage icons/bookmarks icons/myposition icons/help icons/menu icons/routing DESTINATION share/organicmaps/icons)
# Bitmaps on Android too, installed from its resources: router and route point buttons.
set(ANDROID_RES "${OMIM_ROOT}/android/app/src/main/res")
install(FILES
  "${ANDROID_RES}/drawable-xxhdpi/ic_car.webp" "${ANDROID_RES}/drawable-xxhdpi/ic_pedestrian.webp"
  "${ANDROID_RES}/drawable-xxhdpi/ic_transit.webp" "${ANDROID_RES}/drawable-xxhdpi/ic_bike.webp"
  "${ANDROID_RES}/drawable-xxhdpi/ic_route_from.webp" "${ANDROID_RES}/drawable-xxhdpi/ic_route_to.webp"
  "${ANDROID_RES}/drawable-xxhdpi/ic_route_via.webp"
  "${ANDROID_RES}/drawable/route_point_start.png" "${ANDROID_RES}/drawable/route_point_finish.png"
  DESTINATION share/organicmaps/icons/routing)
set(ANDROID_SDK_RES "${OMIM_ROOT}/android/sdk/src/main/res")
install(FILES
  "${ANDROID_SDK_RES}/drawable-xxxhdpi/ic_20px_route_planning_walk.webp"
  "${ANDROID_SDK_RES}/drawable-xxxhdpi/ic_20px_route_planning_metro.webp"
  "${ANDROID_SDK_RES}/drawable-xxxhdpi/ic_20px_route_planning_train.webp"
  "${ANDROID_SDK_RES}/drawable-xxxhdpi/ic_20px_route_planning_lightrail.webp"
  "${ANDROID_SDK_RES}/drawable-xxxhdpi/ic_20px_route_planning_monorail.webp"
  DESTINATION share/organicmaps/icons/routing)
# The Wikipedia logo and the direction arrow are bitmaps on Android too.
install(FILES "${OMIM_ROOT}/android/app/src/main/res/drawable-xxhdpi/ic_wiki.webp"
              "${OMIM_ROOT}/android/app/src/main/res/drawable-xxhdpi/ic_direction_pagepreview.webp"
        DESTINATION share/organicmaps/icons/placepage)
install(FILES organicmaps.desktop DESTINATION share/applications)
foreach(size 86 108 128 172)
  install(FILES icons/${size}x${size}/organicmaps.png DESTINATION share/icons/hicolor/${size}x${size}/apps)
endforeach()
