# BETTER_EXAMPLES.md

## Improving Examples Python Code: From Classes to Functions

This document outlines recommended changes to make the Python code in the `Examples/` folder more aligned with functional programming principles, using standalone functions and composition over inheritance.

## Current Issues with Class-Heavy Design

### 1. Framework.py - Heavy Class-Based Architecture

**Current Problems:**
- `AnalysisFramework` class bundles database connection, query execution, and test assertions into a single large class
- `BaseAnalyzer` abstract base class forces inheritance pattern on all analyzers
- Tight coupling between database operations and test logic
- State management scattered across class instance variables
- Hard to test individual components in isolation

**Recommended Changes:**

#### Replace AnalysisFramework Class with Functional Modules

```python
# database_operations.py
def create_temp_database(project_root: Path) -> str:
    """Create temporary database and return path"""
    
def run_makeindex(makeindex_path: Path, compile_commands: Path, db_path: str) -> bool:
    """Run MakeIndex on compilation database"""
    
def connect_to_database(db_path: str) -> tuple[kuzu.Database, kuzu.Connection]:
    """Connect to Kuzu database and return connection objects"""
    
def cleanup_database(temp_path: str, db: kuzu.Database, conn: kuzu.Connection):
    """Clean up database resources"""

# query_operations.py  
def execute_query(conn: kuzu.Connection, cypher_query: str) -> kuzu.QueryResult:
    """Execute Cypher query"""
    
def query_to_list(conn: kuzu.Connection, cypher_query: str) -> list[dict]:
    """Execute query and return as list of dictionaries"""
    
def count_query_results(conn: kuzu.Connection, cypher_query: str) -> int:
    """Count results from query"""

# assertion_helpers.py
def assert_query_count(conn: kuzu.Connection, query: str, expected: int, message: str = ""):
    """Assert query returns expected count"""
    
def assert_query_has_results(conn: kuzu.Connection, query: str, message: str = ""):
    """Assert query returns at least one result"""
    
def get_table_info(conn: kuzu.Connection) -> dict[str, int]:
    """Get information about database tables"""
```

#### Replace BaseAnalyzer Inheritance with Function Composition

**Instead of:**
```python
class AstAnalysis(BaseAnalyzer):
    def run(self):
        # analysis logic
```

**Use:**
```python
# ast_analysis.py
def analyze_ast_nodes(conn: kuzu.Connection) -> AnalysisResult:
    """Analyze AST node structure and properties"""
    
def verify_required_fields(conn: kuzu.Connection, fields: list[str]) -> list[str]:
    """Verify required fields exist, return missing fields"""
    
def check_node_types(conn: kuzu.Connection, expected_types: list[str]) -> dict[str, bool]:
    """Check for expected node types, return found status"""
    
def validate_source_files(conn: kuzu.Connection, expected_files: list[str]) -> list[str]:
    """Validate source files are recorded, return missing files"""

# Analysis result as simple data structure
@dataclass
class AnalysisResult:
    analyzer_name: str
    passed: bool
    warnings: list[str]
    errors: list[str]
    details: dict[str, any]
```

### 2. Individual Analyzer Files - Forced Inheritance

**Current Problems:**
- All analyzers inherit from `BaseAnalyzer` unnecessarily
- Each analyzer is a class with single `run()` method
- Setup/teardown methods mostly empty
- Analyzer state not needed between method calls

**Recommended Changes:**

#### Convert Analyzers to Pure Functions

```python
# inheritance_analysis.py
def analyze_inheritance_relationships(conn: kuzu.Connection) -> AnalysisResult:
    """Analyze inheritance relationships and virtual functions"""
    warnings = []
    errors = []
    
    # Check for class declarations
    if not has_class_declarations(conn):
        errors.append("No C++ class declarations found")
        return AnalysisResult("InheritanceAnalysis", False, warnings, errors, {})
    
    # Check specific classes
    missing_classes = check_expected_classes(conn, ["Base", "Derived", "TemplateClass"])
    warnings.extend(f"Class {cls} not found" for cls in missing_classes)
    
    # Check inheritance relationships  
    inheritance_results = verify_inheritance_relationships(conn)
    
    return AnalysisResult(
        analyzer_name="InheritanceAnalysis",
        passed=len(errors) == 0,
        warnings=warnings,
        errors=errors,
        details=inheritance_results
    )

def has_class_declarations(conn: kuzu.Connection) -> bool:
    """Check if class declarations exist"""
    
def check_expected_classes(conn: kuzu.Connection, expected: list[str]) -> list[str]:
    """Return list of missing expected classes"""
    
def verify_inheritance_relationships(conn: kuzu.Connection) -> dict:
    """Verify inheritance relationships, return detailed results"""
```

### 3. Run Scripts - Procedural with Class Dependencies

**Current Problems:**
- `run_analysis.py` imports and calls class-based framework
- `run_examples.py` has good functional structure but could be improved
- Mixed paradigms make code harder to follow

**Recommended Changes:**

#### Pure Functional Analysis Runner

```python
# run_analysis.py
def main():
    """Main analysis runner using functional composition"""
    project_root = get_project_root()
    
    # Setup (pure functions)
    db_path = create_temp_database(project_root)
    db, conn = connect_to_database(db_path)
    
    try:
        # Run all analyzers (pure functions)
        analyzers = [
            analyze_ast_nodes,
            analyze_inheritance_relationships, 
            analyze_control_flow,
            analyze_templates,
            # ... more analyzers
        ]
        
        results = run_analyzers(conn, analyzers)
        print_analysis_summary(results)
        
        return all(result.passed for result in results)
        
    finally:
        cleanup_database(db_path, db, conn)

def run_analyzers(conn: kuzu.Connection, analyzers: list[callable]) -> list[AnalysisResult]:
    """Run list of analyzer functions and collect results"""
    results = []
    for analyzer in analyzers:
        try:
            result = analyzer(conn)
            results.append(result)
            print_analyzer_result(result)
        except Exception as e:
            error_result = AnalysisResult(
                analyzer_name=analyzer.__name__,
                passed=False,
                warnings=[],
                errors=[str(e)],
                details={}
            )
            results.append(error_result)
    return results

def print_analyzer_result(result: AnalysisResult):
    """Print individual analyzer result"""
    status = "[PASS]" if result.passed else "[FAIL]"
    print(f"{status} {result.analyzer_name}")
    
    for warning in result.warnings:
        print(f"  Warning: {warning}")
    for error in result.errors:
        print(f"  Error: {error}")
```

## Benefits of Functional Approach

### 1. **Easier Testing**
- Each function can be tested in isolation
- No need to mock class hierarchies
- Clear input/output contracts

### 2. **Better Composability**
- Functions can be easily combined in different ways
- Analyzers can share common helper functions
- Pipeline-style data processing

### 3. **Simpler Mental Model**
- No inheritance hierarchies to understand
- Clear data flow through function calls
- Immutable data structures where possible

### 4. **Reduced Coupling**
- Database operations separated from analysis logic
- Assertion helpers independent of framework
- Each analyzer is self-contained

### 5. **Easier Maintenance**
- Adding new analyzers doesn't require inheritance
- Changing one analyzer doesn't affect others
- Clear separation of concerns

## Implementation Strategy

### Phase 1: Extract Core Functions
1. Extract database operations from `AnalysisFramework` class
2. Extract query operations into separate module
3. Extract assertion helpers into utilities module

### Phase 2: Convert Analyzers
1. Convert one analyzer at a time to functional style
2. Keep both versions during transition
3. Update tests to use functional versions

### Phase 3: Update Runners
1. Modify `run_analysis.py` to use functional approach
2. Update `run_examples.py` for consistency
3. Remove old class-based code

### Phase 4: Add Functional Utilities
1. Add pipeline utilities for chaining operations
2. Add functional error handling helpers
3. Add data transformation utilities

## Example: Before and After

### Before (Class-based)
```python
class ControlFlowAnalysis(BaseAnalyzer):
    def __init__(self, framework: AnalysisFramework):
        super().__init__(framework)
        
    def run(self):
        self.framework.assert_query_has_results(
            "MATCH (n:Statement) RETURN n LIMIT 1",
            "Should have statement declarations"
        )
        # ... more analysis
```

### After (Functional)
```python
def analyze_control_flow(conn: kuzu.Connection) -> AnalysisResult:
    """Analyze control flow statements and CFG blocks"""
    
    if not has_statements(conn):
        return AnalysisResult(
            "ControlFlowAnalysis", 
            False, 
            [], 
            ["No statement declarations found"], 
            {}
        )
    
    statement_analysis = analyze_statement_types(conn)
    cfg_analysis = analyze_cfg_blocks(conn)
    
    return combine_analysis_results("ControlFlowAnalysis", [
        statement_analysis,
        cfg_analysis
    ])

def has_statements(conn: kuzu.Connection) -> bool:
    """Check if statements exist in database"""
    return count_query_results(conn, "MATCH (n:Statement) RETURN n") > 0

def analyze_statement_types(conn: kuzu.Connection) -> AnalysisResult:
    """Analyze different statement types"""
    # Implementation details...
```

This functional approach makes the code more modular, testable, and easier to understand while eliminating unnecessary class hierarchies and inheritance patterns.

## Terminology Improvements: From "Analysis" to "Query"

### Current Terminology Issues

The Examples folder currently uses "analysis" and "analyzer" terminology throughout, which suggests heavyweight, complex processing. However, what the examples actually do is much lighter - they execute database queries to verify that CppGraphIndex correctly captured C++ language constructs.

**Current Heavy-Weight Terminology:**
- `Examples/analysis/` - Suggests complex analytical processing
- `AnalysisFramework` - Implies sophisticated analysis algorithms  
- `BaseAnalyzer` - Suggests complex analysis inheritance hierarchy
- `run_analysis.py` - Implies running heavy analytical computations
- `*_analysis.py` files - Suggests each performs complex analysis
- "Analyze inheritance relationships" - Sounds like complex algorithmic analysis
- "Analysis suite" - Suggests comprehensive analytical processing

**What Actually Happens:**
- Execute simple Cypher queries against the database
- Count results and verify expected data exists
- Check for specific node types, relationships, and properties
- Validate that parsing captured expected C++ constructs
- Simple database verification, not complex analysis

### Recommended Terminology Changes

#### 1. Directory and File Renaming

**Current Structure:**
```
Examples/
├── analysis/                   # Heavy-weight name
│   ├── run_analysis.py        # Suggests complex analysis
│   ├── framework.py           # Generic, unclear purpose
│   └── analyzers/             # Suggests analysis algorithms
│       ├── ast_analysis.py    # Heavy analysis terminology
│       ├── inheritance_analysis.py
│       └── control_flow_analysis.py
```

**Recommended Structure:**
```
Examples/
├── queries/                    # Light-weight, clear purpose
│   ├── run_queries.py         # Clear: runs database queries
│   ├── query_framework.py     # Clear: database query utilities
│   └── verifiers/             # Clear: verification queries
│       ├── ast_queries.py     # Clear: AST verification queries
│       ├── inheritance_queries.py  # Clear: inheritance verification
│       └── control_flow_queries.py # Clear: control flow verification
```

#### 2. Class and Function Renaming

**Current Names:**
```python
class AnalysisFramework:           # Suggests complex analysis
class BaseAnalyzer:                # Suggests analysis algorithms
class InheritanceAnalysis:         # Heavy analysis terminology
def run_analysis_suite():          # Suggests analytical processing
def analyze_inheritance_relationships():  # Complex analysis
```

**Recommended Names:**
```python
class QueryFramework:              # Clear database query utilities
class BaseVerifier:                # Clear verification purpose  
class InheritanceVerifier:         # Clear verification role
def run_verification_suite():      # Clear verification purpose
def verify_inheritance_relationships():  # Clear verification action
```

#### 3. Method and Variable Renaming

**Current Analysis-Heavy Names:**
```python
def analyze_ast_nodes()            # Suggests complex analysis
def analyze_control_flow()         # Heavy analytical processing
analyzer_modules = [...]           # Analysis algorithm modules
analysis_results = [...]           # Analysis computation results
```

**Recommended Query-Focused Names:**
```python
def query_ast_nodes()              # Clear database querying
def verify_control_flow()          # Clear verification purpose
verifier_modules = [...]           # Database verification modules  
verification_results = [...]       # Verification check results
```

#### 4. Documentation Language Changes

**Current Heavy Analysis Language:**
- "Analyze inheritance relationships and virtual function analysis"
- "Comprehensive analysis on the generated database"  
- "Analysis suite provides automated verification"
- "Individual analysis modules"
- "Analysis framework core components"

**Recommended Light Query Language:**
- "Query inheritance relationships and verify virtual functions"
- "Comprehensive verification queries on the generated database"
- "Query suite provides automated verification"  
- "Individual verification query modules"
- "Query framework core components"

#### 5. README and Documentation Updates

**Examples/README.md Changes:**
```markdown
# Current
└── analysis/                   # Python analysis and verification tools
    ├── run_analysis.py         # Main analysis runner
    ├── framework.py            # Analysis framework
    └── analyzers/              # Individual analysis modules

# Recommended  
└── queries/                    # Python query and verification tools
    ├── run_queries.py          # Main verification runner
    ├── query_framework.py      # Database query framework
    └── verifiers/              # Individual verification modules
```

**Examples/analysis/README.md → Examples/queries/README.md:**
```markdown
# Current
# CppGraphIndex Analysis Tools
This directory contains Python tools for analyzing and verifying...

# Recommended
# CppGraphIndex Verification Queries  
This directory contains Python tools for querying and verifying...
```

### Benefits of Query-Focused Terminology

#### 1. **Accurate Expectations**
- "Query" correctly describes the lightweight database operations
- "Verify" clearly indicates the validation purpose
- Users understand they're checking data, not running complex algorithms

#### 2. **Clearer Purpose**
- Database queries are the primary operation
- Verification is the goal, not analysis
- Simple validation checks, not computational analysis

#### 3. **Better Mental Model**
- Think in terms of database queries and expected results
- Focus on data validation rather than algorithmic processing
- Clearer distinction from actual code analysis (done by CppGraphIndex)

#### 4. **Reduced Cognitive Load**
- "Query" is simpler concept than "analysis"
- "Verify" is clearer action than "analyze"
- Less intimidating for new contributors

#### 5. **More Accurate Documentation**
- Documentation matches actual implementation
- Clear separation between parsing (CppGraphIndex) and verification (queries)
- Easier to explain to new users

### Implementation Strategy

#### Phase 1: Rename Core Components
1. Rename `Examples/analysis/` to `Examples/queries/`
2. Rename `AnalysisFramework` to `QueryFramework`
3. Rename `BaseAnalyzer` to `BaseVerifier`
4. Update main runner scripts

#### Phase 2: Rename Individual Modules
1. Rename `analyzers/` to `verifiers/`
2. Rename `*_analysis.py` to `*_queries.py`
3. Rename analyzer classes to verifier classes
4. Update method names from `analyze_*` to `query_*` or `verify_*`

#### Phase 3: Update Documentation
1. Update all README files with new terminology
2. Update code comments and docstrings
3. Update variable and parameter names
4. Update error messages and output text

#### Phase 4: Update Integration Points
1. Update `run_examples.py` to use new terminology
2. Update test framework integration
3. Update CI/CD references
4. Update main project documentation

### Example: Before and After Terminology

**Before (Analysis-Heavy):**
```python
class InheritanceAnalysis(BaseAnalyzer):
    """Analyze inheritance analysis from test_inheritance.cpp"""
    
    def run(self):
        """Analyze inheritance relationships and virtual function overrides"""
        
        # Analyze that we have class declarations  
        self.framework.assert_query_has_results(
            "MATCH (a:ASTNode), (d:Declaration) WHERE a.node_type = 'CXXRecordDecl' RETURN d",
            "Should have C++ class declarations"
        )
        
        # Analyze for specific classes
        for class_name in expected_classes:
            # Complex analysis logic...
```

**After (Query-Focused):**
```python
class InheritanceVerifier(BaseVerifier):
    """Verify inheritance relationships from test_inheritance.cpp"""
    
    def run(self):
        """Query inheritance relationships and verify virtual function overrides"""
        
        # Verify that we have class declarations  
        self.framework.assert_query_has_results(
            "MATCH (a:ASTNode), (d:Declaration) WHERE a.node_type = 'CXXRecordDecl' RETURN d",
            "Should have C++ class declarations"
        )
        
        # Query for specific classes
        for class_name in expected_classes:
            # Simple verification queries...
```

This terminology change makes the Examples folder more approachable and accurately represents the lightweight database verification operations being performed.