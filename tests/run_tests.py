#!/usr/bin/env python3
"""
Main test runner for CppGraphIndex end-to-end tests
"""

import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from test_framework import run_test_suite

if __name__ == "__main__":
    print("=== CppGraphIndex End-to-End Test Suite ===")
    print("This will:")
    print("1. Build a Kuzu database from the test C++ files")
    print("2. Run comprehensive tests on the generated database")
    print("3. Verify that all major C++ constructs are properly captured")
    print()
    
    success = run_test_suite()
    
    print()
    if success:
        print("*** All tests passed! ***")
        print("The CppGraphIndex system is working correctly.")
    else:
        print("*** Some tests failed. ***")
        print("Check the output above for details.")
    
    sys.exit(0 if success else 1)
