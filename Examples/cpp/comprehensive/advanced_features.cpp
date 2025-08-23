/**
 * @file advanced_features.cpp
 * @brief Comprehensive example demonstrating advanced C++ features for complete schema coverage
 * 
 * This example showcases advanced C++ features that are often missing from basic examples:
 * - Static assertions and compile-time evaluation
 * - Advanced template metaprogramming and SFINAE
 * - Exception handling and RAII
 * - Lambda expressions and closures
 * - Attributes and annotations
 * - Complex preprocessor usage
 * - Modern C++ features (C++11-17)
 * - Constant expressions and constexpr
 * - Advanced type traits and concepts
 */

#include <type_traits>
#include <memory>
#include <functional>
#include <exception>
#include <stdexcept>

// Preprocessor directives for conditional compilation
#ifdef ADVANCED_MODE
    #define ENABLE_OPTIMIZATION 1
    #pragma once
    #pragma pack(push, 1)
#else
    #define ENABLE_OPTIMIZATION 0
#endif

// Conditional compilation directives
#if ENABLE_OPTIMIZATION
    #define FORCE_INLINE __forceinline
#else
    #define FORCE_INLINE inline
#endif

// Function-like macros with parameters
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define DECLARE_GETTER_SETTER(type, name) \
    type get##name() const { return name##_; } \
    void set##name(const type& value) { name##_ = value; }

// Complex macro with variadic arguments
#define LOG_DEBUG(...) \
    do { \
        if (DEBUG_ENABLED) { \
            printf(__VA_ARGS__); \
        } \
    } while(0)

// Static assertions for compile-time validation
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");
static_assert(std::is_pod<int>::value, "int should be POD type");

// Template metaprogramming - compile-time factorial
template<int N>
struct Factorial {
    static constexpr int value = N * Factorial<N - 1>::value;
    static_assert(N >= 0, "Factorial requires non-negative input");
};

template<>
struct Factorial<0> {
    static constexpr int value = 1;
};

// Advanced SFINAE (Substitution Failure Is Not An Error)
template<typename T>
struct has_begin {
private:
    template<typename U>
    static auto test(int) -> decltype(std::declval<U>().begin(), std::true_type{});
    
    template<typename>
    static std::false_type test(...);
    
public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

// SFINAE-enabled function templates
template<typename T>
typename std::enable_if<std::is_integral<T>::value, bool>::type
is_even(T value) {
    return value % 2 == 0;
}

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, bool>::type
is_even(T value) {
    return false; // Floating point numbers are never even in this context
}

// Template specialization with SFINAE
template<typename T, typename = void>
struct is_container : std::false_type {};

template<typename T>
struct is_container<T, typename std::enable_if<has_begin<T>::value>::type> : std::true_type {};

// Constexpr functions for compile-time evaluation
constexpr int fibonacci(int n) {
    return (n <= 1) ? n : fibonacci(n - 1) + fibonacci(n - 2);
}

constexpr bool is_prime(int n) {
    if (n < 2) return false;
    for (int i = 2; i * i <= n; ++i) {
        if (n % i == 0) return false;
    }
    return true;
}

// Constant expressions with static assertions
constexpr int fib_10 = fibonacci(10);
static_assert(fib_10 == 55, "Fibonacci(10) should be 55");

constexpr bool prime_17 = is_prime(17);
static_assert(prime_17, "17 should be prime");

// Attributes (C++11 and later)
[[noreturn]] void terminate_program() {
    std::terminate();
}

[[deprecated("Use new_function instead")]]
void old_function() {
    // Legacy implementation
}

[[maybe_unused]] static int unused_variable = 42;

// Class with attributes
class [[nodiscard]] Result {
private:
    bool success_;
    std::string message_;
    
public:
    Result(bool success, const std::string& message) 
        : success_(success), message_(message) {}
    
    [[nodiscard]] bool is_success() const { return success_; }
    [[nodiscard]] const std::string& message() const { return message_; }
};

// Exception handling hierarchy
class BaseException : public std::exception {
protected:
    std::string message_;
    
public:
    explicit BaseException(const std::string& message) : message_(message) {}
    
    const char* what() const noexcept override {
        return message_.c_str();
    }
    
    virtual ~BaseException() = default;
};

class ValidationException : public BaseException {
public:
    explicit ValidationException(const std::string& message) 
        : BaseException("Validation Error: " + message) {}
};

class ResourceException : public BaseException {
public:
    explicit ResourceException(const std::string& message) 
        : BaseException("Resource Error: " + message) {}
};

// RAII resource management
class ResourceManager {
private:
    std::unique_ptr<int[]> buffer_;
    size_t size_;
    
public:
    explicit ResourceManager(size_t size) 
        : buffer_(std::make_unique<int[]>(size)), size_(size) {
        if (size == 0) {
            throw ValidationException("Size cannot be zero");
        }
    }
    
    // Move constructor
    ResourceManager(ResourceManager&& other) noexcept 
        : buffer_(std::move(other.buffer_)), size_(other.size_) {
        other.size_ = 0;
    }
    
    // Move assignment
    ResourceManager& operator=(ResourceManager&& other) noexcept {
        if (this != &other) {
            buffer_ = std::move(other.buffer_);
            size_ = other.size_;
            other.size_ = 0;
        }
        return *this;
    }
    
    // Delete copy operations
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    
    ~ResourceManager() = default;
    
    int& operator[](size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return buffer_[index];
    }
    
    size_t size() const noexcept { return size_; }
};

// Lambda expressions and closures
class LambdaExamples {
public:
    void demonstrate_lambdas() {
        int capture_value = 42;
        
        // Simple lambda
        auto simple_lambda = []() { return 1; };
        
        // Lambda with parameters
        auto add_lambda = [](int a, int b) { return a + b; };
        
        // Lambda with capture by value
        auto capture_by_value = [capture_value](int x) { 
            return x + capture_value; 
        };
        
        // Lambda with capture by reference
        auto capture_by_ref = [&capture_value](int x) { 
            capture_value += x; 
            return capture_value; 
        };
        
        // Lambda with mixed capture
        int another_value = 10;
        auto mixed_capture = [capture_value, &another_value](int x) {
            another_value += x;
            return capture_value + another_value;
        };
        
        // Generic lambda (C++14)
        auto generic_lambda = [](auto a, auto b) { 
            return a + b; 
        };
        
        // Lambda with trailing return type
        auto explicit_return = [](int x) -> double { 
            return static_cast<double>(x) / 2.0; 
        };
        
        // Recursive lambda
        std::function<int(int)> factorial_lambda = [&factorial_lambda](int n) -> int {
            return (n <= 1) ? 1 : n * factorial_lambda(n - 1);
        };
        
        // Use the lambdas
        int result1 = simple_lambda();
        int result2 = add_lambda(5, 3);
        int result3 = capture_by_value(10);
        int result4 = capture_by_ref(5);
        int result5 = mixed_capture(3);
        auto result6 = generic_lambda(1.5, 2.5);
        double result7 = explicit_return(10);
        int result8 = factorial_lambda(5);
        
        // Suppress unused variable warnings
        (void)result1; (void)result2; (void)result3; (void)result4;
        (void)result5; (void)result6; (void)result7; (void)result8;
    }
};

// Template metaprogramming with type traits
template<typename T>
class TypeAnalyzer {
public:
    static constexpr bool is_pointer = std::is_pointer<T>::value;
    static constexpr bool is_reference = std::is_reference<T>::value;
    static constexpr bool is_const = std::is_const<T>::value;
    static constexpr bool is_arithmetic = std::is_arithmetic<T>::value;
    
    // Static assertions based on type properties
    static_assert(!std::is_void<T>::value, "T cannot be void");
    
    using value_type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
    using pointer_type = typename std::add_pointer<value_type>::type;
    using reference_type = typename std::add_lvalue_reference<value_type>::type;
};

// Variadic templates with perfect forwarding
template<typename... Args>
class VariadicContainer {
private:
    std::tuple<Args...> data_;
    
public:
    template<typename... UArgs>
    explicit VariadicContainer(UArgs&&... args) 
        : data_(std::forward<UArgs>(args)...) {}
    
    template<size_t Index>
    auto get() -> decltype(std::get<Index>(data_)) {
        static_assert(Index < sizeof...(Args), "Index out of bounds");
        return std::get<Index>(data_);
    }
    
    static constexpr size_t size() { return sizeof...(Args); }
};

// Template template parameters
template<template<typename> class Container, typename T>
class ContainerAdapter {
private:
    Container<T> container_;
    
public:
    void add(const T& item) {
        // Assume Container has push_back method
        container_.push_back(item);
    }
    
    size_t size() const {
        return container_.size();
    }
    
    Container<T>& get_container() { return container_; }
};

// Complex control flow for CFG analysis
class ControlFlowExamples {
public:
    int complex_control_flow(int input) {
        int result = 0;
        
        try {
            // Nested loops with breaks and continues
            for (int i = 0; i < 10; ++i) {
                if (i % 2 == 0) continue;
                
                for (int j = 0; j < i; ++j) {
                    if (j > 5) break;
                    
                    switch (input % 4) {
                        case 0:
                            result += i * j;
                            break;
                        case 1:
                            result -= i + j;
                            break;
                        case 2:
                            if (i > j) {
                                result *= 2;
                            } else {
                                result /= 2;
                            }
                            break;
                        default:
                            goto cleanup;
                    }
                }
            }
            
            // Exception handling
            if (result < 0) {
                throw ValidationException("Result cannot be negative");
            }
            
            // While loop with complex condition
            while (result > 0 && result < 1000) {
                result = (result * 3 + 1) / 2;
                if (result % 7 == 0) {
                    throw ResourceException("Resource limit exceeded");
                }
            }
            
        } catch (const ValidationException& e) {
            result = -1;
        } catch (const ResourceException& e) {
            result = -2;
        } catch (const std::exception& e) {
            result = -3;
        } catch (...) {
            result = -4;
        }
        
        cleanup:
        return result;
    }
    
    // Function with multiple return paths
    [[nodiscard]] int multiple_returns(int x) {
        if (x < 0) return -1;
        if (x == 0) return 0;
        if (x == 1) return 1;
        
        for (int i = 2; i <= x; ++i) {
            if (i * i == x) return i;
            if (i * i > x) return -1;
        }
        
        return x;
    }
};

// Macro usage examples
#define STRINGIFY(x) #x
#define CONCAT(a, b) a##b

class MacroExamples {
public:
    void demonstrate_macros() {
        int max_val = MAX(10, 20);
        
        // Stringification
        const char* str = STRINGIFY(hello_world);
        
        // Token concatenation
        int CONCAT(var, _name) = 42;
        
        // Conditional compilation
        #if ENABLE_OPTIMIZATION
            // Optimized code path
            int optimized_result = max_val * 2;
        #else
            // Standard code path
            int standard_result = max_val + max_val;
        #endif
        
        // Suppress unused variable warnings
        (void)str; (void)var_name;
        #if ENABLE_OPTIMIZATION
            (void)optimized_result;
        #else
            (void)standard_result;
        #endif
    }
    
    DECLARE_GETTER_SETTER(int, value)
    
private:
    int value_;
};

// Function demonstrating all features
void demonstrate_advanced_features() {
    // Static assertions
    static_assert(Factorial<5>::value == 120, "5! should be 120");
    
    // SFINAE usage
    bool int_even = is_even(42);
    bool float_even = is_even(3.14);
    
    // Exception handling
    try {
        ResourceManager manager(10);
        manager[0] = 100;
        
        if (manager.size() > 5) {
            throw ValidationException("Size too large");
        }
    } catch (const BaseException& e) {
        // Handle custom exceptions
    }
    
    // Lambda usage
    LambdaExamples lambda_demo;
    lambda_demo.demonstrate_lambdas();
    
    // Template metaprogramming
    TypeAnalyzer<int*> int_ptr_analyzer;
    static_assert(int_ptr_analyzer.is_pointer, "Should be pointer");
    
    // Variadic templates
    VariadicContainer<int, double, std::string> container(42, 3.14, "hello");
    auto first_value = container.get<0>();
    
    // Control flow
    ControlFlowExamples control_demo;
    int flow_result = control_demo.complex_control_flow(7);
    int return_result = control_demo.multiple_returns(16);
    
    // Macro usage
    MacroExamples macro_demo;
    macro_demo.demonstrate_macros();
    
    // Suppress unused variable warnings
    (void)int_even; (void)float_even; (void)first_value;
    (void)flow_result; (void)return_result;
}

// Template instantiations for testing
template class TypeAnalyzer<int>;
template class TypeAnalyzer<double*>;
template class VariadicContainer<int, std::string>;

// Explicit template instantiation
template bool is_even<int>(int);
template bool is_even<double>(double);

#ifdef ADVANCED_MODE
    #pragma pack(pop)
#endif

// End of file comment for documentation extraction
/**
 * @brief This file demonstrates comprehensive C++ features for complete AST analysis
 * 
 * Features covered:
 * - Static assertions and compile-time validation
 * - Advanced template metaprogramming and SFINAE
 * - Exception handling hierarchies
 * - Lambda expressions with various capture modes
 * - Attributes and annotations
 * - Complex preprocessor usage
 * - Constexpr functions and constant expressions
 * - RAII and modern C++ resource management
 * - Complex control flow for CFG analysis
 * - Template specializations and instantiations
 */
