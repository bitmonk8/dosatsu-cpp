/**
 * @file simple_example.cpp
 * @brief Comprehensive example demonstrating complex C++ constructs
 * 
 * This is a comprehensive example that combines multiple C++ features:
 * - Inheritance hierarchies and polymorphism
 * - Template classes and functions with specializations
 * - Namespace organization and scope management
 * - Advanced template metaprogramming
 * - Complex type relationships and conversions
 * 
 * This file serves as a complete example for testing C++ analysis tools
 * and demonstrates real-world usage patterns of advanced C++ features.
 */

// Required for size_t and other standard types
typedef unsigned long long size_t;

// Namespace examples
namespace Mathematics {
    namespace Geometry {
        // Template class with inheritance
        template<typename T>
        class Shape {
        private:
            T area_value;
        protected:
            T perimeter_value;
        public:
            Shape(T area = T{}, T perimeter = T{}) 
                : area_value(area), perimeter_value(perimeter) {}
            
            virtual ~Shape() = default;
            virtual T calculateArea() const = 0;  // Pure virtual function
            virtual T calculatePerimeter() const { return perimeter_value; }
            
            // Template member function
            template<typename U>
            U convert(const U& factor) const {
                return static_cast<U>(area_value * factor);
            }
        };
        
        // Inheritance: Derived class from template base
        template<typename T>
        class Rectangle : public Shape<T> {
        private:
            T width, height;
        public:
            Rectangle() : Shape<T>(), width(T{}), height(T{}) {}
            Rectangle(T w, T h) : Shape<T>(w * h, 2 * (w + h)), width(w), height(h) {}
            
            // Override virtual function
            T calculateArea() const override {
                return width * height;
            }
            
            // Function template specialization
            template<typename U = T>
            U getWidth() const { return static_cast<U>(width); }
        };
        
        // Multiple inheritance example
        class Drawable {
        public:
            virtual void draw() const = 0;
            virtual ~Drawable() = default;
        };
        
        class Printable {
        public:
            virtual void print() const = 0;
            virtual ~Printable() = default;
        };
        
        // Class with multiple inheritance
        template<typename T>
        class PrintableRectangle : public Rectangle<T>, public Drawable, public Printable {
        public:
            PrintableRectangle() : Rectangle<T>() {}
            PrintableRectangle(T w, T h) : Rectangle<T>(w, h) {}
            
            void draw() const override {
                // Drawing implementation
            }
            
            void print() const override {
                // Printing implementation  
            }
        };
    }
    
    // Function templates
    template<typename T, typename U>
    auto multiply(const T& a, const U& b) -> decltype(a * b) {
        return a * b;
    }
    
    // Template specialization
    template<>
    auto multiply<int, int>(const int& a, const int& b) -> int {
        return a * b * 2;  // Special behavior for int,int
    }
    
    // Variadic template
    template<typename... Args>
    auto sum(Args... args) -> decltype((args + ...)) {
        return (args + ...);  // C++17 fold expression
    }
    
    // Additional namespaces expected by tests
    namespace Statistics {
        template<typename T>
        T mean(const T* values, int count) {
            T sum = T{};
            for (int i = 0; i < count; ++i) {
                sum += values[i];
            }
            return count > 0 ? sum / static_cast<T>(count) : T{};
        }
    }
}

// Global namespace scope testing
class GlobalBase {
protected:
    int base_value;
public:
    GlobalBase(int val = 0) : base_value(val) {}
    virtual int getValue() const { return base_value; }
    virtual ~GlobalBase() = default;
};

// Additional classes expected by tests
class Animal {
protected:
    int age;
public:
    Animal(int a) : age(a) {}
    virtual ~Animal() = default;
    virtual void makeSound() const = 0;
    virtual void move() const {}
};

class Mammal : public Animal {
public:
    Mammal(int a) : Animal(a) {}
    void makeSound() const override {}
    virtual void breathe() const {}
};

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

class Bat : public Mammal, public Flyable {
public:
    Bat(int a) : Mammal(a) {}
    void makeSound() const override {}
    void fly() const override {}
    double getMaxAltitude() const override { return 3000.0; }
};

class WaterBird : public virtual Animal, public Flyable, public Swimmer {
public:
    WaterBird(int a) : Animal(a) {}
    void makeSound() const override {}
    void fly() const override {}
    void swim() const override {}
};

class Duck : protected WaterBird {
public:
    Duck(int a) : Animal(a), WaterBird(a) {}
    void fly() const override {}
    void swim() const override {}
    using WaterBird::makeSound;
};

class Penguin : protected WaterBird {
public:
    Penguin(int a) : Animal(a), WaterBird(a) {}
    void fly() const override {}
    void swim() const override {}
    using WaterBird::makeSound;
};

class ExpressionExampleClass {
private:
    int value;
public:
    ExpressionExampleClass(int v) : value(v) {}
    ExpressionExampleClass operator+(const ExpressionExampleClass& other) const {
        return ExpressionExampleClass(value + other.value);
    }
    bool operator==(const ExpressionExampleClass& other) const {
        return value == other.value;
    }
    int getValue() const { return value; }
};

class MacroExampleClass {
private:
    double radius;
public:
    MacroExampleClass(double r) : radius(r) {}
    double getArea() const { return 3.14159 * radius * radius; }
};

// Template class expected by tests
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
};

// Function templates expected by tests
template<typename T>
T max(const T& a, const T& b) {
    return (a > b) ? a : b;
}

template<typename T, typename U>
auto multiplyGeneric(const T& a, const U& b) -> decltype(a * b) {
    return a * b;
}

// Additional namespaces
namespace Graphics {
    inline namespace v2 {
        class Renderer {
        public:
            void render() {}
            int getVersion() const { return 2; }
        };
    }
    
    namespace v1 {
        class Renderer {
        public:
            void render() {}
            int getVersion() const { return 1; }
        };
    }
}

namespace CustomTypes {
    class MyClass {
    private:
        int value;
    public:
        MyClass(int v) : value(v) {}
        int getValue() const { return value; }
    };
    
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
        void add(const T& item) {}
        int size() const { return count; }
    };
    
    extern template class Container<int>;
}

namespace Colors {
    enum class RGB { Red, Green, Blue };
}

// Template class template specialization
template<typename T>
class Container {
private:
    T* data;
    size_t size;
public:
    Container(size_t s = 0) : size(s) {
        data = (size > 0) ? new T[size]() : nullptr;
    }
    
    ~Container() { delete[] data; }
    
    // Copy constructor
    Container(const Container& other) : size(other.size) {
        data = (size > 0) ? new T[size]() : nullptr;
        for (size_t i = 0; i < size; ++i) {
            data[i] = other.data[i];
        }
    }
    
    // Move constructor
    Container(Container&& other) noexcept : data(other.data), size(other.size) {
        other.data = nullptr;
        other.size = 0;
    }
    
    // Assignment operators
    Container& operator=(const Container& other) {
        if (this != &other) {
            delete[] data;
            size = other.size;
            data = (size > 0) ? new T[size]() : nullptr;
            for (size_t i = 0; i < size; ++i) {
                data[i] = other.data[i];
            }
        }
        return *this;
    }
    
    T& operator[](size_t index) { return data[index]; }
    const T& operator[](size_t index) const { return data[index]; }
};

// Explicit template instantiation
template class Container<int>;
template class Container<double>;

// Class template with nested types
template<typename T>
class ComplexContainer {
public:
    // Nested type definitions
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    
    // Nested class
    class Iterator {
    private:
        pointer current;
    public:
        Iterator(pointer ptr) : current(ptr) {}
        reference operator*() { return *current; }
        Iterator& operator++() { ++current; return *this; }
        bool operator!=(const Iterator& other) const { return current != other.current; }
    };
    
private:
    T* data;
    size_t capacity;
    
public:
    ComplexContainer() : data(nullptr), capacity(0) {}
    Iterator begin() { return Iterator(data); }
    Iterator end() { return Iterator(data + capacity); }
};

// Function with multiple template parameters and constraints
template<typename T, typename U, typename V = int>
auto complexFunction(T&& t, U&& u, V v = V{}) -> decltype(t + u + v) {
    static_assert(sizeof(T) > 0, "T must have non-zero size");
    return static_cast<decltype(t + u + v)>(t + u + v);
}

// Namespace alias
namespace Math = Mathematics;
namespace Geo = Mathematics::Geometry;

// Using declarations
using namespace Mathematics;
using IntRect = Geo::Rectangle<int>;
using DoubleRect = Geo::Rectangle<double>;

// Example function demonstrating complex interactions
int main() {
    // Basic inheritance and polymorphism examples
    GlobalBase* base = new GlobalBase(10);
    int val = base->getValue();
    delete base;
    
    // Template and namespace examples
    IntRect rect(5, 10);
    auto area = rect.calculateArea();
    auto perimeter = rect.calculatePerimeter();
    
    // Template specialization examples
    auto result1 = multiply(3, 4);        // Uses template
    auto result2 = multiply<int, int>(3, 4); // Uses specialization
    
    // Variadic template examples
    auto total = sum(1, 2, 3, 4, 5);
    
    // Complex template interactions
    ComplexContainer<double> container;
    Container<IntRect> rectContainer(3);
    
    // Multiple inheritance examples
    Geo::PrintableRectangle<float> printableRect(3.5f, 7.2f);
    printableRect.draw();
    printableRect.print();
    
    // Function template with auto return type
    auto complex_result = complexFunction(1.5, 2.5, 3);
    
    return static_cast<int>(area + perimeter + result1 + result2 + total + complex_result);
}
