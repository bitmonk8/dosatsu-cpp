// Test file for control flow analysis, CFG blocks, and statement analysis
#include <iostream>
#include <vector>
#include <string>

// Simple control flow with if-else
int simpleIfElse(int x) {
    int result = 0;
    
    if (x > 0) {
        result = x * 2;
    } else if (x < 0) {
        result = x * -1;
    } else {
        result = 1;
    }
    
    return result;
}

// Complex nested control flow
int complexNested(int a, int b, int c) {
    int result = 0;
    
    if (a > 0) {
        if (b > 0) {
            if (c > 0) {
                result = a + b + c;
            } else {
                result = a + b - c;
            }
        } else {
            if (c > 0) {
                result = a - b + c;
                if (result > 100) {
                    result = 100; // Cap at 100
                }
            } else {
                result = a - b - c;
            }
        }
    } else {
        if (b > 0 && c > 0) {
            result = b + c - a;
        } else {
            result = 0;
        }
    }
    
    return result;
}

// Switch statement with multiple cases
std::string processCommand(int command) {
    std::string result;
    
    switch (command) {
        case 1:
            result = "Starting";
            break;
            
        case 2:
        case 3:
            result = "Processing";
            break;
            
        case 4:
            result = "Intermediate";
            // Fall through intentionally
            
        case 5:
            result += "Completing";
            break;
            
        case 6: {
            // Block scope in case
            int temp = command * 2;
            result = "Special: " + std::to_string(temp);
            break;
        }
        
        default:
            result = "Unknown command";
            break;
    }
    
    return result;
}

// Various loop types
void demonstrateLoops() {
    // For loop with multiple variables
    for (int i = 0, j = 10; i < j; ++i, --j) {
        std::cout << "i: " << i << ", j: " << j << std::endl;
        
        if (i == 3) {
            continue; // Skip to next iteration
        }
        
        if (j == 6) {
            break; // Exit loop early
        }
    }
    
    // Range-based for loop
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    for (const auto& num : numbers) {
        std::cout << "Number: " << num << std::endl;
    }
    
    // While loop with complex condition
    int x = 0, y = 10;
    while (x < 5 && y > 0) {
        std::cout << "x: " << x << ", y: " << y << std::endl;
        
        if (x == 2) {
            x += 2; // Skip x == 3
            continue;
        }
        
        ++x;
        --y;
    }
    
    // Do-while loop
    int counter = 0;
    do {
        std::cout << "Counter: " << counter << std::endl;
        ++counter;
        
        if (counter == 3) {
            break;
        }
    } while (counter < 10);
}

// Nested loops with break and continue
int findFirstMatch(const std::vector<std::vector<int>>& matrix, int target) {
    for (size_t i = 0; i < matrix.size(); ++i) {
        for (size_t j = 0; j < matrix[i].size(); ++j) {
            if (matrix[i][j] == target) {
                return static_cast<int>(i * matrix[i].size() + j);
            }
            
            if (matrix[i][j] < 0) {
                continue; // Skip negative numbers
            }
            
            if (matrix[i][j] > 1000) {
                break; // Break inner loop for large numbers
            }
        }
    }
    
    return -1; // Not found
}

// Goto statement (generally discouraged but still valid C++)
int gotoExample(int value) {
    int result = 0;
    
    if (value < 0) {
        goto negative_handling;
    }
    
    if (value == 0) {
        goto zero_handling;
    }
    
    // Positive number handling
    result = value * 2;
    goto end;
    
negative_handling:
    result = -value;
    goto end;
    
zero_handling:
    result = 1;
    
end:
    return result;
}

// Exception handling control flow
int exceptionHandling(int divisor) {
    try {
        if (divisor == 0) {
            throw std::runtime_error("Division by zero");
        }
        
        int result = 100 / divisor;
        
        if (result > 50) {
            throw std::overflow_error("Result too large");
        }
        
        return result;
    }
    catch (const std::runtime_error& e) {
        std::cout << "Runtime error: " << e.what() << std::endl;
        return -1;
    }
    catch (const std::overflow_error& e) {
        std::cout << "Overflow error: " << e.what() << std::endl;
        return 50; // Return capped value
    }
    catch (...) {
        std::cout << "Unknown error" << std::endl;
        return -2;
    }
}

// RAII and control flow
class ResourceManager {
private:
    std::string name;
    bool acquired;
    
public:
    ResourceManager(const std::string& n) : name(n), acquired(false) {
        std::cout << "Acquiring resource: " << name << std::endl;
        acquired = true;
    }
    
    ~ResourceManager() {
        if (acquired) {
            std::cout << "Releasing resource: " << name << std::endl;
        }
    }
    
    void release() {
        if (acquired) {
            std::cout << "Manually releasing: " << name << std::endl;
            acquired = false;
        }
    }
};

int resourceHandling(bool shouldThrow) {
    ResourceManager resource1("Database");
    
    {
        ResourceManager resource2("Network");
        
        if (shouldThrow) {
            throw std::runtime_error("Simulated error");
        }
        
        resource2.release();
    } // resource2 destructor called here
    
    return 42;
} // resource1 destructor called here

// Conditional operator (ternary)
int conditionalOperatorTest(int a, int b, int c) {
    // Nested ternary operators
    int max = (a > b) ? ((a > c) ? a : c) : ((b > c) ? b : c);
    
    // Ternary with side effects
    int counter = 0;
    int result = (max > 10) ? (++counter, max * 2) : (--counter, max / 2);
    
    return result + counter;
}

// Short-circuit evaluation
bool shortCircuitTest(bool condition1, bool condition2, int value) {
    // AND short-circuit
    if (condition1 && (value > 0) && (value < 100)) {
        std::cout << "All conditions met" << std::endl;
        return true;
    }
    
    // OR short-circuit
    if (condition2 || (value == 42) || (value % 2 == 0)) {
        std::cout << "At least one condition met" << std::endl;
        return true;
    }
    
    return false;
}

// Function with multiple return paths
int multipleReturns(int input) {
    if (input < 0) {
        return -1;
    }
    
    if (input == 0) {
        return 0;
    }
    
    if (input > 100) {
        return 100;
    }
    
    for (int i = 1; i <= input; ++i) {
        if (i * i == input) {
            return i; // Perfect square
        }
        
        if (i * i > input) {
            return -i; // Not a perfect square
        }
    }
    
    return input; // Fallback
}

// Constexpr control flow (compile-time evaluation)
constexpr int constexprFactorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * constexprFactorial(n - 1);
}

constexpr int constexprFibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    
    int a = 0, b = 1, result = 0;
    for (int i = 2; i <= n; ++i) {
        result = a + b;
        a = b;
        b = result;
    }
    
    return result;
}

// Test function that exercises all control flow patterns
void testControlFlow() {
    // Test simple conditions
    int value1 = simpleIfElse(5);
    int value2 = simpleIfElse(-3);
    int value3 = simpleIfElse(0);
    
    // Test complex nested conditions
    int complex1 = complexNested(1, 2, 3);
    int complex2 = complexNested(-1, 2, 3);
    
    // Test switch statements
    std::string cmd1 = processCommand(1);
    std::string cmd2 = processCommand(4); // Fall-through case
    std::string cmd3 = processCommand(99); // Default case
    
    // Test loops
    demonstrateLoops();
    
    // Test nested loops
    std::vector<std::vector<int>> matrix = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    int position = findFirstMatch(matrix, 5);
    
    // Test goto
    int gotoResult = gotoExample(-5);
    
    // Test exception handling
    try {
        int safe = exceptionHandling(5);
        int unsafe = exceptionHandling(0);
    }
    catch (...) {
        std::cout << "Caught exception in test" << std::endl;
    }
    
    // Test RAII
    try {
        resourceHandling(false);
        resourceHandling(true); // Will throw
    }
    catch (...) {
        std::cout << "Resources cleaned up properly" << std::endl;
    }
    
    // Test conditional operators
    int ternaryResult = conditionalOperatorTest(10, 20, 15);
    
    // Test short-circuit
    bool shortResult1 = shortCircuitTest(true, false, 50);
    bool shortResult2 = shortCircuitTest(false, false, 42);
    
    // Test multiple returns
    int returnResult1 = multipleReturns(25); // Perfect square
    int returnResult2 = multipleReturns(26); // Not perfect square
    
    // Test constexpr (compile-time control flow)
    constexpr int fact5 = constexprFactorial(5);
    constexpr int fib10 = constexprFibonacci(10);
    
    static_assert(fact5 == 120, "5! should be 120");
    static_assert(fib10 == 55, "10th Fibonacci should be 55");
    
    // Use all computed values to prevent optimization
    std::cout << "Results: " << value1 << ", " << value2 << ", " << value3
              << ", " << complex1 << ", " << complex2 << ", " << cmd1 
              << ", " << cmd2 << ", " << cmd3 << ", " << position 
              << ", " << gotoResult << ", " << ternaryResult 
              << ", " << shortResult1 << ", " << shortResult2
              << ", " << returnResult1 << ", " << returnResult2
              << ", " << fact5 << ", " << fib10 << std::endl;
}
