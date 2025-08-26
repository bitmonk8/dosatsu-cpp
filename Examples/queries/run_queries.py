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
    """Print individual verifier result with minimal output for success"""
    if result.passed:
        # Minimal output for successful verifiers
        return
    else:
        # Only show failures with details
        print(f"[FAIL] {result.verifier_name}")
        for warning in result.warnings:
            print(f"  Warning: {warning}")
        for error in result.errors:
            print(f"  Error: {error}")


def print_verification_summary(results: List[VerificationResult]):
    """Print concise summary of verification results"""
    passed = sum(1 for result in results if result.passed)
    failed = len(results) - passed
    
    if failed == 0:
        # Success case - minimal output
        print(f"Verification: {passed}/{len(results)} passed")
    else:
        # Failure case - show details
        print(f"\nVerification: {passed}/{len(results)} passed, {failed} failed")
        for result in results:
            if not result.passed:
                print(f"  - {result.verifier_name}")
                for error in result.errors:
                    print(f"    Error: {error}")


def main():
    """Main verification runner using functional composition"""
    # Minimal startup output
    
    # Setup (pure functions)
    db_path, db, conn = setup_example_database()
    
    try:
        # Skip database summary for quiet operation
        # print_database_summary(conn)  # Commented out to reduce verbosity
        
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
        
        if success:
            print("All verifications passed")
        else:
            print("Some verifications failed - check details above")
        
        return success
        
    finally:
        cleanup_database(db_path, db, conn)


if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
