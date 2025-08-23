#!/usr/bin/env python3
"""
Main analysis runner for CppGraphIndex examples
"""

import sys
import os

# Add current directory and parent to Python path
current_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, current_dir)
sys.path.insert(0, os.path.dirname(current_dir))

from framework import run_analysis_suite

if __name__ == "__main__":
    print("=== CppGraphIndex Analysis Suite ===")
    print("This will:")
    print("1. Build a Kuzu database from the example C++ files")
    print("2. Run comprehensive analysis on the generated database")
    print("3. Verify that all major C++ constructs are properly captured")
    print()
    
    success = run_analysis_suite()
    
    print()
    if success:
        print("*** All analysis passed! ***")
        print("The CppGraphIndex system is working correctly.")
    else:
        print("*** Some analysis failed. ***")
        print("Check the output above for details.")
    
    sys.exit(0 if success else 1)
