/**
 * @file modern_cpp_features.cpp
 * @brief Comprehensive example demonstrating modern C++ features (C++11-17)
 * 
 * This example showcases modern C++ language features:
 * - C++11: auto, nullptr, range-based for, lambdas, move semantics
 * - C++14: generic lambdas, variable templates, return type deduction
 * - C++17: structured bindings, if constexpr, fold expressions
 * - Attributes: [[nodiscard]], [[deprecated]], [[maybe_unused]], etc.
 * - Smart pointers and RAII
 * - Perfect forwarding and universal references
 * - Variadic templates and parameter packs
 */

#include <memory>
#include <vector>
#include <string>
#include <tuple>
#include <utility>
#include <type_traits>
#include <functional>
#include <algorithm>
#include <iostream>

// C++17 feature detection
#if __cplusplus >= 201703L
    #define HAS_CPP17 1
#else
    #define HAS_CPP17 0
#endif

// C++14 feature detection
#if __cplusplus >= 201402L
    #define HAS_CPP14 1
#else
    #define HAS_CPP14 0
#endif

// Attributes demonstration
[[nodiscard]] int important_calculation(int x) {
    return x * x + 2 * x + 1;
}

[[deprecated("Use new_api_function instead")]]
void old_api_function() {
    // Legacy implementation
}

void new_api_function() {
    // New implementation
}

[[maybe_unused]] static constexpr int UNUSED_CONSTANT = 42;

// C++11: Strong enum classes
enum class Status : int {
    Success = 0,
    Warning = 1,
    Error = 2,
    Critical = 3
};

enum class Color : unsigned char {
    Red = 0xFF0000,
    Green = 0x00FF00,
    Blue = 0x0000FF
};

// C++11: Delegating constructors and member initializers
class ModernClass {
private:
    std::string name_;
    int value_;
    std::vector<int> data_;
    
public:
    // Default constructor with member initializers
    ModernClass() : ModernClass("default", 0) {}
    
    // Delegating constructor
    explicit ModernClass(const std::string& name) : ModernClass(name, 42) {}
    
    // Main constructor
    ModernClass(const std::string& name, int value) 
        : name_(name), value_(value), data_{1, 2, 3, 4, 5} {}
    
    // C++11: Move constructor
    ModernClass(ModernClass&& other) noexcept 
        : name_(std::move(other.name_)), 
          value_(other.value_), 
          data_(std::move(other.data_)) {
        other.value_ = 0;
    }
    
    // C++11: Move assignment operator
    ModernClass& operator=(ModernClass&& other) noexcept {
        if (this != &other) {
            name_ = std::move(other.name_);
            value_ = other.value_;
            data_ = std::move(other.data_);
            other.value_ = 0;
        }
        return *this;
    }
    
    // Copy constructor and assignment (rule of 5)
    ModernClass(const ModernClass& other) = default;
    ModernClass& operator=(const ModernClass& other) = default;
    
    // Virtual destructor
    virtual ~ModernClass() = default;
    
    // C++11: Explicit conversion operators
    explicit operator bool() const noexcept {
        return value_ != 0;
    }
    
    explicit operator int() const noexcept {
        return value_;
    }
    
    // C++14: Return type deduction
    auto get_name() const {
        return name_;
    }
    
    auto get_value() const -> int {
        return value_;
    }
    
    // C++11: Range-based for loop support
    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.cbegin(); }
    auto end() const { return data_.cend(); }
    
    // C++17: [[nodiscard]] attribute
    [[nodiscard]] bool is_valid() const noexcept {
        return !name_.empty() && value_ >= 0;
    }
    
    // C++11: Override and final specifiers
    virtual void process() {}
    virtual void finalize() final {}  // Cannot be overridden
};

// C++11: Final class
class FinalClass final : public ModernClass {
public:
    FinalClass() : ModernClass("final", 100) {}
    
    // Override virtual function
    void process() override {
        // Implementation
    }
    
    // Cannot override finalize() - it's marked final
};

// C++11: Template aliases
template<typename T>
using Vector = std::vector<T>;

template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename Key, typename Value>
using Pair = std::pair<Key, Value>;

// C++14: Variable templates
template<typename T>
constexpr bool is_pointer_v = std::is_pointer<T>::value;

template<typename T>
constexpr size_t type_size_v = sizeof(T);

// C++11: Variadic templates with perfect forwarding
template<typename T, typename... Args>
UniquePtr<T> make_unique_impl(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

// C++17: Fold expressions (if available)
#if HAS_CPP17
template<typename... Args>
auto sum_all(Args... args) {
    return (args + ...);  // Unary right fold
}

template<typename... Args>
auto multiply_all(Args... args) {
    return (args * ...);  // Unary right fold
}

template<typename... Args>
void print_all(Args... args) {
    ((std::cout << args << " "), ...);  // Binary left fold
    std::cout << std::endl;
}
#endif

// C++11: Lambda expressions with various features
class LambdaExamples {
public:
    void demonstrate_lambdas() {
        int local_var = 42;
        
        // Basic lambda
        auto simple = []() { return 1; };
        
        // Lambda with parameters and return type
        auto add = [](int a, int b) -> int { return a + b; };
        
        // Lambda with capture by value
        auto capture_val = [local_var](int x) { return x + local_var; };
        
        // Lambda with capture by reference
        auto capture_ref = [&local_var](int x) { local_var += x; return local_var; };
        
        // Lambda with mixed capture
        int another_var = 10;
        auto mixed_capture = [local_var, &another_var](int x) {
            another_var += x;
            return local_var + another_var;
        };
        
        // C++14: Generic lambda
        #if HAS_CPP14
        auto generic = [](auto a, auto b) { return a + b; };
        #endif
        
        // C++14: Generalized capture (init capture)
        #if HAS_CPP14
        auto init_capture = [value = local_var * 2](int x) { return value + x; };
        #endif
        
        // Recursive lambda using std::function
        std::function<int(int)> factorial = [&factorial](int n) -> int {
            return (n <= 1) ? 1 : n * factorial(n - 1);
        };
        
        // Use lambdas
        int result1 = simple();
        int result2 = add(5, 3);
        int result3 = capture_val(10);
        int result4 = capture_ref(5);
        int result5 = mixed_capture(3);
        int result6 = factorial(5);
        
        #if HAS_CPP14
        auto result7 = generic(1.5, 2.5);
        int result8 = init_capture(10);
        (void)result7; (void)result8;
        #endif
        
        // Suppress unused variable warnings
        (void)result1; (void)result2; (void)result3; 
        (void)result4; (void)result5; (void)result6;
    }
};

// C++11: Smart pointer examples
class SmartPointerExamples {
public:
    void demonstrate_smart_pointers() {
        // unique_ptr examples
        auto unique_int = std::make_unique<int>(42);
        auto unique_array = std::make_unique<int[]>(10);
        
        // Move semantics with unique_ptr
        auto moved_ptr = std::move(unique_int);
        // unique_int is now nullptr
        
        // shared_ptr examples
        auto shared_int = std::make_shared<int>(100);
        auto another_shared = shared_int;  // Reference count increases
        
        // weak_ptr to break circular references
        std::weak_ptr<int> weak_ref = shared_int;
        
        // Custom deleter
        auto custom_deleter = [](int* ptr) {
            delete ptr;
        };
        std::unique_ptr<int, decltype(custom_deleter)> custom_ptr(
            new int(200), custom_deleter);
        
        // Use the pointers
        if (moved_ptr) {
            *moved_ptr = 50;
        }
        
        if (auto locked = weak_ref.lock()) {
            *locked = 150;
        }
        
        // Arrays with smart pointers
        for (int i = 0; i < 10; ++i) {
            unique_array[i] = i * i;
        }
    }
};

// C++17: Structured bindings (if available)
#if HAS_CPP17
std::tuple<int, std::string, double> get_data() {
    return {42, "hello", 3.14};
}

void demonstrate_structured_bindings() {
    // Structured binding with tuple
    auto [value, text, ratio] = get_data();
    
    // Structured binding with pair
    std::pair<int, std::string> p{100, "world"};
    auto [first, second] = p;
    
    // Structured binding with array
    int arr[] = {1, 2, 3};
    auto [a, b, c] = arr;
    
    // Use the variables
    (void)value; (void)text; (void)ratio;
    (void)first; (void)second;
    (void)a; (void)b; (void)c;
}
#endif

// C++17: if constexpr (if available)
template<typename T>
void process_type() {
    #if HAS_CPP17
    if constexpr (std::is_integral_v<T>) {
        // Compile-time branch for integral types
        T value = 42;
        (void)value;
    } else if constexpr (std::is_floating_point_v<T>) {
        // Compile-time branch for floating-point types
        T value = 3.14;
        (void)value;
    } else {
        // Compile-time branch for other types
        T value{};
        (void)value;
    }
    #else
    // Fallback for older standards
    T value{};
    (void)value;
    #endif
}

// C++11: Constexpr functions
constexpr int factorial_constexpr(int n) {
    return (n <= 1) ? 1 : n * factorial_constexpr(n - 1);
}

constexpr bool is_power_of_two(unsigned int n) {
    return n != 0 && (n & (n - 1)) == 0;
}

// C++14: Relaxed constexpr
#if HAS_CPP14
constexpr int fibonacci_constexpr(int n) {
    if (n <= 1) return n;
    
    int a = 0, b = 1;
    for (int i = 2; i <= n; ++i) {
        int temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}
#endif

// C++11: User-defined literals
namespace literals {
    constexpr long long operator"" _KB(unsigned long long value) {
        return value * 1024;
    }
    
    constexpr long long operator"" _MB(unsigned long long value) {
        return value * 1024 * 1024;
    }
    
    constexpr double operator"" _pi(long double value) {
        return static_cast<double>(value * 3.14159265359);
    }
}

// C++11: Uniform initialization and initializer lists
class InitializationExamples {
private:
    std::vector<int> numbers_;
    std::string name_;
    
public:
    // Constructor with initializer list
    InitializationExamples(std::initializer_list<int> numbers, const std::string& name)
        : numbers_(numbers), name_(name) {}
    
    // Brace initialization
    InitializationExamples() : numbers_{1, 2, 3, 4, 5}, name_{"default"} {}
    
    void demonstrate_initialization() {
        // Uniform initialization
        int x{42};
        double y{3.14};
        std::string s{"hello"};
        
        // Aggregate initialization
        struct Point { int x, y; };
        Point p{10, 20};
        
        // Container initialization
        std::vector<int> vec{1, 2, 3, 4, 5};
        Vector<std::string> strings{"one", "two", "three"};
        
        // Suppress unused variable warnings
        (void)x; (void)y; (void)s; (void)p; (void)vec; (void)strings;
    }
};

// C++11: Thread-local storage
thread_local int thread_local_counter = 0;

// C++11: Alignment specifiers
struct alignas(16) AlignedStruct {
    char data[16];
};

// Function to demonstrate all modern features
void demonstrate_modern_cpp_features() {
    using namespace literals;
    
    // Auto type deduction
    auto x = 42;                    // int
    auto y = 3.14;                  // double
    auto z = "hello";               // const char*
    auto vec = std::vector<int>{1, 2, 3, 4, 5};
    
    // Range-based for loops
    for (const auto& element : vec) {
        (void)element;  // Suppress unused warning
    }
    
    // nullptr instead of NULL
    int* ptr = nullptr;
    
    // Strong enums
    Status status = Status::Success;
    Color color = Color::Red;
    
    // Modern class usage
    ModernClass obj("test", 100);
    if (obj.is_valid()) {
        auto name = obj.get_name();
        (void)name;
    }
    
    // Lambda examples
    LambdaExamples lambda_demo;
    lambda_demo.demonstrate_lambdas();
    
    // Smart pointers
    SmartPointerExamples smart_ptr_demo;
    smart_ptr_demo.demonstrate_smart_pointers();
    
    // Constexpr usage
    constexpr int fact5 = factorial_constexpr(5);
    constexpr bool is_pow2 = is_power_of_two(16);
    
    #if HAS_CPP14
    constexpr int fib10 = fibonacci_constexpr(10);
    (void)fib10;
    #endif
    
    // User-defined literals
    auto size_kb = 64_KB;
    auto size_mb = 2_MB;
    auto angle = 2.0_pi;
    
    // Template processing
    process_type<int>();
    process_type<double>();
    process_type<std::string>();
    
    #if HAS_CPP17
    // Structured bindings
    demonstrate_structured_bindings();
    
    // Fold expressions
    auto sum = sum_all(1, 2, 3, 4, 5);
    auto product = multiply_all(2, 3, 4);
    print_all("Values:", sum, product);
    #endif
    
    // Initialization examples
    InitializationExamples init_demo;
    init_demo.demonstrate_initialization();
    
    // Thread-local storage
    thread_local_counter++;
    
    // Alignment
    AlignedStruct aligned_data;
    
    // Suppress unused variable warnings
    (void)x; (void)y; (void)z; (void)ptr; (void)status; (void)color;
    (void)fact5; (void)is_pow2; (void)size_kb; (void)size_mb; (void)angle;
    (void)aligned_data;
}

// Template instantiations for testing
template void process_type<int>();
template void process_type<double>();
template void process_type<std::string>();

// Static assertions with modern features
static_assert(factorial_constexpr(5) == 120, "5! should be 120");
static_assert(is_power_of_two(16), "16 should be power of 2");
static_assert(!is_power_of_two(15), "15 should not be power of 2");

#if HAS_CPP14
static_assert(fibonacci_constexpr(10) == 55, "Fibonacci(10) should be 55");
#endif

/**
 * @brief End of modern C++ features example
 * 
 * This file demonstrates:
 * - C++11 features: auto, nullptr, lambdas, move semantics, smart pointers
 * - C++14 features: generic lambdas, variable templates, relaxed constexpr
 * - C++17 features: structured bindings, if constexpr, fold expressions
 * - Attributes: [[nodiscard]], [[deprecated]], [[maybe_unused]]
 * - Modern initialization and uniform syntax
 * - Perfect forwarding and universal references
 * - Thread-local storage and alignment specifiers
 */
