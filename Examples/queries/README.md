# CppGraphIndex Verification Queries

This directory contains Python tools for querying and verifying that CppGraphIndex correctly captures C++ language constructs in the generated database.

## Overview

The verification system uses a functional approach with pure functions for database operations, query execution, and verification logic. This makes the code more modular, testable, and easier to understand.

## Structure

```
queries/
├── run_queries.py          # Main verification runner
├── database_operations.py  # Database setup and management functions
├── query_operations.py     # Cypher query execution functions
├── assertion_helpers.py    # Query assertion utilities
├── verification_result.py  # Result data structures
└── verifiers/              # Individual verification modules
    ├── ast_queries.py      # AST node verification queries
    ├── inheritance_queries.py  # Inheritance verification queries
    └── ...                 # More verification modules
```

## Usage

### Running All Verifications

```bash
cd Examples/queries
python run_queries.py
```

This will:
1. Create a temporary Kuzu database
2. Run MakeIndex on the example C++ files
3. Execute all verification queries
4. Report results and clean up

### Using Individual Components

```python
from queries import setup_example_database, cleanup_database
from queries.verifiers import verify_ast_nodes

# Setup database
db_path, db, conn = setup_example_database()

try:
    # Run specific verification
    result = verify_ast_nodes(conn)
    print(f"AST verification: {'PASS' if result.passed else 'FAIL'}")
finally:
    cleanup_database(db_path, db, conn)
```

## Functional Design Principles

### Pure Functions
- Each verifier is a pure function that takes a database connection and returns a `VerificationResult`
- No side effects except for printing progress messages
- Easy to test and compose

### Separation of Concerns
- **Database operations**: Setup, connection, cleanup
- **Query operations**: Cypher execution, result processing
- **Assertion helpers**: Common verification patterns
- **Verifiers**: Specific C++ construct verification

### Composability
- Functions can be easily combined in different ways
- Verifiers can share common helper functions
- Pipeline-style data processing

## Adding New Verifiers

1. Create a new file in `verifiers/` (e.g., `template_queries.py`)
2. Implement verification functions that return `VerificationResult`
3. Add to `verifiers/__init__.py`
4. Add to the verifier list in `run_queries.py`

Example verifier structure:

```python
def verify_templates(conn: kuzu.Connection) -> VerificationResult:
    """Verify template declarations and instantiations"""
    warnings = []
    errors = []
    details = {}
    
    try:
        # Verification logic here
        if not has_template_declarations(conn):
            errors.append("No template declarations found")
        
        # More checks...
        
        return VerificationResult(
            verifier_name="TemplateVerifier",
            passed=len(errors) == 0,
            warnings=warnings,
            errors=errors,
            details=details
        )
    except Exception as e:
        errors.append(f"Exception: {str(e)}")
        return VerificationResult("TemplateVerifier", False, warnings, errors, details)
```

## Migration from Class-Based System

This functional system replaces the previous class-based `analysis/` directory:

- `AnalysisFramework` → Separate functional modules
- `BaseAnalyzer` → Pure verifier functions
- `*_analysis.py` → `*_queries.py` with functional approach
- Class inheritance → Function composition
- Heavy "analysis" terminology → Light "query/verification" terminology

The new system is more maintainable, testable, and accurately reflects the lightweight database verification operations being performed.
