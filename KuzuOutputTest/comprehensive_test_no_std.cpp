// Comprehensive C++ test file without standard library dependencies
// Tests inheritance, templates, namespaces, control flow, expressions, etc.

// Forward declarations
class Base;
class Derived;

// Simple namespace
namespace TestNamespace {
    int globalVar = 42;
    
    void namespaceFunction() {
        // Function in namespace
    }
    
    class NamespaceClass {
    public:
        int value;
        NamespaceClass(int v) : value(v) {}
    };
}

// Base class for inheritance testing
class Base {
public:
    int baseValue;
    
    Base(int val) : baseValue(val) {}
    virtual ~Base() {}
    
    virtual void virtualMethod() {
        // Base implementation
    }
    
    void nonVirtualMethod() {
        // Non-virtual method
    }
};

// Derived class
class Derived : public Base {
private:
    int derivedValue;
    
public:
    Derived(int base, int derived) : Base(base), derivedValue(derived) {}
    
    void virtualMethod() override {
        // Overridden implementation
    }
    
    int getDerivedValue() const {
        return derivedValue;
    }
};

// Template class
template<typename T>
class TemplateClass {
private:
    T data;
    
public:
    TemplateClass(T value) : data(value) {}
    
    T getValue() const {
        return data;
    }
    
    void setValue(T value) {
        data = value;
    }
};

// Template function
template<typename T>
T templateFunction(T a, T b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

// Enum for testing
enum Color {
    RED = 1,
    GREEN = 2,
    BLUE = 3
};

// Enum class
enum class Status {
    ACTIVE,
    INACTIVE,
    PENDING
};

// Function with control flow
int controlFlowFunction(int x, int y) {
    int result = 0;
    
    // If statement
    if (x > y) {
        result = x;
    } else if (x < y) {
        result = y;
    } else {
        result = 0;
    }
    
    // For loop
    for (int i = 0; i < 5; ++i) {
        result += i;
    }
    
    // While loop
    int counter = 0;
    while (counter < 3) {
        result *= 2;
        ++counter;
    }
    
    // Do-while loop
    do {
        result--;
    } while (result > 10);
    
    // Switch statement
    switch (result % 3) {
        case 0:
            result += 10;
            break;
        case 1:
            result += 20;
            break;
        case 2:
            result += 30;
            break;
        default:
            result = 0;
            break;
    }
    
    return result;
}

// Function with various expressions
void expressionFunction() {
    int a = 10;
    int b = 20;
    
    // Arithmetic expressions
    int sum = a + b;
    int diff = a - b;
    int product = a * b;
    int quotient = b / a;
    int remainder = b % a;
    
    // Assignment expressions
    a += 5;
    b -= 3;
    a *= 2;
    b /= 4;
    
    // Comparison expressions
    bool isEqual = (a == b);
    bool isNotEqual = (a != b);
    bool isGreater = (a > b);
    bool isLess = (a < b);
    
    // Logical expressions
    bool andResult = isEqual && isGreater;
    bool orResult = isEqual || isLess;
    bool notResult = !isEqual;
    
    // Increment/decrement
    ++a;
    b--;
    
    // Member access
    Base baseObj(42);
    int baseVal = baseObj.baseValue;
    baseObj.nonVirtualMethod();
    
    // Pointer operations
    Base* ptr = &baseObj;
    ptr->virtualMethod();
    
    // Array access
    int arr[5] = {1, 2, 3, 4, 5};
    int firstElement = arr[0];
    
    // Function call
    int maxVal = templateFunction(a, b);
}

// Function with various declarations
void declarationFunction() {
    // Variable declarations
    int localVar = 100;
    const int constVar = 200;
    static int staticVar = 300;
    
    // Pointer declarations
    int* intPtr = &localVar;
    const int* constIntPtr = &constVar;
    
    // Reference declarations
    int& intRef = localVar;
    const int& constIntRef = constVar;
    
    // Array declarations
    int intArray[10];
    int initArray[3] = {1, 2, 3};
    
    // Object declarations
    Base baseObj(42);
    Derived derivedObj(10, 20);
    TemplateClass<int> templateObj(99);
    
    // Enum usage
    Color color = RED;
    Status status = Status::ACTIVE;
}

// Main function to tie everything together
int main() {
    // Test namespace usage
    TestNamespace::globalVar = 100;
    TestNamespace::namespaceFunction();
    TestNamespace::NamespaceClass nsObj(50);
    
    // Test inheritance
    Base* polymorphicPtr = new Derived(1, 2);
    polymorphicPtr->virtualMethod();
    delete polymorphicPtr;
    
    // Test templates
    TemplateClass<int> intTemplate(42);
    TemplateClass<double> doubleTemplate(3.14);
    int maxInt = templateFunction(10, 20);
    
    // Test control flow
    int result = controlFlowFunction(15, 25);
    
    // Test expressions
    expressionFunction();
    
    // Test declarations
    declarationFunction();
    
    return 0;
}
