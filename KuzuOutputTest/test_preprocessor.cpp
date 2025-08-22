// Test file for preprocessor analysis including macros, includes, and conditional compilation
#include <iostream>
#include <string>
#include <vector>

// Standard system includes
#include <memory>
#include <algorithm>
#include <functional>

// User includes (would be actual files in a real project)
// #include "custom_header.h"
// #include "another_header.hpp"

// Conditional includes
#ifdef _WIN32
    // #include <windows.h>
    #define PLATFORM_NAME "Windows"
#elif defined(__linux__)
    // #include <unistd.h>
    #define PLATFORM_NAME "Linux"
#elif defined(__APPLE__)
    // #include <CoreFoundation/CoreFoundation.h>
    #define PLATFORM_NAME "macOS"
#else
    #define PLATFORM_NAME "Unknown"
#endif

// Simple object-like macros
#define PI 3.14159265359
#define MAX_SIZE 1000
#define DEBUG_MODE 1
#define VERSION_MAJOR 2
#define VERSION_MINOR 1
#define VERSION_PATCH 0

// String macros
#define AUTHOR "Test Author"
#define COPYRIGHT "Copyright 2024"
#define BUILD_INFO "Debug Build"

// Expression macros
#define SQUARE(x) ((x) * (x))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ABS(x) ((x) < 0 ? -(x) : (x))

// Multi-line macros
#define SWAP(type, a, b) \
    do { \
        type temp = (a); \
        (a) = (b); \
        (b) = temp; \
    } while(0)

// Macro with multiple statements
#define DEBUG_PRINT(msg) \
    do { \
        if (DEBUG_MODE) { \
            std::cout << "[DEBUG] " << msg << std::endl; \
        } \
    } while(0)

// Variadic macros (C99/C++11)
#define LOG(format, ...) \
    printf("[LOG] " format "\n", ##__VA_ARGS__)

#define PRINT_ARGS(first, ...) \
    std::cout << "First: " << first << ", Args: " << #__VA_ARGS__ << std::endl

// Stringification operator (#)
#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

// Token pasting operator (##)
#define CONCAT(a, b) a##b
#define MAKE_VARIABLE(prefix, suffix) prefix##_##suffix

// Conditional compilation
#if DEBUG_MODE
    #define ASSERT(condition) \
        do { \
            if (!(condition)) { \
                std::cerr << "Assertion failed: " << #condition \
                         << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
                std::abort(); \
            } \
        } while(0)
#else
    #define ASSERT(condition) ((void)0)
#endif

// Version check macros
#if VERSION_MAJOR >= 2
    #define NEW_FEATURE_AVAILABLE 1
    #define USE_NEW_API
#else
    #define NEW_FEATURE_AVAILABLE 0
    #undef USE_NEW_API
#endif

// Compiler-specific macros
#ifdef __GNUC__
    #define COMPILER_NAME "GCC"
    #define FORCE_INLINE __attribute__((always_inline)) inline
    #define DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
    #define COMPILER_NAME "MSVC"
    #define FORCE_INLINE __forceinline
    #define DEPRECATED __declspec(deprecated)
#elif defined(__clang__)
    #define COMPILER_NAME "Clang"
    #define FORCE_INLINE __attribute__((always_inline)) inline
    #define DEPRECATED __attribute__((deprecated))
#else
    #define COMPILER_NAME "Unknown"
    #define FORCE_INLINE inline
    #define DEPRECATED
#endif

// Feature test macros
#ifdef __cplusplus
    #if __cplusplus >= 201703L
        #define CPP17_AVAILABLE 1
        #define IF_CONSTEXPR if constexpr
    #else
        #define CPP17_AVAILABLE 0
        #define IF_CONSTEXPR if
    #endif
    
    #if __cplusplus >= 202002L
        #define CPP20_AVAILABLE 1
    #else
        #define CPP20_AVAILABLE 0
    #endif
#endif

// Complex macro patterns
#define DECLARE_GETTER_SETTER(type, name) \
    private: \
        type m_##name; \
    public: \
        const type& get##name() const { return m_##name; } \
        void set##name(const type& value) { m_##name = value; }

// Macro with default parameters (using overloading pattern)
#define CREATE_VECTOR_1(type) std::vector<type>()
#define CREATE_VECTOR_2(type, size) std::vector<type>(size)
#define CREATE_VECTOR_3(type, size, value) std::vector<type>(size, value)

// Macro overloading simulation
#define GET_4TH_ARG(arg1, arg2, arg3, arg4, ...) arg4
#define CREATE_VECTOR_CHOOSER(...) \
    GET_4TH_ARG(__VA_ARGS__, CREATE_VECTOR_3, CREATE_VECTOR_2, CREATE_VECTOR_1)
#define CREATE_VECTOR(...) CREATE_VECTOR_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

// Function-like macros with side effects
#define INCREMENT_AND_PRINT(var) \
    do { \
        ++(var); \
        std::cout << #var " = " << (var) << std::endl; \
    } while(0)

// Nested macro definitions
#define OUTER_MACRO(x) INNER_MACRO(x)
#define INNER_MACRO(x) (x * 2 + 1)

// Recursive-like macro (actually just expansion)
#define REPEAT_2(x) x x
#define REPEAT_4(x) REPEAT_2(REPEAT_2(x))
#define REPEAT_8(x) REPEAT_2(REPEAT_4(x))

// Pragma directives
#pragma once  // Header guard (if this were a header)

// Compiler-specific pragmas
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4996)  // Disable deprecation warnings
#endif

#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-variable"
#endif

// Class using macro-generated members
class MacroTestClass {
    DECLARE_GETTER_SETTER(int, Value)
    DECLARE_GETTER_SETTER(std::string, Name)
    DECLARE_GETTER_SETTER(double, Score)
    
public:
    MacroTestClass() : m_Value(0), m_Name(""), m_Score(0.0) {}
    
    void demonstrateMacros() {
        // Simple macro usage
        double circleArea = PI * SQUARE(5.0);
        int maxValue = MAX(10, 20);
        int minValue = MIN(10, 20);
        int absValue = ABS(-42);
        
        // Debug printing
        DEBUG_PRINT("Testing macro functionality");
        
        // Stringification
        std::string piString = TO_STRING(PI);
        std::string maxSizeString = STRINGIFY(MAX_SIZE);
        
        // Token pasting
        int MAKE_VARIABLE(test, value) = 100;
        
        // Variadic macro
        LOG("Value: %d, Name: %s", 42, "Test");
        PRINT_ARGS("first", "second", "third");
        
        // Conditional compilation result
        #if NEW_FEATURE_AVAILABLE
            std::cout << "New features are available!" << std::endl;
        #else
            std::cout << "Using legacy features." << std::endl;
        #endif
        
        // Compiler information
        std::cout << "Compiled with: " << COMPILER_NAME << std::endl;
        std::cout << "Platform: " << PLATFORM_NAME << std::endl;
        std::cout << "Build: " << BUILD_INFO << std::endl;
        
        // Version information
        std::cout << "Version: " << VERSION_MAJOR << "." 
                  << VERSION_MINOR << "." << VERSION_PATCH << std::endl;
        
        // C++ standard information
        #if CPP17_AVAILABLE
            std::cout << "C++17 features available" << std::endl;
        #endif
        
        #if CPP20_AVAILABLE
            std::cout << "C++20 features available" << std::endl;
        #endif
        
        // Use macro-generated vectors
        auto intVec1 = CREATE_VECTOR(int);
        auto intVec2 = CREATE_VECTOR(int, 5);
        auto intVec3 = CREATE_VECTOR(int, 3, 42);
        
        // Demonstrate side-effect macro
        int counter = 0;
        INCREMENT_AND_PRINT(counter);
        INCREMENT_AND_PRINT(counter);
        
        // Nested macro
        int nested = OUTER_MACRO(5);
        
        // Use all computed values
        std::cout << "Results: " << circleArea << " " << maxValue << " " << minValue
                  << " " << absValue << " " << piString << " " << maxSizeString
                  << " " << test_value << " " << nested << std::endl;
        std::cout << "Vector sizes: " << intVec1.size() << " " << intVec2.size() 
                  << " " << intVec3.size() << std::endl;
    }
};

// Predefined macro usage
void testPredefinedMacros() {
    std::cout << "File: " << __FILE__ << std::endl;
    std::cout << "Line: " << __LINE__ << std::endl;
    std::cout << "Function: " << __FUNCTION__ << std::endl;
    std::cout << "Date: " << __DATE__ << std::endl;
    std::cout << "Time: " << __TIME__ << std::endl;
    
    #ifdef __cplusplus
        std::cout << "C++ Standard: " << __cplusplus << std::endl;
    #endif
    
    #ifdef __STDC_VERSION__
        std::cout << "C Standard: " << __STDC_VERSION__ << std::endl;
    #endif
}

// Function using various macro features
void testMacroFeatures() {
    // Test assertion macro
    ASSERT(1 + 1 == 2);
    
    // Test swap macro
    int a = 10, b = 20;
    std::cout << "Before swap: a=" << a << ", b=" << b << std::endl;
    SWAP(int, a, b);
    std::cout << "After swap: a=" << a << ", b=" << b << std::endl;
    
    // Test class with macro-generated methods
    MacroTestClass testObj;
    testObj.setValue(42);
    testObj.setName("Test Object");
    testObj.setScore(95.5);
    
    std::cout << "Object values: " << testObj.getValue() << ", "
              << testObj.getName() << ", " << testObj.getScore() << std::endl;
    
    testObj.demonstrateMacros();
}

// Conditional compilation blocks
#if defined(FEATURE_A) && defined(FEATURE_B)
    void featureAAndB() {
        std::cout << "Both Feature A and B are enabled" << std::endl;
    }
#elif defined(FEATURE_A)
    void featureAOnly() {
        std::cout << "Only Feature A is enabled" << std::endl;
    }
#elif defined(FEATURE_B)
    void featureBOnly() {
        std::cout << "Only Feature B is enabled" << std::endl;
    }
#else
    void noFeatures() {
        std::cout << "No special features enabled" << std::endl;
    }
#endif

// Macro cleanup
#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

// Undefine some macros to show #undef
#undef MAX_SIZE
#undef DEBUG_MODE

// Redefine with different value
#define MAX_SIZE 2000
#define DEBUG_MODE 0

// Test function
void testPreprocessor() {
    std::cout << "Testing preprocessor features..." << std::endl;
    
    testPredefinedMacros();
    testMacroFeatures();
    
    // Test conditional compilation
    #if defined(FEATURE_A) && defined(FEATURE_B)
        featureAAndB();
    #elif defined(FEATURE_A)
        featureAOnly();
    #elif defined(FEATURE_B)
        featureBOnly();
    #else
        noFeatures();
    #endif
    
    std::cout << "Redefined MAX_SIZE: " << MAX_SIZE << std::endl;
    std::cout << "Redefined DEBUG_MODE: " << DEBUG_MODE << std::endl;
}
