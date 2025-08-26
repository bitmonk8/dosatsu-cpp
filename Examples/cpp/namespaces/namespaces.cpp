/**
 * @file example_namespaces.cpp
 * @brief Comprehensive example demonstrating C++ namespace features
 * 
 * This example showcases:
 * - Nested namespaces and namespace hierarchies
 * - Using declarations and using directives
 * - Namespace aliases and qualified names
 * - Argument-dependent lookup (ADL/Koenig lookup)
 * - Anonymous and inline namespaces
 * - Namespace scope resolution
 */
// Simple replacements for std library types
class SimpleString {
private:
    char* data;
    int length;
public:
    SimpleString(const char* str = "") {
        length = 0;
        while (str[length] != '\0') length++;
        data = new char[length + 1];
        for (int i = 0; i <= length; i++) {
            data[i] = str[i];
        }
    }
    SimpleString(const SimpleString& other) {
        length = other.length;
        data = new char[length + 1];
        for (int i = 0; i <= length; i++) {
            data[i] = other.data[i];
        }
    }
    ~SimpleString() { delete[] data; }
    const char* c_str() const { return data; }
};

template<typename T>
class SimpleVector {
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
    T* begin() { return data; }
    T* end() { return data + size_; }
    int size() const { return size_; }
    bool empty() const { return size_ == 0; }
    T& operator[](int index) { return data[index]; }
    const T& operator[](int index) const { return data[index]; }
};

// Math functions for std::sqrt replacement
double simple_sqrt(double x) {
    if (x < 0) return 0;
    double result = x;
    for (int i = 0; i < 10; i++) { // Simple Newton's method approximation
        result = (result + x / result) / 2.0;
    }
    return result;
}

// Global namespace declarations
int globalVar = 42;
void globalFunction() {}

// Basic namespace
namespace Mathematics {
    const double PI = 3.14159;
    
    double square(double x) {
        return x * x;
    }
    
    // Nested namespace (C++17 style)
    namespace Geometry::Advanced {
        class Point {
        private:
            double x, y;
        public:
            Point(double x_, double y_) : x(x_), y(y_) {}
            double distanceFromOrigin() const {
                return Mathematics::square(x) + Mathematics::square(y);
            }
        };
        
        // Function that uses outer namespace
        double calculateArea(double radius) {
            return Mathematics::PI * Mathematics::square(radius);
        }
    }
    
    // Another nested namespace (traditional style)
    namespace Statistics {
        template<typename T>
        T mean(const SimpleVector<T>& values) {
            T sum = T{};
            for (T* it = const_cast<SimpleVector<T>&>(values).begin(); it != const_cast<SimpleVector<T>&>(values).end(); ++it) {
                sum += *it;
            }
            return values.empty() ? T{} : sum / static_cast<T>(values.size());
        }
        
        double standardDeviation(const SimpleVector<double>& values) {
            double avg = mean(values);
            double sumSquares = 0.0;
            for (int i = 0; i < values.size(); i++) {
                double diff = const_cast<SimpleVector<double>&>(values)[i] - avg;
                sumSquares += Mathematics::square(diff);
            }
            return values.size() > 1 ? 
                simple_sqrt(sumSquares / (values.size() - 1)) : 0.0;
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

// Namespace alias
namespace Math = Mathematics;
namespace Geo = Mathematics::Geometry::Advanced;
namespace Stats = Mathematics::Statistics;

// Using declarations (replaced with simple types)
// using std::cout; - removed
// using std::endl; - removed  
// using std::string; - replaced with SimpleString
// using std::vector; - replaced with SimpleVector

// Using directive
using namespace Mathematics::Statistics;

// Namespace with same name as existing (reopening)
namespace Mathematics {
    // Adding more to existing namespace
    double cube(double x) {
        return x * x * x;
    }
    
    namespace Trigonometry {
        double sin(double x) { return x; } // Simplified approximation
        double cos(double x) { return x; } // Simplified approximation  
        double tan(double x) { return x; } // Simplified approximation
        
        // Function using multiple namespaces
        double calculateHypotenuse(double opposite, double adjacent) {
            return simple_sqrt(Mathematics::square(opposite) + 
                           Mathematics::square(adjacent));
        }
    }
}

// Inline namespace (C++11)
namespace Graphics {
    inline namespace v2 {
        class Renderer {
        public:
            void render() { /* Rendering v2 */ }
            int getVersion() const { return 2; }
        };
        
        void initialize() {
            /* Graphics v2 initialized */
        }
    }
    
    namespace v1 {
        class Renderer {
        public:
            void render() { /* Rendering v1 */ }
            int getVersion() const { return 1; }
        };
        
        void initialize() {
            /* Graphics v1 initialized */
        }
    }
}

// Template specialization in namespace
namespace Mathematics {
    template<typename T>
    T absoluteValue(T value) {
        return value < T{} ? -value : value;
    }
    
    // Specialization for unsigned types
    template<>
    unsigned int absoluteValue<unsigned int>(unsigned int value) {
        return value; // Already absolute
    }
}

// Class with using declarations
class UsingDeclarationDemo {
private:
    using PointType = Geo::Point;
    using StringVector = SimpleVector<SimpleString>;
    
public:
    // Type alias visible to class users
    using NumberType = double;
    
    void demonstrateUsing() {
        // Using namespace alias
        PointType point(3.0, 4.0);
        NumberType distance = point.distanceFromOrigin();
        
        // Using type alias
        StringVector names;
        names.push_back(SimpleString("Alice"));
        names.push_back(SimpleString("Bob"));
        names.push_back(SimpleString("Charlie"));
        
        // Using qualified names
        NumberType area = Geo::calculateArea(5.0);
        NumberType result = Math::square(distance);
        
        // Output removed for simplicity (no cout/endl)
        (void)distance; (void)area; (void)result; // Suppress unused warnings
    }
};

// ADL (Argument Dependent Lookup) demonstration
namespace CustomTypes {
    class MyClass {
    private:
        int value;
    public:
        MyClass(int v) : value(v) {}
        int getValue() const { return value; }
    };
    
    // Function in same namespace as MyClass
    void print(const MyClass& obj) {
        // Print removed for simplicity (no cout)
        (void)obj; // Suppress unused warning
    }
    
    // Operator overload for ADL
    bool operator==(const MyClass& a, const MyClass& b) {
        return a.getValue() == b.getValue();
    }
}

// Function template in global namespace
template<typename T>
void process(const T& obj) {
    // This will find CustomTypes::print via ADL
    print(obj);
}

// Namespace with extern template
namespace TemplateDemo {
    template<typename T>
    class Container {
    private:
        SimpleVector<T> data;
    public:
        void add(const T& item) { data.push_back(item); }
        int size() const { return data.size(); }
        const T& operator[](int index) const { return data[index]; }
    };
    
    // Explicit instantiation declaration
    extern template class Container<int>;
    extern template class Container<SimpleString>;
}

// Using enum (C++20 feature - might not be available)
namespace Colors {
    enum class RGB { Red, Green, Blue };
    
    void useColors() {
        // Traditional way
        RGB color1 = RGB::Red;
        
        // With using enum (C++20)
        // using enum RGB;
        // RGB color2 = Red; // Direct access
    }
}

// Example function demonstrating namespace usage
void demonstrateNamespaces() {
    // Direct namespace access
    double piValue = Mathematics::PI;
    double squared = Mathematics::square(5.0);
    double cubed = Mathematics::cube(3.0);
    
    // Namespace alias usage
    double area = Geo::calculateArea(10.0);
    Geo::Point origin(0.0, 0.0);
    
    // Using declarations in action (output removed for simplicity)
    (void)piValue; (void)squared; (void)cubed; (void)area;
    
    // Statistics namespace (via using directive)
    SimpleVector<double> values;
    values.push_back(1.0);
    values.push_back(2.0);
    values.push_back(3.0);
    values.push_back(4.0);
    values.push_back(5.0);
    double average = mean(values); // Found via using directive
    double stdDev = standardDeviation(values);
    
    // Output removed for simplicity
    (void)average; (void)stdDev;
    
    // Anonymous namespace usage
    incrementCounter();
    int counterValue = InternalHelper::getValue();
    (void)counterValue; // Output removed for simplicity
    
    // Inline namespace (Graphics::v2 is accessible as Graphics)
    Graphics::Renderer renderer; // Uses v2 due to inline namespace
    renderer.render();
    Graphics::initialize(); // Calls v2::initialize()
    
    // Explicit version access
    Graphics::v1::Renderer oldRenderer;
    oldRenderer.render();
    
    // Template in namespace
    int absValue = Mathematics::absoluteValue(-42);
    (void)absValue; // Output removed for simplicity
    
    // ADL demonstration
    CustomTypes::MyClass obj(123);
    process(obj); // Finds CustomTypes::print via ADL
    
    // Comparison via ADL
    CustomTypes::MyClass obj2(123);
    bool areEqual = (obj == obj2); // Finds CustomTypes::operator==
    (void)areEqual; // Output removed for simplicity
    
    // Using declarations demo
    UsingDeclarationDemo demo;
    demo.demonstrateUsing();
    
    // Template namespace usage
    TemplateDemo::Container<int> intContainer;
    intContainer.add(42);
    intContainer.add(84);
    (void)intContainer.size(); // Output removed for simplicity
}

// Global scope function that uses various namespaces
int main() {
    demonstrateNamespaces();
    return 0;
}
