/**
 * @file simple.cpp
 * @brief Simple C++ example demonstrating basic language constructs
 * 
 * This example showcases:
 * - Basic class inheritance and virtual functions
 * - Namespaces and enums
 * - Templates and structs
 * - No standard library includes
 */

// Testing basic C++ constructs without standard library includes

//=============================================================================
// Basic inheritance and virtual functions
//=============================================================================

class SimpleClass {
private:
    int value;
public:
    SimpleClass(int v) : value(v) {}
    virtual ~SimpleClass() {}
    
    virtual int getValue() const { return value; }
    void setValue(int v) { value = v; }
};

class DerivedClass : public SimpleClass {
public:
    DerivedClass(int v) : SimpleClass(v) {}
    
    int getValue() const override { 
        return SimpleClass::getValue() * 2; 
    }
};

//=============================================================================
// Namespaces, enums, and structs
//=============================================================================

namespace TestNamespace {
    enum Color { RED, GREEN, BLUE };
    
    struct Point {
        int x, y;
        Point(int x = 0, int y = 0) : x(x), y(y) {}
        
        // Add a simple method
        int distanceSquared() const {
            return x * x + y * y;
        }
    };
}

//=============================================================================
// Template classes
//=============================================================================

template<typename T>
class Container {
private:
    T* data;
    int size;
    int capacity;
public:
    Container() : data(nullptr), size(0), capacity(0) {}
    
    ~Container() { 
        delete[] data; 
    }
    
    void add(const T& item) {
        if (size >= capacity) {
            // Simple resize logic
            int newCapacity = capacity == 0 ? 1 : capacity * 2;
            T* newData = new T[newCapacity];
            
            // Copy existing data
            for (int i = 0; i < size; i++) {
                newData[i] = data[i];
            }
            
            delete[] data;
            data = newData;
            capacity = newCapacity;
        }
        
        data[size++] = item;
    }
    
    int getSize() const { return size; }
    
    T get(int index) const {
        return (index >= 0 && index < size) ? data[index] : T{};
    }
};

//=============================================================================
// Test functions
//=============================================================================

void testInheritance() {
    SimpleClass* obj = new DerivedClass(42);
    int result = obj->getValue();  // Should be 84 (42 * 2)
    delete obj;
}

void testNamespacesAndTemplates() {
    TestNamespace::Point p(10, 20);
    int distSq = p.distanceSquared();  // Should be 500
    
    Container<int> intContainer;
    intContainer.add(42);
    intContainer.add(84);
    
    Container<TestNamespace::Point> pointContainer;
    pointContainer.add(TestNamespace::Point(1, 2));
    pointContainer.add(TestNamespace::Point(3, 4));
}

void testEnums() {
    TestNamespace::Color favorite = TestNamespace::BLUE;
    
    // Switch on enum
    int colorValue = 0;
    switch (favorite) {
        case TestNamespace::RED:
            colorValue = 1;
            break;
        case TestNamespace::GREEN:
            colorValue = 2;
            break;
        case TestNamespace::BLUE:
            colorValue = 3;
            break;
    }
}

int main() {
    testInheritance();
    testNamespacesAndTemplates();
    testEnums();
    return 0;
}

