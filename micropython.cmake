# micropython.cmake
if(NOT DEFINED MICROPY_PY_UGPS)
    set(MICROPY_PY_UGPS 1)
endif()

if(MICROPY_PY_UGPS)
    message(STATUS "module ugps: ENABLED")
    add_library(ugps INTERFACE)
    target_sources(ugps INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/module_ugps.c
        ${CMAKE_CURRENT_LIST_DIR}/nmea.c
        ${CMAKE_CURRENT_LIST_DIR}/ublox.c
        ${CMAKE_CURRENT_LIST_DIR}/tools.c
    )
    target_include_directories(ugps INTERFACE ${CMAKE_CURRENT_LIST_DIR})
    target_compile_definitions(ugps INTERFACE MICROPY_PY_UGPS=1)
    target_link_libraries(usermod INTERFACE ugps)
else()
    message(STATUS "module ugps: DISABLED")
endif()