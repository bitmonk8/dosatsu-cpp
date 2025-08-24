#!/usr/bin/env python3
"""
Assertion helpers for CppGraphIndex verification queries
"""

import kuzu
from query_operations import count_query_results


def assert_query_count(conn: kuzu.Connection, query: str, expected: int, message: str = ""):
    """Assert query returns expected count"""
    actual_count = count_query_results(conn, query)
    if actual_count != expected:
        raise AssertionError(f"{message}: Expected {expected} results, got {actual_count}. Query: {query}")


def assert_query_min_count(conn: kuzu.Connection, query: str, min_count: int, message: str = ""):
    """Assert query returns at least minimum count"""
    actual_count = count_query_results(conn, query)
    if actual_count < min_count:
        raise AssertionError(f"{message}: Expected at least {min_count} results, got {actual_count}. Query: {query}")


def assert_query_has_results(conn: kuzu.Connection, query: str, message: str = ""):
    """Assert query returns at least one result"""
    assert_query_min_count(conn, query, 1, message)


def assert_query_no_results(conn: kuzu.Connection, query: str, message: str = ""):
    """Assert query returns no results"""
    assert_query_count(conn, query, 0, message)
