cmake_minimum_required(VERSION 3.27)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Setup compiler settings
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME comm-dongle)

set(FIRMWARE_VERSION_STR "0.1.0")
set(FIRMWARE_VERSION "")
find_program(GIT_EXECUTABLE git)
if(GIT_EXECUTABLE)
    # only hash
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE HASH_RESULT
        ERROR_QUIET
    )
    
    # check dirty
    execute_process(
        COMMAND ${GIT_EXECUTABLE} status --porcelain
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_STATUS
        ERROR_QUIET
    )
    
    if(HASH_RESULT EQUAL 0 AND GIT_HASH)
        set(FIRMWARE_VERSION "-${GIT_HASH}")
        if(GIT_STATUS)
            set(FIRMWARE_VERSION "-${FIRMWARE_VERSION}-dirty")
        endif()
    endif()
endif()
set(FIRMWARE_NAME "${CMAKE_PROJECT_NAME}-v${FIRMWARE_VERSION_STR}${FIRMWARE_VERSION}")
message(STATUS "Firmware name: ${FIRMWARE_NAME}")

# Driver function selection
set(MCU_TYPE "HC32F460xE" CACHE STRING "Set MCU Type")
set(BOOTLOADER OFF CACHE BOOL "Set to ON if bootloader is enabled")
set(APP OFF CACHE BOOL "Set to ON if app is enabled")
set(RT-THREAD ON CACHE BOOL "Set to ON if RT-Thread is enabled")

# Enable printf and scanf float
option(ENABLE_PRINTF_FLOAT "printf report %f" OFF)
option(ENABLE_SCANF_FLOAT  "scanf report %f"  OFF)
# Include toolchain file
include("drivers/toolchain/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME} VERSION ${FIRMWARE_VERSION_STR})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${CMAKE_PROJECT_NAME}
    ADDITIONAL_CLEAN_FILES "${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_NAME}.map"
)

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_NAME}.map"
)
# "Generate hex and bin files and associate them with the clean target."
add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME}
    POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${FIRMWARE_NAME}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${FIRMWARE_NAME}.bin
    BYPRODUCTS 
        ${FIRMWARE_NAME}.hex
        ${FIRMWARE_NAME}.bin
)

# Include the drivers,Drivers can be loaded using the submodule management method, which is recommended.
add_subdirectory("drivers")

# rt-thread define
math(EXPR RT_HEAP_SIZE "100 * 1024") 
add_compile_definitions(
    RT_HEAP_SIZE=${RT_HEAP_SIZE}
)
add_subdirectory("rt-thread")

set(DEFINES
    # Add user defines
)

file(GLOB SOURCES
    # Add user source files
    "${CMAKE_CURRENT_LIST_DIR}/hardware/src/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/software/src/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/*.c"

)

set(HEADERS
    # Add user headers
    "${CMAKE_CURRENT_LIST_DIR}/hardware/inc"
    "${CMAKE_CURRENT_LIST_DIR}/software/inc"
    "${CMAKE_CURRENT_LIST_DIR}/inc"
)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    ${DEFINES}
)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${SOURCES}
)

# USB support switch, enabled by default.
# Set it to ON only when the application uses the USB driver and provides
# its own usb_app_conf.h in one of the directories listed in HEADERS.

option(USE_USB "Enable USB support in the driver layer" ON)
if(USE_USB)
    # Expose application include directories to the driver layer so that
    # driver sources (e.g. hc32_ll_usb.c) can find application-provided
    # configuration headers such as usb_app_conf.h.
    target_include_directories(hc32f460_driver PUBLIC
        ${HEADERS}
    )
else()
    # Application include directories are only used by the application target.
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${HEADERS}
    )
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined libraries
)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)