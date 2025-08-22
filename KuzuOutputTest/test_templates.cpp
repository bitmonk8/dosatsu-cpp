// Test file for template analysis including specializations, instantiations, and metaprogramming
#include <type_traits>
#include <vector>
#include <memory>

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

// Variadic templates (C++11)
template<typename... Args>
void print(Args... args) {
    // C++17 fold expression
    ((std::cout << args << " "), ...);
}

// Variadic template with recursion (pre-C++17 style)
template<typename T>
void printRecursive(T&& t) {
    std::cout << t << std::endl;
}

template<typename T, typename... Args>
void printRecursive(T&& t, Args&&... args) {
    std::cout << t << " ";
    printRecursive(args...);
}

// SFINAE (Substitution Failure Is Not An Error)
template<typename T>
typename std::enable_if<std::is_integral<T>::value, bool>::type
isEven(T value) {
    return value % 2 == 0;
}

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, bool>::type
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
using Vector = std::vector<T>;

template<typename T>
using UniquePtr = std::unique_ptr<T>;

// Variable templates (C++14)
template<typename T>
constexpr bool is_pointer_v = std::is_pointer<T>::value;

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
    
    size_t size() const {
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
    static_assert(std::is_same<IntType, int>::value, "Should be int");
    
    // Alias templates
    Vector<int> intVector;
    UniquePtr<double> doublePtr = std::make_unique<double>(3.14);
    
    // Variable templates
    static_assert(is_pointer_v<int*>, "int* should be a pointer");
    static_assert(!is_pointer_v<int>, "int should not be a pointer");
    
    // Template template parameters
    ContainerWrapper<std::vector, int> wrapper;
    wrapper.add(1);
    wrapper.add(2);
    
    // Dependent name lookup
    DependentNameTest<std::vector<int>> depTest;
    std::vector<int> vec = {1, 2, 3, 4, 5};
    depTest.processContainer(vec);
    
    // Variadic template template parameters
    VariadicContainerWrapper<std::vector, int> varWrapper;
    varWrapper.get().push_back(42);
}

// Template instantiation forcing
template class FixedArray<double, 20>;
template int max<int>(const int&, const int&);
template auto multiply<float, double>(const float&, const double&) -> double;

// Static assert with template metaprogramming
static_assert(Factorial<5>::value == 120, "5! should be 120");
static_assert(factorial_v<4> == 24, "4! should be 24");
