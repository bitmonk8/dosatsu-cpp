/**
 * @file control_flow_complex.cpp
 * @brief Comprehensive example demonstrating complex control flow patterns for CFG analysis
 * 
 * This example showcases complex control flow constructs to test CFG (Control Flow Graph) analysis:
 * - Nested loops with breaks and continues
 * - Complex switch statements with fall-through
 * - Exception handling with multiple catch blocks
 * - Goto statements and labels
 * - Function calls and recursion
 * - Conditional expressions and short-circuit evaluation
 * - Multiple return paths and unreachable code
 */

// Simple string class to replace std::string
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
    
    SimpleString operator+(const char* str) const {
        int str_len = 0;
        while (str[str_len] != '\0') str_len++;
        char* combined = new char[length + str_len + 1];
        for (int i = 0; i < length; i++) {
            combined[i] = data[i];
        }
        for (int i = 0; i <= str_len; i++) {
            combined[length + i] = str[i];
        }
        SimpleString result(combined);
        delete[] combined;
        return result;
    }
    
    SimpleString operator+(const SimpleString& other) const {
        char* combined = new char[length + other.length + 1];
        for (int i = 0; i < length; i++) {
            combined[i] = data[i];
        }
        for (int i = 0; i <= other.length; i++) {
            combined[length + i] = other.data[i];
        }
        SimpleString result(combined);
        delete[] combined;
        return result;
    }
    
    SimpleString& operator+=(const char* str) {
        int str_len = 0;
        while (str[str_len] != '\0') str_len++;
        char* new_data = new char[length + str_len + 1];
        for (int i = 0; i < length; i++) {
            new_data[i] = data[i];
        }
        for (int i = 0; i <= str_len; i++) {
            new_data[length + i] = str[i];
        }
        delete[] data;
        data = new_data;
        length += str_len;
        return *this;
    }
    
    SimpleString& operator+=(const SimpleString& other) {
        char* new_data = new char[length + other.length + 1];
        for (int i = 0; i < length; i++) {
            new_data[i] = data[i];
        }
        for (int i = 0; i <= other.length; i++) {
            new_data[length + i] = other.data[i];
        }
        delete[] data;
        data = new_data;
        length += other.length;
        return *this;
    }
};

// Simple vector-like container
template<typename T>
class SimpleVector {
private:
    T* data;
    int size_;
    int capacity_;
    
public:
    SimpleVector() : data(nullptr), size_(0), capacity_(0) {}
    
    SimpleVector(int initial_size, const T& value = T{}) : size_(initial_size), capacity_(initial_size) {
        data = new T[capacity_];
        for (int i = 0; i < size_; i++) {
            data[i] = value;
        }
    }
    
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
    
    T& operator[](int index) { return data[index]; }
    const T& operator[](int index) const { return data[index]; }
    
    int size() const { return size_; }
    bool empty() const { return size_ == 0; }
};

// Simple exception hierarchy without std::exception
class BaseControlFlowException {
protected:
    SimpleString message_;
public:
    explicit BaseControlFlowException(const SimpleString& msg) : message_(msg) {}
    virtual ~BaseControlFlowException() = default;
    const char* what() const { return message_.c_str(); }
};

class ValidationError : public BaseControlFlowException {
public:
    explicit ValidationError(const SimpleString& msg) 
        : BaseControlFlowException(SimpleString("Validation: ") + msg.c_str()) {}
};

class ProcessingError : public BaseControlFlowException {
public:
    explicit ProcessingError(const SimpleString& msg) 
        : BaseControlFlowException(SimpleString("Processing: ") + msg.c_str()) {}
};

class ResourceError : public BaseControlFlowException {
public:
    explicit ResourceError(const SimpleString& msg) 
        : BaseControlFlowException(SimpleString("Resource: ") + msg.c_str()) {}
};

// Complex control flow class
class ControlFlowAnalysis {
public:
    // Function with multiple nested loops and complex branching
    int nested_loops_with_breaks(int limit) {
        int result = 0;
        bool found_target = false;
        
        // Outer loop with multiple exit conditions
        for (int i = 0; i < limit && !found_target; ++i) {
            // Skip even numbers
            if (i % 2 == 0) continue;
            
            // Inner loop with break conditions
            for (int j = 1; j <= i; ++j) {
                // Complex condition with short-circuit evaluation
                if (j > 10 || (i * j) > 100) {
                    break;  // Break inner loop
                }
                
                // Nested condition with continue
                if ((i + j) % 3 == 0) {
                    continue;  // Continue inner loop
                }
                
                // Innermost loop with goto
                for (int k = 0; k < j; ++k) {
                    result += i * j * k;
                    
                    // Conditional goto
                    if (result > 1000) {
                        goto cleanup;
                    }
                    
                    // Multiple nested conditions
                    if (k % 2 == 0) {
                        if (i > j) {
                            result -= k;
                        } else if (i == j) {
                            result += k * 2;
                        } else {
                            // Do nothing - empty branch
                        }
                    }
                }
                
                // Check for target found
                if (result == 42) {
                    found_target = true;
                    break;  // Break middle loop
                }
            }
        }
        
        cleanup:
        return result;
    }
    
    // Complex switch statement with fall-through and nested control flow
    SimpleString complex_switch_analysis(int input, bool flag) {
        SimpleString result = "";
        int counter = 0;
        
        switch (input % 10) {
            case 0:
                result += "zero";
                if (flag) {
                    result += "_flag";
                    break;
                }
                // Fall through to case 1
                [[fallthrough]];
                
            case 1:
                result += "one";
                counter++;
                if (counter > 5) {
                    goto switch_exit;
                }
                break;
                
            case 2:
            case 3:
                // Multiple cases, nested loop
                for (int i = 0; i < 3; ++i) {
                    // Simple int to string conversion
                    char digit_str[2] = {static_cast<char>('0' + (input % 10)), '\0'};
                    result = result + SimpleString(digit_str);
                    if (i == 1 && flag) {
                        continue;
                    }
                    counter += i;
                }
                break;
                
            case 4:
                // Exception throwing in switch
                if (input < 0) {
                    throw ValidationError("Negative input not allowed");
                }
                result += "four";
                break;
                
            case 5:
                // Nested switch
                switch (input / 10) {
                    case 0: result += "five_zero"; break;
                    case 1: result += "five_one"; break;
                    default: 
                        result += "five_other";
                        if (flag) {
                            return result;  // Early return
                        }
                        break;
                }
                break;
                
            case 6:
            case 7:
            case 8:
                // Complex condition in case
                if (flag && (input > 50)) {
                    result += "high";
                } else if (!flag && (input < 20)) {
                    result += "low";
                } else {
                    result += "medium";
                    // Nested while loop in switch case
                    while (counter < input % 5) {
                        counter++;
                        if (counter % 2 == 0) continue;
                        // Simple int to string conversion  
                        char counter_str[16];
                        int temp = counter;
                        int pos = 0;
                        if (temp == 0) {
                            counter_str[pos++] = '0';
                        } else {
                            while (temp > 0) {
                                counter_str[pos++] = '0' + (temp % 10);
                                temp /= 10;
                            }
                            // Reverse the string
                            for (int i = 0; i < pos / 2; i++) {
                                char swap = counter_str[i];
                                counter_str[i] = counter_str[pos - 1 - i];
                                counter_str[pos - 1 - i] = swap;
                            }
                        }
                        counter_str[pos] = '\0';
                        result = result + SimpleString("_") + SimpleString(counter_str);
                    }
                }
                break;
                
            case 9:
                // Recursive call in switch
                if (input > 100) {
                    result += complex_switch_analysis(input / 2, !flag);
                } else {
                    result += "nine";
                }
                break;
                
            default:
                // Should never reach here given input % 10
                result += "impossible";
                break;
        }
        
        switch_exit:
        return result;
    }
    
    // Exception handling with complex control flow
    int exception_control_flow(const SimpleVector<int>& data, int threshold) {
        int result = 0;
        int processed = 0;
        
        try {
            // Outer try block
            for (int i = 0; i < data.size(); ++i) {
                try {
                    // Inner try block
                    if (data[i] < 0) {
                        // Simple int to string conversion
                        char index_str[16];
                        int temp = i;
                        int pos = 0;
                        if (temp == 0) {
                            index_str[pos++] = '0';
                        } else {
                            while (temp > 0) {
                                index_str[pos++] = '0' + (temp % 10);
                                temp /= 10;
                            }
                            // Reverse
                            for (int j = 0; j < pos / 2; j++) {
                                char swap = index_str[j];
                                index_str[j] = index_str[pos - 1 - j];
                                index_str[pos - 1 - j] = swap;
                            }
                        }
                        index_str[pos] = '\0';
                        throw ValidationError(SimpleString("Negative value at index ") + SimpleString(index_str));
                    }
                    
                    if (data[i] > threshold) {
                        throw ProcessingError("Value exceeds threshold");
                    }
                    
                    // Complex processing with potential exceptions
                    for (int j = 0; j < data[i]; ++j) {
                        if (j > 100) {
                            throw ResourceError("Processing limit exceeded");
                        }
                        
                        result += j * data[i];
                        processed++;
                        
                        // Nested exception handling
                        try {
                            if (result > 10000) {
                                throw ResourceError("Result overflow");
                            }
                        } catch (const ResourceError& e) {
                            result = result / 2;  // Recovery
                            continue;  // Continue processing
                        }
                    }
                    
                } catch (const ValidationError& e) {
                    // Skip this element and continue
                    continue;
                } catch (const ProcessingError& e) {
                    // Reduce threshold and retry
                    threshold *= 2;
                    --i;  // Retry current element
                    continue;
                }
            }
            
        } catch (const ResourceError& e) {
            // Resource exhaustion - return partial result
            return result / 2;
        } catch (const BaseControlFlowException& e) {
            // Base exception handler
            return -1;
        } catch (...) {
            // Catch-all handler
            return -2;
        }
        
        return result;
    }
    
    // Function with multiple return paths and unreachable code
    int multiple_return_paths(int x, int y, bool condition) {
        // Early returns based on input validation
        if (x < 0) return -1;
        if (y < 0) return -2;
        if (x == 0 && y == 0) return 0;
        
        int result = x + y;
        
        // Complex branching with multiple returns
        if (condition) {
            if (x > y) {
                if (x > 100) {
                    return x * 2;  // Return path 1
                } else {
                    return x + 10;  // Return path 2
                }
            } else if (x < y) {
                switch (y % 3) {
                    case 0: return y;      // Return path 3
                    case 1: return y * 2;  // Return path 4
                    case 2: return y + x;  // Return path 5
                    default: 
                        // Unreachable code (y % 3 can only be 0, 1, or 2)
                        return -999;
                }
            } else {
                // x == y
                return x * y;  // Return path 6
            }
        } else {
            // !condition branch
            for (int i = 0; i < 10; ++i) {
                result += i;
                if (result > 50) {
                    return result;  // Return path 7
                }
            }
            
            // Loop completed without early return
            if (result == 45) {  // 0+1+2+...+9 = 45, plus x+y
                return result * 2;  // Return path 8
            }
        }
        
        // Default return path
        return result;  // Return path 9
        
        // Unreachable code after return
        result += 1000;
        return result;
    }
    
    // Recursive function with complex termination conditions
    int complex_recursion(int n, int depth, bool& overflow_flag) {
        // Base cases with multiple conditions
        if (n <= 0) return 0;
        if (n == 1) return 1;
        if (depth > 100) {
            overflow_flag = true;
            return -1;  // Overflow protection
        }
        
        int result = 0;
        
        // Multiple recursive calls with different conditions
        if (n % 2 == 0) {
            // Even number path
            result = complex_recursion(n / 2, depth + 1, overflow_flag);
            if (overflow_flag) return result;  // Early termination
            
            result += complex_recursion(n / 4, depth + 2, overflow_flag);
        } else {
            // Odd number path
            result = complex_recursion((n - 1) / 2, depth + 1, overflow_flag);
            if (overflow_flag) return result;  // Early termination
            
            result += complex_recursion((n + 1) / 2, depth + 1, overflow_flag);
        }
        
        // Additional processing with potential early returns
        if (result > 1000) {
            return result / 2;
        }
        
        return result + n;
    }
    
    // Function with goto statements and labels
    int goto_control_flow(int input) {
        int result = 0;
        int counter = 0;
        
        start:
        counter++;
        
        if (counter > 10) {
            goto end;  // Exit condition
        }
        
        if (input < 0) {
            input = -input;
            goto process_positive;
        }
        
        if (input == 0) {
            result = 1;
            goto start;  // Loop back
        }
        
        process_positive:
        result += input;
        
        // Complex condition with goto
        if (result > 100) {
            if (counter < 5) {
                goto reset;
            } else {
                goto end;
            }
        }
        
        // Nested goto logic
        for (int i = 0; i < input; ++i) {
            if (i % 7 == 0) {
                goto skip_iteration;
            }
            
            result += i;
            
            if (result > 50) {
                goto start;  // Restart processing
            }
            
            skip_iteration:
            continue;
        }
        
        goto start;  // Continue main loop
        
        reset:
        result = 0;
        input = input / 2;
        goto start;
        
        end:
        return result;
    }
    
    // Function with short-circuit evaluation and complex conditions
    bool complex_boolean_logic(int a, int b, int c, const SimpleVector<int>& vec) {
        // Complex short-circuit evaluation
        if (a > 0 && b > 0 && c > 0 && 
            !vec.empty() && vec.size() > 5 &&
            vec[0] != 0 && (vec[0] % 2 == 0 || vec[1] % 3 == 0)) {
            
            // Nested complex conditions
            for (int i = 0; i < vec.size() && i < 10; ++i) {
                if ((vec[i] > a || vec[i] < b) && 
                    (i % 2 == 0 ? vec[i] > c : vec[i] < c)) {
                    return true;
                }
            }
        }
        
        // Complex OR conditions with function calls
        return (a > b && validate_input(a)) || 
               (b > c && validate_input(b)) || 
               (c > a && validate_input(c)) ||
               (!vec.empty() && process_vector(vec));
    }
    
private:
    // Helper functions for complex boolean logic
    bool validate_input(int value) {
        return value > 0 && value < 1000;
    }
    
    bool process_vector(const SimpleVector<int>& vec) {
        if (vec.empty()) return false;
        
        int sum = 0;
        for (int i = 0; i < vec.size(); ++i) {
            sum += vec[i];
            if (sum > 100) return true;
        }
        return false;
    }
};

// Function to demonstrate all control flow patterns
void demonstrate_control_flow_analysis() {
    ControlFlowAnalysis analyzer;
    
    try {
        // Test nested loops with breaks
        int result1 = analyzer.nested_loops_with_breaks(20);
        
        // Test complex switch statement
        SimpleString result2 = analyzer.complex_switch_analysis(67, true);
        
        // Test exception handling
        SimpleVector<int> test_data;
        test_data.push_back(1);
        test_data.push_back(5);
        test_data.push_back(-2);
        test_data.push_back(10);
        test_data.push_back(150);
        test_data.push_back(3);
        test_data.push_back(7);
        int result3 = analyzer.exception_control_flow(test_data, 100);
        
        // Test multiple return paths
        int result4 = analyzer.multiple_return_paths(15, 25, true);
        
        // Test recursion
        bool overflow = false;
        int result5 = analyzer.complex_recursion(10, 0, overflow);
        
        // Test goto statements
        int result6 = analyzer.goto_control_flow(42);
        
        // Test complex boolean logic
        SimpleVector<int> bool_test_data;
        bool_test_data.push_back(2);
        bool_test_data.push_back(3);
        bool_test_data.push_back(6);
        bool_test_data.push_back(9);
        bool_test_data.push_back(12);
        bool_test_data.push_back(15);
        bool result7 = analyzer.complex_boolean_logic(5, 10, 15, bool_test_data);
        
        // Suppress unused variable warnings
        (void)result1; (void)result2; (void)result3; (void)result4;
        (void)result5; (void)result6; (void)result7;
        
    } catch (const BaseControlFlowException& e) {
        // Control flow exception caught (no output since we removed iostream)
        (void)e; // Suppress unused warning
    } catch (...) {
        // Unknown exception caught
    }
}

// Additional functions for testing different control flow patterns

// Function with deeply nested conditions
int deeply_nested_conditions(int x) {
    if (x > 0) {
        if (x < 100) {
            if (x % 2 == 0) {
                if (x % 4 == 0) {
                    if (x % 8 == 0) {
                        return x * 8;
                    } else {
                        return x * 4;
                    }
                } else {
                    return x * 2;
                }
            } else {
                if (x % 3 == 0) {
                    return x * 3;
                } else {
                    return x;
                }
            }
        } else {
            return x / 2;
        }
    } else {
        return 0;
    }
}

// Function with complex loop interactions
void complex_loop_interactions() {
    // Nested loops with different types
    for (int i = 0; i < 10; ++i) {
        int j = 0;
        while (j < i) {
            int k = j;
            do {
                k++;
                if (k % 3 == 0) continue;
                if (k > 5) break;
            } while (k < 10);
            
            j++;
            if (j % 2 == 0) continue;
        }
        
        if (i % 4 == 0) continue;
        if (i > 7) break;
    }
}

/**
 * @brief End of complex control flow example
 * 
 * This file demonstrates:
 * - Nested loops with breaks and continues
 * - Complex switch statements with fall-through
 * - Exception handling with multiple catch blocks
 * - Goto statements and labels
 * - Multiple return paths and unreachable code
 * - Recursive functions with complex termination
 * - Short-circuit evaluation and complex boolean logic
 * - Deeply nested conditions
 * - Various loop types and interactions
 */
