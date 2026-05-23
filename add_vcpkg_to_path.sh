#!/usr/bin/env bash
# Permanently adds vcpkg debug/release bin dirs to the Windows user PATH.
# Must be run from Git Bash (uses PowerShell via pwsh to write the registry).

DEBUG_BIN="C:\\Users\\alirz\\Projects\\vcpkg\\vcpkg_installed\\x64-windows\\debug\\bin"
RELEASE_BIN="C:\\Users\\alirz\\Projects\\vcpkg\\vcpkg_installed\\x64-windows\\bin"

powershell.exe -NoProfile -Command "
  \$debugBin  = '$DEBUG_BIN'
  \$relBin    = '$RELEASE_BIN'
  \$current   = [Environment]::GetEnvironmentVariable('PATH', 'Machine')
  if (\$current -notlike \"*\$relBin*\") {
    [Environment]::SetEnvironmentVariable('PATH', \"\$relBin;\$debugBin;\$current\", 'Machine')
    Write-Host 'Added to system PATH permanently.'
  } else {
    Write-Host 'Already in system PATH, nothing changed.'
  }
"

echo ""
echo "Open a new terminal for the PATH change to take effect."
