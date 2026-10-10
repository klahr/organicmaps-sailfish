project(departures_tests)

set(SRC
  departures_tests.cpp
  trafiklab_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE departures)
