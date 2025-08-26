/**
 * @file std_library_performance_test.cpp
 * @brief Performance test using standard library to compare against library-free examples
 * 
 * This test uses std::vector to provide a performance comparison baseline against
 * the library-free examples in the Examples directory. This allows measuring
 * the indexing performance impact of standard library headers.
 * 
 * Features demonstrated:
 * - Basic C++ constructs with standard library usage
 * - std::vector for performance comparison
 * - Minimal template usage
 * - Basic inheritance
 */

#include <vector>

// Class with std::vector usage for performance testing
class PerformanceTestContainer {
private:
    std::vector<int> data;
    
public:
    PerformanceTestContainer() {
        data.reserve(100); // Pre-allocate some space
    }
    
    void addValue(int value) {
        data.push_back(value);
    }
    
    int getValue(int index) const {
        return (index < static_cast<int>(data.size())) ? data[index] : 0;
    }
    
    int size() const {
        return static_cast<int>(data.size());
    }
    
    void clear() {
        data.clear();
    }
};

// Simple base class
class Shape {
public:
    virtual ~Shape() = default;
    virtual double calculateArea() const = 0;
};

// Simple derived class
class Rectangle : public Shape {
private:
    double width, height;
    
public:
    Rectangle(double w, double h) : width(w), height(h) {}
    
    double calculateArea() const override {
        return width * height;
    }
    
    double getWidth() const { return width; }
    double getHeight() const { return height; }
};

// Simple template function
template<typename T>
T simpleMax(const T& a, const T& b) {
    return (a > b) ? a : b;
}

// Simple namespace
namespace TestNamespace {
    const double PI = 3.14159;
    
    double circleArea(double radius) {
        return PI * radius * radius;
    }
}

// Test function that uses all features
void performanceTest() {
    // Test container with vector
    PerformanceTestContainer container;
    
    // Add some values
    for (int i = 0; i < 50; ++i) {
        container.addValue(i * 2);
    }
    
    // Access some values
    int sum = 0;
    for (int i = 0; i < container.size(); ++i) {
        sum += container.getValue(i);
    }
    
    // Test inheritance
    Rectangle rect(5.0, 3.0);
    double area = rect.calculateArea();
    
    // Test template
    int maxInt = simpleMax(10, 20);
    double maxDouble = simpleMax(3.14, 2.71);
    
    // Test namespace
    double circleArea = TestNamespace::circleArea(5.0);
    
    // Use all computed values to prevent optimization
    volatile int result = sum + static_cast<int>(area) + maxInt + 
                         static_cast<int>(maxDouble) + static_cast<int>(circleArea);
    (void)result; // Suppress unused warning
}

// Simple main function
int main() {
    performanceTest();
    return 0;
}

