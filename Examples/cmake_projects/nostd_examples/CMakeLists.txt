cmake_minimum_required(VERSION 3.24)
project(DosatsuNostdExamples 
    VERSION 1.0.0
    DESCRIPTION "Dosatsu Nostd C++ Examples"
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
    no_std_example.cpp
    advanced_features_no_std.cpp
    simple_no_includes.cpp
    inheritance_no_std.cpp
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
# Add advanced definitions for specific files
if(TARGET advanced_features_no_std)
    target_compile_definitions(advanced_features_no_std PRIVATE 
        DEBUG 
        ENABLE_OPTIMIZATION=1 
        ADVANCED_MODE
    )
endif()

# Special target to build all examples
add_custom_target(build_all_nostd_examples
    DEPENDS ${EXAMPLE_FILES}
)
