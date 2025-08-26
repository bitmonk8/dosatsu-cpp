import sys
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


def main():
    if len(sys.argv) < 4 or sys.argv[1] != '--tracefile':
        print("Usage: python profile.py --tracefile <tracefile_path> <command> [command_args ...]")
        sys.exit(1)

    tracefile = sys.argv[2]
    command = sys.argv[3:]

    if not command:
        print("You must specify a command to profile")
        sys.exit(1)

    cmdline = ' '.join(shlex.quote(arg) for arg in command)

    process_info = create_suspended_process(cmdline)

    # Start etwprof profiling the target process
    etwprof_cmd = ["etwprof", "/start", "/file", tracefile, "/pid", str(process_info.dwProcessId)]
    etwprof_proc = subprocess.Popen(etwprof_cmd)

    resume_thread(process_info.hThread)

    # Wait for the target process to finish
    kernel32.WaitForSingleObject(process_info.hProcess, 0xFFFFFFFF)

    # Wait for etwprof process to exit
    etwprof_proc.wait()

    kernel32.CloseHandle(process_info.hThread)
    kernel32.CloseHandle(process_info.hProcess)

if __name__ == '__main__':
    main()
