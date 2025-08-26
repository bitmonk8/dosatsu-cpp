/**
 * @file example_expressions.cpp
 * @brief Comprehensive example demonstrating C++ expression constructs
 * 
 * This example showcases:
 * - Arithmetic, logical, and bitwise operators
 * - Assignment and compound assignment operators
 * - Comparison and relational operators
 * - Type casting and conversions (static_cast, dynamic_cast, etc.)
 * - Literals (numeric, string, user-defined)
 * - Lambda expressions and function objects
 * - Operator overloading and precedence
 */
// Simple replacements for standard library types
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
    SimpleString& operator=(const SimpleString& other) {
        if (this != &other) {
            delete[] data;
            length = other.length;
            data = new char[length + 1];
            for (int i = 0; i <= length; i++) {
                data[i] = other.data[i];
            }
        }
        return *this;
    }
    ~SimpleString() { delete[] data; }
    const char* c_str() const { return data; }
};

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
    T* get() const { return ptr; }
};

// Simple function wrapper (replacing std::function)
template<typename T>
class SimpleFunction; // Forward declaration

template<typename R, typename... Args>
class SimpleFunction<R(Args...)> {
private:
    R (*func_ptr)(Args...);
public:
    SimpleFunction() : func_ptr(nullptr) {}
    SimpleFunction(R (*f)(Args...)) : func_ptr(f) {}
    R operator()(Args... args) const { return func_ptr(args...); }
    operator bool() const { return func_ptr != nullptr; }
};

// Global variables for expression testing
int globalInt = 42;
double globalDouble = 3.14159;
const char* globalString = "Hello, World!";

// Class for member access expressions
class ExpressionTestClass {
private:
    int privateValue;
    static int staticCounter;
    
public:
    int publicValue;
    mutable int mutableValue;
    
    ExpressionTestClass(int val) : privateValue(val), publicValue(val * 2), mutableValue(0) {
        ++staticCounter;
    }
    
    // Member functions for call expressions
    int getValue() const { return privateValue; }
    void setValue(int val) { privateValue = val; }
    
    // Static member function
    static int getCount() { return staticCounter; }
    
    // Operator overloads
    ExpressionTestClass operator+(const ExpressionTestClass& other) const {
        return ExpressionTestClass(privateValue + other.privateValue);
    }
    
    ExpressionTestClass& operator+=(const ExpressionTestClass& other) {
        privateValue += other.privateValue;
        publicValue += other.publicValue;
        return *this;
    }
    
    bool operator==(const ExpressionTestClass& other) const {
        return privateValue == other.privateValue;
    }
    
    bool operator<(const ExpressionTestClass& other) const {
        return privateValue < other.privateValue;
    }
    
    // Subscript operator
    int operator[](int index) const {
        return privateValue + index;
    }
    
    // Function call operator
    int operator()(int multiplier) const {
        return privateValue * multiplier;
    }
    
    // Conversion operators
    operator int() const { return privateValue; }
    operator double() const { return static_cast<double>(privateValue); }
    
    // Member access
    int* getValuePtr() { return &privateValue; }
    const int* getValuePtr() const { return &privateValue; }
};

int ExpressionTestClass::staticCounter = 0;

// Function for testing call expressions
int simpleFunction(int a, int b) {
    return a + b;
}

double overloadedFunction(double x) {
    return x * 2.0;
}

int overloadedFunction(int x) {
    return x * 3;
}

// Template function for call expressions
template<typename T>
T templateFunction(const T& value) {
    return value;
}

// Function pointer expressions
int (*functionPtr)(int, int) = simpleFunction;

// Test all types of literal expressions
void testLiterals() {
    // Integer literals
    int decimal = 42;
    int octal = 052;           // Octal
    int hexadecimal = 0x2A;    // Hexadecimal
    int binary = 0b101010;     // Binary (C++14)
    
    // Integer literals with suffixes
    long longValue = 42L;
    long long longLongValue = 42LL;
    unsigned int unsignedValue = 42U;
    unsigned long unsignedLongValue = 42UL;
    
    // Floating-point literals
    float floatValue = 3.14f;
    double doubleValue = 3.14;
    long double longDoubleValue = 3.14L;
    
    // Scientific notation
    double scientific1 = 1.23e4;    // 12300
    double scientific2 = 1.23e-4;   // 0.000123
    float scientific3 = 5.67E+2f;   // 567.0
    
    // Hexadecimal floating-point (C++17)
    double hexFloat = 0x1.4p3;      // 1.25 * 2^3 = 10.0
    
    // Character literals
    char charValue = 'A';
    char escapeChar = '\n';
    char hexChar = '\x41';          // 'A' in hex
    char octalChar = '\101';        // 'A' in octal
    
    // Wide character literals
    wchar_t wideChar = L'A';
    char16_t char16Value = u'A';    // C++11
    char32_t char32Value = U'A';    // C++11
    
    // String literals
    const char* cString = "Hello";
    SimpleString cppString = "World";
    
    // Raw string literals (C++11) - simplified to regular string
    SimpleString rawString = "This is a \"raw\" string with \\n backslashes";
    
    // Wide string literals
    const wchar_t* wideString = L"Wide string";
    // UTF strings removed for simplicity
    
    // Boolean literals
    bool trueValue = true;
    bool falseValue = false;
    
    // Null pointer literal
    int* nullPtr = nullptr;
    
    // User-defined literals (C++11) - would need to define them first
    // auto customLiteral = 42_custom;
    
    // Digit separators (C++14)
    int bigNumber = 1'000'000;
    double pi = 3.141'592'653'589'793;
    
    // Use all literals to prevent optimization (output removed for library-free)
    volatile int prevent_optimization = decimal + octal + hexadecimal + binary +
              static_cast<int>(longValue + longLongValue + unsignedValue + unsignedLongValue +
              floatValue + doubleValue + longDoubleValue +
              scientific1 + scientific2 + scientific3 + hexFloat +
              charValue + escapeChar + hexChar + octalChar +
              wideChar + char16Value + char32Value) +
              static_cast<int>(trueValue + falseValue) +
              static_cast<int>(bigNumber + pi);
    (void)prevent_optimization; (void)cString; (void)cppString; (void)rawString; 
    (void)wideString; (void)nullPtr;
}

// Test arithmetic expressions
void testArithmeticExpressions() {
    int a = 10, b = 3;
    double x = 5.5, y = 2.2;
    
    // Basic arithmetic operators
    int sum = a + b;
    int difference = a - b;
    int product = a * b;
    int quotient = a / b;
    int remainder = a % b;
    
    // Floating-point arithmetic
    double floatSum = x + y;
    double floatDifference = x - y;
    double floatProduct = x * y;
    double floatQuotient = x / y;
    
    // Mixed arithmetic (implicit conversions)
    double mixed1 = a + x;
    double mixed2 = b * y;
    
    // Unary operators
    int positive = +a;
    int negative = -a;
    int preIncrement = ++a;
    int postIncrement = a++;
    int preDecrement = --b;
    int postDecrement = b--;
    
    // Compound assignment operators
    int compound = 10;
    compound += 5;   // compound = compound + 5
    compound -= 3;   // compound = compound - 3
    compound *= 2;   // compound = compound * 2
    compound /= 4;   // compound = compound / 4
    compound %= 3;   // compound = compound % 3
    
    // Complex arithmetic expressions
    int complex1 = (a + b) * (a - b);
    double complex2 = (x * y) / (x + y);
    int complex3 = a * b + a / b - a % b;
    
    // Operator precedence testing
    int precedence1 = a + b * a;        // b * a first
    int precedence2 = (a + b) * a;      // a + b first
    int precedence3 = a + b * a / b;    // b * a / b first
    
    // Results computed (output removed for library-free)
    volatile int arithmetic_result = sum + difference + product + quotient + remainder +
                                   static_cast<int>(floatSum + floatDifference + floatProduct + floatQuotient +
                                   mixed1 + mixed2) + positive + negative + preIncrement + postIncrement +
                                   preDecrement + postDecrement + compound + complex1 + 
                                   static_cast<int>(complex2) + complex3 + precedence1 + precedence2 + precedence3;
    (void)arithmetic_result;
}

// Test logical and relational expressions
void testLogicalExpressions() {
    int a = 5, b = 10, c = 5;
    bool flag1 = true, flag2 = false;
    
    // Relational operators
    bool equal = (a == c);
    bool notEqual = (a != b);
    bool lessThan = (a < b);
    bool lessEqual = (a <= c);
    bool greaterThan = (b > a);
    bool greaterEqual = (c >= a);
    
    // Logical operators
    bool logicalAnd = flag1 && flag2;
    bool logicalOr = flag1 || flag2;
    bool logicalNot = !flag1;
    
    // Short-circuit evaluation
    bool shortCircuit1 = (a > 0) && (b / a > 1);  // Safe division
    bool shortCircuit2 = (a == 0) || (b / a > 1); // Potential division by zero avoided
    
    // Complex logical expressions
    bool complex1 = (a < b) && (b > c) && (a == c);
    bool complex2 = (a != b) || (a == c) || (b < c);
    bool complex3 = (!(a > b) && (c <= a)) || (b >= 10); // Added parentheses to fix warning
    
    // Bitwise operators (not logical, but related)
    int bitwiseAnd = a & b;     // Bitwise AND
    int bitwiseOr = a | b;      // Bitwise OR
    int bitwiseXor = a ^ b;     // Bitwise XOR
    int bitwiseNot = ~a;        // Bitwise NOT
    int leftShift = a << 2;     // Left shift
    int rightShift = b >> 1;    // Right shift
    
    // Compound bitwise assignments
    int bitwise = a;
    bitwise &= b;   // bitwise = bitwise & b
    bitwise |= c;   // bitwise = bitwise | c
    bitwise ^= a;   // bitwise = bitwise ^ a
    bitwise <<= 1;  // bitwise = bitwise << 1
    bitwise >>= 1;  // bitwise = bitwise >> 1
    
    // Results computed (output removed for library-free)
    volatile int logical_result = equal + notEqual + lessThan + lessEqual + greaterThan + greaterEqual +
                                logicalAnd + logicalOr + logicalNot + shortCircuit1 + shortCircuit2 + 
                                complex1 + complex2 + complex3 + bitwiseAnd + bitwiseOr + bitwiseXor + 
                                bitwiseNot + leftShift + rightShift + bitwise;
    (void)logical_result;
}

// Test member access and pointer expressions
void testMemberAccessExpressions() {
    ExpressionTestClass obj(42);
    ExpressionTestClass* objPtr = &obj;
    
    // Direct member access
    int publicVal = obj.publicValue;
    obj.publicValue = 100;
    
    // Member function calls
    int privateVal = obj.getValue();
    obj.setValue(200);
    
    // Static member access
    int count = ExpressionTestClass::getCount();
    
    // Pointer member access
    int ptrPublicVal = objPtr->publicValue;
    objPtr->publicValue = 150;
    
    // Pointer member function calls
    int ptrPrivateVal = objPtr->getValue();
    objPtr->setValue(250);
    
    // Address-of operator
    int* valueAddress = obj.getValuePtr();
    ExpressionTestClass* objAddress = &obj;
    
    // Dereference operator
    int dereferenced = *valueAddress;
    ExpressionTestClass& objRef = *objPtr;
    
    // Array subscript (via operator overload)
    int subscripted = obj[5];
    
    // Function call operator
    int called = obj(3);
    
    // Member function pointers
    int (ExpressionTestClass::*memberFuncPtr)() const = &ExpressionTestClass::getValue;
    int memberResult = (obj.*memberFuncPtr)();
    int ptrMemberResult = (objPtr->*memberFuncPtr)();
    
    // Pointer arithmetic
    int array[5] = {1, 2, 3, 4, 5};
    int* arrayPtr = array;
    int* nextPtr = arrayPtr + 1;
    int* prevPtr = nextPtr - 1;
    long ptrDiff = nextPtr - arrayPtr; // Using long instead of ptrdiff_t
    
    // Array access
    int firstElement = array[0];
    int secondElement = *(array + 1);
    int thirdElement = arrayPtr[2];
    
    // Results computed (output removed for library-free)
    volatile int member_result = publicVal + privateVal + count + ptrPublicVal + ptrPrivateVal +
                               dereferenced + subscripted + called + memberResult + ptrMemberResult +
                               static_cast<int>(ptrDiff) + firstElement + secondElement + thirdElement;
    (void)member_result;
}

// Test cast expressions
void testCastExpressions() {
    int intValue = 42;
    double doubleValue = 3.14159;
    const char* cString = "123";
    
    // C-style casts
    double cStyleCast1 = (double)intValue;
    int cStyleCast2 = (int)doubleValue;
    
    // static_cast
    double staticCastResult = static_cast<double>(intValue);
    int staticCastResult2 = static_cast<int>(doubleValue);
    
    // const_cast
    const int constValue = 100;
    int& nonConstRef = const_cast<int&>(constValue);
    
    // reinterpret_cast
    unsigned long addressAsInt = reinterpret_cast<unsigned long>(&intValue); // Using unsigned long instead of uintptr_t
    int* intFromAddress = reinterpret_cast<int*>(addressAsInt);
    
    // dynamic_cast (would need polymorphic types)
    // Base* basePtr = new Derived();
    // Derived* derivedPtr = dynamic_cast<Derived*>(basePtr);
    
    // Functional cast notation
    double functionalCast = double(intValue);
    
    // Implicit conversions
    double implicitConv1 = intValue;        // int to double
    bool implicitConv2 = intValue;          // int to bool
    int implicitConv3 = 'A';                // char to int
    
    // User-defined conversions
    ExpressionTestClass testObj(50);
    int userConv1 = testObj;                // operator int()
    double userConv2 = testObj;             // operator double()
    
    // Conversion with constructor
    ExpressionTestClass constructorConv(intValue);
    
    // Results computed (output removed for library-free)
    volatile int cast_result = static_cast<int>(cStyleCast1) + cStyleCast2 + static_cast<int>(staticCastResult) +
                             staticCastResult2 + nonConstRef + static_cast<int>(addressAsInt) +
                             static_cast<int>(reinterpret_cast<unsigned long>(intFromAddress)) +
                             static_cast<int>(functionalCast + implicitConv1) + implicitConv2 + implicitConv3 +
                             userConv1 + static_cast<int>(userConv2);
    (void)cast_result;
}

// Test operator overload expressions
void testOperatorOverloads() {
    ExpressionTestClass obj1(10);
    ExpressionTestClass obj2(20);
    
    // Binary operator overloads
    ExpressionTestClass sum = obj1 + obj2;
    obj1 += obj2;
    
    // Comparison operator overloads
    bool isEqual = (obj1 == obj2);
    bool isLess = (obj1 < obj2);
    
    // Subscript operator
    int subscriptResult = obj1[3];
    
    // Function call operator
    int callResult = obj1(5);
    
    // Assignment operators
    ExpressionTestClass obj3 = obj1;  // Copy constructor
    obj3 = obj2;                      // Assignment operator
    
    // Increment/decrement (if implemented)
    // ++obj1; // Pre-increment
    // obj1++; // Post-increment
    
    // Results computed (output removed for library-free)
    volatile int overload_result = sum.getValue() + isEqual + isLess + subscriptResult + callResult;
    (void)overload_result;
}

// Test conditional expressions
void testConditionalExpressions() {
    int a = 10, b = 20;
    
    // Simple ternary operator
    int max = (a > b) ? a : b;
    
    // Nested ternary operators
    int sign = (a > 0) ? 1 : (a < 0) ? -1 : 0;
    
    // Ternary with different types (common type deduction)
    auto result = (a > b) ? a : 3.14;  // Result type is double
    
    // Ternary with side effects
    int counter = 0;
    int sideEffect = (a > 5) ? (++counter, a * 2) : (--counter, a / 2);
    
    // Complex condition
    bool complexCondition = (a > 0) && (b > 0) && (a + b > 25);
    int complexResult = complexCondition ? (a * b) : (a + b);
    
    // Results computed (output removed for library-free)
    volatile int conditional_result = max + sign + static_cast<int>(result) + sideEffect + counter + complexResult;
    (void)conditional_result;
}

// Test lambda expressions (C++11)
void testLambdaExpressions() {
    int x = 10, y = 20;
    
    // Simple lambda
    auto simpleLambda = []() { return 42; };
    int simpleResult = simpleLambda();
    
    // Lambda with parameters
    auto addLambda = [](int a, int b) { return a + b; };
    int addResult = addLambda(x, y);
    
    // Lambda with capture by value
    auto captureByValue = [x, y]() { return x + y; };
    int captureResult = captureByValue();
    
    // Lambda with capture by reference
    auto captureByRef = [&x, &y]() { x++; y++; return x + y; };
    int refResult = captureByRef();
    
    // Lambda with mixed capture
    auto mixedCapture = [x, &y](int z) { y++; return x + y + z; };
    int mixedResult = mixedCapture(5);
    
    // Lambda with capture all by value
    auto captureAllValue = [=]() { return x + y; };
    int allValueResult = captureAllValue();
    
    // Lambda with capture all by reference
    auto captureAllRef = [&]() { x++; return x + y; };
    int allRefResult = captureAllRef();
    
    // Lambda with mutable
    auto mutableLambda = [x]() mutable { x++; return x; };
    int mutableResult = mutableLambda();
    
    // Lambda with explicit return type
    auto explicitReturn = [](double d) -> int { return static_cast<int>(d); };
    int explicitResult = explicitReturn(3.14);
    
    // Generic lambda (C++14)
    auto genericLambda = [](auto a, auto b) { return a + b; };
    auto genericResult1 = genericLambda(5, 10);      // int + int
    auto genericResult2 = genericLambda(3.14, 2.86); // double + double
    
    // Results computed (output removed for library-free)
    volatile int lambda_result = simpleResult + addResult + captureResult + refResult + mixedResult +
                               allValueResult + allRefResult + mutableResult + explicitResult +
                               genericResult1 + static_cast<int>(genericResult2);
    (void)lambda_result;
}

// Main test function
void testExpressions() {
    // All tests run without output (library-free)
    testLiterals();
    testArithmeticExpressions();
    testLogicalExpressions();
    testMemberAccessExpressions();
    testCastExpressions();
    testOperatorOverloads();
    testConditionalExpressions();
    testLambdaExpressions();
}
