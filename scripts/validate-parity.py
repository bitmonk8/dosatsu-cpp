#!/usr/bin/env python3
"""
Automated Feature Parity Validation Script
Validates that Meson build system has complete feature parity with xmake.
"""

import os
import sys
import subprocess
import tempfile
import time
import json
import argparse
from pathlib import Path
from typing import Dict, List, Tuple, Optional

class ParityValidator:
    """Validates feature parity between xmake and Meson build systems."""
    
    def __init__(self, project_root: str):
        self.project_root = Path(project_root).resolve()
        self.results = {}
        self.errors = []
        
    def run_cmd(self, cmd: List[str], cwd: Optional[Path] = None, timeout: int = 60) -> Tuple[int, str, str]:
        """Run a command and return (returncode, stdout, stderr)."""
        if cwd is None:
            cwd = self.project_root
            
        try:
            result = subprocess.run(
                cmd, 
                cwd=cwd, 
                capture_output=True, 
                text=True, 
                timeout=timeout,
                shell=True if os.name == 'nt' else False
            )
            return result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            return -1, "", f"Command timed out after {timeout}s"
        except Exception as e:
            return -1, "", str(e)
    
    def validate_build_targets(self) -> bool:
        """Validate that both build systems can build the main target."""
        print("[INFO] Validating build targets...")
        
        # Test xmake build
        print("  Testing xmake build...")
        ret, out, err = self.run_cmd(["xmake", "clean"])
        if ret != 0:
            self.errors.append(f"xmake clean failed: {err}")
            return False
            
        ret, out, err = self.run_cmd(["xmake"])
        xmake_build_success = ret == 0
        if not xmake_build_success:
            self.errors.append(f"xmake build failed: {err}")
        
        # Test Meson build
        print("  Testing Meson build...")
        
        # Clean previous builds
        builddir = self.project_root / "builddir"
        if builddir.exists():
            ret, out, err = self.run_cmd(["ninja", "-C", "builddir", "clean"])
        
        # Setup and build with Meson
        ret, out, err = self.run_cmd(["meson", "setup", "builddir", "--buildtype=debug"])
        if ret != 0:
            self.errors.append(f"meson setup failed: {err}")
            return False
            
        ret, out, err = self.run_cmd(["ninja", "-C", "builddir"])
        meson_build_success = ret == 0
        if not meson_build_success:
            self.errors.append(f"ninja build failed: {err}")
        
        success = xmake_build_success and meson_build_success
        self.results['build_targets'] = {
            'xmake': xmake_build_success,
            'meson': meson_build_success,
            'parity': success
        }
        
        status = "PASS" if success else "FAIL"
        print(f"  [SUCCESS] Build targets: {status}")
        return success
    
    def validate_build_modes(self) -> bool:
        """Validate debug and release build modes."""
        print("[INFO] Validating build modes...")
        
        success = True
        
        # Test xmake modes
        print("  Testing xmake debug mode...")
        ret, out, err = self.run_cmd(["xmake", "f", "-m", "debug"])
        if ret == 0:
            ret, out, err = self.run_cmd(["xmake"])
        xmake_debug = ret == 0
        
        print("  Testing xmake release mode...")
        ret, out, err = self.run_cmd(["xmake", "f", "-m", "release"])
        if ret == 0:
            ret, out, err = self.run_cmd(["xmake"])
        xmake_release = ret == 0
        
        # Reset to debug
        self.run_cmd(["xmake", "f", "-m", "debug"])
        
        # Test Meson modes
        print("  Testing Meson debug mode...")
        ret, out, err = self.run_cmd(["meson", "setup", "builddir_debug", "--buildtype=debug"])
        if ret == 0:
            ret, out, err = self.run_cmd(["ninja", "-C", "builddir_debug"])
        meson_debug = ret == 0
        
        print("  Testing Meson release mode...")
        ret, out, err = self.run_cmd(["meson", "setup", "builddir_release", "--buildtype=release"])
        if ret == 0:
            ret, out, err = self.run_cmd(["ninja", "-C", "builddir_release"])
        meson_release = ret == 0
        
        success = xmake_debug and xmake_release and meson_debug and meson_release
        
        self.results['build_modes'] = {
            'xmake_debug': xmake_debug,
            'xmake_release': xmake_release,
            'meson_debug': meson_debug,
            'meson_release': meson_release,
            'parity': success
        }
        
        status = "PASS" if success else "FAIL"
        print(f"  [SUCCESS] Build modes: {status}")
        return success
    
    def validate_development_tools(self) -> bool:
        """Validate format, lint, and test tools."""
        print("[INFO] Validating development tools...")
        
        # Ensure we have a working build for tools
        ret, out, err = self.run_cmd(["meson", "setup", "builddir", "--buildtype=debug"])
        if ret != 0:
            self.errors.append(f"Could not setup builddir for tools: {err}")
            return False
        
        success = True
        tools_results = {}
        
        # Test format
        print("  Testing format tools...")
        ret, out, err = self.run_cmd(["xmake", "run", "format"])
        xmake_format = ret == 0
        
        ret, out, err = self.run_cmd(["ninja", "-C", "builddir", "format"])
        meson_format = ret == 0
        
        tools_results['format'] = {
            'xmake': xmake_format,
            'meson': meson_format,
            'parity': xmake_format and meson_format
        }
        
        # Test lint
        print("  Testing lint tools...")
        ret, out, err = self.run_cmd(["xmake", "run", "lint"], timeout=120)
        xmake_lint = ret == 0
        
        ret, out, err = self.run_cmd(["ninja", "-C", "builddir", "lint"], timeout=120)
        meson_lint = ret == 0
        
        tools_results['lint'] = {
            'xmake': xmake_lint,
            'meson': meson_lint,
            'parity': xmake_lint and meson_lint
        }
        
        # Test testing
        print("  Testing test tools...")
        ret, out, err = self.run_cmd(["xmake", "test"])
        xmake_test = ret == 0
        
        ret, out, err = self.run_cmd(["meson", "test", "-C", "builddir"])
        meson_test = ret == 0
        
        tools_results['test'] = {
            'xmake': xmake_test,
            'meson': meson_test,
            'parity': xmake_test and meson_test
        }
        
        success = all(tool['parity'] for tool in tools_results.values())
        self.results['development_tools'] = tools_results
        
        status = "PASS" if success else "FAIL"
        print(f"  [SUCCESS] Development tools: {status}")
        return success
    
    def validate_output_consistency(self) -> bool:
        """Validate that both build systems produce working executables."""
        print("[INFO] Validating output consistency...")
        
        # Build both systems
        ret, out, err = self.run_cmd(["xmake"])
        if ret != 0:
            self.errors.append(f"xmake build failed for output test: {err}")
            return False
        
        ret, out, err = self.run_cmd(["ninja", "-C", "builddir"])
        if ret != 0:
            self.errors.append(f"meson build failed for output test: {err}")
            return False
        
        # Find executables
        xmake_exe = None
        meson_exe = None
        
        # Find xmake executable
        if os.name == 'nt':
            patterns = ['build/windows/x86_64/debug/MakeIndex.exe', 'build/windows/*/debug/MakeIndex.exe']
        else:
            patterns = ['build/linux/x86_64/debug/MakeIndex', 'build/*/debug/MakeIndex']
        
        for pattern in patterns:
            candidates = list(self.project_root.glob(pattern))
            if candidates:
                xmake_exe = candidates[0]
                break
        
        # Find meson executable
        meson_exe = self.project_root / "builddir" / "MakeIndex" / "makeindex_exe"
        if os.name == 'nt':
            meson_exe = meson_exe.with_suffix('.exe')
        
        if not meson_exe.exists():
            # Try alternative locations
            candidates = list((self.project_root / "builddir").glob("**/MakeIndex*"))
            if candidates:
                meson_exe = candidates[0]
        
        success = True
        
        # Test xmake executable
        if xmake_exe and xmake_exe.exists():
            ret, out, err = self.run_cmd([str(xmake_exe), "--selftest"], timeout=30)
            xmake_exec_success = ret == 0
        else:
            xmake_exec_success = False
            self.errors.append("Could not find xmake executable")
        
        # Test meson executable
        if meson_exe and meson_exe.exists():
            ret, out, err = self.run_cmd([str(meson_exe), "--selftest"], timeout=30)
            meson_exec_success = ret == 0
        else:
            meson_exec_success = False
            self.errors.append("Could not find meson executable")
        
        success = xmake_exec_success and meson_exec_success
        
        self.results['output_consistency'] = {
            'xmake_executable': xmake_exec_success,
            'meson_executable': meson_exec_success,
            'xmake_path': str(xmake_exe) if xmake_exe else 'Not found',
            'meson_path': str(meson_exe) if meson_exe else 'Not found',
            'parity': success
        }
        
        status = "PASS" if success else "FAIL"
        print(f"  [SUCCESS] Output consistency: {status}")
        return success
    
    def validate_dependencies(self) -> bool:
        """Validate that dependencies are properly configured."""
        print("[INFO] Validating dependencies...")
        
        success = True
        
        # Check if Conan dependencies are installed
        ret, out, err = self.run_cmd(["conan", "install", ".", "--build=missing"], timeout=300)
        conan_deps = ret == 0
        if not conan_deps:
            self.errors.append(f"Conan dependency installation failed: {err}")
        
        # Check if LLVM is properly linked in both builds
        # This is implicit if builds succeed, but we can check for key libraries
        
        self.results['dependencies'] = {
            'conan_install': conan_deps,
            'parity': conan_deps  # If conan works and builds work, deps are good
        }
        
        status = "PASS" if conan_deps else "FAIL"
        print(f"  [SUCCESS] Dependencies: {status}")
        return conan_deps
    
    def run_full_validation(self) -> bool:
        """Run complete feature parity validation."""
        print("[START] Starting Feature Parity Validation")
        print("=" * 50)
        
        start_time = time.time()
        
        # Run all validation tests
        validations = [
            ('Build Targets', self.validate_build_targets),
            ('Build Modes', self.validate_build_modes),
            ('Development Tools', self.validate_development_tools),
            ('Output Consistency', self.validate_output_consistency),
            ('Dependencies', self.validate_dependencies),
        ]
        
        all_passed = True
        
        for name, validator in validations:
            try:
                result = validator()
                if not result:
                    all_passed = False
                    print(f"[FAILED] {name} validation failed")
                else:
                    print(f"[PASS] {name} validation passed")
            except Exception as e:
                all_passed = False
                self.errors.append(f"{name} validation error: {str(e)}")
                print(f"[ERROR] {name} validation error: {str(e)}")
        
        end_time = time.time()
        duration = end_time - start_time
        
        print("=" * 50)
        print(f"[FINISH] Validation completed in {duration:.2f}s")
        
        if all_passed:
            print("[SUCCESS] All validations PASSED - Feature parity achieved!")
        else:
            print("[FAILED] Some validations FAILED")
            if self.errors:
                print("\nErrors encountered:")
                for error in self.errors:
                    print(f"  • {error}")
        
        return all_passed
    
    def generate_report(self) -> str:
        """Generate a detailed validation report."""
        report = []
        report.append("# Feature Parity Validation Report")
        report.append(f"Generated: {time.strftime('%Y-%m-%d %H:%M:%S')}")
        report.append("")
        
        for category, results in self.results.items():
            report.append(f"## {category.replace('_', ' ').title()}")
            
            if isinstance(results, dict):
                for key, value in results.items():
                    if key == 'parity':
                        status = "[PASS]" if value else "[FAIL]"
                        report.append(f"**Overall {category}**: {status}")
                    else:
                        status = "[PASS]" if value else "[FAIL]"
                        report.append(f"- {key}: {status}")
            report.append("")
        
        if self.errors:
            report.append("## Errors")
            for error in self.errors:
                report.append(f"- {error}")
            report.append("")
        
        return "\n".join(report)

def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(description="Validate feature parity between xmake and Meson build systems")
    parser.add_argument("project_root", nargs="?", default=os.getcwd(), 
                       help="Project root directory (default: current directory)")
    parser.add_argument("--ci-mode", action="store_true",
                       help="CI mode: simplified output, no report file generation")
    
    args = parser.parse_args()
    
    validator = ParityValidator(args.project_root)
    
    try:
        success = validator.run_full_validation()
        
        if not args.ci_mode:
            # Generate report file in non-CI mode
            report = validator.generate_report()
            report_file = Path(args.project_root) / "PARITY_VALIDATION_REPORT.md"
            
            with open(report_file, 'w') as f:
                f.write(report)
            
            print(f"\n[REPORT] Detailed report written to: {report_file}")
        else:
            # CI mode: just print summary
            print(f"\n[CI] Feature parity validation: {'PASSED' if success else 'FAILED'}")
        
        sys.exit(0 if success else 1)
        
    except KeyboardInterrupt:
        print("\n[WARNING] Validation interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"[ERROR] Unexpected error: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    main()