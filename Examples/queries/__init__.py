#!/usr/bin/env python3
"""
Dosatsu Verification Queries

This module provides functional tools for querying and verifying that Dosatsu
correctly captures C++ language constructs in the generated database.
"""

from .database_operations import (
    create_temp_database,
    run_dosatsu,
    connect_to_database,
    cleanup_database,
    setup_example_database,
    get_project_paths
)

from .query_operations import (
    execute_query,
    query_to_list,
    count_query_results,
    get_table_info,
    print_database_summary
)

from .assertion_helpers import (
    assert_query_count,
    assert_query_min_count,
    assert_query_has_results,
    assert_query_no_results
)

from .verification_result import (
    VerificationResult,
    combine_verification_results
)

__all__ = [
    # Database operations
    'create_temp_database',
    'run_dosatsu', 
    'connect_to_database',
    'cleanup_database',
    'setup_example_database',
    'get_project_paths',
    
    # Query operations
    'execute_query',
    'query_to_list',
    'count_query_results',
    'get_table_info',
    'print_database_summary',
    
    # Assertion helpers
    'assert_query_count',
    'assert_query_min_count',
    'assert_query_has_results',
    'assert_query_no_results',
    
    # Results
    'VerificationResult',
    'combine_verification_results'
]
