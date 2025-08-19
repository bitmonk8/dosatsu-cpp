// Comprehensive test file for Phase 4 - Complex C++ constructs
// Testing: inheritance, templates, namespaces, and advanced features

// Required for size_t and other standard types
typedef unsigned long long size_t;

// Namespace testing
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

// Template class template specialization
template<typename T>
class Container {
private:
    T* data;
    size_t size;
public:
    Container(size_t s = 0) : size(s) {
        data = new T[size];
    }
    
    ~Container() { delete[] data; }
    
    // Copy constructor
    Container(const Container& other) : size(other.size) {
        data = new T[size];
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
            data = new T[size];
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

// Test function with complex interactions
int main() {
    // Basic inheritance and polymorphism testing
    GlobalBase* base = new GlobalBase(10);
    int val = base->getValue();
    delete base;
    
    // Template and namespace testing
    IntRect rect(5, 10);
    auto area = rect.calculateArea();
    auto perimeter = rect.calculatePerimeter();
    
    // Template specialization testing
    auto result1 = multiply(3, 4);        // Uses template
    auto result2 = multiply<int, int>(3, 4); // Uses specialization
    
    // Variadic template testing
    auto total = sum(1, 2, 3, 4, 5);
    
    // Complex template interactions
    ComplexContainer<double> container;
    Container<IntRect> rectContainer(3);
    
    // Multiple inheritance testing
    Geo::PrintableRectangle<float> printableRect(3.5f, 7.2f);
    printableRect.draw();
    printableRect.print();
    
    // Function template with auto return type
    auto complex_result = complexFunction(1.5, 2.5, 3);
    
    return static_cast<int>(area + perimeter + result1 + result2 + total + complex_result);
}
