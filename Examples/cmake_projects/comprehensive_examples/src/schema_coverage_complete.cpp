/**
 * @file schema_coverage_complete.cpp
 * @brief Complete schema coverage example for all database tables and relationships
 * 
 * This example is specifically designed to exercise every table and relationship
 * defined in the SCHEMA.md to ensure complete coverage:
 * 
 * Node Tables Covered:
 * - ASTNode (base for all)
 * - Declaration (functions, classes, variables, namespaces)
 * - Type (builtin, user-defined, templates, pointers, references)
 * - Statement (all control flow constructs)
 * - Expression (operators, literals, calls, member access)
 * - TemplateParameter (type, non-type, template template, variadic)
 * - UsingDeclaration (using declarations, directives, aliases)
 * - Comment (documentation comments)
 * - ConstantExpression (constexpr evaluation)
 * 
 * Relationship Tables Covered:
 * - PARENT_OF (AST hierarchy)
 * - HAS_TYPE (declaration-type relationships)
 * - REFERENCES (usage relationships)
 * - IN_SCOPE (scoping relationships)
 * - INHERITS_FROM (class inheritance)
 * - OVERRIDES (virtual function overriding)
 * - TEMPLATE_RELATION (template relationships)
 * - SPECIALIZES (template specializations)
 */

// Forward declarations for complex relationships
template<typename T> class Container;
template<typename T, int N> class FixedArray;
class BaseClass;
class DerivedClass;

// Namespace for testing namespace relationships and using declarations
namespace TestNamespace {
    namespace Inner {
        /**
         * @brief Inner namespace class for testing nested namespaces
         * @details This class demonstrates nested namespace usage
         */
        class InnerClass {
        public:
            /// @brief Simple member function
            /// @return Always returns 42
            int getValue() const { return 42; }
        };
        
        /// @brief Template function in nested namespace
        template<typename T>
        T processValue(const T& value) {
            return value;
        }
    }
    
    /// @brief Namespace-level function
    void namespaceFunction();
    
    /// @brief Namespace-level variable
    extern int namespaceVariable;
    
    /// @brief Type alias in namespace
    using InnerType = Inner::InnerClass;
}

// Using declarations and directives
using TestNamespace::namespaceFunction;
using TestNamespace::Inner::InnerClass;
using namespace TestNamespace::Inner;

// Namespace alias
namespace TN = TestNamespace;
namespace TNI = TestNamespace::Inner;

// Template parameter examples covering all types
template<typename T>                           // Type parameter
class TypeParameter {
public:
    T value;
};

template<int N>                               // Non-type parameter
class NonTypeParameter {
private:
    int data[N];
public:
    static constexpr int size = N;
};

template<template<typename> class Container>   // Template template parameter
class TemplateTemplateParameter {
private:
    Container<int> container;
public:
    void addValue(int value) {
        // Assume container has appropriate interface
        (void)value;
    }
};

template<typename... Args>                     // Variadic parameter pack
class VariadicParameter {
public:
    static constexpr int count = sizeof...(Args);
    
    template<typename... UArgs>
    VariadicParameter(UArgs&&... args) {
        // Perfect forwarding
        (void)sizeof...(args);
    }
};

// Template with default parameters
template<typename T = int, int N = 10>
class DefaultParameters {
private:
    T data[N];
public:
    static constexpr int default_size = N;
};

// Complex template metaprogramming for TemplateMetaprogramming table
template<int N>
struct Factorial {
    static constexpr int value = N * Factorial<N - 1>::value;
};

template<>
struct Factorial<0> {
    static constexpr int value = 1;
};

template<typename T>
struct TypeTraits {
    static constexpr bool is_pointer = false;
    static constexpr bool is_const = false;
    using base_type = T;
};

template<typename T>
struct TypeTraits<T*> {
    static constexpr bool is_pointer = true;
    static constexpr bool is_const = false;
    using base_type = T;
};

template<typename T>
struct TypeTraits<const T> {
    static constexpr bool is_pointer = TypeTraits<T>::is_pointer;
    static constexpr bool is_const = true;
    using base_type = typename TypeTraits<T>::base_type;
};

// Static assertions for StaticAssertion table
static_assert(Factorial<5>::value == 120, "5! should equal 120");
static_assert(TypeTraits<int*>::is_pointer, "int* should be detected as pointer");
static_assert(TypeTraits<const int>::is_const, "const int should be detected as const");
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");

// Inheritance hierarchy for INHERITS_FROM and OVERRIDES relationships
class BaseClass {
protected:
    int base_value;
    
public:
    BaseClass(int value = 0) : base_value(value) {}
    
    /// @brief Virtual destructor
    virtual ~BaseClass() = default;
    
    /// @brief Pure virtual function
    virtual void pureVirtualMethod() = 0;
    
    /// @brief Virtual function with implementation
    virtual int virtualMethod() { return base_value; }
    
    /// @brief Non-virtual function
    int nonVirtualMethod() const { return base_value * 2; }
    
    /// @brief Protected virtual for derived access
protected:
    virtual void protectedVirtual() {}
};

// Single inheritance
class DerivedClass : public BaseClass {
private:
    int derived_value;
    
public:
    DerivedClass(int base_val, int derived_val) 
        : BaseClass(base_val), derived_value(derived_val) {}
    
    /// @brief Override pure virtual function
    void pureVirtualMethod() override {
        // Implementation
    }
    
    /// @brief Override virtual function with covariant return type
    int virtualMethod() override {
        return base_value + derived_value;
    }
    
    /// @brief Final virtual function (cannot be overridden further)
    virtual void finalMethod() final {}
    
    /// @brief New virtual function
    virtual void newVirtualMethod() {}
};

// Multiple inheritance for complex INHERITS_FROM relationships
class Printable {
public:
    virtual ~Printable() = default;
    virtual void print() const = 0;
};

class Serializable {
public:
    virtual ~Serializable() = default;
    virtual void serialize() const = 0;
};

// Multiple inheritance with virtual inheritance
class MultipleInheritance : public virtual BaseClass, public Printable, public Serializable {
public:
    MultipleInheritance(int value) : BaseClass(value) {}
    
    void pureVirtualMethod() override {}
    void print() const override {}
    void serialize() const override {}
};

// Private and protected inheritance
class PrivateInheritance : private BaseClass {
public:
    PrivateInheritance(int value) : BaseClass(value) {}
    
    void pureVirtualMethod() override {}
    
    // Expose specific base functionality
    using BaseClass::nonVirtualMethod;
};

class ProtectedInheritance : protected BaseClass {
public:
    ProtectedInheritance(int value) : BaseClass(value) {}
    
    void pureVirtualMethod() override {}
    
    // Make some protected members public
    using BaseClass::protectedVirtual;
};

// Template class for template specialization
template<typename T>
class Container {
private:
    T* data;
    int size_;
    int capacity_;
    
public:
    Container() : data(nullptr), size_(0), capacity_(0) {}
    
    ~Container() { delete[] data; }
    
    void push_back(const T& item) {
        if (size_ >= capacity_) {
            int new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            T* new_data = new T[new_capacity];
            
            for (int i = 0; i < size_; ++i) {
                new_data[i] = data[i];
            }
            
            delete[] data;
            data = new_data;
            capacity_ = new_capacity;
        }
        
        data[size_++] = item;
    }
    
    T& operator[](int index) { return data[index]; }
    const T& operator[](int index) const { return data[index]; }
    
    int size() const { return size_; }
    
    // Iterator support for range-based for
    T* begin() { return data; }
    T* end() { return data + size_; }
    const T* begin() const { return data; }
    const T* end() const { return data + size_; }
};

// Full template specialization
template<>
class Container<bool> {
private:
    unsigned char* bits;
    int size_;
    int capacity_;
    
public:
    Container() : bits(nullptr), size_(0), capacity_(0) {}
    
    ~Container() { delete[] bits; }
    
    void push_back(bool value) {
        if (size_ >= capacity_ * 8) {
            int new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            unsigned char* new_bits = new unsigned char[new_capacity];
            
            for (int i = 0; i < capacity_; ++i) {
                new_bits[i] = bits[i];
            }
            
            delete[] bits;
            bits = new_bits;
            capacity_ = new_capacity;
        }
        
        int byte_index = size_ / 8;
        int bit_index = size_ % 8;
        
        if (value) {
            bits[byte_index] |= (1 << bit_index);
        } else {
            bits[byte_index] &= ~(1 << bit_index);
        }
        
        size_++;
    }
    
    bool operator[](int index) const {
        int byte_index = index / 8;
        int bit_index = index % 8;
        return (bits[byte_index] & (1 << bit_index)) != 0;
    }
    
    int size() const { return size_; }
};

// Partial template specialization
template<typename T>
class Container<T*> {
private:
    T** pointers;
    int size_;
    int capacity_;
    
public:
    Container() : pointers(nullptr), size_(0), capacity_(0) {}
    
    ~Container() {
        for (int i = 0; i < size_; ++i) {
            delete pointers[i];
        }
        delete[] pointers;
    }
    
    void push_back(T* ptr) {
        if (size_ >= capacity_) {
            int new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            T** new_pointers = new T*[new_capacity];
            
            for (int i = 0; i < size_; ++i) {
                new_pointers[i] = pointers[i];
            }
            
            delete[] pointers;
            pointers = new_pointers;
            capacity_ = new_capacity;
        }
        
        pointers[size_++] = ptr;
    }
    
    T*& operator[](int index) { return pointers[index]; }
    T* const& operator[](int index) const { return pointers[index]; }
    
    int size() const { return size_; }
};

// Function templates for TEMPLATE_RELATION and SPECIALIZES
template<typename T>
T maximum(const T& a, const T& b) {
    return (a > b) ? a : b;
}

template<typename T, typename U>
auto multiply(const T& a, const U& b) -> decltype(a * b) {
    return a * b;
}

// Function template specialization
template<>
int maximum<int>(const int& a, const int& b) {
    return (a > b) ? a : b;
}

// Variadic function template
template<typename... Args>
void processAll(Args... args) {
    // Simple variadic processing without fold expressions
    (void)sizeof...(args); // Suppress unused parameter warning
}

// Complex expression examples for Expression table
class ExpressionExamples {
private:
    int value_;
    double ratio_;
    
public:
    ExpressionExamples(int value, double ratio) : value_(value), ratio_(ratio) {}
    
    /// @brief Arithmetic operators
    int arithmetic_operations(int a, int b) {
        int result = 0;
        
        // Binary operators
        result += a + b;        // Addition
        result -= a - b;        // Subtraction
        result *= a * b;        // Multiplication
        result /= (b != 0) ? a / b : 1;  // Division with conditional
        result %= (b != 0) ? a % b : 1;  // Modulo with conditional
        
        // Unary operators
        result = +result;       // Unary plus
        result = -result;       // Unary minus
        ++result;               // Pre-increment
        result++;               // Post-increment
        --result;               // Pre-decrement
        result--;               // Post-decrement
        
        // Compound assignment
        result += 10;
        result -= 5;
        result *= 2;
        result /= 3;
        result %= 7;
        
        return result;
    }
    
    /// @brief Comparison and logical operators
    bool logical_operations(int a, int b, bool flag) {
        // Comparison operators
        bool result = (a == b);     // Equality
        result = result || (a != b); // Inequality
        result = result && (a < b);  // Less than
        result = result || (a > b);  // Greater than
        result = result && (a <= b); // Less than or equal
        result = result || (a >= b); // Greater than or equal
        
        // Logical operators with short-circuit evaluation
        result = result && flag && (a > 0);
        result = result || !flag || (b < 0);
        result = !result;
        
        // Bitwise operators
        int bitwise = a & b;        // Bitwise AND
        bitwise |= a | b;           // Bitwise OR
        bitwise ^= a ^ b;           // Bitwise XOR
        bitwise = ~bitwise;         // Bitwise NOT
        bitwise <<= 2;              // Left shift
        bitwise >>= 1;              // Right shift
        
        return result && (bitwise != 0);
    }
    
    /// @brief Member access and function calls
    void member_access_examples() {
        // Member access
        this->value_ = 100;
        this->ratio_ = 3.14;
        
        // Method calls
        int arith_result = this->arithmetic_operations(10, 20);
        bool logic_result = this->logical_operations(5, 15, true);
        
        // Chained member access
        ExpressionExamples other(50, 2.71);
        int other_value = other.value_;
        
        // Function calls
        int max_val = maximum(arith_result, other_value);
        auto mult_result = multiply(max_val, ratio_);
        
        // Suppress unused variable warnings
        (void)logic_result; (void)mult_result;
    }
    
    /// @brief Array and pointer operations
    void array_pointer_examples() {
        // Array operations
        int array[10];
        for (int i = 0; i < 10; ++i) {
            array[i] = i * i;       // Array subscript
        }
        
        // Pointer operations
        int* ptr = array;
        int* end_ptr = ptr + 10;    // Pointer arithmetic
        
        while (ptr < end_ptr) {
            *ptr *= 2;              // Dereference
            ++ptr;                  // Pointer increment
        }
        
        // Address-of operator
        int* addr = &array[5];
        int value = *addr;
        
        (void)value; // Suppress unused warning
    }
    
    /// @brief Conditional and cast expressions
    int conditional_cast_examples(int input) {
        // Conditional (ternary) operator
        int result = (input > 0) ? input : -input;
        
        // C-style casts
        double d = (double)result;
        int back_to_int = (int)d;
        
        // Static cast
        float f = static_cast<float>(result);
        
        // Const cast (remove const)
        const int const_val = 42;
        int* non_const_ptr = const_cast<int*>(&const_val);
        
        // Reinterpret cast
        long long ll = reinterpret_cast<long long>(non_const_ptr);
        
        // Dynamic cast (would need polymorphic types)
        BaseClass* base = new DerivedClass(10, 20);
        DerivedClass* derived = dynamic_cast<DerivedClass*>(base);
        
        delete base;
        
        return back_to_int + static_cast<int>(f) + static_cast<int>(ll) + 
               (derived ? 1 : 0);
    }
};

// Complex control flow for Statement and CFGBlock tables
class ControlFlowExamples {
public:
    /// @brief Nested loops with all types of control flow
    int complex_nested_loops(int limit) {
        int result = 0;
        bool found = false;
        
        // Outer for loop
        for (int i = 0; i < limit && !found; ++i) {
            if (i % 2 == 0) continue;   // Continue statement
            
            // Inner while loop
            int j = 0;
            while (j < i) {
                if (j > 10) break;      // Break statement
                
                // Do-while loop
                int k = 0;
                do {
                    result += i * j * k;
                    k++;
                } while (k < 3);
                
                j++;
            }
            
            // Switch statement with fall-through
            switch (i % 5) {
                case 0:
                    result += 10;
                    break;
                case 1:
                case 2:                 // Fall-through
                    result += 20;
                    break;
                case 3:
                    result += 30;
                    if (result > 100) {
                        found = true;
                        break;
                    }
                    [[fallthrough]];    // Explicit fall-through
                case 4:
                    result += 40;
                    break;
                default:
                    result += 50;
                    break;
            }
        }
        
        return result;
    }
    
    /// @brief Exception handling with multiple catch blocks
    int exception_handling_example(int input) {
        int result = 0;
        
        try {
            if (input < 0) {
                throw "Negative input";
            }
            
            if (input == 0) {
                throw 42;
            }
            
            if (input > 1000) {
                throw 3.14;
            }
            
            result = input * 2;
            
            // Nested try-catch
            try {
                if (result > 500) {
                    throw result;
                }
            } catch (int nested_exception) {
                result = nested_exception / 2;
            }
            
        } catch (const char* str_exception) {
            result = -1;
        } catch (int int_exception) {
            result = int_exception;
        } catch (double double_exception) {
            result = static_cast<int>(double_exception);
        } catch (...) {
            result = -999;
        }
        
        return result;
    }
    
    /// @brief Goto statements and labels
    int goto_example(int input) {
        int result = 0;
        int counter = 0;
        
        start:
        counter++;
        
        if (counter > 10) {
            goto end;
        }
        
        if (input < 0) {
            input = -input;
            goto process;
        }
        
        if (input == 0) {
            result = 1;
            goto start;
        }
        
        process:
        result += input;
        
        if (result > 100) {
            goto reset;
        }
        
        goto start;
        
        reset:
        result = 0;
        input /= 2;
        goto start;
        
        end:
        return result;
    }
};

// Constexpr examples for ConstantExpression table
constexpr int constexpr_factorial(int n) {
    return (n <= 1) ? 1 : n * constexpr_factorial(n - 1);
}

constexpr bool constexpr_is_prime(int n) {
    if (n < 2) return false;
    for (int i = 2; i * i <= n; ++i) {
        if (n % i == 0) return false;
    }
    return true;
}

constexpr int constexpr_fibonacci(int n) {
    if (n <= 1) return n;
    
    int a = 0, b = 1;
    for (int i = 2; i <= n; ++i) {
        int temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}

// Compile-time constants
constexpr int FACT_5 = constexpr_factorial(5);
constexpr bool PRIME_17 = constexpr_is_prime(17);
constexpr int FIB_10 = constexpr_fibonacci(10);

// More static assertions
static_assert(FACT_5 == 120, "5! should be 120");
static_assert(PRIME_17, "17 should be prime");
static_assert(FIB_10 == 55, "Fibonacci(10) should be 55");

// Function to exercise all relationships and demonstrate usage
void demonstrate_complete_schema_coverage() {
    // Test inheritance relationships
    DerivedClass derived(10, 20);
    BaseClass* base_ptr = &derived;
    base_ptr->virtualMethod();      // Virtual function call
    derived.finalMethod();          // Final method call
    
    // Test multiple inheritance
    MultipleInheritance multi(30);
    multi.print();
    multi.serialize();
    
    // Test template instantiation and specialization
    Container<int> int_container;
    Container<bool> bool_container;
    Container<double*> ptr_container;
    
    int_container.push_back(42);
    bool_container.push_back(true);
    ptr_container.push_back(new double(3.14));
    
    // Test template functions
    int max_int = maximum(10, 20);
    auto mult_result = multiply(5, 3.14);
    
    // Test using declarations
    InnerClass inner;
    int inner_value = inner.getValue();
    
    TN::InnerType aliased_inner;
    int aliased_value = aliased_inner.getValue();
    
    // Test expressions
    ExpressionExamples expr_demo(100, 2.5);
    expr_demo.member_access_examples();
    expr_demo.array_pointer_examples();
    int conditional_result = expr_demo.conditional_cast_examples(50);
    
    // Test control flow
    ControlFlowExamples flow_demo;
    int loop_result = flow_demo.complex_nested_loops(15);
    int exception_result = flow_demo.exception_handling_example(75);
    int goto_result = flow_demo.goto_example(25);
    
    // Test template parameters
    TypeParameter<int> type_param;
    NonTypeParameter<10> non_type_param;
    VariadicParameter<int, double, char> variadic_param(1, 2.0, 'c');
    
    // Suppress unused variable warnings
    (void)max_int; (void)mult_result; (void)inner_value; (void)aliased_value;
    (void)conditional_result; (void)loop_result; (void)exception_result;
    (void)goto_result; (void)type_param; (void)non_type_param; (void)variadic_param;
}

// Explicit template instantiations for testing
template class Container<int>;
template class Container<double>;
template class Container<char*>;
template class TypeParameter<float>;
template class NonTypeParameter<5>;
template class VariadicParameter<int, double>;

// Note: Don't instantiate maximum<int> since it's specialized
template double maximum<double>(const double&, const double&);
template auto multiply<int, double>(const int&, const double&) -> double;

/**
 * @brief Complete schema coverage demonstration
 * 
 * This file exercises every table and relationship in the database schema:
 * 
 * Tables Covered:
 * - ASTNode: All C++ constructs create AST nodes
 * - Declaration: Functions, classes, variables, namespaces, templates
 * - Type: Builtin types, user types, templates, pointers, references, arrays
 * - Statement: All control flow (if, for, while, do-while, switch, try-catch)
 * - Expression: All operators, literals, calls, member access, casts
 * - TemplateParameter: Type, non-type, template template, variadic parameters
 * - UsingDeclaration: Using declarations, directives, namespace aliases
 * - Comment: Documentation comments throughout
 * - ConstantExpression: Constexpr functions and compile-time evaluation
 * 
 * Relationships Covered:
 * - PARENT_OF: AST parent-child relationships
 * - HAS_TYPE: Declaration-to-type relationships
 * - REFERENCES: Usage and reference relationships
 * - IN_SCOPE: Scoping relationships
 * - INHERITS_FROM: Single, multiple, virtual, private, protected inheritance
 * - OVERRIDES: Virtual function overriding with covariant returns
 * - TEMPLATE_RELATION: Template dependencies and relationships
 * - SPECIALIZES: Full and partial template specializations
 */
