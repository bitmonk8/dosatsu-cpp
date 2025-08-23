#!/usr/bin/env python3
"""
Test framework for CppGraphIndex - End-to-end testing using Python and Kuzu
"""

import os
import sys
import subprocess
import tempfile
import shutil
from pathlib import Path
from typing import Optional, Dict, Any, List
import kuzu

class AnalysisFramework:
    """Base class for CppGraphIndex analysis"""
    
    def __init__(self, project_root: str = None):
        if project_root is None:
            # Auto-detect project root - go up from Examples/analysis to project root
            current_dir = Path(__file__).parent.absolute()
            self.project_root = current_dir.parent.parent
        else:
            self.project_root = Path(project_root)
        
        self.makeindex_path = self.project_root / "artifacts" / "debug" / "bin" / "MakeIndex.exe"
        self.example_data_path = self.project_root / "Examples" / "cpp"
        self.temp_db_path = None
        self.db = None
        self.conn = None
        
    def setup_example_database(self) -> str:
        """Create a temporary database and run MakeIndex on the example files"""
        # Create temporary directory for database
        self.temp_db_path = tempfile.mkdtemp(prefix="kuzu_test_")
        db_path = os.path.join(self.temp_db_path, "test_db")
        
        # Verify MakeIndex exists
        if not self.makeindex_path.exists():
            raise FileNotFoundError(f"MakeIndex not found at {self.makeindex_path}. Run 'please build' first.")
        
        # Run MakeIndex on the compilation database
        compile_commands_path = self.example_data_path / "compilation" / "comprehensive_no_std_compile_commands.json"
        if not compile_commands_path.exists():
            raise FileNotFoundError(f"Compilation database not found at {compile_commands_path}")
        
        print(f"Running MakeIndex on {compile_commands_path}...")
        print(f"Output database: {db_path}")
        
        # Execute MakeIndex
        cmd = [
            str(self.makeindex_path),
            str(compile_commands_path),
            "--output-db", db_path
        ]
        
        try:
            result = subprocess.run(cmd, 
                                  capture_output=True, 
                                  text=True, 
                                  cwd=str(self.project_root),
                                  timeout=300)  # 5 minute timeout
            
            if result.returncode != 0:
                print(f"MakeIndex stdout: {result.stdout}")
                print(f"MakeIndex stderr: {result.stderr}")
                raise RuntimeError(f"MakeIndex failed with return code {result.returncode}")
            
            print("MakeIndex completed successfully")
            print(f"stdout: {result.stdout}")
            
        except subprocess.TimeoutExpired:
            raise RuntimeError("MakeIndex timed out after 5 minutes")
        
        # Connect to the database
        self.db = kuzu.Database(db_path)
        self.conn = kuzu.Connection(self.db)
        
        return db_path
    
    def cleanup_example_database(self):
        """Clean up temporary database"""
        if self.conn:
            self.conn.close()
            self.conn = None
        if self.db:
            self.db.close()
            self.db = None
        if self.temp_db_path and os.path.exists(self.temp_db_path):
            shutil.rmtree(self.temp_db_path)
            self.temp_db_path = None
    
    def query(self, cypher_query: str) -> kuzu.QueryResult:
        """Execute a Cypher query and return results"""
        if not self.conn:
            raise RuntimeError("Database not connected. Call setup_test_database() first.")
        
        return self.conn.execute(cypher_query)
    
    def query_to_list(self, cypher_query: str) -> List[Dict[str, Any]]:
        """Execute a query and return results as a list of dictionaries"""
        result = self.query(cypher_query)
        results = []
        try:
            columns = result.get_column_names()
            for row in result:
                row_dict = {}
                for i, value in enumerate(row):
                    if i < len(columns):
                        row_dict[columns[i]] = value
                results.append(row_dict)
        except Exception as e:
            print(f"Error converting query results: {e}")
        return results
    
    def query_count(self, cypher_query: str) -> int:
        """Execute a query and return the count of results"""
        try:
            results = self.query_to_list(cypher_query)
            
            # If this is a count query that returns a single value, extract it
            if len(results) == 1 and len(results[0]) == 1:
                count_value = list(results[0].values())[0]
                # Handle different types of return values
                if isinstance(count_value, (int, float)):
                    return int(count_value)
                elif isinstance(count_value, str) and count_value.isdigit():
                    return int(count_value)
                elif count_value is None:
                    return 0
                else:
                    # If it's not a simple count value (e.g., a dict/object), treat as one result
                    return 1
            
            # For non-count queries, return the number of result rows
            return len(results)
        except Exception as e:
            print(f"Query error: {e}")
            return 0
    
    def assert_query_count(self, cypher_query: str, expected_count: int, message: str = ""):
        """Assert that a query returns the expected number of results"""
        actual_count = self.query_count(cypher_query)
        if actual_count != expected_count:
            raise AssertionError(f"{message}: Expected {expected_count} results, got {actual_count}. Query: {cypher_query}")
    
    def assert_query_min_count(self, cypher_query: str, min_count: int, message: str = ""):
        """Assert that a query returns at least the minimum number of results"""
        actual_count = self.query_count(cypher_query)
        if actual_count < min_count:
            raise AssertionError(f"{message}: Expected at least {min_count} results, got {actual_count}. Query: {cypher_query}")
    
    def assert_query_has_results(self, cypher_query: str, message: str = ""):
        """Assert that a query returns at least one result"""
        self.assert_query_min_count(cypher_query, 1, message)
    
    def assert_query_no_results(self, cypher_query: str, message: str = ""):
        """Assert that a query returns no results"""
        self.assert_query_count(cypher_query, 0, message)
    
    def get_table_info(self) -> Dict[str, int]:
        """Get information about all tables in the database"""
        tables = {}
        
        # Get node table counts
        node_tables = [
            "ASTNode", "Declaration", "Type", "Statement", "Expression",
            "TemplateParameter", "UsingDeclaration", "MacroDefinition",
            "IncludeDirective", "Comment", "ConstantExpression", "CFGBlock"
        ]
        
        for table in node_tables:
            try:
                # Use a simple count query
                result_list = self.query_to_list(f"MATCH (n:{table}) RETURN count(n) as count")
                if result_list and 'count' in result_list[0]:
                    tables[table] = result_list[0]['count']
                else:
                    tables[table] = 0
            except Exception as e:
                tables[table] = f"Error: {e}"
        
        return tables
    
    def print_database_summary(self):
        """Print a summary of the database contents"""
        print("\n=== Database Summary ===")
        table_info = self.get_table_info()
        
        for table, count in table_info.items():
            print(f"{table}: {count}")
        
        print("\n=== Sample ASTNode entries ===")
        try:
            sample_nodes = self.query_to_list("MATCH (n:ASTNode) RETURN n.node_type as node_type, n.source_file as source_file, n.start_line as start_line LIMIT 10")
            for node in sample_nodes:
                print(f"  {node['node_type']} in {node['source_file']}:{node['start_line']}")
        except Exception as e:
            print(f"Error getting sample nodes: {e}")


class BaseAnalyzer:
    """Base class for individual analyzers"""
    
    def __init__(self, framework: AnalysisFramework):
        self.framework = framework
        self.analyzer_name = self.__class__.__name__
    
    def setup(self):
        """Setup method called before running the test"""
        pass
    
    def run(self):
        """Main test method - should be overridden by subclasses"""
        raise NotImplementedError("Subclasses must implement run() method")
    
    def teardown(self):
        """Teardown method called after running the test"""
        pass
    
    def execute(self):
        """Execute the complete analyzer lifecycle"""
        print(f"\n--- Running {self.analyzer_name} ---")
        try:
            self.setup()
            self.run()
            print(f"[PASS] {self.analyzer_name}")
            return True
        except Exception as e:
            print(f"[FAIL] {self.analyzer_name}: {e}")
            return False
        finally:
            self.teardown()


def run_analysis_suite():
    """Run all available analyzers"""
    framework = AnalysisFramework()
    
    try:
        # Setup the example database
        framework.setup_example_database()
        framework.print_database_summary()
        
        # Import and run all analyzer modules
        analyzer_modules = [
            'analyzers.ast_analysis',
            'analyzers.inheritance_analysis',
            'analyzers.template_analysis', 
            'analyzers.namespace_analysis',
            'analyzers.control_flow_analysis',
            'analyzers.expression_analysis',
            'analyzers.preprocessor_analysis',
            'analyzers.type_analysis',
            'analyzers.declaration_analysis',
            'analyzers.statement_analysis'
        ]
        
        passed = 0
        failed = 0
        
        for module_name in analyzer_modules:
            try:
                module = __import__(module_name, fromlist=[''])
                # Get the class name from module name (e.g., ast_analysis -> AstAnalysis)
                class_name = ''.join(word.capitalize() for word in module_name.split('.')[-1].split('_'))
                analyzer_class = getattr(module, class_name)
                analyzer = analyzer_class(framework)
                if analyzer.execute():
                    passed += 1
                else:
                    failed += 1
            except ImportError as e:
                print(f"Warning: Analyzer module {module_name} not found, skipping... ({e})")
                continue
            except Exception as e:
                print(f"Error running analyzer {module_name}: {e}")
                failed += 1
        
        print(f"\n=== Analysis Results ===")
        print(f"Passed: {passed}")
        print(f"Failed: {failed}")
        print(f"Total: {passed + failed}")
        
        return failed == 0
        
    finally:
        framework.cleanup_example_database()


if __name__ == "__main__":
    success = run_test_suite()
    sys.exit(0 if success else 1)
