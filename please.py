#!/usr/bin/env python3
"""
Dosatsu Build Orchestrator

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
import time


class CleanLogger:
    """Custom logger that provides clean output without timestamps/levels for console."""
    
    def __init__(self, file_logger):
        self.file_logger = file_logger
        
    def info(self, message):
        # Clean output to console
        print(message)
        # Full logging to file
        self.file_logger.info(message)
        
    def warning(self, message):
        print(f"WARNING: {message}")
        self.file_logger.warning(message)
        
    def error(self, message):
        print(f"ERROR: {message}")
        self.file_logger.error(message)
        
    def debug(self, message):
        # Debug only goes to file
        self.file_logger.debug(message)


class BuildOrchestrator:
    """Main build orchestration class that handles all build operations."""
    
    def __init__(self):
        self.project_root = Path(__file__).parent.absolute()
        self.artifacts_dir = self.project_root / "artifacts"
        self.platform_info = self._detect_platform()
        self.start_time = None
        self.setup_logging()
        
    def setup_logging(self):
        """Configure logging with both console and file output."""
        # Create logs directory if it doesn't exist
        log_dir = self.artifacts_dir / "debug" / "logs"
        log_dir.mkdir(parents=True, exist_ok=True)
        
        # Configure file logging only (no console handler)
        file_logger = logging.getLogger(f"{__name__}_file")
        file_logger.setLevel(logging.INFO)
        
        # Remove any existing handlers
        if file_logger.handlers:
            file_logger.handlers.clear()
            
        # Add file handler with timestamp format
        file_handler = logging.FileHandler(log_dir / "build.log")
        file_formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
        file_handler.setFormatter(file_formatter)
        file_logger.addHandler(file_handler)
        
        # Use custom clean logger
        self.logger = CleanLogger(file_logger)
        
    def start_timing(self):
        """Start timing the current operation."""
        self.start_time = time.time()
        
    def print_execution_time(self, operation_name: str = "Operation"):
        """Print the total execution time."""
        if self.start_time is not None:
            elapsed_time = time.time() - self.start_time
            minutes = int(elapsed_time // 60)
            seconds = elapsed_time % 60
            if minutes > 0:
                print(f"{operation_name} completed successfully in {minutes}m {seconds:.2f}s")
            else:
                print(f"{operation_name} completed successfully in {seconds:.2f}s")
                
    def make_relative_path(self, path):
        """Convert absolute path to relative path from project root."""
        try:
            path_obj = Path(path)
            if path_obj.is_absolute() and path_obj.is_relative_to(self.project_root):
                return str(path_obj.relative_to(self.project_root))
            return str(path_obj)
        except (ValueError, OSError):
            return str(path)
    
    def _read_slow_checks_file(self):
        """Read the list of slow clang-tidy checks from .slow-clang-tidy-checks file."""
        slow_checks_file = self.project_root / ".slow-clang-tidy-checks"
        slow_checks = []
        
        if slow_checks_file.exists():
            try:
                with open(slow_checks_file, 'r', encoding='utf-8') as f:
                    for line in f:
                        line = line.strip()
                        # Skip empty lines and comments
                        if line and not line.startswith('#'):
                            slow_checks.append(line)
            except Exception as e:
                self.logger.warning(f"Failed to read {slow_checks_file}: {e}")
                
        return slow_checks
        
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
                   shell: bool = False, env: Optional[Dict[str, str]] = None, 
                   silent: bool = False, concise: bool = False) -> subprocess.CompletedProcess:
        """Run a command with proper logging and error handling."""
        if cwd is None:
            cwd = self.project_root
            
        # Convert Path objects to strings for subprocess
        cmd_str = [str(c) for c in cmd]
        
        if not silent:
            if concise:
                # Show condensed command - just executable name and files
                exe_path = Path(cmd_str[0])
                exe_name = exe_path.stem  # Gets filename without extension
                
                # Extract just the source files (not flags/options)
                source_files = []
                for part in cmd_str[1:]:
                    if not part.startswith('-') and (part.endswith('.cpp') or part.endswith('.h') or part.endswith('.hpp')):
                        source_files.append(self.make_relative_path(part))
                
                if source_files:
                    print(f"{exe_name} {' '.join(source_files)}")
                else:
                    print(exe_name)
            else:
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
            elif not concise:
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
        
        if not self.validate_environment():
            return 1
            
        # Determine build type
        build_type = "Debug"  # Default
        if args.release:
            build_type = "Release"
        elif args.debug:
            build_type = "Debug"
            
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
            
        result = self.run_command(cmake_cmd, concise=True)
        
        if result.returncode == 0:
            self.logger.info("CMake configuration completed successfully!")
            return 0
        else:
            self.logger.error("CMake configuration failed!")
            return 1
        
    def cmd_build(self, args):
        """Build the project."""

        
        # Determine build type
        build_type = "Debug"  # Default
        if args.release:
            build_type = "Release"
        elif args.debug:
            build_type = "Debug"
            
        # Get build directory
        build_dir = self.get_build_directory(build_type.lower())
        
        if not build_dir.exists():
            self.logger.error(f"Build directory not found: {build_dir}")
            self.logger.info("Run 'please configure' first")
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
            
        result = self.run_command(cmake_cmd, concise=True, capture_output=True)
        
        # Filter out unwanted output lines
        if result.stdout:
            filtered_lines = []
            for line in result.stdout.split('\n'):
                line = line.strip()
                if line and not line.startswith('ninja: no work to do'):
                    filtered_lines.append(line)
            if filtered_lines:
                print('\n'.join(filtered_lines))
        
        if result.returncode == 0:
            # Show output location with relative path
            output_dir = self.get_output_directory(build_type.lower(), "bin")
            rel_output_dir = self.make_relative_path(output_dir)
            print(f"Executables available in: {rel_output_dir}")
            return 0
        else:
            self.logger.error("Build failed!")
            
            # Provide helpful suggestions
            self.logger.info("Troubleshooting suggestions:")
            self.logger.info("1. Check if dependencies are properly downloaded (LLVM build may take 30+ minutes)")
            self.logger.info("2. Try running 'please configure --clean' to reconfigure")
            self.logger.info("3. Check build logs in artifacts/debug/logs/")
            return 1
            
    def cmd_install_git_hooks(self, args):
        """Install git pre-commit hooks for automatic formatting checks."""

        
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
# Pre-commit hook for Dosatsu formatting checks
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
please format --check-only --files $STAGED_FILES

# Check the exit code
if [ $? -ne 0 ]; then
    echo ""
    echo "[ERROR] Pre-commit formatting check failed!"
    echo "Fix formatting issues before committing:"
    echo "  please format"
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
            self.logger.info("2. Try running 'please configure --clean' to reconfigure")
            self.logger.info("3. Check build logs in artifacts/debug/logs/")
            return 1
        
    def cmd_clean(self, args):
        """Clean build artifacts."""

        
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
        return self._cmd_test_impl(args)
        
    def cmd_format(self, args):
        """Format source code."""

        
        # Find clang-format tool
        clang_format_tool = self.find_tool("clang-format")
        if not clang_format_tool:
            self.logger.error("clang-format not found in PATH")
            self.logger.info("Please install clang-format to use code formatting")
            return 1
            
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
            # Find all source files in source directory
            source_dirs = [self.project_root / "source"]
            file_patterns = ["*.h", "*.cpp", "*.hpp", "*.cxx"]
            files_to_format = []
            
            for source_dir in source_dirs:
                if source_dir.exists():
                    for pattern in file_patterns:
                        files_to_format.extend(source_dir.glob(pattern))
                        
        if not files_to_format:
            self.logger.warning("No source files found to format")
            return 0
            
        # Check if .clang-format exists
        clang_format_config = self.project_root / ".clang-format"
        if not clang_format_config.exists():
            self.logger.warning("No .clang-format configuration file found")
            self.logger.info("Using clang-format default configuration")
            
        # Format files
        if args.check_only:
            # Use --dry-run and --Werror to check formatting
            cmd = [str(clang_format_tool), "--dry-run", "--Werror"]
        else:
            cmd = [str(clang_format_tool), "-i"]
            
        # Add files to command
        cmd.extend([str(f) for f in files_to_format])
        
        # Run clang-format
        result = self.run_command(cmd, capture_output=args.check_only, concise=True)
        
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

            return 0
        else:
            if args.check_only:
                self.logger.error("Formatting issues found!")
                if result.stderr:
                    self.logger.error("clang-format output:")
                    self.logger.error(result.stderr)
                self.logger.info("Run 'please format' to fix formatting issues")
                self.logger.info(f"Detailed results saved to: {format_log_file}")
            else:
                self.logger.error("Formatting failed!")
            return 1
        
    def cmd_lint(self, args):
        """Fast lint with slow checks disabled."""
        return self._lint_with_config(args, fast_mode=True)
        
    def cmd_full_lint(self, args):
        """Full lint with all checks enabled."""
        return self._lint_with_config(args, fast_mode=False)
        
    def _lint_with_config(self, args, fast_mode=True):
        """Lint source code with configurable check filtering."""
        mode_desc = "Fast Mode (Auto-fix only)" if args.fast else "Two-Phase Mode"
        # Default to per-file unless explicitly requested batch mode
        use_batch_processing = args.batch
        processing_desc = "Batch processing" if use_batch_processing else "Per-file processing"

        
        # Profiling will be used for timing in fast mode, no separate timing needed
        # Add total execution timing to understand overhead vs check time
        import time
        total_start_time = time.time()
        
        # Find clang-tidy tool
        clang_tidy_tool = self.find_tool("clang-tidy")
        if not clang_tidy_tool:
            self.logger.error("clang-tidy not found in PATH")
            self.logger.info("Please install clang-tidy to use code linting")
            return 1
            
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
            # Find all source files in source directory (only .cpp files for linting)
            source_dirs = [self.project_root / "source"]
            file_patterns = ["*.cpp"]  # Only lint .cpp files to avoid header duplication
            files_to_lint = []
            
            for source_dir in source_dirs:
                if source_dir.exists():
                    for pattern in file_patterns:
                        files_to_lint.extend(source_dir.glob(pattern))
                        
        if not files_to_lint:
            self.logger.warning("No source files found to lint")
            return 0
            

        
        # Check for compile_commands.json
        compile_commands = self.artifacts_dir / "debug" / "build" / "compile_commands.json"
        if not compile_commands.exists():
            self.logger.error("compile_commands.json not found")
            self.logger.info("Run 'please configure' first to generate compilation database")
            return 1
            
        # Check if .clang-tidy exists
        clang_tidy_config = self.project_root / ".clang-tidy"
        if not clang_tidy_config.exists():
            self.logger.warning("No .clang-tidy configuration file found")
            self.logger.info("Using clang-tidy default configuration")
            
        # Create lint output directories
        lint_log_dir = self.artifacts_dir / "lint"
        self.ensure_directory(lint_log_dir)
        
        # Prepare base clang-tidy command
        build_dir = compile_commands.parent
        base_cmd = [str(clang_tidy_tool), f"-p={build_dir}", f"--config-file={clang_tidy_config}"]
        
        # Add profiling for lint mode (fast_mode=True)
        profile_dir = None
        if fast_mode:
            import datetime
            timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
            profile_dir = self.artifacts_dir / "lint" / f"profile_{timestamp}"
            self.ensure_directory(profile_dir)
            base_cmd.extend(["--enable-check-profile", f"--store-check-profile={profile_dir}"])
        
        # Add check filtering for fast mode
        if fast_mode:
            slow_checks = self._read_slow_checks_file()
            if slow_checks:
                # Build the -checks argument to disable slow checks
                disabled_checks = ','.join(f'-{check}' for check in slow_checks)
                base_cmd.extend([f"-checks={disabled_checks}"])
        
        # Choose processing method: per-file is default, batch only when explicitly requested
        if use_batch_processing:
            result = self._lint_batch(args, base_cmd, files_to_lint, lint_log_dir, profile_dir)
        else:
            result = self._lint_per_file(args, base_cmd, files_to_lint, lint_log_dir, profile_dir)
        
        # Show timing breakdown for analysis
        total_end_time = time.time()
        total_execution_time = total_end_time - total_start_time
        
        # Store timing for analysis in profiling function
        if fast_mode and profile_dir:
            print(f"Total execution time: {total_execution_time:.3f}s")
        
        return result
            
    def _filter_clang_tidy_output(self, output: str) -> str:
        """Filter clang-tidy output to remove noise and organize results."""
        filtered_lines = []
        for line in output.split('\n'):
            # Skip suppressed warnings messages and header filter messages
            if (not line.strip().startswith("Suppressed") and 
                "Use -header-filter=" not in line and
                "warnings generated" not in line and
                line.strip()):
                filtered_lines.append(line)
        return '\n'.join(filtered_lines)
    
    def _analyze_clang_tidy_output(self, output: str) -> dict:
        """Analyze clang-tidy output and generate comprehensive statistics."""
        stats = {
            'total_issues': 0,
            'error_count': 0,
            'warning_count': 0,
            'note_count': 0,
            'by_severity': {},
            'by_check': {},
            'by_file': {},
            'issues': []
        }
        
        current_file = None
        for line in output.split('\n'):
            line = line.strip()
            if not line:
                continue
                
            # Match file paths (lines ending with issues like "error:" or "warning:")
            if ':' in line and ('error:' in line or 'warning:' in line or 'note:' in line):
                parts = line.split(':')
                if len(parts) >= 4:
                    file_path = parts[0]
                    line_num = parts[1]
                    col_num = parts[2]
                    rest = ':'.join(parts[3:]).strip()
                    
                    # Determine severity
                    severity = 'info'
                    if 'error:' in rest:
                        severity = 'error'
                        stats['error_count'] += 1
                    elif 'warning:' in rest:
                        severity = 'warning'  
                        stats['warning_count'] += 1
                    elif 'note:' in rest:
                        severity = 'note'
                        stats['note_count'] += 1
                        
                    stats['total_issues'] += 1
                    
                    # Update severity counts
                    stats['by_severity'][severity] = stats['by_severity'].get(severity, 0) + 1
                    
                    # Extract check name (usually in brackets)
                    check_name = 'unknown'
                    if '[' in rest and ']' in rest:
                        start = rest.rfind('[')
                        end = rest.rfind(']')
                        if start != -1 and end != -1 and end > start:
                            check_name = rest[start+1:end]
                            
                    stats['by_check'][check_name] = stats['by_check'].get(check_name, 0) + 1
                    
                    # Update file counts
                    stats['by_file'][file_path] = stats['by_file'].get(file_path, 0) + 1
                    
                    # Store issue details
                    stats['issues'].append({
                        'file': file_path,
                        'line': line_num,
                        'column': col_num,
                        'severity': severity,
                        'check': check_name,
                        'message': rest,
                        'raw_line': line
                    })
                    
        return stats
    
    def _generate_lint_report(self, stats: dict, output_file: Path, format_type: str = 'markdown') -> None:
        """Generate comprehensive lint report."""
        with open(output_file, 'w', encoding='utf-8') as f:
            if format_type == 'markdown':
                self._write_markdown_report(f, stats)
            else:
                self._write_text_report(f, stats)
    
    def _write_markdown_report(self, f, stats: dict) -> None:
        """Write markdown-formatted report."""
        f.write("# Clang-Tidy Analysis Report\n\n")
        
        # Summary
        f.write("## Summary\n")
        f.write(f"- **Total Issues**: {stats['total_issues']}\n")
        f.write(f"- **Errors**: {stats['error_count']}\n")
        f.write(f"- **Warnings**: {stats['warning_count']}\n")
        f.write(f"- **Notes**: {stats['note_count']}\n\n")
        
        # Issues by severity
        if stats['by_severity']:
            f.write("## Issues by Severity\n")
            for severity, count in sorted(stats['by_severity'].items()):
                f.write(f"- **{severity.title()}**: {count}\n")
            f.write("\n")
        
        # Top check types
        if stats['by_check']:
            f.write("## Most Common Check Types\n")
            sorted_checks = sorted(stats['by_check'].items(), key=lambda x: x[1], reverse=True)
            for check, count in sorted_checks[:10]:  # Top 10
                f.write(f"- **{check}**: {count} issues\n")
            f.write("\n")
        
        # Files with most issues
        if stats['by_file']:
            f.write("## Files with Most Issues\n")
            sorted_files = sorted(stats['by_file'].items(), key=lambda x: x[1], reverse=True)
            for file_path, count in sorted_files[:10]:  # Top 10
                f.write(f"- **{file_path}**: {count} issues\n")
            f.write("\n")
        
        # Detailed issues
        if stats['issues']:
            f.write("## Detailed Issues\n\n")
            current_file = None
            for issue in stats['issues']:
                if issue['file'] != current_file:
                    current_file = issue['file']
                    f.write(f"### {current_file}\n\n")
                
                severity_icon = {'error': '[ERROR]', 'warning': '[WARN]', 'note': '[NOTE]'}.get(issue['severity'], '[INFO]')
                f.write(f"{severity_icon} **Line {issue['line']}**: {issue['message']}\n")
                f.write(f"   - Check: `{issue['check']}`\n")
                f.write(f"   - Severity: {issue['severity']}\n\n")
    
    def _write_text_report(self, f, stats: dict) -> None:
        """Write plain text-formatted report."""
        f.write("CLANG-TIDY ANALYSIS REPORT\n")
        f.write("=" * 50 + "\n\n")
        
        # Summary
        f.write("SUMMARY\n")
        f.write("-" * 20 + "\n")
        f.write(f"Total Issues: {stats['total_issues']}\n")
        f.write(f"Errors: {stats['error_count']}\n")
        f.write(f"Warnings: {stats['warning_count']}\n")
        f.write(f"Notes: {stats['note_count']}\n\n")
        
        # Issues by severity
        if stats['by_severity']:
            f.write("ISSUES BY SEVERITY\n")
            f.write("-" * 20 + "\n")
            for severity, count in sorted(stats['by_severity'].items()):
                f.write(f"{severity.title()}: {count}\n")
            f.write("\n")
        
        # Top check types
        if stats['by_check']:
            f.write("MOST COMMON CHECK TYPES\n")
            f.write("-" * 30 + "\n")
            sorted_checks = sorted(stats['by_check'].items(), key=lambda x: x[1], reverse=True)
            for check, count in sorted_checks[:10]:  # Top 10
                f.write(f"{check}: {count} issues\n")
            f.write("\n")
        
        # Files with most issues
        if stats['by_file']:
            f.write("FILES WITH MOST ISSUES\n")
            f.write("-" * 25 + "\n")
            sorted_files = sorted(stats['by_file'].items(), key=lambda x: x[1], reverse=True)
            for file_path, count in sorted_files[:10]:  # Top 10
                f.write(f"{file_path}: {count} issues\n")
            f.write("\n")
        
        # Detailed issues
        if stats['issues']:
            f.write("DETAILED ISSUES\n")
            f.write("-" * 20 + "\n\n")
            current_file = None
            for issue in stats['issues']:
                if issue['file'] != current_file:
                    current_file = issue['file']
                    f.write(f"FILE: {current_file}\n")
                    f.write("~" * len(current_file) + "\n")
                
                severity_prefix = {'error': '[ERROR]', 'warning': '[WARN]', 'note': '[NOTE]'}.get(issue['severity'], '[INFO]')
                f.write(f"{severity_prefix} Line {issue['line']}: {issue['message']}\n")
                f.write(f"         Check: {issue['check']}\n")
                f.write(f"         Severity: {issue['severity']}\n\n")
                    
    def _print_lint_summary(self, stats: dict) -> None:
        """Print a concise lint summary to console."""
        if stats['total_issues'] == 0:
            self.logger.info("[OK] No issues found!")
            return
            
        self.logger.info(f"Analysis completed: {stats['total_issues']} issues found")
        
        if stats['error_count'] > 0:
            self.logger.error(f"  Errors: {stats['error_count']}")
        if stats['warning_count'] > 0:
            self.logger.warning(f"  Warnings: {stats['warning_count']}")
        if stats['note_count'] > 0:
            self.logger.info(f"  Notes: {stats['note_count']}")
            
        # Show top 3 most common check types
        if stats['by_check']:
            sorted_checks = sorted(stats['by_check'].items(), key=lambda x: x[1], reverse=True)
            self.logger.info("Most common issues:")
            for check, count in sorted_checks[:3]:
                self.logger.info(f"  - {check}: {count}")
                
        # Show files with most issues
        if stats['by_file']:
            sorted_files = sorted(stats['by_file'].items(), key=lambda x: x[1], reverse=True)
            if len(sorted_files) > 1:
                self.logger.info("Files needing attention:")
                for file_path, count in sorted_files[:3]:
                    rel_path = Path(file_path).name  # Just filename for brevity
                    self.logger.info(f"  - {rel_path}: {count} issues")
    
    def _lint_batch(self, args, base_cmd, files_to_lint, lint_log_dir, profile_dir):
        """Execute lint in traditional batch mode with performance optimizations."""
        file_args = [str(f) for f in files_to_lint]
        
        # ===== PHASE 1: Auto-fix phase =====
        phase1_cmd = base_cmd + ["--fix", "--fix-errors"] + file_args
        phase1_result = self.run_command(phase1_cmd, capture_output=True, concise=True)
        
        # Analyze Phase 1 output for summary
        phase1_output_to_analyze = phase1_result.stdout if phase1_result.stdout else ""
        phase1_stats = self._analyze_clang_tidy_output(phase1_output_to_analyze)
        
        if phase1_stats['total_issues'] > 0:
            self.logger.info(f"Phase 1: Applied automatic fixes to {phase1_stats['total_issues']} issues")
        else:
            self.logger.info("Phase 1: No issues found that could be automatically fixed")
        
        # Initialize phase2_stats in case we skip Phase 2
        phase2_stats = {'total_issues': 0, 'error_count': 0, 'warning_count': 0, 'note_count': 0}
        raw_output_file = lint_log_dir / "clang-tidy-raw.txt"
        
        # ===== PERFORMANCE OPTIMIZATION: Skip Phase 2 if fast mode or no issues in Phase 1 =====
        skip_phase2 = args.fast or (phase1_stats['total_issues'] == 0 and phase1_result.returncode == 0)
        
        if skip_phase2:
            if args.fast:
                self.logger.info("\n--- Phase 2: Skipped (Fast mode) ---")
            else:
                self.logger.info("\n--- Phase 2: Skipped (No issues found in Phase 1) ---")
            
            # No raw output file needed when Phase 2 is skipped
        else:
            # ===== PHASE 2: Report phase (only files with issues) =====
            files_with_issues = self._get_files_with_issues(phase1_stats, files_to_lint)
            
            if not files_with_issues:
                # This shouldn't happen since we already check phase1_stats['total_issues'] == 0 above
                # But just in case, skip Phase 2
                self.logger.info("\n--- Phase 2: Skipped (No files with issues identified) ---")
                # No raw output file needed when no files have issues
                phase2_stats = {'total_issues': 0, 'error_count': 0, 'warning_count': 0, 'note_count': 0}
            else:
                self.logger.info("\n--- Phase 2: Analysis and reporting ---")
                files_with_issues_count = len(files_with_issues)
                files_skipped_count = len(files_to_lint) - files_with_issues_count
                
                if files_skipped_count > 0:
                    self.logger.info(f"Processing {files_with_issues_count} files with issues (skipping {files_skipped_count} clean files)")
                else:
                    self.logger.info(f"Processing {files_with_issues_count} files with issues")
                
                # Only process files that had issues in Phase 1
                phase2_file_args = [str(f) for f in files_with_issues]
                phase2_cmd = base_cmd + phase2_file_args
                phase2_result = self.run_command(phase2_cmd, capture_output=True, concise=True)
                
                # Analyze Phase 2 output and generate statistics
                phase2_output_to_analyze = phase2_result.stdout if phase2_result.stdout else ""
                phase2_stats = self._analyze_clang_tidy_output(phase2_output_to_analyze)
                
                # Only save Phase 2 output if there are issues requiring manual fixes
                if phase2_stats['total_issues'] > 0:
                    with open(raw_output_file, 'w', encoding='utf-8') as f:
                        f.write(f"Clang-tidy Phase 2 (Report) results:\n")
                        f.write(f"Command: {' '.join(phase2_cmd)}\n")
                        f.write(f"Files analyzed: {files_with_issues_count} (of {len(files_to_lint)} total)\n")
                        f.write(f"Files with issues from Phase 1: {[str(f) for f in files_with_issues]}\n")
                        f.write(f"Files skipped (clean): {files_skipped_count}\n")
                        f.write(f"Return code: {phase2_result.returncode}\n\n")
                        if phase2_result.stdout:
                            f.write("stdout:\n")
                            f.write(phase2_result.stdout)
                        if phase2_result.stderr:
                            f.write("\nstderr:\n")
                            f.write(phase2_result.stderr)
        
        return self._finalize_lint_results(args, phase1_stats, phase2_stats, None, raw_output_file, lint_log_dir, skip_phase2, profile_dir, None)
    
    def _lint_per_file(self, args, base_cmd, files_to_lint, lint_log_dir, profile_dir):
        """Execute lint with per-file processing and progress reporting."""
        total_files = len(files_to_lint)

        
        # Track timing for this method
        import time
        method_start_time = time.time()
        
        # Aggregate statistics
        aggregate_phase1_stats = {'total_issues': 0, 'error_count': 0, 'warning_count': 0, 'note_count': 0, 'by_severity': {}, 'by_check': {}, 'by_file': {}, 'issues': []}
        aggregate_phase2_stats = {'total_issues': 0, 'error_count': 0, 'warning_count': 0, 'note_count': 0, 'by_severity': {}, 'by_check': {}, 'by_file': {}, 'issues': []}
        
        # Output files for aggregated results
        phase1_output_file = lint_log_dir / "clang-tidy-phase1-autofix.txt"
        raw_output_file = lint_log_dir / "clang-tidy-raw.txt"
        
        all_phase1_output = []
        all_phase2_output = []
        files_with_issues = []
        
        for i, file_path in enumerate(files_to_lint, 1):
            rel_path = file_path.relative_to(self.project_root) if file_path.is_relative_to(self.project_root) else file_path
            
            file_arg = [str(file_path)]
            
            # ===== PHASE 1 for this file =====
            phase1_cmd = base_cmd + ["--fix", "--fix-errors"] + file_arg
            phase1_result = self.run_command(phase1_cmd, capture_output=True, concise=True)
            
            # Analyze Phase 1 output
            phase1_output = phase1_result.stdout if phase1_result.stdout else ""
            phase1_stats = self._analyze_clang_tidy_output(phase1_output)
            
            if phase1_stats['total_issues'] > 0:
                files_with_issues.append(str(rel_path))
            
            # Aggregate Phase 1 statistics
            self._merge_stats(aggregate_phase1_stats, phase1_stats)
            all_phase1_output.append(f"=== {file_path} ===\n{phase1_output}\n")
            
            # ===== PHASE 2 for this file (conditional) =====
            skip_phase2_for_file = args.fast or (phase1_stats['total_issues'] == 0 and phase1_result.returncode == 0)
            
            if skip_phase2_for_file:
                reason = "Fast mode" if args.fast else "No issues in Phase 1"
                all_phase2_output.append(f"=== {file_path} ===\nSKIPPED: {reason}\n")
            else:
                phase2_cmd = base_cmd + file_arg
                phase2_result = self.run_command(phase2_cmd, capture_output=True, concise=True)
                
                # Analyze Phase 2 output
                phase2_output = phase2_result.stdout if phase2_result.stdout else ""
                phase2_stats = self._analyze_clang_tidy_output(phase2_output)
                
                # Phase 2 processing completed silently
                
                # Aggregate Phase 2 statistics
                self._merge_stats(aggregate_phase2_stats, phase2_stats)
                all_phase2_output.append(f"=== {file_path} ===\n{phase2_output}\n")
        
        # Only write raw output file if there are manual issues to fix
        if aggregate_phase2_stats['total_issues'] > 0:
            with open(raw_output_file, 'w', encoding='utf-8') as f:
                f.write(f"Clang-tidy Phase 2 (Report) results - Per-file processing:\n")
                f.write(f"Total files processed: {total_files}\n")
                f.write(f"Total remaining issues: {aggregate_phase2_stats['total_issues']}\n\n")
                f.write("=== PER-FILE RESULTS ===\n")
                f.write('\n'.join(all_phase2_output))
        
        # Determine if Phase 2 was skipped for all files
        skip_phase2 = args.fast or aggregate_phase1_stats['total_issues'] == 0
        
        # Calculate total execution time for this method
        method_end_time = time.time()
        method_execution_time = method_end_time - method_start_time
        
        return self._finalize_lint_results(args, aggregate_phase1_stats, aggregate_phase2_stats, None, raw_output_file, lint_log_dir, skip_phase2, profile_dir, method_execution_time)
    
    def _get_files_with_issues(self, stats, files_to_lint):
        """Get list of files that had issues based on analysis statistics."""
        if not stats or stats['total_issues'] == 0:
            return []
        
        # Get files that had issues from the by_file statistics
        files_with_issues = set(stats['by_file'].keys())
        
        # Convert to Path objects and filter to only include files we were actually linting
        files_to_lint_str = {str(f) for f in files_to_lint}
        result_files = []
        
        for file_path in files_with_issues:
            # clang-tidy might report absolute paths, so we need to match them properly
            file_path_obj = Path(file_path)
            if str(file_path_obj) in files_to_lint_str:
                result_files.append(file_path_obj)
            else:
                # Try to match by relative path
                for lint_file in files_to_lint:
                    if file_path_obj.name == lint_file.name and file_path_obj.suffix == lint_file.suffix:
                        result_files.append(lint_file)
                        break
        
        return result_files
    
    def _merge_stats(self, aggregate_stats, file_stats):
        """Merge file-level statistics into aggregate statistics."""
        aggregate_stats['total_issues'] += file_stats['total_issues']
        aggregate_stats['error_count'] += file_stats['error_count']
        aggregate_stats['warning_count'] += file_stats['warning_count']
        aggregate_stats['note_count'] += file_stats['note_count']
        
        # Merge dictionaries
        for key, value in file_stats['by_severity'].items():
            aggregate_stats['by_severity'][key] = aggregate_stats['by_severity'].get(key, 0) + value
        
        for key, value in file_stats['by_check'].items():
            aggregate_stats['by_check'][key] = aggregate_stats['by_check'].get(key, 0) + value
        
        for key, value in file_stats['by_file'].items():
            aggregate_stats['by_file'][key] = aggregate_stats['by_file'].get(key, 0) + value
        
        # Extend issues list
        aggregate_stats['issues'].extend(file_stats['issues'])
    
    def _finalize_lint_results(self, args, phase1_stats, phase2_stats, phase1_output_file, raw_output_file, lint_log_dir, skip_phase2, profile_dir, total_execution_time=None):
        """Finalize lint results and generate reports."""
        # Generate comprehensive report based on Phase 2 results (or Phase 1 if Phase 2 was skipped)
        report_stats = phase2_stats if not skip_phase2 else phase1_stats
        
        if args.report_format == 'markdown':
            report_file = lint_log_dir / "analysis-report.md"
        else:
            report_file = lint_log_dir / "analysis-report.txt"
        self._generate_lint_report(report_stats, report_file, args.report_format)
        
        # Print concise summary
        total_fixed = phase1_stats['total_issues']
        total_manual = phase2_stats['total_issues'] if not skip_phase2 else 0
        
        if total_fixed == 0 and total_manual == 0:
            print("No issues found")
        elif total_fixed > 0 and total_manual == 0:
            print(f"{total_fixed} issues automatically fixed")
        elif total_fixed == 0 and total_manual > 0:
            print(f"{total_manual} issues need manual fixing")
        else:
            print(f"{total_fixed} issues automatically fixed, {total_manual} issues need manual fixing")
        
        # Show filtered output if there are remaining issues (unless summary-only mode)
        if not skip_phase2 and phase2_stats['total_issues'] > 0 and not args.summary_only:
            # Read the raw output file and filter it
            with open(raw_output_file, 'r', encoding='utf-8') as f:
                raw_content = f.read()
            
            # Extract just the clang-tidy output from the file
            if "=== PER-FILE RESULTS ===" in raw_content:
                # For per-file processing, extract output from each file section
                sections = raw_content.split("=== PER-FILE RESULTS ===")[1].split("=== ")
                filtered_sections = []
                for section in sections[1:]:  # Skip first empty section
                    if "SKIPPED:" not in section:
                        file_output = section.split("\n", 1)[1] if "\n" in section else ""
                        filtered_output = self._filter_clang_tidy_output(file_output)
                        if filtered_output.strip():
                            file_path = section.split("\n")[0].replace(" ===", "")
                            filtered_sections.append(f"=== {file_path} ===\n{filtered_output}")
                
                if filtered_sections:
                    self.logger.info("\nRemaining issues that require manual attention:")
                    print('\n'.join(filtered_sections))
            else:
                # For batch processing, filter the stdout section
                if "stdout:\n" in raw_content:
                    stdout_section = raw_content.split("stdout:\n")[1]
                    if "\nstderr:\n" in stdout_section:
                        stdout_section = stdout_section.split("\nstderr:\n")[0]
                    filtered_output = self._filter_clang_tidy_output(stdout_section)
                    if filtered_output.strip():
                        self.logger.info("\nRemaining issues that require manual attention:")
                        print(filtered_output)
        
        # Analyze clang-tidy profiling data (only for lint mode, show performance suggestions)
        if profile_dir:
            self._analyze_clang_tidy_profiling(profile_dir, lint_log_dir, show_performance_suggestions=True, total_execution_time=total_execution_time)
        
        # Log file locations with relative paths
        rel_report_file = self.make_relative_path(report_file)
        print(f"Analysis report saved to: {rel_report_file}")
        
        # Only log Phase 2 output if there were manual issues
        if not skip_phase2 and phase2_stats['total_issues'] > 0:
            rel_raw_file = self.make_relative_path(raw_output_file)
            print(f"Issues requiring manual fixes saved to: {rel_raw_file}")
        
        # Determine return status based on results
        final_stats = phase2_stats if not skip_phase2 else phase1_stats
        
        if final_stats['error_count'] == 0:

            return 0
        else:
            self.logger.error("Critical issues found that require manual attention")
            return 1
    
    def _analyze_clang_tidy_profiling(self, profile_dir, lint_log_dir, show_performance_suggestions=True, total_execution_time=None):
        """Analyze clang-tidy profiling data and generate a summary of the most expensive checks."""
        import json
        import glob
        
        try:
            # Find all profile JSON files in the profile directory
            profile_files = glob.glob(str(profile_dir / "*.json"))
            
            if not profile_files:
                self.logger.info("No profiling data found.")
                return
            
            # Aggregate timing data from all profile files
            check_timings = {}
            total_time = 0.0
            
            for profile_file in profile_files:
                try:
                    with open(profile_file, 'r', encoding='utf-8') as f:
                        content = f.read()
                        # Fix Windows path escaping in JSON - double backslashes in paths
                        import re
                        # Replace single backslashes in file paths with double backslashes
                        content = re.sub(r'"file": "([^"]*)"', lambda m: f'"file": "{m.group(1).replace(chr(92), chr(92)+chr(92))}"', content)
                        profile_data = json.loads(content)
                    
                    # Extract timing data from the profile structure
                    if 'profile' in profile_data:
                        for key, value in profile_data['profile'].items():
                            # Parse timing keys like "time.clang-tidy.bugprone-sizeof-expression.wall"
                            if key.startswith('time.clang-tidy.') and key.endswith('.wall'):
                                # Extract check name from the key
                                check_name = key[len('time.clang-tidy.'):-len('.wall')]
                                check_time = float(value)
                                check_timings[check_name] = check_timings.get(check_name, 0.0) + check_time
                                total_time += check_time
                                
                except (json.JSONDecodeError, KeyError, ValueError) as e:
                    self.logger.warning(f"Failed to parse profile file {profile_file}: {e}")
                    continue
            
            if not check_timings:
                self.logger.info("No timing data found in profile files.")
                return
            
            # Sort checks by total time (descending)
            sorted_checks = sorted(check_timings.items(), key=lambda x: x[1], reverse=True)
            top_10_checks = sorted_checks[:10]
            
            # Generate summary
            summary_file = lint_log_dir / "clang-tidy-profile-summary.txt"
            with open(summary_file, 'w', encoding='utf-8') as f:
                f.write("Clang-Tidy Performance Profile Summary\n")
                f.write("=" * 50 + "\n\n")
                f.write(f"Total profile files analyzed: {len(profile_files)}\n")
                f.write(f"Total time spent in checks: {total_time:.3f} seconds\n\n")
                f.write("Top 10 Most Expensive Checks:\n")
                f.write("-" * 50 + "\n")
                
                for i, (check_name, check_time) in enumerate(top_10_checks, 1):
                    percentage = (check_time / total_time * 100) if total_time > 0 else 0
                    f.write(f"{i:2d}. {check_name:<40} {check_time:8.3f}s ({percentage:5.1f}%)\n")
                
                f.write(f"\nRemaining {len(sorted_checks) - 10} checks: {sum(time for _, time in sorted_checks[10:]):.3f}s\n")
                
                # Add all checks for reference
                f.write(f"\nAll {len(sorted_checks)} checks (sorted by time):\n")
                f.write("-" * 70 + "\n")
                for i, (check_name, check_time) in enumerate(sorted_checks, 1):
                    percentage = (check_time / total_time * 100) if total_time > 0 else 0
                    f.write(f"{i:3d}. {check_name:<45} {check_time:8.3f}s ({percentage:5.1f}%)\n")
            
            # Show performance analysis if requested
            if show_performance_suggestions:
                print(f"Total time in checks: {total_time:.3f}s")
                
                # Show overhead analysis if total execution time is available
                if total_execution_time and total_execution_time > total_time:
                    overhead = total_execution_time - total_time
                    overhead_percentage = (overhead / total_execution_time * 100) if total_execution_time > 0 else 0
                    print(f"Overhead (startup, parsing, etc.): {overhead:.3f}s ({overhead_percentage:.1f}%)")
                                    
                # Only show check-specific suggestions if check time > 5s
                if total_time > 5.0:
                    # Calculate which checks to remove to get under 5 seconds
                    target_time = 5.0
                    cumulative_time = total_time
                    checks_to_remove = []
                    
                    for check_name, check_time in sorted_checks:
                        if cumulative_time <= target_time:
                            break
                        checks_to_remove.append((check_name, check_time))
                        cumulative_time -= check_time
                    
                    if checks_to_remove:
                        total_removed_time = sum(time for _, time in checks_to_remove)
                        remaining_time = total_time - total_removed_time
                        
                        print(f"To reduce lint time to <={target_time}s, consider disabling these checks:")
                        for i, (check_name, check_time) in enumerate(checks_to_remove, 1):
                            percentage = (check_time / total_time * 100) if total_time > 0 else 0
                            print(f"  {i:2d}. {check_name:<35} {check_time:6.3f}s ({percentage:4.1f}%)")
                        
                        print(f"Removing these {len(checks_to_remove)} checks would save {total_removed_time:.1f}s")
                        print(f"Estimated remaining time: {remaining_time:.1f}s")
            
            rel_summary_file = self.make_relative_path(summary_file)
            print(f"Profile summary saved to: {rel_summary_file}")
            
        except Exception as e:
            self.logger.error(f"Failed to analyze profiling data: {e}")
    
    def cmd_compile_db(self, args):
        """Generate or manage compilation database."""

        
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
            self.logger.info("Try running 'please build' to trigger compilation")
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
please format --check-only --files $STAGED_FILES

if [ $? -ne 0 ]; then
    echo "[ERROR] Code formatting issues found!"
    echo "Run 'please format' to fix formatting issues"
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

    def _cmd_test_impl(self, args):
        """Run tests using CTest."""

        
        # Determine build directory based on build type
        build_type = "debug" if args.debug else "release" if args.release else "debug"
        build_dir = self.get_build_directory(build_type)
        
        if not build_dir.exists():
            self.logger.error(f"Build directory not found: {build_dir}")
            self.logger.info("Run 'please configure' first")
            return 1
        
        # Create test artifacts directory
        test_log_dir = self.artifacts_dir / "test"
        self.ensure_directory(test_log_dir)
        
        # Build CTest command
        cmd = ["ctest"]
        
        # Add build directory
        cmd.extend(["--test-dir", str(build_dir)])
        
        # Add parallel execution if specified
        if hasattr(args, 'parallel') and args.parallel:
            if args.parallel == "auto":
                import multiprocessing
                parallel_count = multiprocessing.cpu_count()
            else:
                try:
                    parallel_count = int(args.parallel)
                except ValueError:
                    self.logger.error(f"Invalid parallel count: {args.parallel}")
                    return 1
            cmd.extend(["--parallel", str(parallel_count)])
        
        # Add verbosity
        if hasattr(args, 'verbose') and args.verbose:
            cmd.append("--verbose")
        else:
            cmd.append("--output-on-failure")
        
        # Add specific test target if specified
        if hasattr(args, 'target') and args.target:
            cmd.extend(["--tests-regex", args.target])
        
        # Add output format for CI/reporting
        junit_output = test_log_dir / "results.xml"
        cmd.extend(["--output-junit", str(junit_output)])
        
        # Add test labels filter if specified
        if hasattr(args, 'labels') and args.labels:
            cmd.extend(["--label-regex", args.labels])
        
        # Execute CTest with concise output
        result = self.run_command(cmd, cwd=build_dir, capture_output=True, concise=True)
        
        # Save detailed test output
        test_output_file = test_log_dir / "test-output.log"
        with open(test_output_file, 'w', encoding='utf-8') as f:
            f.write(f"Test Command: {' '.join(cmd)}\n")
            f.write(f"Working Directory: {build_dir}\n")
            f.write(f"Return Code: {result.returncode}\n\n")
            f.write("=== STDOUT ===\n")
            f.write(result.stdout or "")
            f.write("\n=== STDERR ===\n")
            f.write(result.stderr or "")
        
        # Parse and display test results
        test_stats = self._analyze_test_results(result, test_log_dir, junit_output)
        
        # Generate enhanced reports
        self._generate_enhanced_test_reports(args, test_log_dir, junit_output, test_stats, result)
        
        # Handle coverage if requested
        if hasattr(args, 'coverage') and args.coverage:
            self._handle_test_coverage(test_log_dir, build_dir)
        
        # Historical tracking
        if hasattr(args, 'historical') and args.historical:
            self._update_test_history(test_log_dir, test_stats, result.returncode)
        
        if result.returncode == 0:
            self.logger.info("[OK] All tests passed")
            self.logger.info(f"Test results saved to: {junit_output}")
            self.logger.info(f"Test output saved to: {test_output_file}")
            
            # Additional success reporting for CI/CD
            if hasattr(args, 'ci_mode') and args.ci_mode:
                self._generate_ci_test_summary(test_log_dir, junit_output)
            
            return 0
        else:
            self.logger.error("Some tests failed")
            self.logger.info(f"Test output saved to: {test_output_file}")
            if junit_output.exists():
                self.logger.info(f"Test results saved to: {junit_output}")
            
            # Generate failure report for CI/CD
            if hasattr(args, 'ci_mode') and args.ci_mode:
                self._generate_ci_failure_report(test_log_dir, result)
            
            return 1
    
    def _generate_ci_test_summary(self, test_log_dir: Path, junit_output: Path):
        """Generate CI-friendly test summary."""
        ci_summary = test_log_dir / "ci-summary.json"
        import json
        from datetime import datetime
        
        summary_data = {
            "timestamp": datetime.now().isoformat(),
            "status": "success",
            "junit_report": str(junit_output.relative_to(self.project_root)),
            "total_tests": 0,
            "passed_tests": 0,
            "failed_tests": 0,
            "execution_time": 0
        }
        
        # Parse JUnit XML if available
        if junit_output.exists():
            try:
                import xml.etree.ElementTree as ET
                tree = ET.parse(junit_output)
                root = tree.getroot()
                
                # Extract test statistics from JUnit XML
                if root.tag == 'testsuites':
                    for testsuite in root.findall('testsuite'):
                        summary_data["total_tests"] += int(testsuite.get('tests', 0))
                        summary_data["failed_tests"] += int(testsuite.get('failures', 0))
                        summary_data["execution_time"] += float(testsuite.get('time', 0))
                
                summary_data["passed_tests"] = summary_data["total_tests"] - summary_data["failed_tests"]
                        
            except Exception as e:
                self.logger.warning(f"Could not parse JUnit XML: {e}")
        
        with open(ci_summary, 'w', encoding='utf-8') as f:
            json.dump(summary_data, f, indent=2)
        
        self.logger.info(f"CI summary saved to: {ci_summary}")
    
    def _generate_ci_failure_report(self, test_log_dir: Path, result):
        """Generate CI-friendly failure report."""
        ci_failure = test_log_dir / "ci-failure.json"
        import json
        from datetime import datetime
        
        failure_data = {
            "timestamp": datetime.now().isoformat(),
            "status": "failure",
            "return_code": result.returncode,
            "stderr": result.stderr or "",
            "stdout": result.stdout or ""
        }
        
        with open(ci_failure, 'w', encoding='utf-8') as f:
            json.dump(failure_data, f, indent=2)
        
        self.logger.info(f"CI failure report saved to: {ci_failure}")
    
    def _analyze_test_results(self, result, test_log_dir: Path, junit_output: Path):
        """Analyze and summarize test results."""
        # Parse basic results from CTest output
        stdout = result.stdout or ""
        
        # Extract test summary from CTest output
        lines = stdout.split('\n')
        test_summary = {
            'total': 0,
            'passed': 0,
            'failed': 0,
            'passed_percent': 0,
            'execution_time': 0.0,
            'individual_tests': []
        }
        
        import re
        for line in lines:
            if "tests passed" in line.lower():
                # Parse lines like "100% tests passed, 0 tests failed out of 3"
                match = re.search(r'(\d+)% tests passed, (\d+) tests failed out of (\d+)', line)
                if match:
                    test_summary['passed_percent'] = int(match.group(1))
                    test_summary['failed'] = int(match.group(2))
                    test_summary['total'] = int(match.group(3))
                    test_summary['passed'] = test_summary['total'] - test_summary['failed']
            elif "Total Test time" in line:
                # Parse "Total Test time (real) =   0.02 sec"
                match = re.search(r'Total Test time.*?=\s*([0-9.]+)', line)
                if match:
                    test_summary['execution_time'] = float(match.group(1))
            elif re.match(r'\d+/\d+ Test #\d+:', line):
                # Parse individual test results: "1/3 Test #1: Dosatsu_SelfTest ............... Passed 0.01 sec"
                match = re.match(r'(\d+)/(\d+) Test #(\d+):\s*(\S+)\s*\.+\s*(\w+)\s*([0-9.]+)', line)
                if match:
                    test_info = {
                        'index': int(match.group(1)),
                        'test_id': int(match.group(3)),
                        'name': match.group(4),
                        'status': match.group(5),
                        'duration': float(match.group(6))
                    }
                    test_summary['individual_tests'].append(test_info)
        
        if test_summary['total'] > 0:
            self.logger.info("\nTest Summary:")
            self.logger.info(f"  Total tests: {test_summary['total']}")
            self.logger.info(f"  Passed: {test_summary['passed']} ({test_summary['passed_percent']}%)")
            if test_summary['failed'] > 0:
                self.logger.info(f"  Failed: {test_summary['failed']}")
            self.logger.info(f"  Execution time: {test_summary['execution_time']:.2f} seconds")
        
        # Save summary to file
        summary_file = test_log_dir / "test-summary.txt"
        with open(summary_file, 'w', encoding='utf-8') as f:
            f.write("TEST EXECUTION SUMMARY\n")
            f.write("=" * 50 + "\n\n")
            
            if test_summary['total'] > 0:
                f.write(f"Total Tests: {test_summary['total']}\n")
                f.write(f"Passed: {test_summary['passed']} ({test_summary['passed_percent']}%)\n")
                f.write(f"Failed: {test_summary['failed']}\n")
                f.write(f"Execution Time: {test_summary['execution_time']:.2f} seconds\n\n")
                
                # Individual test details
                if test_summary['individual_tests']:
                    f.write("INDIVIDUAL TEST RESULTS\n")
                    f.write("-" * 30 + "\n")
                    for test in test_summary['individual_tests']:
                        f.write(f"{test['name']}: {test['status']} ({test['duration']:.3f}s)\n")
                    f.write("\n")
            
            f.write("Return Code: " + str(result.returncode) + "\n")
            f.write("Status: " + ("PASSED" if result.returncode == 0 else "FAILED") + "\n")
            
            # Add timestamp
            from datetime import datetime
            f.write(f"Execution Time: {datetime.now().isoformat()}\n")
        
        return test_summary

    def _generate_enhanced_test_reports(self, args, test_log_dir: Path, junit_output: Path, test_stats: dict, result):
        """Generate enhanced test reports in multiple formats."""
        from datetime import datetime
        
        # Determine report format
        report_format = getattr(args, 'report_format', 'auto')
        if report_format == 'auto':
            report_format = 'html' if test_stats['total'] > 0 else 'text'
        
        # Generate HTML report
        if report_format in ['auto', 'html']:
            self._generate_html_test_report(test_log_dir, test_stats, result)
        
        # Generate JSON report
        if report_format in ['auto', 'json']:
            self._generate_json_test_report(test_log_dir, test_stats, result)
    
    def _generate_html_test_report(self, test_log_dir: Path, test_stats: dict, result):
        """Generate HTML test report."""
        from datetime import datetime
        
        html_file = test_log_dir / "test-report.html"
        
        # Generate HTML content
        html_content = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Test Report - Dosatsu</title>
    <style>
        body {{ font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 40px; background-color: #f5f5f5; }}
        .container {{ max-width: 1200px; margin: 0 auto; background-color: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }}
        .header {{ border-bottom: 2px solid #007acc; padding-bottom: 20px; margin-bottom: 30px; }}
        .header h1 {{ color: #007acc; margin: 0; }}
        .summary {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin-bottom: 30px; }}
        .stat-card {{ background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 20px; border-radius: 8px; text-align: center; }}
        .stat-card.success {{ background: linear-gradient(135deg, #56ab2f 0%, #a8e6cf 100%); }}
        .stat-card.failure {{ background: linear-gradient(135deg, #ff416c 0%, #ff4b2b 100%); }}
        .stat-value {{ font-size: 2em; font-weight: bold; margin-bottom: 5px; }}
        .stat-label {{ font-size: 0.9em; opacity: 0.9; }}
        .test-details {{ margin-top: 30px; }}
        .test-item {{ padding: 15px; margin: 10px 0; border-radius: 5px; border-left: 4px solid #007acc; background-color: #f8f9fa; }}
        .test-item.passed {{ border-left-color: #28a745; }}
        .test-item.failed {{ border-left-color: #dc3545; }}
        .test-name {{ font-weight: bold; color: #333; }}
        .test-duration {{ color: #666; font-size: 0.9em; }}
        .timestamp {{ color: #666; font-size: 0.9em; margin-top: 20px; text-align: center; }}
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Test Execution Report</h1>
            <p>Dosatsu - Test Results Summary</p>
        </div>
        
        <div class="summary">
            <div class="stat-card">
                <div class="stat-value">{test_stats['total']}</div>
                <div class="stat-label">Total Tests</div>
            </div>
            <div class="stat-card success">
                <div class="stat-value">{test_stats['passed']}</div>
                <div class="stat-label">Passed</div>
            </div>
            <div class="stat-card {'success' if test_stats['failed'] == 0 else 'failure'}">
                <div class="stat-value">{test_stats['failed']}</div>
                <div class="stat-label">Failed</div>
            </div>
            <div class="stat-card">
                <div class="stat-value">{test_stats['execution_time']:.2f}s</div>
                <div class="stat-label">Execution Time</div>
            </div>
            <div class="stat-card">
                <div class="stat-value">{test_stats['passed_percent']}%</div>
                <div class="stat-label">Success Rate</div>
            </div>
        </div>
        
        <div class="test-details">
            <h2>Individual Test Results</h2>
"""
        
        if test_stats['individual_tests']:
            for test in test_stats['individual_tests']:
                status_class = 'passed' if test['status'].lower() == 'passed' else 'failed'
                html_content += f"""
            <div class="test-item {status_class}">
                <div class="test-name">{test['name']}</div>
                <div class="test-duration">Duration: {test['duration']:.3f} seconds | Status: {test['status']}</div>
            </div>"""
        else:
            html_content += "<p>No individual test details available.</p>"
        
        html_content += f"""
        </div>
        
        <div class="timestamp">
            Generated on {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
        </div>
    </div>
</body>
</html>"""
        
        with open(html_file, 'w', encoding='utf-8') as f:
            f.write(html_content)
        
        self.logger.info(f"HTML test report saved to: {html_file}")
    
    def _generate_json_test_report(self, test_log_dir: Path, test_stats: dict, result):
        """Generate JSON test report."""
        import json
        from datetime import datetime
        
        json_file = test_log_dir / "test-report.json"
        
        report_data = {
            "metadata": {
                "timestamp": datetime.now().isoformat(),
                "project": "Dosatsu",
                "build_system": "CMake + CTest",
                "return_code": result.returncode,
                "status": "passed" if result.returncode == 0 else "failed"
            },
            "summary": {
                "total_tests": test_stats['total'],
                "passed_tests": test_stats['passed'],
                "failed_tests": test_stats['failed'],
                "success_rate": test_stats['passed_percent'],
                "execution_time_seconds": test_stats['execution_time']
            },
            "individual_tests": test_stats['individual_tests'],
            "artifacts": {
                "junit_xml": "results.xml",
                "raw_output": "test-output.log",
                "summary": "test-summary.txt"
            }
        }
        
        with open(json_file, 'w', encoding='utf-8') as f:
            json.dump(report_data, f, indent=2)
        
        self.logger.info(f"JSON test report saved to: {json_file}")
    
    def _handle_test_coverage(self, test_log_dir: Path, build_dir: Path):
        """Handle test coverage collection if available."""
        coverage_dir = test_log_dir / "coverage"
        self.ensure_directory(coverage_dir)
        
        # Check for common coverage tools
        coverage_tools = {
            'gcov': 'gcov',
            'llvm-cov': 'llvm-cov',
            'opencppcoverage': 'OpenCppCoverage.exe'
        }
        
        available_tool = None
        for tool_name, executable in coverage_tools.items():
            if self.find_tool(executable):
                available_tool = tool_name
                break
        
        if available_tool:
            self.logger.info(f"Found coverage tool: {available_tool}")
            if available_tool == 'opencppcoverage':
                self._collect_windows_coverage(coverage_dir, build_dir)
            else:
                self.logger.info("Coverage collection for this tool not yet implemented")
        else:
            self.logger.warning("No coverage tools found. Coverage collection skipped.")
            
        # Create coverage placeholder
        coverage_info = coverage_dir / "coverage-info.txt"
        with open(coverage_info, 'w', encoding='utf-8') as f:
            from datetime import datetime
            f.write("COVERAGE COLLECTION REPORT\n")
            f.write("=" * 40 + "\n\n")
            f.write(f"Timestamp: {datetime.now().isoformat()}\n")
            f.write(f"Available tool: {available_tool or 'None'}\n")
            f.write("Status: Coverage collection is available but requires manual setup\n")
            f.write("For detailed coverage, consider integrating gcov/llvm-cov in CMake configuration\n")
    
    def _collect_windows_coverage(self, coverage_dir: Path, build_dir: Path):
        """Collect coverage using OpenCppCoverage on Windows."""
        # This is a placeholder for OpenCppCoverage integration
        self.logger.info("Windows coverage collection would be implemented here")
        
    def _update_test_history(self, test_log_dir: Path, test_stats: dict, return_code: int):
        """Update historical test tracking."""
        import json
        from datetime import datetime
        
        history_file = test_log_dir / "test-history.json"
        history_data = []
        
        # Load existing history
        if history_file.exists():
            try:
                with open(history_file, 'r', encoding='utf-8') as f:
                    history_data = json.load(f)
            except Exception as e:
                self.logger.warning(f"Could not load test history: {e}")
        
        # Add current results
        current_entry = {
            "timestamp": datetime.now().isoformat(),
            "total_tests": test_stats['total'],
            "passed_tests": test_stats['passed'],
            "failed_tests": test_stats['failed'],
            "success_rate": test_stats['passed_percent'],
            "execution_time": test_stats['execution_time'],
            "return_code": return_code,
            "status": "passed" if return_code == 0 else "failed"
        }
        
        history_data.append(current_entry)
        
        # Keep only last 100 entries
        history_data = history_data[-100:]
        
        # Save updated history
        with open(history_file, 'w', encoding='utf-8') as f:
            json.dump(history_data, f, indent=2)
        
        self.logger.info(f"Test history updated: {len(history_data)} entries")
        
        # Generate trend analysis
        if len(history_data) >= 5:
            self._generate_trend_analysis(test_log_dir, history_data)
    
    def _generate_trend_analysis(self, test_log_dir: Path, history_data: list):
        """Generate test trend analysis."""
        trend_file = test_log_dir / "test-trends.txt"
        
        recent_runs = history_data[-10:]  # Last 10 runs
        success_rates = [entry['success_rate'] for entry in recent_runs]
        execution_times = [entry['execution_time'] for entry in recent_runs]
        
        avg_success = sum(success_rates) / len(success_rates)
        avg_time = sum(execution_times) / len(execution_times)
        
        # Calculate trends
        if len(success_rates) >= 2:
            success_trend = "improving" if success_rates[-1] > success_rates[-2] else "declining" if success_rates[-1] < success_rates[-2] else "stable"
            time_trend = "faster" if execution_times[-1] < execution_times[-2] else "slower" if execution_times[-1] > execution_times[-2] else "stable"
        else:
            success_trend = "stable"
            time_trend = "stable"
        
        with open(trend_file, 'w', encoding='utf-8') as f:
            f.write("TEST TREND ANALYSIS\n")
            f.write("=" * 30 + "\n\n")
            f.write(f"Analysis based on last {len(recent_runs)} test runs:\n\n")
            f.write(f"Average Success Rate: {avg_success:.1f}%\n")
            f.write(f"Average Execution Time: {avg_time:.2f} seconds\n\n")
            f.write(f"Recent Trend - Success Rate: {success_trend}\n")
            f.write(f"Recent Trend - Execution Time: {time_trend}\n\n")
            
            f.write("RECENT HISTORY:\n")
            f.write("-" * 20 + "\n")
            for entry in recent_runs[-5:]:  # Last 5 entries
                timestamp = entry['timestamp'][:19].replace('T', ' ')  # Format timestamp
                f.write(f"{timestamp}: {entry['success_rate']}% success, {entry['execution_time']:.2f}s\n")
        
        self.logger.info(f"Trend analysis saved to: {trend_file}")

    def cmd_rebuild(self, args):
        """Clean, build, and optionally test in sequence."""

        
        # Step 1: Clean
        self.logger.info("Step 1: Cleaning artifacts...")
        clean_result = self.cmd_clean(args)
        if clean_result != 0:
            self.logger.error("Clean step failed")
            return clean_result
        
        # Step 2: Configure with clean flag
        self.logger.info("Step 2: Configuring build...")
        
        # Create configure args from rebuild args
        class ConfigureArgs:
            def __init__(self, rebuild_args):
                self.debug = rebuild_args.debug
                self.release = rebuild_args.release
                self.clean = True  # Always do clean configure after clean
        
        configure_args = ConfigureArgs(args)
        configure_result = self.cmd_configure(configure_args)
        if configure_result != 0:
            self.logger.error("Configure step failed")
            return configure_result
        
        # Step 3: Build
        self.logger.info("Step 3: Building project...")
        build_result = self.cmd_build(args)
        if build_result != 0:
            self.logger.error("Build step failed")
            return build_result
        
        # Step 4: Test (optional)
        if not args.skip_tests:
            self.logger.info("Step 4: Running tests...")
            
            # Create test args from rebuild args
            class TestArgs:
                def __init__(self, rebuild_args):
                    self.debug = rebuild_args.debug
                    self.release = rebuild_args.release
                    self.parallel = "auto"
                    self.verbose = False
                    self.target = getattr(rebuild_args, 'test_target', None)
                    self.labels = None
                    self.ci_mode = False
            
            test_args = TestArgs(args)
            test_result = self.cmd_test(test_args)
            if test_result != 0:
                self.logger.error("Test step failed")
                return test_result
        else:
            self.logger.info("Step 4: Skipping tests (--skip-tests specified)")
        
        self.logger.info("[OK] Rebuild sequence completed successfully!")
        return 0

    def cmd_reconfigure(self, args):
        """Clean configure from scratch."""

        
        # Step 1: Clean
        self.logger.info("Step 1: Cleaning all artifacts...")
        clean_result = self.cmd_clean(args)
        if clean_result != 0:
            self.logger.error("Clean step failed")
            return clean_result
        
        # Step 2: Fresh configure with --clean flag
        self.logger.info("Step 2: Fresh configuration...")
        
        # Create configure args with clean flag
        class ConfigureArgs:
            def __init__(self, reconfig_args):
                self.debug = reconfig_args.debug
                self.release = reconfig_args.release
                self.clean = True  # Always clean for reconfigure
        
        configure_args = ConfigureArgs(args)
        configure_result = self.cmd_configure(configure_args)
        if configure_result != 0:
            self.logger.error("Configure step failed")
            return configure_result
        
        self.logger.info("[OK] Reconfigure completed successfully!")
        return 0

    # === Git Integration Commands ===
    
    def _validate_git_repository(self) -> bool:
        """Validate that we're in a git repository."""
        git_dir = self.project_root / ".git"
        if not git_dir.exists():
            self.logger.error("Not a git repository - .git directory not found")
            self.logger.info("Initialize a git repository with: git init")
            return False
        return True
    
    def _get_git_status(self) -> dict:
        """Get comprehensive git status information."""
        if not self._validate_git_repository():
            return {}
        
        status_info = {
            'clean': False,
            'staged_files': [],
            'modified_files': [],
            'untracked_files': [],
            'current_branch': '',
            'commits_ahead': 0,
            'commits_behind': 0,
            'has_remote': False
        }
        
        try:
            # Get current branch
            result = self.run_command(['git', 'branch', '--show-current'], capture_output=True)
            if result.returncode == 0:
                status_info['current_branch'] = result.stdout.strip()
            
            # Get status info
            result = self.run_command(['git', 'status', '--porcelain'], capture_output=True)
            if result.returncode == 0:
                for line in result.stdout.split('\n'):
                    if not line.strip():
                        continue
                    status = line[:2]
                    filename = line[3:]
                    
                    if status[0] in ['A', 'D', 'M', 'R', 'C']:
                        status_info['staged_files'].append(filename)
                    if status[1] in ['M', 'D']:
                        status_info['modified_files'].append(filename)
                    if status == '??':
                        status_info['untracked_files'].append(filename)
                
                status_info['clean'] = (len(status_info['staged_files']) == 0 and 
                                      len(status_info['modified_files']) == 0 and 
                                      len(status_info['untracked_files']) == 0)
            
            # Check remote tracking
            result = self.run_command(['git', 'remote'], capture_output=True)
            if result.returncode == 0 and result.stdout.strip():
                status_info['has_remote'] = True
                
                # Get ahead/behind info
                result = self.run_command(['git', 'status', '-b', '--porcelain'], capture_output=True)
                if result.returncode == 0:
                    first_line = result.stdout.split('\n')[0] if result.stdout else ''
                    if '[ahead' in first_line:
                        import re
                        match = re.search(r'ahead (\d+)', first_line)
                        if match:
                            status_info['commits_ahead'] = int(match.group(1))
                    if '[behind' in first_line:
                        import re
                        match = re.search(r'behind (\d+)', first_line)
                        if match:
                            status_info['commits_behind'] = int(match.group(1))
            
        except Exception as e:
            self.logger.warning(f"Failed to get git status: {e}")
        
        return status_info
    
    def cmd_git_status(self, args):
        """Display comprehensive git status."""

        
        if not self._validate_git_repository():
            return 1
        
        status = self._get_git_status()
        
        # Display basic status
        self.logger.info(f"Current branch: {status.get('current_branch', 'unknown')}")
        
        if status.get('has_remote'):
            ahead = status.get('commits_ahead', 0)
            behind = status.get('commits_behind', 0)
            if ahead > 0:
                self.logger.info(f"Ahead of remote by {ahead} commit(s)")
            if behind > 0:
                self.logger.warning(f"Behind remote by {behind} commit(s)")
            if ahead == 0 and behind == 0:
                self.logger.info("Up to date with remote")
        else:
            self.logger.warning("No remote repository configured")
        
        # Display file changes
        if status.get('clean', False):
            self.logger.info("✅ Working directory is clean")
        else:
            self.logger.warning("Working directory has changes:")
            
            staged = status.get('staged_files', [])
            if staged:
                self.logger.info(f"Staged files ({len(staged)}):")
                for file in staged[:10]:  # Show first 10
                    self.logger.info(f"  + {file}")
                if len(staged) > 10:
                    self.logger.info(f"  ... and {len(staged) - 10} more")
            
            modified = status.get('modified_files', [])
            if modified:
                self.logger.warning(f"Modified files ({len(modified)}):")
                for file in modified[:10]:  # Show first 10
                    self.logger.warning(f"  M {file}")
                if len(modified) > 10:
                    self.logger.warning(f"  ... and {len(modified) - 10} more")
            
            untracked = status.get('untracked_files', [])
            if untracked:
                self.logger.info(f"Untracked files ({len(untracked)}):")
                for file in untracked[:10]:  # Show first 10
                    self.logger.info(f"  ? {file}")
                if len(untracked) > 10:
                    self.logger.info(f"  ... and {len(untracked) - 10} more")
        
        return 0
    
    def cmd_git_pull(self, args):
        """Pull latest changes from remote repository."""

        
        if not self._validate_git_repository():
            return 1
        
        # Check for clean working directory if requested
        if args.check_clean:
            status = self._get_git_status()
            if not status.get('clean', False):
                self.logger.error("Working directory is not clean")
                self.logger.info("Commit or stash your changes before pulling")
                return 1
        
        # Pull changes
        pull_cmd = ['git', 'pull']
        if args.rebase:
            pull_cmd.append('--rebase')
        
        result = self.run_command(pull_cmd)
        
        if result.returncode == 0:
            self.logger.info("[OK] Pull completed successfully")
            
            # Suggest rebuild if there were changes
            if not args.skip_rebuild_suggestion:
                self.logger.info("Consider running 'please rebuild' to ensure everything works with the latest changes")
            
            return 0
        else:
            self.logger.error("Pull failed")
            return 1
    
    def cmd_git_push(self, args):
        """Push local commits to remote repository."""

        
        if not self._validate_git_repository():
            return 1
        
        status = self._get_git_status()
        
        # Check if there are commits to push
        if status.get('commits_ahead', 0) == 0:
            self.logger.info("No commits to push")
            return 0
        
        # Check for clean working directory
        if not status.get('clean', False) and not args.allow_dirty:
            self.logger.warning("Working directory has uncommitted changes")
            if not args.force:
                self.logger.error("Use --allow-dirty or --force to push with uncommitted changes")
                return 1
        
        # Push changes
        push_cmd = ['git', 'push']
        if args.force:
            push_cmd.append('--force')
        if args.set_upstream:
            push_cmd.extend(['--set-upstream', 'origin', status.get('current_branch', 'HEAD')])
        
        result = self.run_command(push_cmd)
        
        if result.returncode == 0:
            self.logger.info(f"[OK] Pushed {status.get('commits_ahead', 0)} commit(s) successfully")
            return 0
        else:
            self.logger.error("Push failed")
            return 1
    
    def cmd_git_commit(self, args):
        """Commit staged changes with automatic pre-commit checks."""

        
        if not self._validate_git_repository():
            return 1
        
        status = self._get_git_status()
        
        # Check if there are staged changes
        if len(status.get('staged_files', [])) == 0:
            self.logger.warning("No staged changes to commit")
            self.logger.info("Stage files with: git add <files>")
            return 1
        
        # Run pre-commit checks if enabled
        if not args.skip_checks:
            self.logger.info("Running pre-commit checks...")
            
            # Check formatting
            if not args.skip_format_check:
                self.logger.info("Checking code formatting...")
                format_result = self.cmd_format(argparse.Namespace(check_only=True, files=None))
                if format_result != 0:
                    self.logger.error("Formatting check failed")
                    self.logger.info("Fix formatting with: please format")
                    if not args.force:
                        return 1
            
            # Quick build check
            if not args.skip_build_check:
                self.logger.info("Running quick build check...")
                build_result = self.cmd_build(argparse.Namespace(debug=True, release=False, target=None, parallel=None))
                if build_result != 0:
                    self.logger.error("Build check failed")
                    if not args.force:
                        return 1
        
        # Commit changes
        commit_cmd = ['git', 'commit']
        if args.message:
            commit_cmd.extend(['-m', args.message])
        if args.amend:
            commit_cmd.append('--amend')
        
        result = self.run_command(commit_cmd)
        
        if result.returncode == 0:
            self.logger.info("[OK] Commit completed successfully")
            return 0
        else:
            self.logger.error("Commit failed")
            return 1
    
    def cmd_git_clean(self, args):
        """Clean git repository and build artifacts."""

        
        if not self._validate_git_repository():
            return 1
        
        if args.interactive:
            # Interactive clean
            result = self.run_command(['git', 'clean', '-i'])
        else:
            # Show what would be cleaned
            self.logger.info("Files that would be cleaned:")
            result = self.run_command(['git', 'clean', '-n'], capture_output=True)
            if result.returncode == 0 and result.stdout:
                for line in result.stdout.split('\n'):
                    if line.strip():
                        self.logger.info(f"  {line}")
            
            if args.force:
                # Actually clean
                clean_cmd = ['git', 'clean', '-f']
                if args.include_directories:
                    clean_cmd.append('-d')
                if args.include_ignored:
                    clean_cmd.append('-x')
                
                result = self.run_command(clean_cmd)
                
                if result.returncode == 0:
                    self.logger.info("[OK] Git clean completed")
                    
                    # Also clean build artifacts
                    if args.include_build_artifacts:
                        self.logger.info("Also cleaning build artifacts...")
                        clean_result = self.cmd_clean(args)
                        return clean_result
                else:
                    self.logger.error("Git clean failed")
                    return 1
            else:
                self.logger.info("Use --force to actually clean these files")
        
        return 0

    # === Performance Optimization Commands ===
    
    def cmd_build_stats(self, args):
        """Display build statistics and performance analysis."""

        
        # Analyze build directories
        debug_build = self.get_build_directory("debug")
        release_build = self.get_build_directory("release")
        
        build_stats = {
            'debug': self._analyze_build_directory(debug_build),
            'release': self._analyze_build_directory(release_build)
        }
        
        # Display statistics
        for build_type, stats in build_stats.items():
            if stats['configured']:
                self.logger.info(f"\n{build_type.capitalize()} Build Statistics:")
                self.logger.info(f"  Build directory size: {self._format_size(stats['build_size'])}")
                self.logger.info(f"  Dependencies size: {self._format_size(stats['deps_size'])}")
                self.logger.info(f"  Output size: {self._format_size(stats['output_size'])}")
                self.logger.info(f"  Configuration time: {stats['config_time']}")
                self.logger.info(f"  Last build time: {stats['last_build']}")
                
                if stats['cache_hits'] > 0:
                    cache_ratio = stats['cache_hits'] / (stats['cache_hits'] + stats['cache_misses']) * 100
                    self.logger.info(f"  Cache hit ratio: {cache_ratio:.1f}%")
            else:
                self.logger.info(f"\n{build_type.capitalize()} Build: Not configured")
        
        # Recommendations
        self._suggest_performance_improvements(build_stats)
        
        return 0
    
    def _analyze_build_directory(self, build_dir: Path) -> dict:
        """Analyze a build directory for performance metrics."""
        stats = {
            'configured': False,
            'build_size': 0,
            'deps_size': 0,
            'output_size': 0,
            'config_time': 'Unknown',
            'last_build': 'Never',
            'cache_hits': 0,
            'cache_misses': 0
        }
        
        if not build_dir.exists():
            return stats
        
        stats['configured'] = True
        
        # Calculate directory sizes
        stats['build_size'] = self._get_directory_size(build_dir)
        
        deps_dir = build_dir / "_deps"
        if deps_dir.exists():
            stats['deps_size'] = self._get_directory_size(deps_dir)
        
        # Check output directories
        build_type = build_dir.parent.name
        output_bin = self.get_output_directory(build_type, "bin")
        output_lib = self.get_output_directory(build_type, "lib")
        
        if output_bin.exists():
            stats['output_size'] += self._get_directory_size(output_bin)
        if output_lib.exists():
            stats['output_size'] += self._get_directory_size(output_lib)
        
        # Get timestamps
        cmake_cache = build_dir / "CMakeCache.txt"
        if cmake_cache.exists():
            import datetime
            config_time = datetime.datetime.fromtimestamp(cmake_cache.stat().st_mtime)
            stats['config_time'] = config_time.strftime("%Y-%m-%d %H:%M:%S")
        
        # Check for build artifacts
        ninja_build = build_dir / "build.ninja"
        if ninja_build.exists():
            import datetime
            build_time = datetime.datetime.fromtimestamp(ninja_build.stat().st_mtime)
            stats['last_build'] = build_time.strftime("%Y-%m-%d %H:%M:%S")
        
        return stats
    
    def _get_directory_size(self, directory: Path) -> int:
        """Calculate total size of a directory in bytes."""
        total_size = 0
        try:
            for dirpath, dirnames, filenames in os.walk(directory):
                for filename in filenames:
                    filepath = Path(dirpath) / filename
                    try:
                        total_size += filepath.stat().st_size
                    except (OSError, FileNotFoundError):
                        continue
        except (OSError, FileNotFoundError):
            pass
        return total_size
    
    def _format_size(self, size_bytes: int) -> str:
        """Format size in bytes to human readable format."""
        if size_bytes == 0:
            return "0 B"
        
        size_names = ["B", "KB", "MB", "GB", "TB"]
        import math
        i = int(math.floor(math.log(size_bytes, 1024)))
        p = math.pow(1024, i)
        s = round(size_bytes / p, 2)
        return f"{s} {size_names[i]}"
    
    def _suggest_performance_improvements(self, build_stats: dict):
        """Suggest performance improvements based on build statistics."""

        
        total_deps_size = sum(stats.get('deps_size', 0) for stats in build_stats.values())
        total_build_size = sum(stats.get('build_size', 0) for stats in build_stats.values())
        
        # Large dependency recommendations
        if total_deps_size > 2 * 1024 * 1024 * 1024:  # 2GB
            self.logger.info("Large dependencies detected:")
            self.logger.info("  - Consider using ccache for faster rebuilds")
            self.logger.info("  - Use shared libraries if building multiple targets")
            self.logger.info("  - Enable dependency caching in CI/CD")
        
        # Build directory recommendations
        if total_build_size > 5 * 1024 * 1024 * 1024:  # 5GB
            self.logger.info("Large build directory:")
            self.logger.info("  - Run 'please clean' periodically")
            self.logger.info("  - Consider separate debug/release builds")
        
        # General recommendations
        self.logger.info("General optimizations:")
        self.logger.info("  - Use 'please build --parallel N' for faster builds")
        self.logger.info("  - Enable LTO for release builds if needed")
        self.logger.info("  - Use incremental builds when possible")
    
    def cmd_cache_management(self, args):
        """Manage build caches and temporary files."""

        
        cache_dirs = []
        
        # Find cache directories
        for build_type in ["debug", "release"]:
            build_dir = self.get_build_directory(build_type)
            if build_dir.exists():
                deps_dir = build_dir / "_deps"
                if deps_dir.exists():
                    cache_dirs.append(("Dependencies", deps_dir))
                
                cmake_files = build_dir / "CMakeFiles"
                if cmake_files.exists():
                    cache_dirs.append(("CMake Files", cmake_files))
        
        if not cache_dirs:
            self.logger.info("No cache directories found")
            return 0
        
        # Display cache information
        total_size = 0
        self.logger.info("Cache directories found:")
        for name, path in cache_dirs:
            size = self._get_directory_size(path)
            total_size += size
            self.logger.info(f"  {name}: {self._format_size(size)} ({path})")
        
        self.logger.info(f"\nTotal cache size: {self._format_size(total_size)}")
        
        # Cache management actions
        if args.clean_cmake:
            self.logger.info("\nCleaning CMake caches...")
            for name, path in cache_dirs:
                if "CMake" in name:
                    import shutil
                    try:
                        shutil.rmtree(path)
                        self.logger.info(f"  Cleaned: {name}")
                    except Exception as e:
                        self.logger.error(f"  Failed to clean {name}: {e}")
        
        if args.clean_deps:
            self.logger.info("\nCleaning dependency caches...")
            for name, path in cache_dirs:
                if "Dependencies" in name:
                    import shutil
                    try:
                        shutil.rmtree(path)
                        self.logger.info(f"  Cleaned: {name}")
                    except Exception as e:
                        self.logger.error(f"  Failed to clean {name}: {e}")
        
        if args.optimize:
            self.logger.info("\nOptimizing caches...")
            # This could be extended to implement cache optimization strategies
            self.logger.info("  Cache optimization not yet implemented")
        
        return 0


def create_parser() -> argparse.ArgumentParser:
    """Create and configure the argument parser."""
    parser = argparse.ArgumentParser(
        description="Dosatsu Build Orchestrator",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  please setup                    # Initial environment setup
  please info                     # Show environment information
  please configure --debug        # Configure debug build
  please build --release          # Build release version
  please test                     # Run all tests
  please format --check-only      # Check formatting
  please lint                     # Run two-phase linter with per-file progress (default)
  please lint --fast              # Run fast linter (auto-fix only, skip Phase 2)
  please lint --batch             # Run linter in batch mode (faster but less responsive)
  please clean                    # Clean artifacts
  please rebuild                  # Clean, build, and test
  please git-status               # Show git repository status
  please git-commit -m "Fix bug"  # Commit with pre-commit checks
  please build-stats              # Show build performance stats
  please cache-mgmt --clean-cmake # Clean CMake caches

Note: Use 'please.bat' on Windows or './please' on Unix/Linux/macOS
      These wrapper scripts automatically call 'python please.py' with all arguments
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
    test_parser = subparsers.add_parser('test', help='Run tests using CTest')
    test_parser.add_argument('--debug', action='store_true', help='Run tests for debug build')
    test_parser.add_argument('--release', action='store_true', help='Run tests for release build')
    test_parser.add_argument('--parallel', default="auto", help='Number of parallel test jobs (default: auto)')
    test_parser.add_argument('--verbose', action='store_true', help='Verbose test output')
    test_parser.add_argument('--target', help='Run specific test (regex pattern)')
    test_parser.add_argument('--labels', help='Run tests with specific labels (regex pattern)')
    test_parser.add_argument('--ci-mode', action='store_true', help='Enable CI-friendly output and reporting')
    test_parser.add_argument('--coverage', action='store_true', help='Enable test coverage collection (if available)')
    test_parser.add_argument('--report-format', choices=['auto', 'html', 'json', 'text'], default='auto', help='Test report format')
    test_parser.add_argument('--historical', action='store_true', help='Enable historical test result tracking')
    
    # Format command
    format_parser = subparsers.add_parser('format', help='Format source code')
    format_parser.add_argument('--check-only', action='store_true', help='Check formatting without changes')
    format_parser.add_argument('--files', nargs='*', help='Specific files to format')
    
    # Lint command
    lint_parser = subparsers.add_parser('lint', help='Fast lint with slow checks disabled')
    lint_parser.add_argument('--files', nargs='*', help='Specific files to lint')
    lint_parser.add_argument('--target', help='Lint specific file')
    lint_parser.add_argument('--summary-only', action='store_true', help='Show only summary, skip detailed output')
    lint_parser.add_argument('--report-format', choices=['text', 'markdown'], default='markdown', help='Report format for generated files')
    lint_parser.add_argument('--fast', action='store_true', help='Fast mode: skip Phase 2 analysis (auto-fix only)')
    lint_parser.add_argument('--per-file', action='store_true', help='Process files individually with progress reporting (default)')
    lint_parser.add_argument('--batch', action='store_true', help='Process all files in batch mode (faster but less responsive)')
    
    # Full-lint command (all checks enabled)
    full_lint_parser = subparsers.add_parser('full-lint', help='Full lint with all checks enabled and profiling')
    full_lint_parser.add_argument('--files', nargs='*', help='Specific files to lint')
    full_lint_parser.add_argument('--target', help='Lint specific file')
    full_lint_parser.add_argument('--summary-only', action='store_true', help='Show only summary, skip detailed output')
    full_lint_parser.add_argument('--report-format', choices=['text', 'markdown'], default='markdown', help='Report format for generated files')
    full_lint_parser.add_argument('--fast', action='store_true', help='Fast mode: skip Phase 2 analysis (auto-fix only)')
    full_lint_parser.add_argument('--per-file', action='store_true', help='Process files individually with progress reporting (default)')
    full_lint_parser.add_argument('--batch', action='store_true', help='Process all files in batch mode (faster but less responsive)')
    
    # Compilation Database command
    compile_db_parser = subparsers.add_parser('compile-db', help='Generate and manage compilation database')
    compile_db_group = compile_db_parser.add_mutually_exclusive_group()
    compile_db_group.add_argument('--debug', action='store_true', help='Use debug build database')
    compile_db_group.add_argument('--release', action='store_true', help='Use release build database')
    compile_db_parser.add_argument('--copy-to-root', action='store_true', help='Copy database to project root')
    compile_db_parser.add_argument('--show-files', action='store_true', help='Show all files in database')
    
    # Git Integration Commands
    git_hooks_parser = subparsers.add_parser('install-git-hooks', help='Install git pre-commit hooks')
    
    # Integrated workflow commands
    rebuild_parser = subparsers.add_parser('rebuild', help='Clean, build, and test in sequence')
    rebuild_parser.add_argument('--debug', action='store_true', help='Use debug build type')
    rebuild_parser.add_argument('--release', action='store_true', help='Use release build type')
    rebuild_parser.add_argument('--parallel', type=int, help='Number of parallel build jobs')
    rebuild_parser.add_argument('--skip-tests', action='store_true', help='Skip test execution after rebuild')
    rebuild_parser.add_argument('--test-target', help='Run specific test pattern after build')

    # Reconfigure command (clean configure from scratch)
    reconfig_parser = subparsers.add_parser('reconfigure', help='Clean configure from scratch')
    reconfig_parser.add_argument('--debug', action='store_true', help='Configure for debug build')
    reconfig_parser.add_argument('--release', action='store_true', help='Configure for release build')
    
    # Git Integration Commands
    git_status_parser = subparsers.add_parser('git-status', help='Display comprehensive git repository status')
    
    git_pull_parser = subparsers.add_parser('git-pull', help='Pull latest changes from remote repository')
    git_pull_parser.add_argument('--check-clean', action='store_true', help='Check for clean working directory before pulling')
    git_pull_parser.add_argument('--rebase', action='store_true', help='Use rebase instead of merge')
    git_pull_parser.add_argument('--skip-rebuild-suggestion', action='store_true', help='Skip rebuild suggestion after pull')
    
    git_push_parser = subparsers.add_parser('git-push', help='Push local commits to remote repository')
    git_push_parser.add_argument('--allow-dirty', action='store_true', help='Allow push with uncommitted changes')
    git_push_parser.add_argument('--force', action='store_true', help='Force push (use with caution)')
    git_push_parser.add_argument('--set-upstream', action='store_true', help='Set upstream branch')
    
    git_commit_parser = subparsers.add_parser('git-commit', help='Commit staged changes with pre-commit checks')
    git_commit_parser.add_argument('-m', '--message', help='Commit message')
    git_commit_parser.add_argument('--amend', action='store_true', help='Amend the previous commit')
    git_commit_parser.add_argument('--skip-checks', action='store_true', help='Skip pre-commit checks')
    git_commit_parser.add_argument('--skip-format-check', action='store_true', help='Skip formatting check')
    git_commit_parser.add_argument('--skip-build-check', action='store_true', help='Skip build check')
    git_commit_parser.add_argument('--force', action='store_true', help='Force commit even if checks fail')
    
    git_clean_parser = subparsers.add_parser('git-clean', help='Clean git repository and build artifacts')
    git_clean_parser.add_argument('--force', action='store_true', help='Actually clean files (default is dry-run)')
    git_clean_parser.add_argument('--interactive', action='store_true', help='Interactive clean mode')
    git_clean_parser.add_argument('--include-directories', action='store_true', help='Also clean untracked directories')
    git_clean_parser.add_argument('--include-ignored', action='store_true', help='Also clean ignored files')
    git_clean_parser.add_argument('--include-build-artifacts', action='store_true', help='Also clean build artifacts')
    
    # Performance and Analysis Commands
    build_stats_parser = subparsers.add_parser('build-stats', help='Display build statistics and performance analysis')
    
    cache_mgmt_parser = subparsers.add_parser('cache-mgmt', help='Manage build caches and temporary files')
    cache_mgmt_parser.add_argument('--clean-cmake', action='store_true', help='Clean CMake cache files')
    cache_mgmt_parser.add_argument('--clean-deps', action='store_true', help='Clean dependency cache files')
    cache_mgmt_parser.add_argument('--optimize', action='store_true', help='Optimize cache storage')
    
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
        'full-lint': orchestrator.cmd_full_lint,
        'compile-db': orchestrator.cmd_compile_db,
        'install-git-hooks': orchestrator.cmd_install_git_hooks,
        'rebuild': orchestrator.cmd_rebuild,
        'reconfigure': orchestrator.cmd_reconfigure,
        'git-status': orchestrator.cmd_git_status,
        'git-pull': orchestrator.cmd_git_pull,
        'git-push': orchestrator.cmd_git_push,
        'git-commit': orchestrator.cmd_git_commit,
        'git-clean': orchestrator.cmd_git_clean,
        'build-stats': orchestrator.cmd_build_stats,
        'cache-mgmt': orchestrator.cmd_cache_management,
    }
    
    try:
        command_func = command_map[args.command]
        orchestrator.start_timing()
        result = command_func(args)
        orchestrator.print_execution_time(f"Command '{args.command}'")
        return result
    except KeyError:
        orchestrator.logger.error(f"Unknown command: {args.command}")
        return 1
    except Exception as e:
        orchestrator.logger.error(f"Unexpected error: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
