#!/usr/bin/env python3
"""
Main verification runner for Dosatsu queries
"""

import sys
from pathlib import Path
from typing import List, Callable

# Add current directory to Python path
current_dir = Path(__file__).parent.absolute()
sys.path.insert(0, str(current_dir))

from database_operations import setup_example_database, cleanup_database
from query_operations import print_database_summary
from verification_result import VerificationResult
from verifiers import verify_inheritance_relationships, verify_ast_nodes, verify_control_flow


def run_verifiers(conn, verifiers: List[Callable]) -> List[VerificationResult]:
    """Run list of verifier functions and collect results"""
    results = []
    for verifier in verifiers:
        try:
            result = verifier(conn)
            results.append(result)
            print_verifier_result(result)
        except Exception as e:
            error_result = VerificationResult(
                verifier_name=verifier.__name__,
                passed=False,
                warnings=[],
                errors=[str(e)],
                details={}
            )
            results.append(error_result)
            print_verifier_result(error_result)
    return results


def print_verifier_result(result: VerificationResult):
    """Print individual verifier result"""
    status = "[PASS]" if result.passed else "[FAIL]"
    print(f"{status} {result.verifier_name}")
    
    for warning in result.warnings:
        print(f"  Warning: {warning}")
    for error in result.errors:
        print(f"  Error: {error}")


def print_verification_summary(results: List[VerificationResult]):
    """Print summary of all verification results"""
    passed = sum(1 for result in results if result.passed)
    failed = len(results) - passed
    
    print(f"\n=== Verification Results ===")
    print(f"Passed: {passed}")
    print(f"Failed: {failed}")
    print(f"Total: {len(results)}")
    
    if failed > 0:
        print("\nFailed verifiers:")
        for result in results:
            if not result.passed:
                print(f"  - {result.verifier_name}")
                for error in result.errors:
                    print(f"    Error: {error}")


def main():
    """Main verification runner using functional composition"""
    print("=== Dosatsu Verification Suite ===")
    print("This will:")
    print("1. Build a Kuzu database from the example C++ files")
    print("2. Run comprehensive verification queries on the generated database")
    print("3. Verify that all major C++ constructs are properly captured")
    print()
    
    # Setup (pure functions)
    db_path, db, conn = setup_example_database()
    
    try:
        # Print database summary
        print_database_summary(conn)
        
        # Run all verifiers (pure functions)
        verifiers = [
            verify_ast_nodes,
            verify_inheritance_relationships,
            verify_control_flow,
            # TODO: Add more verifiers as they are converted
        ]
        
        results = run_verifiers(conn, verifiers)
        print_verification_summary(results)
        
        success = all(result.passed for result in results)
        
        print()
        if success:
            print("*** All verification queries passed! ***")
            print("The Dosatsu system is working correctly.")
        else:
            print("*** Some verification queries failed. ***")
            print("Check the output above for details.")
        
        return success
        
    finally:
        cleanup_database(db_path, db, conn)


if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
