cmake_minimum_required(VERSION 3.24)
project(DosatsuBasicExamples 
    VERSION 1.0.0
    DESCRIPTION "Dosatsu Basic C++ Examples"
    LANGUAGES CXX
)

# Match main project configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Windows MSVC settings (aligned with main CMakeLists.txt)
if(WIN32 AND MSVC)
    add_compile_options(/W4 /EHsc)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
endif()

# Example files
set(EXAMPLE_FILES
    inheritance.cpp
    templates.cpp
    namespaces.cpp
    control_flow.cpp
    expressions.cpp
    preprocessor.cpp
)

# Create targets for each source file
foreach(SOURCE ${EXAMPLE_FILES})
    get_filename_component(TARGET_NAME ${SOURCE} NAME_WE)
    add_executable(${TARGET_NAME} "src/${SOURCE}")
    set_target_properties(${TARGET_NAME} PROPERTIES
        EXCLUDE_FROM_ALL TRUE
    )
    # Add example mode definition
    target_compile_definitions(${TARGET_NAME} PRIVATE EXAMPLE_MODE)
endforeach()

# Special target to build all examples
add_custom_target(build_all_basic_examples
    DEPENDS ${EXAMPLE_FILES}
)
