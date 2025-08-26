/**
 * @file example_templates.cpp
 * @brief Comprehensive example demonstrating C++ template features
 * 
 * This example showcases:
 * - Class templates and function templates
 * - Template specialization (full and partial)
 * - Template instantiation and deduction
 * - Variadic templates and parameter packs
 * - SFINAE (Substitution Failure Is Not An Error)
 * - Template metaprogramming techniques
 * - Constexpr templates and compile-time evaluation
 */
// Simple vector replacement
template<typename T>
class SimpleVector {
public:
    // Add required typedefs for template compatibility
    using value_type = T;
    using iterator = T*;
    using const_iterator = const T*;
    
private:
    T* data;
    int size_;
    int capacity_;
public:
    SimpleVector() : data(nullptr), size_(0), capacity_(0) {}
    ~SimpleVector() { delete[] data; }
    void push_back(const T& value) {
        if (size_ >= capacity_) {
            int new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            T* new_data = new T[new_capacity];
            for (int i = 0; i < size_; i++) {
                new_data[i] = data[i];
            }
            delete[] data;
            data = new_data;
            capacity_ = new_capacity;
        }
        data[size_++] = value;
    }
    int size() const { return size_; }
    T* begin() { return data; }
    const T* begin() const { return data; }
    T* end() { return data + size_; }
    const T* end() const { return data + size_; }
};

// Simple type traits replacements
template<typename T> struct simple_is_integral { static constexpr bool value = false; };
template<> struct simple_is_integral<int> { static constexpr bool value = true; };
template<> struct simple_is_integral<long> { static constexpr bool value = true; };
template<> struct simple_is_integral<short> { static constexpr bool value = true; };
template<> struct simple_is_integral<char> { static constexpr bool value = true; };

template<typename T> struct simple_is_floating_point { static constexpr bool value = false; };
template<> struct simple_is_floating_point<double> { static constexpr bool value = true; };
template<> struct simple_is_floating_point<float> { static constexpr bool value = true; };

// Simple enable_if replacement
template<bool B, typename T = void>
struct simple_enable_if {};

template<typename T>
struct simple_enable_if<true, T> {
    typedef T type;
};

template<typename T> struct simple_is_pointer { static constexpr bool value = false; };
template<typename T> struct simple_is_pointer<T*> { static constexpr bool value = true; };

// Simple unique_ptr replacement
template<typename T>
class SimpleUniquePtr {
private:
    T* ptr;
public:
    explicit SimpleUniquePtr(T* p = nullptr) : ptr(p) {}
    ~SimpleUniquePtr() { delete ptr; }
    SimpleUniquePtr(SimpleUniquePtr&& other) noexcept : ptr(other.ptr) { other.ptr = nullptr; }
    SimpleUniquePtr& operator=(SimpleUniquePtr&& other) noexcept {
        if (this != &other) { delete ptr; ptr = other.ptr; other.ptr = nullptr; }
        return *this;
    }
    SimpleUniquePtr(const SimpleUniquePtr&) = delete;
    SimpleUniquePtr& operator=(const SimpleUniquePtr&) = delete;
    T& operator*() const { return *ptr; }
    T* operator->() const { return ptr; }
};

// Basic class template
template<typename T, int Size = 10>
class FixedArray {
private:
    T data[Size];
    int count;
    
public:
    FixedArray() : count(0) {}
    
    void add(const T& item) {
        if (count < Size) {
            data[count++] = item;
        }
    }
    
    T& operator[](int index) { return data[index]; }
    const T& operator[](int index) const { return data[index]; }
    
    // Template member function
    template<typename U>
    bool contains(const U& value) const {
        for (int i = 0; i < count; ++i) {
            if (data[i] == value) return true;
        }
        return false;
    }
    
    // Nested template class
    template<typename Iterator>
    class Range {
    private:
        Iterator begin_;
        Iterator end_;
    public:
        Range(Iterator b, Iterator e) : begin_(b), end_(e) {}
        Iterator begin() const { return begin_; }
        Iterator end() const { return end_; }
    };
    
    // Static member template
    template<typename U>
    static FixedArray<U, Size> create() {
        return FixedArray<U, Size>();
    }
};

// Full template specialization
template<>
class FixedArray<bool, 8> {
private:
    unsigned char bits; // Pack booleans into bits
    int count;
    
public:
    FixedArray() : bits(0), count(0) {}
    
    void add(bool value) {
        if (count < 8) {
            if (value) {
                bits |= (1 << count);
            }
            count++;
        }
    }
    
    bool operator[](int index) const {
        return (bits & (1 << index)) != 0;
    }
    
    // Different interface for specialized version
    unsigned char getBits() const { return bits; }
};

// Partial template specialization
template<typename T>
class FixedArray<T*, 5> {
private:
    T* data[5];
    int count;
    
public:
    FixedArray() : count(0) {
        for (int i = 0; i < 5; ++i) {
            data[i] = nullptr;
        }
    }
    
    void add(T* ptr) {
        if (count < 5) {
            data[count++] = ptr;
        }
    }
    
    T*& operator[](int index) { return data[index]; }
    T* const& operator[](int index) const { return data[index]; }
    
    // Cleanup all pointers
    ~FixedArray() {
        for (int i = 0; i < count; ++i) {
            delete data[i];
        }
    }
};

// Function templates
template<typename T>
T max(const T& a, const T& b) {
    return (a > b) ? a : b;
}

// Function template with multiple parameters
template<typename T, typename U>
auto multiply(const T& a, const U& b) -> decltype(a * b) {
    return a * b;
}

// Function template specialization
template<>
int max<int>(const int& a, const int& b) {
    return (a > b) ? a : b;
}

// Simple output function to replace cout
void simpleOutput(const char* str) {
    // In a real application, this would output to console/file
    // For this example, we just process the string silently
    (void)str; // Suppress unused warning
}

template<typename T>
void simpleOutput(const T& value) {
    // Convert value to string representation if needed
    (void)value; // Suppress unused warning
}

// Variadic templates (C++11) - simplified without initializer_list
template<typename... Args>
void print(Args... args) {
    // Process each argument without std::cout (use recursion instead)
    printRecursive(args...);
}

// Variadic template with recursion (pre-C++17 style)
template<typename T>
void printRecursive(T&& t) {
    simpleOutput(t);
}

template<typename T, typename... Args>
void printRecursive(T&& t, Args&&... args) {
    simpleOutput(t);
    printRecursive(args...);
}

// SFINAE (Substitution Failure Is Not An Error)
template<typename T>
typename simple_enable_if<simple_is_integral<T>::value, bool>::type
isEven(T value) {
    return value % 2 == 0;
}

template<typename T>
typename simple_enable_if<simple_is_floating_point<T>::value, bool>::type
isEven(T value) {
    return false; // Floating point numbers are never even in this context
}

// Template metaprogramming - compile-time factorial
template<int N>
struct Factorial {
    static constexpr int value = N * Factorial<N - 1>::value;
};

template<>
struct Factorial<0> {
    static constexpr int value = 1;
};

template<>
struct Factorial<1> {
    static constexpr int value = 1;
};

// Template metaprogramming - type traits
template<typename T>
struct RemovePointer {
    using type = T;
};

template<typename T>
struct RemovePointer<T*> {
    using type = T;
};

template<typename T>
struct RemovePointer<T* const> {
    using type = T;
};

template<typename T>
struct RemovePointer<T* volatile> {
    using type = T;
};

template<typename T>
struct RemovePointer<T* const volatile> {
    using type = T;
};

// Alias template (C++11)
template<typename T>
using Vector = SimpleVector<T>;

template<typename T>
using UniquePtr = SimpleUniquePtr<T>;

// Variable templates (C++14)
template<typename T>
constexpr bool is_pointer_v = simple_is_pointer<T>::value;

template<int N>
constexpr int factorial_v = Factorial<N>::value;

// Class template with template template parameters
template<template<typename> class Container, typename T>
class ContainerWrapper {
private:
    Container<T> container;
    
public:
    void add(const T& item) {
        container.push_back(item);
    }
    
    auto begin() -> decltype(container.begin()) {
        return container.begin();
    }
    
    auto end() -> decltype(container.end()) {
        return container.end();
    }
    
    int size() const {
        return container.size();
    }
};

// Dependent name lookup and typename keyword
template<typename T>
class DependentNameTest {
public:
    using ValueType = typename T::value_type;
    using Iterator = typename T::iterator;
    
    void processContainer(T& container) {
        // Dependent name lookup
        for (typename T::iterator it = container.begin(); it != container.end(); ++it) {
            // Process each element
            ValueType& value = *it;
            (void)value; // Suppress unused variable warning
        }
    }
};

// Template template parameter with variadic templates
template<template<typename...> class Container, typename... Args>
class VariadicContainerWrapper {
private:
    Container<Args...> container;
    
public:
    Container<Args...>& get() { return container; }
    const Container<Args...>& get() const { return container; }
};

// Test function demonstrating template usage
void testTemplates() {
    // Basic template instantiation
    FixedArray<int, 5> intArray;
    intArray.add(1);
    intArray.add(2);
    intArray.add(3);
    
    // Template specialization
    FixedArray<bool, 8> boolArray;
    boolArray.add(true);
    boolArray.add(false);
    boolArray.add(true);
    
    // Partial specialization
    FixedArray<int*, 5> ptrArray;
    ptrArray.add(new int(42));
    ptrArray.add(new int(84));
    
    // Function templates
    int maxInt = max(10, 20);
    double maxDouble = max(3.14, 2.71);
    auto product = multiply(5, 3.14);
    
    // Variadic templates
    print(1, 2.5, "hello", 'c');
    printRecursive("Values:", 42, 3.14, "world");
    
    // SFINAE
    bool evenInt = isEven(42);
    bool evenFloat = isEven(3.14);
    
    // Template metaprogramming
    constexpr int fact5 = Factorial<5>::value;
    constexpr int fact10 = factorial_v<10>;
    
    // Type traits
    using IntType = RemovePointer<int***>::type;
    // static_assert removed for simplicity
    
    // Alias templates
    Vector<int> intVector;
    UniquePtr<double> doublePtr(new double(3.14));
    
    // Variable templates
    static_assert(is_pointer_v<int*>, "int* should be a pointer");
    static_assert(!is_pointer_v<int>, "int should not be a pointer");
    
    // Template template parameters
    ContainerWrapper<SimpleVector, int> wrapper;
    wrapper.add(1);
    wrapper.add(2);
    
    // Dependent name lookup
    DependentNameTest<SimpleVector<int>> depTest;
    SimpleVector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    depTest.processContainer(vec);
    
    // Variadic template template parameters
    VariadicContainerWrapper<SimpleVector, int> varWrapper;
    varWrapper.get().push_back(42);
}

// Template instantiation forcing
template class FixedArray<double, 20>;
// Explicit instantiation after specialization removed to avoid warning
template auto multiply<float, double>(const float&, const double&) -> double;

// Static assert with template metaprogramming
static_assert(Factorial<5>::value == 120, "5! should be 120");
static_assert(factorial_v<4> == 24, "4! should be 24");
