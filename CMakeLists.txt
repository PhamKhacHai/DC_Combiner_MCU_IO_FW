cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME DC_Combiner_MCU_IO_FW)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

set(APP_TARGET ${CMAKE_PROJECT_NAME})
set(BOOTLOADER_TARGET ${CMAKE_PROJECT_NAME}_Bootloader)
set(APP_LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/Linker/STM32F103C8_APP.ld)
set(BOOTLOADER_LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/Linker/STM32F103C8_BOOTLOADER.ld)

function(configure_firmware_image target linker_script)
    target_link_options(${target} PRIVATE
        -T${linker_script}
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${target}.map
    )

    set_target_properties(${target} PROPERTIES
        LINK_DEPENDS ${linker_script}
        ADDITIONAL_CLEAN_FILES "${target}.map;${target}.hex;${target}.bin"
    )

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${target}> ${CMAKE_CURRENT_BINARY_DIR}/${target}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target}> ${CMAKE_CURRENT_BINARY_DIR}/${target}.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${target}>
        COMMENT "Generating ${target}.hex and ${target}.bin"
    )
endfunction()

# Create an executable object type
add_executable(${APP_TARGET})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

add_executable(${BOOTLOADER_TARGET})
target_sources(${BOOTLOADER_TARGET} PRIVATE
    Bootloader/Src/bootloader_main.c
    Core/Src/bootloader_request.c
    Core/Src/system_stm32f1xx.c
    startup_stm32f103xb.s
)
target_link_libraries(${BOOTLOADER_TARGET} PRIVATE
    stm32cubemx
)

# Link directories setup
target_link_directories(${APP_TARGET} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${APP_TARGET} PRIVATE
    # Add user sources here
    Core/Src/app.c
    Core/Src/bsp_gpio.c
    Core/Src/feedback.c
    Core/Src/contactor.c
    Core/Src/can_comm.c
    Core/Src/diagnostic.c
    Core/Src/bootloader_request.c
)

# Add include paths
target_include_directories(${APP_TARGET} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${APP_TARGET} PRIVATE
    # Add user defined symbols
    APP_VECTOR_TABLE_OFFSET=0x00004000U
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${APP_TARGET}
    stm32cubemx

    # Add user defined libraries
)

configure_firmware_image(${APP_TARGET} ${APP_LINKER_SCRIPT})
configure_firmware_image(${BOOTLOADER_TARGET} ${BOOTLOADER_LINKER_SCRIPT})
