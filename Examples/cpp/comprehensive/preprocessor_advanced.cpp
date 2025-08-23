/**
 * @file preprocessor_advanced.cpp
 * @brief Comprehensive example demonstrating advanced preprocessor features
 * 
 * This example showcases advanced preprocessor directives and macro usage:
 * - Complex conditional compilation
 * - Function-like macros with variadic arguments
 * - Macro stringification and token pasting
 * - Pragma directives
 * - Include guards and header management
 * - Macro debugging and diagnostics
 * - Platform-specific conditional compilation
 */

// Include guards (traditional method)
#ifndef PREPROCESSOR_ADVANCED_H
#define PREPROCESSOR_ADVANCED_H

// System includes
#include <stdio.h>
#include <stdarg.h>

// Conditional includes based on platform
#ifdef _WIN32
    #include <windows.h>
    #define PLATFORM_NAME "Windows"
#elif defined(__linux__)
    #include <unistd.h>
    #define PLATFORM_NAME "Linux"
#elif defined(__APPLE__)
    #include <sys/types.h>
    #define PLATFORM_NAME "macOS"
#else
    #define PLATFORM_NAME "Unknown"
#endif

// Compiler-specific conditionals
#ifdef __GNUC__
    #define COMPILER_NAME "GCC"
    #define FORCE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
    #define COMPILER_NAME "MSVC"
    #define FORCE_INLINE __forceinline
#elif defined(__clang__)
    #define COMPILER_NAME "Clang"
    #define FORCE_INLINE __attribute__((always_inline)) inline
#else
    #define COMPILER_NAME "Unknown"
    #define FORCE_INLINE inline
#endif

// Version checking
#if __cplusplus >= 201703L
    #define CPP17_AVAILABLE 1
#else
    #define CPP17_AVAILABLE 0
#endif

#if __cplusplus >= 201402L
    #define CPP14_AVAILABLE 1
#else
    #define CPP14_AVAILABLE 0
#endif

#if __cplusplus >= 201103L
    #define CPP11_AVAILABLE 1
#else
    #define CPP11_AVAILABLE 0
#endif

// Feature detection macros
#ifdef __has_feature
    #if __has_feature(cxx_constexpr)
        #define HAS_CONSTEXPR 1
    #else
        #define HAS_CONSTEXPR 0
    #endif
#else
    #define HAS_CONSTEXPR CPP11_AVAILABLE
#endif

// Debug/Release configuration
#ifdef DEBUG
    #define DEBUG_ENABLED 1
    #define ASSERT(condition) \
        do { \
            if (!(condition)) { \
                fprintf(stderr, "Assertion failed: %s at %s:%d\n", \
                        #condition, __FILE__, __LINE__); \
                abort(); \
            } \
        } while(0)
#else
    #define DEBUG_ENABLED 0
    #define ASSERT(condition) ((void)0)
#endif

// Optimization hints
#ifdef OPTIMIZE_FOR_SIZE
    #define OPTIMIZATION_TARGET "size"
#elif defined(OPTIMIZE_FOR_SPEED)
    #define OPTIMIZATION_TARGET "speed"
#else
    #define OPTIMIZATION_TARGET "balanced"
#endif

// Pragma directives for compiler control
#pragma once  // Modern include guard

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4996)  // Disable deprecated function warnings
    #pragma pack(push, 1)           // Pack structures tightly
#endif

#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-variable"
#endif

// Function-like macros
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ABS(x) ((x) < 0 ? -(x) : (x))

// Multi-line macro with do-while(0) idiom
#define SWAP(type, a, b) \
    do { \
        type temp = (a); \
        (a) = (b); \
        (b) = temp; \
    } while(0)

// Stringification operator (#)
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// Token pasting operator (##)
#define CONCAT(a, b) a##b
#define MAKE_UNIQUE(name) CONCAT(name, __LINE__)

// Variadic macros (C99/C++11)
#define LOG_INFO(...) \
    do { \
        if (DEBUG_ENABLED) { \
            printf("[INFO] " __VA_ARGS__); \
            printf("\n"); \
        } \
    } while(0)

#define LOG_ERROR(...) \
    do { \
        fprintf(stderr, "[ERROR] " __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while(0)

// Advanced variadic macro with argument counting
#define GET_ARG_COUNT(...) GET_ARG_COUNT_IMPL(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define GET_ARG_COUNT_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N

// Macro overloading based on argument count
#define PRINT_1(a) printf("%d\n", a)
#define PRINT_2(a, b) printf("%d %d\n", a, b)
#define PRINT_3(a, b, c) printf("%d %d %d\n", a, b, c)

#define PRINT_IMPL(count, ...) CONCAT(PRINT_, count)(__VA_ARGS__)
#define PRINT(...) PRINT_IMPL(GET_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)

// Conditional macro definitions
#if CPP17_AVAILABLE
    #define IF_CONSTEXPR if constexpr
    #define NODISCARD [[nodiscard]]
#else
    #define IF_CONSTEXPR if
    #define NODISCARD
#endif

// Macro for generating boilerplate code
#define DECLARE_CLASS_BASICS(ClassName) \
    public: \
        ClassName() = default; \
        ClassName(const ClassName&) = default; \
        ClassName& operator=(const ClassName&) = default; \
        ClassName(ClassName&&) = default; \
        ClassName& operator=(ClassName&&) = default; \
        virtual ~ClassName() = default;

#define DECLARE_GETTER_SETTER(type, name) \
    private: \
        type name##_; \
    public: \
        const type& get_##name() const { return name##_; } \
        void set_##name(const type& value) { name##_ = value; } \
        type& name##_ref() { return name##_; }

// X-Macro pattern for code generation
#define COLOR_LIST \
    X(RED, 0xFF0000) \
    X(GREEN, 0x00FF00) \
    X(BLUE, 0x0000FF) \
    X(YELLOW, 0xFFFF00) \
    X(CYAN, 0x00FFFF) \
    X(MAGENTA, 0xFF00FF)

// Generate enum using X-Macro
enum Color {
#define X(name, value) COLOR_##name = value,
    COLOR_LIST
#undef X
    COLOR_COUNT
};

// Generate string array using X-Macro
const char* color_names[] = {
#define X(name, value) #name,
    COLOR_LIST
#undef X
};

// Macro for compile-time assertions (pre-C++11)
#define STATIC_ASSERT(condition, message) \
    typedef char static_assertion_##__LINE__[(condition) ? 1 : -1]

// Macro for unused parameter suppression
#define UNUSED(x) ((void)(x))

// Macro for array size calculation
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// Conditional function declarations
#if HAS_CONSTEXPR
    #define CONSTEXPR_FUNC constexpr
#else
    #define CONSTEXPR_FUNC inline
#endif

// Platform-specific function declarations
#ifdef _WIN32
    #define EXPORT __declspec(dllexport)
    #define IMPORT __declspec(dllimport)
    #define CALLING_CONVENTION __stdcall
#else
    #define EXPORT __attribute__((visibility("default")))
    #define IMPORT
    #define CALLING_CONVENTION
#endif

// Macro for generating test functions
#define DEFINE_TEST(test_name) \
    void test_##test_name() { \
        printf("Running test: %s\n", #test_name); \
        /* Test implementation goes here */ \
    }

// Complex conditional compilation
#if defined(FEATURE_A) && defined(FEATURE_B)
    #define COMBINED_FEATURE_AVAILABLE 1
    #if FEATURE_A_VERSION >= 2 && FEATURE_B_VERSION >= 3
        #define ADVANCED_COMBINED_FEATURE 1
    #else
        #define ADVANCED_COMBINED_FEATURE 0
    #endif
#else
    #define COMBINED_FEATURE_AVAILABLE 0
    #define ADVANCED_COMBINED_FEATURE 0
#endif

// Nested conditional compilation
#ifdef ENABLE_LOGGING
    #ifdef VERBOSE_LOGGING
        #define LOG_LEVEL 3
        #define LOG_VERBOSE(...) LOG_INFO(__VA_ARGS__)
    #else
        #define LOG_LEVEL 1
        #define LOG_VERBOSE(...) ((void)0)
    #endif
#else
    #define LOG_LEVEL 0
    #define LOG_VERBOSE(...) ((void)0)
    #undef LOG_INFO
    #define LOG_INFO(...) ((void)0)
#endif

// Macro for code generation with loops
#define REPEAT_10(macro, arg) \
    macro(arg, 0) macro(arg, 1) macro(arg, 2) macro(arg, 3) macro(arg, 4) \
    macro(arg, 5) macro(arg, 6) macro(arg, 7) macro(arg, 8) macro(arg, 9)

#define DECLARE_ARRAY_ELEMENT(name, index) int name##_##index;

// Example class using generated code
class PreprocessorExample {
    DECLARE_CLASS_BASICS(PreprocessorExample)
    DECLARE_GETTER_SETTER(int, value)
    DECLARE_GETTER_SETTER(double, ratio)
    
    // Generate array elements using macro
    REPEAT_10(DECLARE_ARRAY_ELEMENT, element)
    
public:
    NODISCARD CONSTEXPR_FUNC int calculate(int input) const {
        return MAX(MIN(input * 2, 100), 0);
    }
    
    void demonstrate_macros() {
        // Use stringification
        const char* platform = TOSTRING(PLATFORM_NAME);
        const char* compiler = TOSTRING(COMPILER_NAME);
        
        // Use token pasting
        int MAKE_UNIQUE(variable) = 42;
        
        // Use variadic macros
        LOG_INFO("Platform: %s, Compiler: %s", platform, compiler);
        
        // Use conditional macros
        #if DEBUG_ENABLED
            ASSERT(MAKE_UNIQUE(variable) == 42);
        #endif
        
        // Use array size macro
        int array[] = {1, 2, 3, 4, 5};
        size_t array_size = ARRAY_SIZE(array);
        
        // Use swap macro
        int a = 10, b = 20;
        SWAP(int, a, b);
        
        // Use print macro with different argument counts
        PRINT(a);
        PRINT(a, b);
        PRINT(a, b, array_size);
        
        // Suppress unused variable warnings
        UNUSED(platform);
        UNUSED(compiler);
        UNUSED(array_size);
    }
    
    // Platform-specific function
    EXPORT int CALLING_CONVENTION platform_function(int param) {
        #ifdef _WIN32
            // Windows-specific implementation
            return param * 2;
        #elif defined(__linux__)
            // Linux-specific implementation
            return param + 10;
        #else
            // Generic implementation
            return param;
        #endif
    }
};

// Generate test functions using macro
DEFINE_TEST(basic_functionality)
DEFINE_TEST(error_handling)
DEFINE_TEST(performance)

// Conditional static assertions
#if CPP11_AVAILABLE
    static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");
    static_assert(COLOR_COUNT == 6, "Should have 6 colors");
#else
    STATIC_ASSERT(sizeof(int) >= 4, int_must_be_at_least_4_bytes);
#endif

// Function to demonstrate all preprocessor features
void demonstrate_preprocessor_features() {
    PreprocessorExample example;
    
    // Test basic functionality
    example.set_value(42);
    example.set_ratio(3.14);
    
    int result = example.calculate(50);
    UNUSED(result);
    
    // Demonstrate macro usage
    example.demonstrate_macros();
    
    // Test platform-specific function
    int platform_result = example.platform_function(10);
    UNUSED(platform_result);
    
    // Use color enum and names
    Color favorite_color = COLOR_RED;
    const char* color_name = color_names[favorite_color];
    
    LOG_INFO("Favorite color: %s (0x%06X)", color_name, favorite_color);
    
    // Conditional feature usage
    #if COMBINED_FEATURE_AVAILABLE
        LOG_INFO("Combined feature is available");
        #if ADVANCED_COMBINED_FEATURE
            LOG_INFO("Advanced combined feature is available");
        #endif
    #else
        LOG_INFO("Combined feature is not available");
    #endif
    
    // Run generated tests
    test_basic_functionality();
    test_error_handling();
    test_performance();
}

// Cleanup pragma directives
#ifdef _MSC_VER
    #pragma pack(pop)
    #pragma warning(pop)
#endif

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

// Undefine temporary macros to avoid pollution
#undef MAKE_UNIQUE
#undef REPEAT_10
#undef DECLARE_ARRAY_ELEMENT

#endif // PREPROCESSOR_ADVANCED_H

/**
 * @brief End of preprocessor advanced example
 * 
 * This file demonstrates:
 * - Complex conditional compilation with nested #if/#ifdef
 * - Platform and compiler detection
 * - Feature detection and version checking
 * - Advanced macro techniques (stringification, token pasting, variadic)
 * - X-Macro pattern for code generation
 * - Pragma directives for compiler control
 * - Macro overloading and argument counting
 * - Include guards and header management
 */
