# Setting Up Cursor for Visual Studio C++ Development

This guide helps you configure Cursor to automatically set up the Visual Studio environment for C++ development.

## Quick Solutions

### Option 1: Manual Activation (Use Immediately)
Before running any build commands in the terminal:
```bash
.\conanbuild.bat
```
Then run your commands normally:
```bash
xmake
xmake test
python tools/build.py full
```

### Option 2: Use the Development Environment Script
We've created `dev-env.bat` for convenient development:
```bash
# Build the project
.\dev-env.bat build

# Run tests
.\dev-env.bat test

# Format code
.\dev-env.bat format

# Start a shell with environment set up
.\dev-env.bat shell

# Run Python build scripts
.\dev-env.bat python tools/build.py full
```

### Option 3: Configure Cursor Terminal (Automatic)

#### Method A: PowerShell Profile (Recommended)
1. Copy the PowerShell profile to your PowerShell profile directory:
   ```powershell
   # Check if profile directory exists
   if (!(Test-Path (Split-Path $PROFILE))) {
       New-Item -ItemType Directory -Force -Path (Split-Path $PROFILE)
   }
   
   # Copy our profile to your PowerShell profile
   Copy-Item "cursor-powershell-profile.ps1" $PROFILE -Force
   ```

2. Restart Cursor or reload the terminal. The VS environment will be automatically set up.

#### Method B: Cursor Workspace Settings
1. Open Cursor Settings (Ctrl+,)
2. Go to Terminal settings
3. Set the terminal shell args to run your environment setup:
   - For PowerShell: Add `"-Command", "& { . './cursor-powershell-profile.ps1'; }"`

#### Method C: Create a Custom Terminal Profile
1. In Cursor, go to Terminal -> Configure Terminal Profiles
2. Create a new profile:
   ```json
   {
     "terminal.integrated.profiles.windows": {
       "CppGraphIndex PowerShell": {
         "source": "PowerShell",
         "args": ["-NoExit", "-Command", "& { if (Test-Path './conanbuild.bat') { Write-Host 'Setting up VS environment...' -ForegroundColor Yellow; cmd /c './conanbuild.bat' >$null 2>&1; Write-Host 'VS environment ready!' -ForegroundColor Green } }"]
       }
     },
     "terminal.integrated.defaultProfile.windows": "CppGraphIndex PowerShell"
   }
   ```

## Verification

To verify the environment is set up correctly:
```bash
# Check if VS environment is active
echo $env:VCINSTALLDIR

# Or use our helper command (if using the PowerShell profile)
vs-check

# Test building
xmake
```

## Troubleshooting

### Environment Not Loading
- Make sure you're in the project root directory
- Verify `conanbuild.bat` exists and works: `.\conanbuild.bat`
- Check that Visual Studio 2022 Professional is installed at the expected path

### Commands Not Found
- The Python build scripts now automatically set up the VS environment
- Use `.\dev-env.bat` for guaranteed environment setup
- Manually run `.\conanbuild.bat` if needed

### Cursor Not Using the Profile
- Check your PowerShell execution policy: `Get-ExecutionPolicy`
- You may need to set it: `Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser`
- Restart Cursor completely

## What the Environment Setup Does

The `conanbuild.bat` script calls:
1. `conanvcvars.bat` - Sets up Visual Studio 2022 Professional environment
2. `conanbuildenv-release-x86_64.bat` - Sets up build dependencies

This ensures all tools (compiler, linker, etc.) are available in the PATH and environment variables are correctly set for MSVC builds.