// Clean comprehensive C++ test file without standard library dependencies
// Contains all constructs expected by the test suite

// ============================================================================
// INHERITANCE TEST CLASSES
// ============================================================================

// Base class with virtual functions
class Animal {
private:
    int id;
protected:
    int age;
public:
    Animal(int i, int a) : id(i), age(a) {}
    virtual ~Animal() = default;
    
    // Pure virtual function
    virtual void makeSound() const = 0;
    
    // Virtual function with implementation
    virtual void move() const {
        // Base implementation
    }
    
    // Non-virtual function
    int getId() const { return id; }
    
protected:
    virtual void grow() { age++; }
};

// Single inheritance
class Mammal : public Animal {
protected:
    bool hasFur;
public:
    Mammal(int i, int a, bool fur) : Animal(i, a), hasFur(fur) {}
    
    void makeSound() const override {
        // Mammal sound
    }
    
    void move() const override {
        // Mammal movement
    }
    
    virtual void breathe() const {
        // Mammal breathing
    }
};

// Multiple inheritance interfaces
class Flyable {
public:
    virtual ~Flyable() = default;
    virtual void fly() const = 0;
    virtual double getMaxAltitude() const { return 1000.0; }
};

class Swimmer {
public:
    virtual ~Swimmer() = default;
    virtual void swim() const = 0;
    virtual double getMaxDepth() const { return 100.0; }
};

// Multiple inheritance class
class Bat : public Mammal, public Flyable {
public:
    Bat(int i, int a) : Mammal(i, a, true) {}
    
    void makeSound() const override {
        // Bat echolocation
    }
    
    void fly() const override {
        // Bat flight
    }
    
    double getMaxAltitude() const override { return 3000.0; }
    
    void breathe() const final override {
        // Bat breathing
    }
};

// Virtual inheritance
class WaterBird : public virtual Animal, public Flyable, public Swimmer {
public:
    WaterBird(int i, int a) : Animal(i, a) {}
    
    void makeSound() const override {
        // Water bird sound
    }
    
    void fly() const override {
        // Water bird flight
    }
    
    void swim() const override {
        // Water bird swimming
    }
};

// Protected inheritance
class Duck : protected WaterBird {
public:
    Duck(int i, int a) : Animal(i, a), WaterBird(i, a) {}
    
    void fly() const override {
        // Duck flight
    }
    
    void swim() const override {
        // Duck swimming
    }
    
    using WaterBird::makeSound;
};

// Protected inheritance
class Penguin : protected WaterBird {
public:
    Penguin(int i, int a) : Animal(i, a), WaterBird(i, a) {}
    
    void fly() const override {
        // Penguins cannot fly
    }
    
    void swim() const override {
        // Penguin swimming
    }
    
    using WaterBird::makeSound;
};

// Abstract shape classes
class Shape {
public:
    virtual ~Shape() = 0;
    virtual double area() const = 0;
    virtual double perimeter() const = 0;
};

Shape::~Shape() = default;

// Template inheritance
template<typename T>
class Rectangle : public Shape {
private:
    T width, height;
public:
    Rectangle(T w, T h) : width(w), height(h) {}
    
    double area() const override {
        return static_cast<double>(width * height);
    }
    
    double perimeter() const override {
        return 2.0 * static_cast<double>(width + height);
    }
    
    template<typename U>
    Rectangle<U> convert() const {
        return Rectangle<U>(static_cast<U>(width), static_cast<U>(height));
    }
};

// ============================================================================
// TEMPLATE TEST CLASSES
// ============================================================================

// Basic class template with default parameter
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
    
    template<typename U>
    bool contains(const U& value) const {
        for (int i = 0; i < count; ++i) {
            if (data[i] == value) return true;
        }
        return false;
    }
    
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
    
    template<typename U>
    static FixedArray<U, Size> create() {
        return FixedArray<U, Size>();
    }
};

// Function templates
template<typename T>
T max(const T& a, const T& b) {
    return (a > b) ? a : b;
}

template<typename T, typename U>
auto multiply(const T& a, const U& b) -> decltype(a * b) {
    return a * b;
}

// Function template specialization
template<>
int max<int>(const int& a, const int& b) {
    return (a > b) ? a : b;
}

// Variadic templates
template<typename... Args>
void print(Args... args) {
    // Variadic template implementation
}

template<typename T>
void printRecursive(T&& t) {
    // Base case
}

template<typename T, typename... Args>
void printRecursive(T&& t, Args&&... args) {
    // Recursive case
    printRecursive(args...);
}

// Template metaprogramming
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

// Type traits
template<typename T>
struct RemovePointer {
    using type = T;
};

template<typename T>
struct RemovePointer<T*> {
    using type = T;
};

// Alias templates
template<typename T>
using Ptr = T*;

// Variable templates
template<typename T>
constexpr bool is_pointer_v = false;

template<typename T>
constexpr bool is_pointer_v<T*> = true;

template<int N>
constexpr int factorial_v = Factorial<N>::value;

// Template template parameters
template<template<typename> class Container, typename T>
class ContainerWrapper {
private:
    Container<T> container;
    
public:
    void add(const T& item) {
        // Implementation
    }
    
    int size() const {
        return 0; // Simplified
    }
};

// Generic container for template template parameter testing
template<typename T>
class Container {
private:
    T* data;
    int capacity;
    int count;
    
public:
    Container() : data(nullptr), capacity(0), count(0) {}
    
    void push_back(const T& item) {
        // Simplified implementation
        if (count < capacity) {
            data[count++] = item;
        }
    }
    
    int size() const { return count; }
    
    T* begin() { return data; }
    T* end() { return data + count; }
    const T* begin() const { return data; }
    const T* end() const { return data + count; }
    
    using value_type = T;
    using iterator = T*;
};

// ============================================================================
// NAMESPACE TEST CONSTRUCTS
// ============================================================================

// Global declarations
int globalVar = 42;
void globalFunction() {}

// Basic namespace
namespace Mathematics {
    const double PI_VALUE = 3.14159;
    
    double square(double x) {
        return x * x;
    }
    
    double cube(double x) {
        return x * x * x;
    }
    
    // Nested namespace
    namespace Geometry {
        namespace Advanced {
            class Point {
            private:
                double x, y;
            public:
                Point(double x_, double y_) : x(x_), y(y_) {}
                double distanceFromOrigin() const {
                    return Mathematics::square(x) + Mathematics::square(y);
                }
            };
            
            double calculateArea(double radius) {
                return Mathematics::PI_VALUE * Mathematics::square(radius);
            }
        }
    }
    
    namespace Statistics {
        template<typename T>
        T mean(const T* values, int count) {
            T sum = T{};
            for (int i = 0; i < count; ++i) {
                sum += values[i];
            }
            return count > 0 ? sum / static_cast<T>(count) : T{};
        }
        
        double standardDeviation(const double* values, int count) {
            double avg = mean(values, count);
            double sumSquares = 0.0;
            for (int i = 0; i < count; ++i) {
                double diff = values[i] - avg;
                sumSquares += Mathematics::square(diff);
            }
            return count > 1 ? sumSquares / (count - 1) : 0.0;
        }
    }
}

// Anonymous namespace
namespace {
    int internalCounter = 0;
    
    void incrementCounter() {
        ++internalCounter;
    }
    
    class InternalHelper {
    public:
        static int getValue() { return internalCounter; }
    };
}

// Namespace aliases
namespace Math = Mathematics;
namespace Geo = Mathematics::Geometry::Advanced;
namespace Stats = Mathematics::Statistics;

// Inline namespace
namespace Graphics {
    inline namespace v2 {
        class Renderer {
        public:
            void render() { /* v2 rendering */ }
            int getVersion() const { return 2; }
        };
        
        void initialize() {
            /* v2 initialization */
        }
    }
    
    namespace v1 {
        class Renderer {
        public:
            void render() { /* v1 rendering */ }
            int getVersion() const { return 1; }
        };
        
        void initialize() {
            /* v1 initialization */
        }
    }
}

// ADL demonstration namespace
namespace CustomTypes {
    class MyClass {
    private:
        int value;
    public:
        MyClass(int v) : value(v) {}
        int getValue() const { return value; }
    };
    
    void print(const MyClass& obj) {
        // Print implementation
    }
    
    bool operator==(const MyClass& a, const MyClass& b) {
        return a.getValue() == b.getValue();
    }
}

namespace TemplateDemo {
    template<typename T>
    class Container {
    private:
        T* data;
        int count;
    public:
        Container() : data(nullptr), count(0) {}
        void add(const T& item) { /* implementation */ }
        int size() const { return count; }
        const T& operator[](int index) const { return data[index]; }
    };
    
    extern template class Container<int>;
}

namespace Colors {
    enum class RGB { Red, Green, Blue };
    
    void useColors() {
        RGB color1 = RGB::Red;
    }
}

// ============================================================================
// EXPRESSION AND CONTROL FLOW TEST CLASSES
// ============================================================================

class ExpressionExampleClass {
private:
    int value;
    double* array;
    int size;
    
public:
    ExpressionExampleClass(int v) : value(v), array(nullptr), size(0) {}
    
    ~ExpressionExampleClass() {
        delete[] array;
    }
    
    // Arithmetic operators
    ExpressionExampleClass operator+(const ExpressionExampleClass& other) const {
        return ExpressionExampleClass(value + other.value);
    }
    
    ExpressionExampleClass& operator+=(const ExpressionExampleClass& other) {
        value += other.value;
        return *this;
    }
    
    ExpressionExampleClass& operator++() { // Prefix
        ++value;
        return *this;
    }
    
    ExpressionExampleClass operator++(int) { // Postfix
        ExpressionExampleClass temp(*this);
        ++value;
        return temp;
    }
    
    // Comparison operators
    bool operator==(const ExpressionExampleClass& other) const {
        return value == other.value;
    }
    
    bool operator!=(const ExpressionExampleClass& other) const {
        return value != other.value;
    }
    
    bool operator<(const ExpressionExampleClass& other) const {
        return value < other.value;
    }
    
    bool operator>(const ExpressionExampleClass& other) const {
        return value > other.value;
    }
    
    // Array access
    double& operator[](int index) {
        return array[index];
    }
    
    const double& operator[](int index) const {
        return array[index];
    }
    
    // Member access
    int getValue() const { return value; }
    void setValue(int v) { value = v; }
    
    // Pointer operations
    ExpressionExampleClass* getThis() { return this; }
    
    // Conditional expressions and control flow
    void testControlFlow() {
        // If statements
        if (value > 0) {
            value *= 2;
        } else if (value < 0) {
            value = -value;
        } else {
            value = 1;
        }
        
        // Switch statement
        switch (value % 3) {
            case 0:
                value += 10;
                break;
            case 1:
                value += 20;
                break;
            default:
                value += 30;
                break;
        }
        
        // For loops
        for (int i = 0; i < 10; ++i) {
            value += i;
        }
        
        for (int j = 0; j < size; ++j) {
            array[j] = static_cast<double>(j);
        }
        
        // While loop
        int counter = 0;
        while (counter < 5) {
            value += counter;
            ++counter;
        }
        
        // Do-while loop
        do {
            value--;
        } while (value > 100);
        
        // Range-based for (C++11)
        int numbers[] = {1, 2, 3, 4, 5};
        for (int num : numbers) {
            value += num;
        }
    }
    
    // Exception handling
    void testExceptions() {
        try {
            if (value < 0) {
                throw value;
            }
        } catch (int error) {
            value = 0;
        } catch (...) {
            value = -1;
        }
    }
    
    // Ternary operator
    int getAbsoluteValue() const {
        return value >= 0 ? value : -value;
    }
    
    // Logical operators
    bool isValid() const {
        return (value > 0) && (array != nullptr) && (size > 0);
    }
    
    bool shouldProcess() const {
        return (value != 0) || (size == 0);
    }
    
    // Bitwise operations
    int getBitwiseOr(int mask) const {
        return value | mask;
    }
    
    int getBitwiseAnd(int mask) const {
        return value & mask;
    }
    
    int getBitwiseXor(int mask) const {
        return value ^ mask;
    }
    
    // Cast expressions
    double getAsDouble() const {
        return static_cast<double>(value);
    }
    
    void* getAsVoidPtr() {
        return reinterpret_cast<void*>(this);
    }
    
    const ExpressionExampleClass* getAsConstPtr() const {
        return const_cast<const ExpressionExampleClass*>(this);
    }
};

// ============================================================================
// PREPROCESSOR TEST CONSTRUCTS (without conflicts)
// ============================================================================

class MacroExampleClass {
private:
    double radius;
    
public:
    MacroExampleClass(double r) : radius(r) {}
    
    double getArea() const {
        return 3.14159 * radius * radius;
    }
    
    double getCircumference() const {
        return 2 * 3.14159 * radius;
    }
    
    void testMacros() {
        int a = 10, b = 20;
        int maxVal = (a > b) ? a : b;
        int minVal = (a < b) ? a : b;
        
        // Swap without macro
        int temp = a;
        a = b;
        b = temp;
    }
};

// ============================================================================
// TEST FUNCTIONS
// ============================================================================

void testInheritance() {
    Bat bat(1, 2);
    bat.makeSound();
    bat.fly();
    
    Duck duck(2, 3);
    duck.fly();
    duck.swim();
    
    Rectangle<int> intRect(5, 3);
    Rectangle<double> doubleRect = intRect.convert<double>();
    
    double area1 = intRect.area();
    double area2 = doubleRect.area();
    
    Penguin penguin(3, 1);
    penguin.swim();
    penguin.makeSound();
}

void testTemplates() {
    FixedArray<int, 5> intArray;
    intArray.add(1);
    intArray.add(2);
    intArray.add(3);
    
    int maxInt = max(10, 20);
    double maxDouble = max(3.14, 2.71);
    auto product = multiply(5, 3.14);
    
    print(1, 2.5, 'c');
    printRecursive(42, 3.14);
    
    constexpr int fact5 = Factorial<5>::value;
    constexpr int fact10 = factorial_v<10>;
    
    using IntType = RemovePointer<int***>::type;
    
    ContainerWrapper<Container, int> wrapper;
    wrapper.add(1);
    wrapper.add(2);
}

void testNamespaces() {
    double piValue = Mathematics::PI_VALUE;
    double squared = Mathematics::square(5.0);
    double cubed = Mathematics::cube(3.0);
    
    double area = Geo::calculateArea(10.0);
    Geo::Point origin(0.0, 0.0);
    
    double values[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double average = Stats::mean(values, 5);
    double stdDev = Stats::standardDeviation(values, 5);
    
    incrementCounter();
    int counterValue = InternalHelper::getValue();
    
    Graphics::Renderer renderer;
    renderer.render();
    Graphics::initialize();
    
    Graphics::v1::Renderer oldRenderer;
    oldRenderer.render();
    
    int absValue = Mathematics::square(42);
    
    CustomTypes::MyClass obj(123);
    CustomTypes::MyClass obj2(123);
    bool areEqual = (obj == obj2);
    
    TemplateDemo::Container<int> intContainer;
    intContainer.add(42);
    intContainer.add(84);
}

void testExpressions() {
    ExpressionExampleClass obj1(10);
    ExpressionExampleClass obj2(20);
    
    ExpressionExampleClass result = obj1 + obj2;
    obj1 += obj2;
    
    ++obj1;
    obj2++;
    
    bool isEqual = (obj1 == obj2);
    bool isNotEqual = (obj1 != obj2);
    bool isLess = (obj1 < obj2);
    bool isGreater = (obj1 > obj2);
    
    int value = obj1.getValue();
    obj1.setValue(42);
    
    ExpressionExampleClass* ptr = obj1.getThis();
    
    obj1.testControlFlow();
    obj1.testExceptions();
    
    int absVal = obj1.getAbsoluteValue();
    bool valid = obj1.isValid();
    bool shouldProc = obj1.shouldProcess();
    
    int orResult = obj1.getBitwiseOr(0xFF);
    int andResult = obj1.getBitwiseAnd(0xFF);
    int xorResult = obj1.getBitwiseXor(0xFF);
    
    double doubleVal = obj1.getAsDouble();
    void* voidPtr = obj1.getAsVoidPtr();
    const ExpressionExampleClass* constPtr = obj1.getAsConstPtr();
}

void testMacros() {
    MacroExampleClass obj(5.0);
    double area = obj.getArea();
    double circumference = obj.getCircumference();
    obj.testMacros();
}

// Main function to tie everything together
int main() {
    testInheritance();
    testTemplates();
    testNamespaces();
    testExpressions();
    testMacros();
    return 0;
}
