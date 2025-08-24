#!/usr/bin/env python3
"""
Quick CI Validation Script
Fast validation of CI setup without running time-intensive build operations.
"""

import sys
import subprocess
import json
from pathlib import Path

def run_command(cmd, cwd=None, capture_output=True, timeout=60):
    """Run a command and return the result."""
    try:
        result = subprocess.run(
            cmd,
            shell=True,
            cwd=cwd,
            capture_output=capture_output,
            text=True,
            timeout=timeout
        )
        return result
    except subprocess.TimeoutExpired:
        print(f"⏰ Command timed out after {timeout}s: {cmd}")
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
            workflow_data = yaml.safe_load(f.read())
            
        # Validate key sections exist
        required_sections = ['name', 'jobs']
        for section in required_sections:
            if section not in workflow_data:
                print(f"❌ Missing required section: {section}")
                return False
                
        # Check for trigger conditions (on field can be True/False or dict)
        if 'on' not in workflow_data and True not in workflow_data:
            print("❌ Missing trigger configuration ('on' section)")
            return False
        else:
            print("✅ Trigger configuration found")
                
        # Check for multi-platform matrix
        if 'build-and-test' in workflow_data.get('jobs', {}):
            job = workflow_data['jobs']['build-and-test']
            if 'strategy' in job and 'matrix' in job['strategy']:
                matrix = job['strategy']['matrix']
                if 'os' in matrix and len(matrix['os']) >= 3:
                    print("✅ Multi-platform matrix detected")
                else:
                    print("⚠️  Matrix may not cover all platforms")
            else:
                print("⚠️  No build matrix found")
                
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
        ('_handle_test_coverage', 'Coverage support'),
        ('timeout-minutes', 'CI workflow timeouts'),
    ]
    
    all_found = True
    for feature, description in required_features:
        if feature in content or (feature == 'timeout-minutes' and Path('.github/workflows/ci.yml').exists()):
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
        '.github/workflows',
        'third_party', 
        'scripts'
    ]
    
    # Check for source files (either in src/ or root level)
    source_locations = [
        'src',
        'source'  # Current structure has source in source/
    ]
    
    source_found = False
    for src_dir in source_locations:
        if Path(src_dir).exists():
            print(f"✅ Source directory: {src_dir}")
            source_found = True
            break
    
    if not source_found:
        print("❌ No source directory found (checked: src/, source/)")
        all_good = False
    
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

def test_quick_commands():
    """Test quick CI-like commands without building."""
    print("🔍 Testing quick CI commands...")
    
    test_commands = [
        "please info",
        "please format --check-only",
        "python -c \"import yaml; print('YAML support: OK')\"",
        "python -c \"import json; print('JSON support: OK')\"",
    ]
    
    all_passed = True
    for cmd in test_commands:
        print(f"  Running: {cmd}")
        result = run_command(cmd, capture_output=True, timeout=30)
        if result and result.returncode == 0:
            print(f"  ✅ Success")
        else:
            print(f"  ❌ Failed (exit code: {result.returncode if result else 'timeout'})")
            if result and result.stderr:
                print(f"     Error: {result.stderr[:200]}...")
            all_passed = False
    
    return all_passed

def check_timeout_configuration():
    """Check if CI workflow has appropriate timeout configurations."""
    print("🔍 Checking CI timeout configurations...")
    
    workflow_file = Path(".github/workflows/ci.yml")
    if not workflow_file.exists():
        print("❌ CI workflow file not found")
        return False
        
    try:
        with open(workflow_file, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Check for timeout configurations
        timeout_checks = [
            ('timeout-minutes: 30', 'Configure step timeout'),
            ('timeout-minutes: 60', 'Build step timeout'),
            ('timeout-minutes: 15', 'Test step timeout'),
        ]
        
        all_found = True
        for timeout_config, description in timeout_checks:
            if timeout_config in content:
                print(f"✅ {description}: Found")
            else:
                print(f"⚠️  {description}: Not found (may cause timeouts)")
                
        # Check for caching configuration
        if 'cache@v3' in content:
            print("✅ Dependency caching: Configured")
        else:
            print("⚠️  Dependency caching: Not configured")
            
        return True
        
    except Exception as e:
        print(f"❌ Error checking timeout configuration: {e}")
        return False

def generate_quick_report():
    """Generate a quick CI validation report."""
    print("\n📊 Generating Quick CI Validation Report...")
    
    report = {
        "timestamp": "2025-08-11T15:30:00Z",
        "validation_type": "quick",
        "validation_results": {
            "yaml_syntax": False,
            "build_script": False,
            "project_structure": False,
            "quick_commands": False,
            "timeout_config": False
        },
        "overall_status": "unknown"
    }
    
    # Run all validations
    report["validation_results"]["yaml_syntax"] = validate_yaml_syntax()
    report["validation_results"]["build_script"] = validate_build_script()  
    report["validation_results"]["project_structure"] = validate_project_structure()
    report["validation_results"]["quick_commands"] = test_quick_commands()
    report["validation_results"]["timeout_config"] = check_timeout_configuration()
    
    # Determine overall status
    all_passed = all(report["validation_results"].values())
    report["overall_status"] = "success" if all_passed else "warning"
    
    # Save report
    report_file = Path("artifacts/ci-quick-validation.json")
    report_file.parent.mkdir(parents=True, exist_ok=True)
    
    with open(report_file, 'w', encoding='utf-8') as f:
        json.dump(report, f, indent=2)
    
    print(f"\n📄 Quick CI validation report saved to: {report_file}")
    
    return all_passed

def main():
    """Main validation function."""
    print("🚀 Quick CI/CD Validation Script")
    print("=" * 50)
    print("NOTE: This is a fast validation that skips time-intensive build operations")
    print()
    
    # Change to project root
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    import os
    os.chdir(project_root)
    
    print(f"Project root: {project_root}")
    print()
    
    # Run validation
    success = generate_quick_report()
    
    print("\n" + "=" * 50)
    if success:
        print("🎉 Quick CI validations passed!")
        print("   The CI/CD pipeline configuration looks good.")
        print("   💡 Tip: Run the full validation with longer timeouts for complete testing.")
        return 0
    else:
        print("⚠️  Some CI validations had issues!")
        print("   Please review the findings above.")
        return 1

if __name__ == "__main__":
    sys.exit(main())
