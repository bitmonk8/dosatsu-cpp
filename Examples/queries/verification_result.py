#!/usr/bin/env python3
"""
Data structures for verification results
"""

from dataclasses import dataclass
from typing import List, Dict, Any


@dataclass
class VerificationResult:
    """Result of a verification query"""
    verifier_name: str
    passed: bool
    warnings: List[str]
    errors: List[str]
    details: Dict[str, Any]


def combine_verification_results(verifier_name: str, results: List[VerificationResult]) -> VerificationResult:
    """Combine multiple verification results into one"""
    all_warnings = []
    all_errors = []
    all_details = {}
    
    for result in results:
        all_warnings.extend(result.warnings)
        all_errors.extend(result.errors)
        all_details.update(result.details)
    
    # Overall pass if all individual results passed
    overall_passed = all(result.passed for result in results)
    
    return VerificationResult(
        verifier_name=verifier_name,
        passed=overall_passed,
        warnings=all_warnings,
        errors=all_errors,
        details=all_details
    )
