cmake_minimum_required(VERSION 3.20)

# Namespace examples
# Focus on C++ namespace features, scope resolution, and organization

set(EXAMPLE_SOURCES
    namespaces.cpp
)

# Create executable targets for each source file
foreach(SOURCE_FILE ${EXAMPLE_SOURCES})
    get_filename_component(TARGET_NAME ${SOURCE_FILE} NAME_WE)
    add_executable(${TARGET_NAME} ${SOURCE_FILE})
    
    # Set C++ standard
    set_target_properties(${TARGET_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    
    # Compiler-specific settings - minimal warnings for examples
    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /W0 /EHsc)
        target_compile_definitions(${TARGET_NAME} PRIVATE EXAMPLE_MODE WIN32 _WINDOWS)
    else()
        target_compile_options(${TARGET_NAME} PRIVATE -w)
        target_compile_definitions(${TARGET_NAME} PRIVATE EXAMPLE_MODE)
    endif()
endforeach()
