#!/usr/bin/env python3
"""
CI Validation Script
Validates that the CI/CD setup is properly configured and all components work together.
"""

import sys
import subprocess
import json
from pathlib import Path

def run_command(cmd, cwd=None, capture_output=True, timeout=None):
    """Run a command and return the result."""
    try:
        # Use much longer timeout for build commands, or no timeout at all
        actual_timeout = timeout
        if timeout is None and any(x in cmd for x in ['build', 'configure', 'test']):
            actual_timeout = 3600  # 1 hour for build operations
        elif timeout is None:
            actual_timeout = 300   # 5 minutes for other operations
            
        result = subprocess.run(
            cmd,
            shell=True,
            cwd=cwd,
            capture_output=capture_output,
            text=True,
            timeout=actual_timeout
        )
        return result
    except subprocess.TimeoutExpired:
        print(f"⏰ Command timed out after {actual_timeout}s: {cmd}")
        return None
    except Exception as e:
        print(f"❌ Command failed: {cmd} - {e}")
        return None

def validate_yaml_syntax():
    """Validate GitHub Actions workflow YAML syntax."""
    print("🔍 Validating CI workflow YAML syntax...")
    
    workflow_file = Path(".github/workflows/ci.yml")
    if not workflow_file.exists():
        print("❌ CI workflow file not found")
        return False
    
    try:
        import yaml
        with open(workflow_file, 'r', encoding='utf-8') as f:
            yaml.safe_load(f.read())
        print("✅ CI workflow YAML is valid")
        return True
    except Exception as e:
        print(f"❌ CI workflow YAML is invalid: {e}")
        return False

def validate_build_script():
    """Validate that the build script has required CI features."""
    print("🔍 Validating build script CI integration...")
    
    # Check if please.py exists
    build_script = Path("please.py")
    if not build_script.exists():
        print("❌ please.py not found")
        return False
    
    # Check for CI-related features in build script
    with open(build_script, 'r', encoding='utf-8') as f:
        content = f.read()
    
    required_features = [
        ('--ci-mode', 'CI mode support'),
        ('--report-format', 'Multiple report formats'),
        ('--historical', 'Historical tracking'),
        ('cmd_info', 'Environment info command'),
        ('_generate_ci_test_summary', 'CI test summaries'),
        ('_generate_enhanced_test_reports', 'Enhanced reporting'),
    ]
    
    all_found = True
    for feature, description in required_features:
        if feature in content:
            print(f"✅ {description}: Found")
        else:
            print(f"❌ {description}: Missing")
            all_found = False
    
    return all_found

def validate_project_structure():
    """Validate project structure for CI compatibility."""
    print("🔍 Validating project structure...")
    
    required_files = [
        'CMakeLists.txt',
        'please.py',
        '.github/workflows/ci.yml',
        'third_party/dependencies.cmake',
        'scripts/setup_deps.cmake'
    ]
    
    required_dirs = [
        'src',
        '.github/workflows',
        'third_party',
        'scripts'
    ]
    
    all_good = True
    
    for file_path in required_files:
        if Path(file_path).exists():
            print(f"✅ Required file: {file_path}")
        else:
            print(f"❌ Missing file: {file_path}")
            all_good = False
    
    for dir_path in required_dirs:
        if Path(dir_path).exists():
            print(f"✅ Required directory: {dir_path}")
        else:
            print(f"❌ Missing directory: {dir_path}")
            all_good = False
    
    return all_good

def test_local_ci_simulation():
    """Test CI-like commands locally."""
    print("🔍 Testing CI command simulation...")
    
    test_commands = [
        "python please.py info",
        "python please.py configure --debug --clean",
        "python please.py build --debug",
        "python please.py test --ci-mode --report-format json",
        "python please.py format --check-only",
    ]
    
    all_passed = True
    for cmd in test_commands:
        print(f"  Running: {cmd}")
        result = run_command(cmd, capture_output=True)
        if result and result.returncode == 0:
            print(f"  ✅ Success")
        else:
            print(f"  ❌ Failed (exit code: {result.returncode if result else 'timeout'})")
            if result and result.stderr:
                print(f"     Error: {result.stderr[:200]}...")
            all_passed = False
    
    return all_passed

def validate_artifact_generation():
    """Validate that CI artifacts are generated correctly."""
    print("🔍 Validating artifact generation...")
    
    expected_artifacts = [
        'artifacts/test/test-report.json',
        'artifacts/test/test-report.html',
        'artifacts/test/ci-summary.json',
        'artifacts/test/results.xml'
    ]
    
    all_found = True
    for artifact in expected_artifacts:
        if Path(artifact).exists():
            print(f"✅ Artifact generated: {artifact}")
            
            # Validate JSON artifacts
            if artifact.endswith('.json'):
                try:
                    with open(artifact, 'r', encoding='utf-8') as f:
                        json.load(f)
                    print(f"  ✅ Valid JSON format")
                except:
                    print(f"  ❌ Invalid JSON format")
                    all_found = False
                    
        else:
            print(f"❌ Missing artifact: {artifact}")
            all_found = False
    
    return all_found

def generate_ci_report():
    """Generate a CI validation report."""
    print("\n📊 Generating CI Validation Report...")
    
    report = {
        "timestamp": "2025-08-11T15:15:00Z",
        "validation_results": {
            "yaml_syntax": False,
            "build_script": False,
            "project_structure": False,
            "local_simulation": False,
            "artifact_generation": False
        },
        "overall_status": "unknown"
    }
    
    # Run all validations
    report["validation_results"]["yaml_syntax"] = validate_yaml_syntax()
    report["validation_results"]["build_script"] = validate_build_script()
    report["validation_results"]["project_structure"] = validate_project_structure()
    report["validation_results"]["local_simulation"] = test_local_ci_simulation()
    report["validation_results"]["artifact_generation"] = validate_artifact_generation()
    
    # Determine overall status
    all_passed = all(report["validation_results"].values())
    report["overall_status"] = "success" if all_passed else "failure"
    
    # Save report
    report_file = Path("artifacts/ci-validation-report.json")
    report_file.parent.mkdir(parents=True, exist_ok=True)
    
    with open(report_file, 'w', encoding='utf-8') as f:
        json.dump(report, f, indent=2)
    
    print(f"\n📄 CI validation report saved to: {report_file}")
    
    return all_passed

def main():
    """Main validation function."""
    print("🚀 CI/CD Validation Script")
    print("=" * 50)
    
    # Change to project root
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    import os
    os.chdir(project_root)
    
    print(f"Project root: {project_root}")
    print()
    
    # Run validation
    success = generate_ci_report()
    
    print("\n" + "=" * 50)
    if success:
        print("🎉 All CI validations passed!")
        print("   The CI/CD pipeline is ready for deployment.")
        return 0
    else:
        print("❌ Some CI validations failed!")
        print("   Please fix the issues before deploying.")
        return 1

if __name__ == "__main__":
    sys.exit(main())
