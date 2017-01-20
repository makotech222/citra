set(SRCS
            input_core.cpp
            key_map.cpp
            devices/device.cpp
            devices/keyboard.cpp
            devices/sdl_joystick.cpp
            )

set(HEADERS
            input_core.h
            key_map.h
            devices/device.h
            devices/keyboard.h
            devices/sdl_joystick.h
            )


if(SDL2_FOUND)
    include_directories(${SDL2_INCLUDE_DIR})
endif()

create_directory_groups(${SRCS} ${HEADERS})

add_library(input_core STATIC ${SRCS} ${HEADERS})

if(SDL2_FOUND)
    target_link_libraries(input_core ${SDL2_LIBRARY})
    set_property(TARGET input_core APPEND PROPERTY COMPILE_DEFINITIONS HAVE_SDL2)
endif()
