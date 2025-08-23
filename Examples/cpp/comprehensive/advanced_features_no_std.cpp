/**
 * @file advanced_features_no_std.cpp
 * @brief Comprehensive example demonstrating advanced C++ features without standard library
 * 
 * This example showcases advanced C++ features that are often missing from basic examples,
 * but without relying on standard library headers to ensure compilation compatibility:
 * - Static assertions and compile-time evaluation
 * - Advanced template metaprogramming and SFINAE
 * - Exception handling and RAII (basic)
 * - Lambda expressions and closures
 * - Attributes and annotations
 * - Complex preprocessor usage
 * - Modern C++ features (C++11-17)
 * - Constant expressions and constexpr
 * - Advanced type traits (custom implementations)
 */

// Custom type traits without standard library
template<typename T, T v>
struct integral_constant {
    static constexpr T value = v;
    typedef T value_type;
    typedef integral_constant type;
    constexpr operator value_type() const noexcept { return value; }
    constexpr value_type operator()() const noexcept { return value; }
};

typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;

template<typename T> struct remove_const { typedef T type; };
template<typename T> struct remove_const<const T> { typedef T type; };

template<typename T> struct remove_volatile { typedef T type; };
template<typename T> struct remove_volatile<volatile T> { typedef T type; };

template<typename T> 
struct remove_cv {
    typedef typename remove_volatile<typename remove_const<T>::type>::type type;
};

template<typename T> struct is_pointer : false_type {};
template<typename T> struct is_pointer<T*> : true_type {};

template<typename T> struct is_integral : false_type {};
template<> struct is_integral<bool> : true_type {};
template<> struct is_integral<char> : true_type {};
template<> struct is_integral<signed char> : true_type {};
template<> struct is_integral<unsigned char> : true_type {};
template<> struct is_integral<short> : true_type {};
template<> struct is_integral<unsigned short> : true_type {};
template<> struct is_integral<int> : true_type {};
template<> struct is_integral<unsigned int> : true_type {};
template<> struct is_integral<long> : true_type {};
template<> struct is_integral<unsigned long> : true_type {};
template<> struct is_integral<long long> : true_type {};
template<> struct is_integral<unsigned long long> : true_type {};

template<typename T> struct is_floating_point : false_type {};
template<> struct is_floating_point<float> : true_type {};
template<> struct is_floating_point<double> : true_type {};
template<> struct is_floating_point<long double> : true_type {};

template<bool B, typename T = void>
struct enable_if {};

template<typename T>
struct enable_if<true, T> { typedef T type; };

// Preprocessor directives for conditional compilation
#ifdef ADVANCED_MODE
    #define ENABLE_OPTIMIZATION 1
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
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Complex macro with variadic arguments
#define LOG_DEBUG(...) \
    do { \
        if (DEBUG_ENABLED) { \
            /* Debug output would go here */ \
        } \
    } while(0)

// Static assertions for compile-time validation
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");
static_assert(is_integral<int>::value, "int should be integral type");

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
struct has_value_type {
private:
    template<typename U>
    static auto test(int) -> decltype(typename U::value_type{}, true_type{});
    
    template<typename>
    static false_type test(...);
    
public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

// SFINAE-enabled function templates
template<typename T>
typename enable_if<is_integral<T>::value, bool>::type
is_even(T value) {
    return value % 2 == 0;
}

template<typename T>
typename enable_if<is_floating_point<T>::value, bool>::type
is_even(T value) {
    return false; // Floating point numbers are never even in this context
}

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
    // Would call std::terminate() if available
    while(true) {}
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
    const char* message_;
    
public:
    Result(bool success, const char* message) 
        : success_(success), message_(message) {}
    
    [[nodiscard]] bool is_success() const { return success_; }
    [[nodiscard]] const char* message() const { return message_; }
};

// Exception handling hierarchy (basic without std::exception)
class BaseException {
protected:
    const char* message_;
    
public:
    explicit BaseException(const char* message) : message_(message) {}
    
    const char* what() const noexcept {
        return message_;
    }
    
    virtual ~BaseException() = default;
};

class ValidationException : public BaseException {
public:
    explicit ValidationException(const char* message) 
        : BaseException(message) {}
};

class ResourceException : public BaseException {
public:
    explicit ResourceException(const char* message) 
        : BaseException(message) {}
};

// Simple RAII resource management without smart pointers
class ResourceManager {
private:
    int* buffer_;
    int size_;
    
public:
    explicit ResourceManager(int size) 
        : buffer_(new int[size]), size_(size) {
        if (size == 0) {
            throw ValidationException("Size cannot be zero");
        }
        
        // Initialize buffer
        for (int i = 0; i < size_; ++i) {
            buffer_[i] = 0;
        }
    }
    
    // Move constructor
    ResourceManager(ResourceManager&& other) noexcept 
        : buffer_(other.buffer_), size_(other.size_) {
        other.buffer_ = nullptr;
        other.size_ = 0;
    }
    
    // Move assignment
    ResourceManager& operator=(ResourceManager&& other) noexcept {
        if (this != &other) {
            delete[] buffer_;
            buffer_ = other.buffer_;
            size_ = other.size_;
            other.buffer_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }
    
    // Delete copy operations
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    
    ~ResourceManager() { 
        delete[] buffer_; 
    }
    
    int& operator[](int index) {
        if (index >= size_ || index < 0) {
            throw ValidationException("Index out of range");
        }
        return buffer_[index];
    }
    
    int size() const noexcept { return size_; }
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
        
        // Use the lambdas
        int result1 = simple_lambda();
        int result2 = add_lambda(5, 3);
        int result3 = capture_by_value(10);
        int result4 = capture_by_ref(5);
        int result5 = mixed_capture(3);
        auto result6 = generic_lambda(1.5, 2.5);
        double result7 = explicit_return(10);
        
        // Suppress unused variable warnings
        (void)result1; (void)result2; (void)result3; (void)result4;
        (void)result5; (void)result6; (void)result7;
    }
};

// Template metaprogramming with type traits
template<typename T>
class TypeAnalyzer {
public:
    static constexpr bool is_pointer_type = is_pointer<T>::value;
    static constexpr bool is_integral_type = is_integral<T>::value;
    static constexpr bool is_floating_type = is_floating_point<T>::value;
    
    // Static assertions based on type properties
    static_assert(!is_pointer<void>::value || is_pointer<T>::value, "Type analysis constraint");
    
    using value_type = typename remove_cv<T>::type;
};

// Variadic templates with perfect forwarding
template<typename... Args>
class VariadicContainer {
private:
    // Simple tuple-like storage (without std::tuple)
    static constexpr int arg_count = sizeof...(Args);
    
public:
    template<typename... UArgs>
    explicit VariadicContainer(UArgs&&... args) {
        // Would store arguments if we had proper tuple implementation
        (void)sizeof...(args); // Suppress unused parameter warning
    }
    
    static constexpr int size() { return sizeof...(Args); }
};

// Template template parameters
template<template<typename> class Container, typename T>
class ContainerAdapter {
private:
    Container<T> container_;
    
public:
    void add(const T& item) {
        // Assume Container has appropriate interface
        (void)item; // Suppress unused warning
    }
    
    Container<T>& get_container() { return container_; }
};

// Simple container for template template parameter testing
template<typename T>
class SimpleContainer {
private:
    T* data_;
    int size_;
    int capacity_;
    
public:
    SimpleContainer() : data_(nullptr), size_(0), capacity_(0) {}
    
    ~SimpleContainer() {
        delete[] data_;
    }
    
    void push_back(const T& item) {
        if (size_ >= capacity_) {
            // Simple growth strategy
            int new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            T* new_data = new T[new_capacity];
            
            // Copy existing data
            for (int i = 0; i < size_; ++i) {
                new_data[i] = data_[i];
            }
            
            delete[] data_;
            data_ = new_data;
            capacity_ = new_capacity;
        }
        
        data_[size_++] = item;
    }
    
    int size() const { return size_; }
    T& operator[](int index) { return data_[index]; }
    const T& operator[](int index) const { return data_[index]; }
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
        } catch (const BaseException& e) {
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
        int min_val = MIN(5, 15);
        
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
        (void)min_val; (void)str; (void)var_name;
        #if ENABLE_OPTIMIZATION
            (void)optimized_result;
        #else
            (void)standard_result;
        #endif
    }
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
    static_assert(int_ptr_analyzer.is_pointer_type, "Should be pointer");
    
    // Variadic templates
    VariadicContainer<int, double> container(42, 3.14);
    
    // Template template parameters
    ContainerAdapter<SimpleContainer, int> adapter;
    adapter.add(42);
    
    // Control flow
    ControlFlowExamples control_demo;
    int flow_result = control_demo.complex_control_flow(7);
    int return_result = control_demo.multiple_returns(16);
    
    // Macro usage
    MacroExamples macro_demo;
    macro_demo.demonstrate_macros();
    
    // Suppress unused variable warnings
    (void)int_even; (void)float_even; (void)flow_result; (void)return_result;
}

// Template instantiations for testing
template class TypeAnalyzer<int>;
template class TypeAnalyzer<double*>;
template class VariadicContainer<int>;
template class SimpleContainer<int>;
template class ContainerAdapter<SimpleContainer, int>;

// Explicit template instantiation
template bool is_even<int>(int);
template bool is_even<double>(double);

/**
 * @brief This file demonstrates comprehensive C++ features for complete AST analysis
 * 
 * Features covered without standard library dependencies:
 * - Static assertions and compile-time validation
 * - Advanced template metaprogramming and SFINAE
 * - Exception handling hierarchies (custom)
 * - Lambda expressions with various capture modes
 * - Attributes and annotations
 * - Complex preprocessor usage
 * - Constexpr functions and constant expressions
 * - RAII and resource management (manual)
 * - Complex control flow for CFG analysis
 * - Template specializations and instantiations
 * - Custom type traits implementation
 */
