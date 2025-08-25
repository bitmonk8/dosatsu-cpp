cmake_minimum_required(VERSION 3.24)
project(DosatsuComprehensiveExamples 
    VERSION 1.0.0
    DESCRIPTION "Dosatsu Comprehensive C++ Examples"
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
    advanced_features.cpp
    complete_example.cpp
    modern_cpp_features.cpp
    control_flow_complex.cpp
    standard_example.cpp
    clean_example.cpp
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
if(TARGET advanced_features)
    target_compile_definitions(advanced_features PRIVATE 
        DEBUG 
        ENABLE_OPTIMIZATION=1 
        ADVANCED_MODE 
        FEATURE_A 
        FEATURE_B 
        FEATURE_A_VERSION=2 
        FEATURE_B_VERSION=3 
        ENABLE_LOGGING 
        VERBOSE_LOGGING
    )
endif()

if(TARGET preprocessor_advanced)
    target_compile_definitions(preprocessor_advanced PRIVATE 
        DEBUG 
        ENABLE_OPTIMIZATION=1 
        ADVANCED_MODE
    )
endif()

# Special target to build all examples
add_custom_target(build_all_comprehensive_examples
    DEPENDS ${EXAMPLE_FILES}
)
