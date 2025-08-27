import sys
import os
import subprocess
import ctypes
from ctypes import wintypes
import shlex

CREATE_SUSPENDED = 0x00000004

kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)

LPVOID = ctypes.c_void_p

class STARTUPINFO(ctypes.Structure):
    _fields_ = [
        ("cb", wintypes.DWORD),
        ("lpReserved", wintypes.LPWSTR),
        ("lpDesktop", wintypes.LPWSTR),
        ("lpTitle", wintypes.LPWSTR),
        ("dwX", wintypes.DWORD),
        ("dwY", wintypes.DWORD),
        ("dwXSize", wintypes.DWORD),
        ("dwYSize", wintypes.DWORD),
        ("dwXCountChars", wintypes.DWORD),
        ("dwYCountChars", wintypes.DWORD),
        ("dwFillAttribute", wintypes.DWORD),
        ("dwFlags", wintypes.DWORD),
        ("wShowWindow", wintypes.WORD),
        ("cbReserved2", wintypes.WORD),
        ("lpReserved2", LPVOID),
        ("hStdInput", wintypes.HANDLE),
        ("hStdOutput", wintypes.HANDLE),
        ("hStdError", wintypes.HANDLE),
    ]

class PROCESS_INFORMATION(ctypes.Structure):
    _fields_ = [
        ("hProcess", wintypes.HANDLE),
        ("hThread", wintypes.HANDLE),
        ("dwProcessId", wintypes.DWORD),
        ("dwThreadId", wintypes.DWORD),
    ]

def create_suspended_process(cmdline):
    startup_info = STARTUPINFO()
    startup_info.cb = ctypes.sizeof(STARTUPINFO)
    process_info = PROCESS_INFORMATION()

    # CreateProcessW arguments
    lpApplicationName = None
    lpCommandLine = ctypes.create_unicode_buffer(cmdline)
    lpProcessAttributes = None
    lpThreadAttributes = None
    bInheritHandles = False
    dwCreationFlags = CREATE_SUSPENDED
    lpEnvironment = None
    lpCurrentDirectory = None

    success = kernel32.CreateProcessW(
        lpApplicationName,
        lpCommandLine,
        lpProcessAttributes,
        lpThreadAttributes,
        bInheritHandles,
        dwCreationFlags,
        lpEnvironment,
        lpCurrentDirectory,
        ctypes.byref(startup_info),
        ctypes.byref(process_info)
    )

    if not success:
        err = ctypes.get_last_error()
        raise ctypes.WinError(err)

    return process_info

def resume_thread(thread_handle):
    res = kernel32.ResumeThread(thread_handle)
    if res == -1:
        err = ctypes.get_last_error()
        raise ctypes.WinError(err)


def get_etwprof_path():
    """Get the path to etwprof.exe in our third_party directory."""
    import os
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    etwprof_path = os.path.join(project_root, "third_party", "custom_etwprof", "etwprof.exe")
    return etwprof_path

def main():
    if len(sys.argv) < 4 or sys.argv[1] != '--tracefile':
        print("Usage: python profile.py --tracefile <tracefile_path> <command> [command_args ...]")
        sys.exit(1)

    tracefile = sys.argv[2]
    command = sys.argv[3:]

    if not command:
        print("You must specify a command to profile")
        sys.exit(1)

    # Get the correct path to etwprof.exe
    etwprof_path = get_etwprof_path()
    if not os.path.exists(etwprof_path):
        print(f"ERROR: etwprof.exe not found at {etwprof_path}")
        sys.exit(1)

    # On Windows, we need to properly quote arguments
    cmdline = ' '.join(f'"{arg}"' if ' ' in arg else arg for arg in command)
    
    print(f"Executing command: {cmdline}")
    
    process_info = create_suspended_process(cmdline)

    # Start etwprof profiling the target process
    output_dir = os.path.dirname(os.path.abspath(tracefile))
    output_file = os.path.abspath(tracefile)
    etwprof_cmd = [etwprof_path, "profile", f"--target={process_info.dwProcessId}", f"--output={output_file}", "--minbuffers=128", "--maxbuffers=512", "--buffersize=1024"]
    print(f"Starting profiling with: {' '.join(etwprof_cmd)}")
    etwprof_proc = subprocess.Popen(etwprof_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    resume_thread(process_info.hThread)

    # Wait for the target process to finish
    kernel32.WaitForSingleObject(process_info.hProcess, 0xFFFFFFFF)

    # Wait for etwprof process to exit
    stdout, stderr = etwprof_proc.communicate()
    
    print(f"etwprof exit code: {etwprof_proc.returncode}")
    if stdout:
        print(f"etwprof stdout: {stdout}")
    if stderr:
        print(f"etwprof stderr: {stderr}")

    kernel32.CloseHandle(process_info.hThread)
    kernel32.CloseHandle(process_info.hProcess)

if __name__ == '__main__':
    main()
