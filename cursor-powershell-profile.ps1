# PowerShell Profile for CppGraphIndex Development
# This script automatically sets up the Visual Studio environment for C++ development

# Check if we're in the CppGraphIndex project directory
$projectRoot = Get-Location
$conanBuildScript = Join-Path $projectRoot "conanbuild.bat"

if (Test-Path $conanBuildScript) {
    Write-Host "[CURSOR-PROFILE] Detected CppGraphIndex project" -ForegroundColor Green
    
    # Check if VS environment is already set up (by looking for VCINSTALLDIR)
    if (-not $env:VCINSTALLDIR) {
        Write-Host "[CURSOR-PROFILE] Setting up Visual Studio 2022 environment..." -ForegroundColor Yellow
        
        try {
            # Run conanbuild.bat and capture environment
            $tempBat = Join-Path $projectRoot "temp_env_setup.bat"
            
            @"
@echo off
call "$conanBuildScript"
echo ==ENV_START==
set
echo ==ENV_END==
"@ | Out-File -FilePath $tempBat -Encoding ASCII
            
            $envOutput = & cmd.exe /c $tempBat 2>&1
            Remove-Item $tempBat -ErrorAction SilentlyContinue
            
            # Parse environment variables
            $captureStarted = $false
            foreach ($line in $envOutput) {
                if ($line -eq "==ENV_START==") {
                    $captureStarted = $true
                    continue
                } elseif ($line -eq "==ENV_END==") {
                    break
                } elseif ($captureStarted -and $line -match "^([^=]+)=(.*)$") {
                    $envName = $matches[1]
                    $envValue = $matches[2]
                    
                    # Only set important environment variables
                    if ($envName -match "^(PATH|INCLUDE|LIB|LIBPATH|VCINSTALLDIR|VS.*|WINDOWS.*|Platform)$") {
                        [Environment]::SetEnvironmentVariable($envName, $envValue, "Process")
                    }
                }
            }
            
            Write-Host "[CURSOR-PROFILE] ✅ Visual Studio environment configured" -ForegroundColor Green
            Write-Host "[CURSOR-PROFILE] You can now run: xmake, xmake test, python tools/build.py, etc." -ForegroundColor Cyan
            
        } catch {
            Write-Warning "[CURSOR-PROFILE] Failed to set up VS environment: $_"
            Write-Host "[CURSOR-PROFILE] You may need to run '.\dev-env.bat shell' manually" -ForegroundColor Yellow
        }
    } else {
        Write-Host "[CURSOR-PROFILE] ✅ Visual Studio environment already configured" -ForegroundColor Green
    }
    
    # Add helpful aliases
    function dev-build { & ".\dev-env.bat" "build" @args }
    function dev-test { & ".\dev-env.bat" "test" @args }
    function dev-format { & ".\dev-env.bat" "format" @args }
    function dev-lint { & ".\dev-env.bat" "lint" @args }
    function dev-clean { & ".\dev-env.bat" "clean" @args }
    
    Write-Host "[CURSOR-PROFILE] Available commands: dev-build, dev-test, dev-format, dev-lint, dev-clean" -ForegroundColor Cyan
    
} else {
    Write-Host "[CURSOR-PROFILE] Not in CppGraphIndex project directory" -ForegroundColor Gray
}

# General development helpers
function Get-VSVersion {
    if ($env:VCINSTALLDIR) {
        Write-Host "Visual Studio Environment: ACTIVE" -ForegroundColor Green
        Write-Host "VCINSTALLDIR: $env:VCINSTALLDIR" -ForegroundColor Gray
    } else {
        Write-Host "Visual Studio Environment: NOT ACTIVE" -ForegroundColor Red
    }
}

# Alias for quick VS environment check
Set-Alias -Name "vs-check" -Value Get-VSVersion