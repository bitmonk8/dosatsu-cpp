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
#include <iostream>
#include <string>
#include <vector>

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
        T mean(const std::vector<T>& values) {
            T sum = T{};
            for (const auto& value : values) {
                sum += value;
            }
            return values.empty() ? T{} : sum / static_cast<T>(values.size());
        }
        
        double standardDeviation(const std::vector<double>& values) {
            double avg = mean(values);
            double sumSquares = 0.0;
            for (double val : values) {
                double diff = val - avg;
                sumSquares += Mathematics::square(diff);
            }
            return values.size() > 1 ? 
                std::sqrt(sumSquares / (values.size() - 1)) : 0.0;
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

// Using declarations
using std::cout;
using std::endl;
using std::string;
using std::vector;

// Using directive
using namespace Mathematics::Statistics;

// Namespace with same name as existing (reopening)
namespace Mathematics {
    // Adding more to existing namespace
    double cube(double x) {
        return x * x * x;
    }
    
    namespace Trigonometry {
        double sin(double x) { return std::sin(x); }
        double cos(double x) { return std::cos(x); }
        double tan(double x) { return std::tan(x); }
        
        // Function using multiple namespaces
        double calculateHypotenuse(double opposite, double adjacent) {
            return std::sqrt(Mathematics::square(opposite) + 
                           Mathematics::square(adjacent));
        }
    }
}

// Inline namespace (C++11)
namespace Graphics {
    inline namespace v2 {
        class Renderer {
        public:
            void render() { cout << "Rendering v2" << endl; }
            int getVersion() const { return 2; }
        };
        
        void initialize() {
            cout << "Graphics v2 initialized" << endl;
        }
    }
    
    namespace v1 {
        class Renderer {
        public:
            void render() { cout << "Rendering v1" << endl; }
            int getVersion() const { return 1; }
        };
        
        void initialize() {
            cout << "Graphics v1 initialized" << endl;
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
    using StringVector = vector<string>;
    
public:
    // Type alias visible to class users
    using NumberType = double;
    
    void demonstrateUsing() {
        // Using namespace alias
        PointType point(3.0, 4.0);
        NumberType distance = point.distanceFromOrigin();
        
        // Using type alias
        StringVector names = {"Alice", "Bob", "Charlie"};
        
        // Using qualified names
        NumberType area = Geo::calculateArea(5.0);
        NumberType result = Math::square(distance);
        
        cout << "Distance: " << distance << endl;
        cout << "Area: " << area << endl;
        cout << "Result: " << result << endl;
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
        cout << "MyClass value: " << obj.getValue() << endl;
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
        vector<T> data;
    public:
        void add(const T& item) { data.push_back(item); }
        size_t size() const { return data.size(); }
        const T& operator[](size_t index) const { return data[index]; }
    };
    
    // Explicit instantiation declaration
    extern template class Container<int>;
    extern template class Container<string>;
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
    
    // Using declarations in action
    cout << "Pi: " << piValue << endl;
    cout << "Squared: " << squared << endl;
    cout << "Cubed: " << cubed << endl;
    cout << "Area: " << area << endl;
    
    // Statistics namespace (via using directive)
    vector<double> values = {1.0, 2.0, 3.0, 4.0, 5.0};
    double average = mean(values); // Found via using directive
    double stdDev = standardDeviation(values);
    
    cout << "Mean: " << average << endl;
    cout << "Std Dev: " << stdDev << endl;
    
    // Anonymous namespace usage
    incrementCounter();
    int counterValue = InternalHelper::getValue();
    cout << "Counter: " << counterValue << endl;
    
    // Inline namespace (Graphics::v2 is accessible as Graphics)
    Graphics::Renderer renderer; // Uses v2 due to inline namespace
    renderer.render();
    Graphics::initialize(); // Calls v2::initialize()
    
    // Explicit version access
    Graphics::v1::Renderer oldRenderer;
    oldRenderer.render();
    
    // Template in namespace
    int absValue = Mathematics::absoluteValue(-42);
    cout << "Absolute value: " << absValue << endl;
    
    // ADL demonstration
    CustomTypes::MyClass obj(123);
    process(obj); // Finds CustomTypes::print via ADL
    
    // Comparison via ADL
    CustomTypes::MyClass obj2(123);
    bool areEqual = (obj == obj2); // Finds CustomTypes::operator==
    cout << "Objects equal: " << areEqual << endl;
    
    // Using declarations demo
    UsingDeclarationDemo demo;
    demo.demonstrateUsing();
    
    // Template namespace usage
    TemplateDemo::Container<int> intContainer;
    intContainer.add(42);
    intContainer.add(84);
    cout << "Container size: " << intContainer.size() << endl;
}

// Global scope function that uses various namespaces
int main() {
    testNamespaces();
    return 0;
}
