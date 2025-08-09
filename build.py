#!/usr/bin/env python3
"""
CppGraphIndex Build Orchestrator

This script serves as the single entry point for all developer operations,
providing a unified interface for building, testing, and maintaining the project.
"""

import argparse
import sys
import os
import platform
import subprocess
import shutil
from pathlib import Path
from typing import List, Optional, Dict
import logging


class BuildOrchestrator:
    """Main build orchestration class that handles all build operations."""
    
    def __init__(self):
        self.project_root = Path(__file__).parent.absolute()
        self.artifacts_dir = self.project_root / "artifacts"
        self.platform_info = self._detect_platform()
        self.setup_logging()
        
    def setup_logging(self):
        """Configure logging with both console and file output."""
        # Create logs directory if it doesn't exist
        log_dir = self.artifacts_dir / "debug" / "logs"
        log_dir.mkdir(parents=True, exist_ok=True)
        
        # Configure logging
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s - %(levelname)s - %(message)s',
            handlers=[
                logging.StreamHandler(sys.stdout),
                logging.FileHandler(log_dir / "build.log")
            ]
        )
        self.logger = logging.getLogger(__name__)
        
    def _detect_platform(self) -> Dict[str, str]:
        """Detect platform and set platform-specific configurations."""
        system = platform.system().lower()
        platform_info = {
            'system': system,
            'architecture': platform.machine(),
            'executable_ext': '',
            'library_ext': '',
            'shared_library_ext': '',
        }
        
        if system == 'windows':
            platform_info.update({
                'executable_ext': '.exe',
                'library_ext': '.lib',
                'shared_library_ext': '.dll',
                'preferred_generator': 'Ninja',
                'default_compiler': 'MSVC',
                'shell_cmd': ['cmd', '/c'],
            })
        elif system == 'linux':
            platform_info.update({
                'executable_ext': '',
                'library_ext': '.a',
                'shared_library_ext': '.so',
                'preferred_generator': 'Ninja',
                'default_compiler': 'GCC',
                'shell_cmd': ['/bin/bash', '-c'],
            })
        elif system == 'darwin':
            platform_info.update({
                'executable_ext': '',
                'library_ext': '.a',
                'shared_library_ext': '.dylib',
                'preferred_generator': 'Ninja',
                'default_compiler': 'Clang',
                'shell_cmd': ['/bin/bash', '-c'],
            })
            
        return platform_info
        
    def get_executable_name(self, base_name: str) -> str:
        """Get platform-specific executable name."""
        return base_name + self.platform_info['executable_ext']
        
    def find_tool(self, tool_name: str) -> Optional[Path]:
        """Find a tool in the system PATH with platform-specific handling."""
        # Platform-specific tool name variations
        tool_variants = [tool_name]
        
        if self.platform_info['system'] == 'windows':
            if not tool_name.endswith('.exe'):
                tool_variants.append(tool_name + '.exe')
                
        # Search in PATH
        for variant in tool_variants:
            tool_path = shutil.which(variant)
            if tool_path:
                return Path(tool_path)
                
        return None
        
    def validate_environment(self):
        """Validate that the build environment is properly set up."""
        self.logger.info("Validating build environment...")
        
        # Check platform
        system = platform.system().lower()
        self.logger.info(f"Platform: {system}")
        
        if system == "windows":
            self.logger.info("[OK] Windows platform detected")
        elif system == "linux":
            self.logger.info("[OK] Linux platform detected")
        elif system == "darwin":
            self.logger.info("[OK] macOS platform detected")
        else:
            self.logger.error(f"[ERROR] Unsupported platform: {system}")
            return False
            
        # Check Python version
        python_version = sys.version_info
        if python_version.major >= 3 and python_version.minor >= 8:
            self.logger.info(f"[OK] Python {python_version.major}.{python_version.minor}.{python_version.micro}")
        else:
            self.logger.error(f"[ERROR] Python 3.8+ required, found {python_version.major}.{python_version.minor}.{python_version.micro}")
            return False
            
        return True
        
    def run_command(self, cmd: List[str], cwd: Optional[Path] = None, capture_output: bool = False, 
                   shell: bool = False, env: Optional[Dict[str, str]] = None) -> subprocess.CompletedProcess:
        """Run a command with proper logging and error handling."""
        if cwd is None:
            cwd = self.project_root
            
        # Convert Path objects to strings for subprocess
        cmd_str = [str(c) for c in cmd]
        
        self.logger.info(f"Running: {' '.join(cmd_str)}")
        self.logger.info(f"Working directory: {cwd}")
        
        # Set up environment
        run_env = os.environ.copy()
        if env:
            run_env.update(env)
            
        try:
            # Platform-specific command execution
            if shell and self.platform_info['system'] == 'windows':
                # On Windows, use cmd /c for shell commands
                full_cmd = ['cmd', '/c'] + cmd_str
            elif shell and self.platform_info['system'] in ['linux', 'darwin']:
                # On Unix-like systems, use bash -c
                full_cmd = ['/bin/bash', '-c', ' '.join(cmd_str)]
            else:
                full_cmd = cmd_str
                
            if capture_output:
                result = subprocess.run(full_cmd, cwd=cwd, capture_output=True, text=True, 
                                      check=False, env=run_env, shell=shell)
            else:
                result = subprocess.run(full_cmd, cwd=cwd, check=False, env=run_env, shell=shell)
                
            if result.returncode != 0:
                self.logger.error(f"Command failed with return code {result.returncode}")
                if capture_output and result.stderr:
                    self.logger.error(f"Error output: {result.stderr}")
            else:
                self.logger.info("Command completed successfully")
                
            return result
        except FileNotFoundError:
            self.logger.error(f"Command not found: {cmd_str[0]}")
            raise
        except Exception as e:
            self.logger.error(f"Unexpected error running command: {e}")
            raise
            
    def ensure_directory(self, path: Path) -> None:
        """Ensure a directory exists with proper error handling."""
        try:
            path.mkdir(parents=True, exist_ok=True)
            self.logger.debug(f"Ensured directory exists: {path}")
        except PermissionError:
            self.logger.error(f"Permission denied creating directory: {path}")
            raise
        except Exception as e:
            self.logger.error(f"Failed to create directory {path}: {e}")
            raise
            
    def get_build_directory(self, build_type: str = "debug") -> Path:
        """Get the build directory for a specific build type."""
        return self.artifacts_dir / build_type.lower() / "build"
        
    def get_output_directory(self, build_type: str = "debug", output_type: str = "bin") -> Path:
        """Get output directory for binaries, libraries, etc."""
        return self.artifacts_dir / build_type.lower() / output_type
            
    def cmd_setup(self, args):
        """Initial environment setup."""
        self.logger.info("=== Initial Environment Setup ===")
        
        if not self.validate_environment():
            return 1
            
        # Create artifact directories
        self.logger.info("Creating artifact directories...")
        for build_type in ["debug", "release"]:
            for subdir in ["build", "bin", "lib", "logs"]:
                dir_path = self.artifacts_dir / build_type / subdir
                dir_path.mkdir(parents=True, exist_ok=True)
                self.logger.info(f"Created: {dir_path}")
                
        # Create other artifact directories
        for subdir in ["lint", "format", "test"]:
            dir_path = self.artifacts_dir / subdir
            dir_path.mkdir(parents=True, exist_ok=True)
            self.logger.info(f"Created: {dir_path}")
            
        self.logger.info("Environment setup completed successfully!")
        return 0
        
    def cmd_info(self, args):
        """Display build environment information."""
        self.logger.info("=== Build Environment Information ===")
        
        # System information
        self.logger.info(f"Platform: {platform.system()} {platform.release()}")
        self.logger.info(f"Architecture: {platform.machine()}")
        self.logger.info(f"Python: {sys.version}")
        
        # Platform-specific information
        self.logger.info(f"Detected platform: {self.platform_info['system']}")
        self.logger.info(f"Preferred generator: {self.platform_info['preferred_generator']}")
        self.logger.info(f"Default compiler: {self.platform_info['default_compiler']}")
        self.logger.info(f"Executable extension: '{self.platform_info['executable_ext']}'")
        
        # Project information
        self.logger.info(f"Project root: {self.project_root}")
        self.logger.info(f"Artifacts directory: {self.artifacts_dir}")
        
        # Check for required tools with improved detection
        core_tools = ["cmake", "ninja", "git"]
        compiler_tools = []
        
        if self.platform_info['system'] == "windows":
            compiler_tools = ["cl"]  # MSVC compiler
        else:
            compiler_tools = ["gcc", "g++", "clang", "clang++"]
            
        all_tools = core_tools + compiler_tools
            
        self.logger.info("Tool availability:")
        for tool in all_tools:
            tool_path = self.find_tool(tool)
            if tool_path:
                try:
                    # Try to get version info
                    version_args = ["--version"]
                    if tool == "cl":  # MSVC compiler uses different syntax
                        version_args = []
                        
                    result = subprocess.run([str(tool_path)] + version_args, 
                                          capture_output=True, text=True, timeout=30)
                    if result.returncode == 0:
                        version_line = result.stdout.split('\n')[0].strip()
                        if not version_line and result.stderr:
                            version_line = result.stderr.split('\n')[0].strip()
                        self.logger.info(f"  [OK] {tool}: {version_line} ({tool_path})")
                    else:
                        self.logger.info(f"  [OK] {tool}: found at {tool_path}")
                except (subprocess.TimeoutExpired, Exception):
                    self.logger.info(f"  [OK] {tool}: found at {tool_path}")
            else:
                self.logger.info(f"  [MISSING] {tool}: not found in PATH")
                
        # Display build directories
        self.logger.info("Build directories:")
        for build_type in ["debug", "release"]:
            build_dir = self.get_build_directory(build_type)
            bin_dir = self.get_output_directory(build_type, "bin")
            self.logger.info(f"  {build_type.capitalize()}: {build_dir}")
            self.logger.info(f"  {build_type.capitalize()} output: {bin_dir}")
                
        return 0
        
    def cmd_configure(self, args):
        """Configure the build system."""
        self.logger.info("=== Build Configuration ===")
        
        if not self.validate_environment():
            return 1
            
        # Determine build type
        build_type = "Debug"  # Default
        if args.release:
            build_type = "Release"
        elif args.debug:
            build_type = "Debug"
            
        self.logger.info(f"Configuring for {build_type} build")
        
        # Get build directory
        build_dir = self.get_build_directory(build_type.lower())
        self.ensure_directory(build_dir)
        
        # Clean configure if requested
        if args.clean:
            self.logger.info("Cleaning build directory for fresh configuration")
            import shutil
            if build_dir.exists():
                shutil.rmtree(build_dir)
            self.ensure_directory(build_dir)
            
        # CMake configuration command
        cmake_cmd = [
            "cmake",
            "-S", str(self.project_root),
            "-B", str(build_dir),
            "-G", self.platform_info['preferred_generator'],
            f"-DCMAKE_BUILD_TYPE={build_type}"
        ]
        
        # Add platform-specific options
        if self.platform_info['system'] == 'windows':
            # Ensure we use the right runtime library
            cmake_cmd.extend([
                "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebugDLL" if build_type == "Debug" else "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded"
            ])
            
        self.logger.info("Running CMake configuration...")
        result = self.run_command(cmake_cmd)
        
        if result.returncode == 0:
            self.logger.info("CMake configuration completed successfully!")
            return 0
        else:
            self.logger.error("CMake configuration failed!")
            return 1
        
    def cmd_build(self, args):
        """Build the project."""
        self.logger.info("=== Project Build ===")
        
        # Determine build type
        build_type = "Debug"  # Default
        if args.release:
            build_type = "Release"
        elif args.debug:
            build_type = "Debug"
            
        self.logger.info(f"Building {build_type} configuration")
        
        # Get build directory
        build_dir = self.get_build_directory(build_type.lower())
        
        if not build_dir.exists():
            self.logger.error(f"Build directory not found: {build_dir}")
            self.logger.info("Run 'python build.py configure' first")
            return 1
            
        # CMake build command
        cmake_cmd = ["cmake", "--build", str(build_dir)]
        
        # Add target if specified
        if args.target:
            cmake_cmd.extend(["--target", args.target])
            
        # Add parallel jobs if specified
        if args.parallel:
            cmake_cmd.extend(["--parallel", str(args.parallel)])
        else:
            # Use reasonable default for parallel jobs
            import multiprocessing
            jobs = max(1, multiprocessing.cpu_count() - 1)
            cmake_cmd.extend(["--parallel", str(jobs)])
            
        self.logger.info("Running CMake build...")
        result = self.run_command(cmake_cmd)
        
        if result.returncode == 0:
            self.logger.info("Build completed successfully!")
            
            # Show output location
            output_dir = self.get_output_directory(build_type.lower(), "bin")
            self.logger.info(f"Executables available in: {output_dir}")
            return 0
        else:
            self.logger.error("Build failed!")
            
            # Provide helpful suggestions
            self.logger.info("Troubleshooting suggestions:")
            self.logger.info("1. Check if dependencies are properly downloaded (LLVM build may take 30+ minutes)")
            self.logger.info("2. Try running 'python build.py configure --clean' to reconfigure")
            self.logger.info("3. Check build logs in artifacts/debug/logs/")
            return 1
            
    def cmd_install_git_hooks(self, args):
        """Install git pre-commit hooks for automatic formatting checks."""
        self.logger.info("=== Installing Git Pre-commit Hooks ===")
        
        # Check if we're in a git repository
        git_dir = self.project_root / ".git"
        if not git_dir.exists():
            self.logger.error("Not a git repository - .git directory not found")
            return 1
            
        hooks_dir = git_dir / "hooks"
        hooks_dir.mkdir(exist_ok=True)
        
        # Create pre-commit hook
        pre_commit_hook = hooks_dir / "pre-commit"
        
        hook_content = f'''#!/bin/bash
# Pre-commit hook for CppGraphIndex formatting checks
# Generated by build system

echo "Running pre-commit formatting checks..."

# Change to project root
cd "{self.project_root}"

# Get staged C++ files
STAGED_FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E "\\.(cpp|h|hpp|cxx|cc)$")

if [ -z "$STAGED_FILES" ]; then
    echo "No C++ files staged for commit"
    exit 0
fi

echo "Checking formatting for staged files..."

# Run format check on staged files
python build.py format --check-only --files $STAGED_FILES

# Check the exit code
if [ $? -ne 0 ]; then
    echo ""
    echo "[ERROR] Pre-commit formatting check failed!"
    echo "Fix formatting issues before committing:"
    echo "  python build.py format"
    echo ""
    echo "Or skip this check with: git commit --no-verify"
    exit 1
fi

echo "[OK] Pre-commit formatting check passed!"
exit 0
'''
        
        try:
            with open(pre_commit_hook, 'w') as f:
                f.write(hook_content)
                
            # Make the hook executable (on Unix-like systems)
            if self.platform_info['system'] != 'windows':
                import stat
                pre_commit_hook.chmod(stat.S_IRWXU | stat.S_IRGRP | stat.S_IROTH)
                
            self.logger.info(f"[OK] Pre-commit hook installed: {pre_commit_hook}")
            self.logger.info("Formatting checks will now run automatically before each commit")
            self.logger.info("Use 'git commit --no-verify' to skip the formatting check if needed")
            return 0
            
        except Exception as e:
            self.logger.error(f"Failed to install pre-commit hook: {e}")
            return 1
            
        self.logger.info("Running CMake build...")
        result = self.run_command(cmake_cmd)
        
        if result.returncode == 0:
            self.logger.info("Build completed successfully!")
            
            # Show output location
            output_dir = self.get_output_directory(build_type.lower(), "bin")
            self.logger.info(f"Executables available in: {output_dir}")
            return 0
        else:
            self.logger.error("Build failed!")
            
            # Provide helpful suggestions
            self.logger.info("Troubleshooting suggestions:")
            self.logger.info("1. Check if dependencies are properly downloaded (LLVM build may take 30+ minutes)")
            self.logger.info("2. Try running 'python build.py configure --clean' to reconfigure")
            self.logger.info("3. Check build logs in artifacts/debug/logs/")
            return 1
        
    def cmd_clean(self, args):
        """Clean build artifacts."""
        self.logger.info("=== Clean Build Artifacts ===")
        
        import shutil
        
        # Clean debug and release build directories
        for build_type in ["debug", "release"]:
            build_dir = self.get_build_directory(build_type)
            if build_dir.exists():
                self.logger.info(f"Cleaning {build_type} build directory: {build_dir}")
                try:
                    shutil.rmtree(build_dir)
                    self.logger.info(f"[OK] Cleaned {build_type} build directory")
                except Exception as e:
                    self.logger.error(f"Failed to clean {build_type} build directory: {e}")
                    
            # Also clean output directories
            for output_type in ["bin", "lib"]:
                output_dir = self.get_output_directory(build_type, output_type)
                if output_dir.exists():
                    try:
                        shutil.rmtree(output_dir)
                        output_dir.mkdir(parents=True, exist_ok=True)
                        self.logger.info(f"[OK] Cleaned {build_type} {output_type} directory")
                    except Exception as e:
                        self.logger.error(f"Failed to clean {build_type} {output_type} directory: {e}")
                        
        self.logger.info("Clean completed!")
        return 0
        
    def cmd_test(self, args):
        """Run tests."""
        self.logger.info("=== Run Tests ===")
        self.logger.info("Note: Test execution will be implemented in Phase 4")
        return 0
        
    def cmd_format(self, args):
        """Format source code."""
        self.logger.info("=== Code Formatting ===")
        
        # Find clang-format tool
        clang_format_tool = self.find_tool("clang-format")
        if not clang_format_tool:
            self.logger.error("clang-format not found in PATH")
            self.logger.info("Please install clang-format to use code formatting")
            return 1
            
        self.logger.info(f"Using clang-format: {clang_format_tool}")
        
        # Determine files to format
        if args.files:
            # Use specified files
            files_to_format = []
            for file_pattern in args.files:
                file_path = Path(file_pattern)
                if file_path.is_absolute():
                    files_to_format.append(file_path)
                else:
                    files_to_format.append(self.project_root / file_path)
        else:
            # Find all source files in MakeIndex directory
            source_dirs = [self.project_root / "MakeIndex"]
            file_patterns = ["*.h", "*.cpp", "*.hpp", "*.cxx"]
            files_to_format = []
            
            for source_dir in source_dirs:
                if source_dir.exists():
                    for pattern in file_patterns:
                        files_to_format.extend(source_dir.glob(pattern))
                        
        if not files_to_format:
            self.logger.warning("No source files found to format")
            return 0
            
        self.logger.info(f"Found {len(files_to_format)} files to format")
        
        # Check if .clang-format exists
        clang_format_config = self.project_root / ".clang-format"
        if not clang_format_config.exists():
            self.logger.warning("No .clang-format configuration file found")
            self.logger.info("Using clang-format default configuration")
            
        # Format files
        if args.check_only:
            self.logger.info("Checking formatting (no changes will be made)...")
            # Use --dry-run and --Werror to check formatting
            cmd = [str(clang_format_tool), "--dry-run", "--Werror"]
        else:
            self.logger.info("Formatting files...")
            cmd = [str(clang_format_tool), "-i"]
            
        # Add files to command
        cmd.extend([str(f) for f in files_to_format])
        
        # Run clang-format
        result = self.run_command(cmd, capture_output=args.check_only)
        
        # Save formatting log
        format_log_dir = self.artifacts_dir / "format"
        self.ensure_directory(format_log_dir)
        format_log_file = format_log_dir / "format-check.log"
        
        if args.check_only and result.stdout:
            with open(format_log_file, 'w') as f:
                f.write(f"Format check results:\n")
                f.write(f"Files checked: {len(files_to_format)}\n")
                f.write(f"Return code: {result.returncode}\n\n")
                if result.stdout:
                    f.write("stdout:\n")
                    f.write(result.stdout)
                if result.stderr:
                    f.write("\nstderr:\n")
                    f.write(result.stderr)
        
        if result.returncode == 0:
            if args.check_only:
                self.logger.info("[OK] All files are properly formatted")
            else:
                self.logger.info(f"[OK] Successfully formatted {len(files_to_format)} files")
            return 0
        else:
            if args.check_only:
                self.logger.error("Formatting issues found!")
                if result.stderr:
                    self.logger.error("clang-format output:")
                    self.logger.error(result.stderr)
                self.logger.info("Run 'python build.py format' to fix formatting issues")
                self.logger.info(f"Detailed results saved to: {format_log_file}")
            else:
                self.logger.error("Formatting failed!")
            return 1
        
    def cmd_lint(self, args):
        """Lint source code."""
        self.logger.info("=== Code Linting ===")
        
        # Find clang-tidy tool
        clang_tidy_tool = self.find_tool("clang-tidy")
        if not clang_tidy_tool:
            self.logger.error("clang-tidy not found in PATH")
            self.logger.info("Please install clang-tidy to use code linting")
            return 1
            
        self.logger.info(f"Using clang-tidy: {clang_tidy_tool}")
        
        # Determine files to lint
        if args.files:
            # Use specified files
            files_to_lint = []
            for file_pattern in args.files:
                file_path = Path(file_pattern)
                if file_path.is_absolute():
                    files_to_lint.append(file_path)
                else:
                    files_to_lint.append(self.project_root / file_path)
        elif args.target:
            # Use specific target file
            target_file = Path(args.target)
            if target_file.is_absolute():
                files_to_lint = [target_file]
            else:
                files_to_lint = [self.project_root / target_file]
        else:
            # Find all source files in MakeIndex directory (only .cpp files for linting)
            source_dirs = [self.project_root / "MakeIndex"]
            file_patterns = ["*.cpp"]  # Only lint .cpp files to avoid header duplication
            files_to_lint = []
            
            for source_dir in source_dirs:
                if source_dir.exists():
                    for pattern in file_patterns:
                        files_to_lint.extend(source_dir.glob(pattern))
                        
        if not files_to_lint:
            self.logger.warning("No source files found to lint")
            return 0
            
        self.logger.info(f"Found {len(files_to_lint)} files to lint")
        
        # Check for compile_commands.json
        compile_commands = self.artifacts_dir / "debug" / "build" / "compile_commands.json"
        if not compile_commands.exists():
            self.logger.error("compile_commands.json not found")
            self.logger.info("Run 'python build.py configure' first to generate compilation database")
            return 1
            
        # Check if .clang-tidy exists
        clang_tidy_config = self.project_root / ".clang-tidy"
        if not clang_tidy_config.exists():
            self.logger.warning("No .clang-tidy configuration file found")
            self.logger.info("Using clang-tidy default configuration")
            
        # Prepare clang-tidy command
        build_dir = compile_commands.parent
        cmd = [str(clang_tidy_tool), f"-p={build_dir}"]
        
        if args.fix:
            cmd.extend(["--fix", "--fix-errors"])
            self.logger.info("Running clang-tidy with automatic fixes...")
        else:
            self.logger.info("Running clang-tidy analysis...")
            
        # Add files to command
        cmd.extend([str(f) for f in files_to_lint])
        
        # Run clang-tidy with output capture
        result = self.run_command(cmd, capture_output=True)
        
        # Save linting results
        lint_log_dir = self.artifacts_dir / "lint"
        self.ensure_directory(lint_log_dir)
        lint_log_file = lint_log_dir / "clang-tidy-output.txt"
        
        with open(lint_log_file, 'w') as f:
            f.write(f"Clang-tidy analysis results:\n")
            f.write(f"Files analyzed: {len(files_to_lint)}\n")
            f.write(f"Return code: {result.returncode}\n\n")
            if result.stdout:
                f.write("stdout:\n")
                f.write(result.stdout)
            if result.stderr:
                f.write("\nstderr:\n")
                f.write(result.stderr)
                
        # Filter and display output
        if result.stdout:
            filtered_output = self._filter_clang_tidy_output(result.stdout)
            if filtered_output.strip():
                self.logger.info("Clang-tidy analysis results:")
                print(filtered_output)
                
        self.logger.info(f"Detailed results saved to: {lint_log_file}")
        
        if result.returncode == 0:
            self.logger.info("[OK] Linting completed successfully!")
            return 0
        else:
            self.logger.warning("Linting completed with issues")
            if args.fix:
                self.logger.info("Some issues were automatically fixed")
            else:
                self.logger.info("Use --fix flag to automatically fix some issues")
            return 1
            
    def _filter_clang_tidy_output(self, output: str) -> str:
        """Filter clang-tidy output to remove noise."""
        filtered_lines = []
        for line in output.split('\n'):
            # Skip suppressed warnings messages and header filter messages
            if (not line.strip().startswith("Suppressed") and 
                "Use -header-filter=" not in line and
                "warnings generated" not in line and
                line.strip()):
                filtered_lines.append(line)
        return '\n'.join(filtered_lines)
    
    def cmd_compile_db(self, args):
        """Generate or manage compilation database."""
        self.logger.info("=== Compilation Database Management ===")
        
        # Determine build type
        if args.debug:
            build_type = "Debug"
        elif args.release:
            build_type = "Release"
        else:
            build_type = "Debug"  # Default
            
        build_dir = self.get_build_directory(build_type)
        compile_commands_src = build_dir / "compile_commands.json"
        
        # Check if build directory exists and is configured
        if not build_dir.exists() or not compile_commands_src.exists():
            self.logger.warning("Compilation database not found")
            self.logger.info("Running configure to generate compilation database...")
            
            # Run configure first
            configure_args = argparse.Namespace(build_type=build_type, clean=False)
            result = self.cmd_configure(configure_args)
            if result != 0:
                return result
                
        # Check again after configure
        if not compile_commands_src.exists():
            self.logger.error("Failed to generate compilation database")
            self.logger.info("Try running 'python build.py build' to trigger compilation")
            return 1
            
        self.logger.info(f"Found compilation database: {compile_commands_src}")
        
        # Copy to project root for easier tool access
        compile_commands_root = self.project_root / "compile_commands.json"
        
        if args.copy_to_root:
            try:
                import shutil
                shutil.copy2(compile_commands_src, compile_commands_root)
                self.logger.info(f"[OK] Copied compilation database to: {compile_commands_root}")
            except Exception as e:
                self.logger.error(f"Failed to copy compilation database: {e}")
                return 1
                
        # Display statistics
        try:
            import json
            with open(compile_commands_src, 'r') as f:
                compile_db = json.load(f)
                
            self.logger.info(f"Compilation database statistics:")
            self.logger.info(f"  Total entries: {len(compile_db)}")
            
            # Count by file type
            cpp_files = sum(1 for entry in compile_db if entry.get('file', '').endswith('.cpp'))
            h_files = sum(1 for entry in compile_db if entry.get('file', '').endswith('.h'))
            other_files = len(compile_db) - cpp_files - h_files
            
            self.logger.info(f"  C++ source files: {cpp_files}")
            self.logger.info(f"  Header files: {h_files}")
            if other_files > 0:
                self.logger.info(f"  Other files: {other_files}")
                
            # Show unique directories
            directories = set()
            for entry in compile_db:
                file_path = Path(entry.get('file', ''))
                if file_path.is_absolute():
                    directories.add(str(file_path.parent))
                    
            self.logger.info(f"  Source directories: {len(directories)}")
            
            if args.show_files:
                self.logger.info("\nFiles in compilation database:")
                for entry in compile_db:
                    file_path = entry.get('file', '')
                    if file_path:
                        try:
                            rel_path = Path(file_path).relative_to(self.project_root) if Path(file_path).is_absolute() else file_path
                            self.logger.info(f"  {rel_path}")
                        except ValueError:
                            # Path is not relative to project root
                            self.logger.info(f"  {file_path}")
                        
        except Exception as e:
            self.logger.warning(f"Could not parse compilation database: {e}")
            
        self.logger.info("[OK] Compilation database management completed!")
        return 0

    def cmd_install_git_hooks(self, args):
        """Install git pre-commit hooks."""
        self.logger.info("=== Installing Git Hooks ===")
        
        git_hooks_dir = self.project_root / ".git" / "hooks"
        if not git_hooks_dir.exists():
            self.logger.error("Git hooks directory not found")
            self.logger.info("Ensure you are in a git repository")
            return 1
            
        pre_commit_hook = git_hooks_dir / "pre-commit"
        
        # Create pre-commit hook script
        hook_content = '''#!/bin/sh
# Pre-commit hook for code formatting checks

echo "Running pre-commit formatting checks..."

# Get list of staged C++ files
STAGED_FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E "\\.(cpp|h|hpp)$" || true)

if [ -z "$STAGED_FILES" ]; then
    echo "[OK] No C++ files to check"
    exit 0
fi

echo "Checking formatting for staged C++ files..."

# Run format check
python build.py format --check-only --files $STAGED_FILES

if [ $? -ne 0 ]; then
    echo "[ERROR] Code formatting issues found!"
    echo "Run 'python build.py format' to fix formatting issues"
    echo "Then stage the corrected files and commit again"
    exit 1
fi

echo "[OK] All staged files are properly formatted"
exit 0
'''
        
        try:
            with open(pre_commit_hook, 'w') as f:
                f.write(hook_content)
                
            # Make executable (Unix-like systems)
            if not self.platform_info.is_windows:
                import stat
                pre_commit_hook.chmod(pre_commit_hook.stat().st_mode | stat.S_IEXEC)
                
            self.logger.info("[OK] Pre-commit hook installed successfully!")
            self.logger.info(f"Hook location: {pre_commit_hook}")
            return 0
            
        except Exception as e:
            self.logger.error(f"Failed to install pre-commit hook: {e}")
            return 1


def create_parser() -> argparse.ArgumentParser:
    """Create and configure the argument parser."""
    parser = argparse.ArgumentParser(
        description="CppGraphIndex Build Orchestrator",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python build.py setup                    # Initial environment setup
  python build.py info                     # Show environment information
  python build.py configure --debug        # Configure debug build
  python build.py build --release          # Build release version
  python build.py test                     # Run all tests
  python build.py format --check-only      # Check formatting
  python build.py lint --fix               # Run linter with fixes
  python build.py clean                    # Clean artifacts
        """
    )
    
    subparsers = parser.add_subparsers(dest='command', help='Available commands')
    
    # Setup command
    setup_parser = subparsers.add_parser('setup', help='Initial environment setup')
    
    # Info command
    info_parser = subparsers.add_parser('info', help='Display build environment info')
    
    # Configure command
    config_parser = subparsers.add_parser('configure', help='Configure build system')
    config_group = config_parser.add_mutually_exclusive_group()
    config_group.add_argument('--debug', action='store_true', help='Configure for debug build')
    config_group.add_argument('--release', action='store_true', help='Configure for release build')
    config_parser.add_argument('--clean', action='store_true', help='Clean configure from scratch')
    
    # Build command
    build_parser = subparsers.add_parser('build', help='Build the project')
    build_group = build_parser.add_mutually_exclusive_group()
    build_group.add_argument('--debug', action='store_true', help='Build debug version')
    build_group.add_argument('--release', action='store_true', help='Build release version')
    build_parser.add_argument('--target', help='Build specific target')
    build_parser.add_argument('--parallel', type=int, help='Number of parallel jobs')
    
    # Clean command
    clean_parser = subparsers.add_parser('clean', help='Clean build artifacts')
    
    # Test command
    test_parser = subparsers.add_parser('test', help='Run tests')
    test_parser.add_argument('--parallel', type=int, help='Number of parallel test jobs')
    test_parser.add_argument('--verbose', action='store_true', help='Verbose test output')
    test_parser.add_argument('--target', help='Run specific test')
    
    # Format command
    format_parser = subparsers.add_parser('format', help='Format source code')
    format_parser.add_argument('--check-only', action='store_true', help='Check formatting without changes')
    format_parser.add_argument('--files', nargs='*', help='Specific files to format')
    
    # Lint command
    lint_parser = subparsers.add_parser('lint', help='Lint source code')
    lint_parser.add_argument('--fix', action='store_true', help='Apply automatic fixes')
    lint_parser.add_argument('--files', nargs='*', help='Specific files to lint')
    lint_parser.add_argument('--target', help='Lint specific file')
    
    # Compilation Database command
    compile_db_parser = subparsers.add_parser('compile-db', help='Generate and manage compilation database')
    compile_db_group = compile_db_parser.add_mutually_exclusive_group()
    compile_db_group.add_argument('--debug', action='store_true', help='Use debug build database')
    compile_db_group.add_argument('--release', action='store_true', help='Use release build database')
    compile_db_parser.add_argument('--copy-to-root', action='store_true', help='Copy database to project root')
    compile_db_parser.add_argument('--show-files', action='store_true', help='Show all files in database')
    
    # Git Integration Commands
    git_hooks_parser = subparsers.add_parser('install-git-hooks', help='Install git pre-commit hooks')
    
    return parser


def main():
    """Main entry point."""
    parser = create_parser()
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
        
    orchestrator = BuildOrchestrator()
    
    # Map commands to methods
    command_map = {
        'setup': orchestrator.cmd_setup,
        'info': orchestrator.cmd_info,
        'configure': orchestrator.cmd_configure,
        'build': orchestrator.cmd_build,
        'clean': orchestrator.cmd_clean,
        'test': orchestrator.cmd_test,
        'format': orchestrator.cmd_format,
        'lint': orchestrator.cmd_lint,
        'compile-db': orchestrator.cmd_compile_db,
        'install-git-hooks': orchestrator.cmd_install_git_hooks,
    }
    
    try:
        command_func = command_map[args.command]
        return command_func(args)
    except KeyError:
        orchestrator.logger.error(f"Unknown command: {args.command}")
        return 1
    except Exception as e:
        orchestrator.logger.error(f"Unexpected error: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
