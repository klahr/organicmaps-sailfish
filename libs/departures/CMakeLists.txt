project(departures)

set(SRC
  departures.cpp
  departures.hpp
  trafiklab.cpp
  trafiklab.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    geometry

  PRIVATE
    base
    coding
    platform
    glaze::glaze
)

omim_add_test_subdirectory(departures_tests)
