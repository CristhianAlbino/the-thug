# Installs the third-party art The Thug uses locally (not in the repository; see 05_ART/ASSETS.md).
#
#   1. Epic Mannequin (Manny/Quinn + ABP_Unarmed): copied from the engine's template resources.
#      Needs "Templates and Feature Packs" in the Epic Launcher (Unreal Engine 5.8 > ... > Manage modules).
#   2. Horror Hospital (Fab, Higher Fusion Studio, free): Epic Launcher > Development > Fab > Horror Hospital >
#      Add to project > TheThug. Then this script runs build_kit.py to pick the pieces and cap textures at 2K.
#
# Usage:  powershell -ExecutionPolicy Bypass -File UnrealProject\Scripts\setup_art.ps1 [-Engine "C:\...\UE_5.8"]
param([string]$Engine = "$env:USERPROFILE\Epic Games\UE_5.8")

$Project = Split-Path -Parent $PSScriptRoot
$Uproject = Join-Path $Project 'TheThug.uproject'

$Mannequins = Join-Path $Engine 'Templates\TemplateResources\High\Characters\Content\Mannequins'
$Target = Join-Path $Project 'Content\Characters\Mannequins'
if (Test-Path $Mannequins) {
    New-Item -ItemType Directory -Force (Split-Path $Target) | Out-Null
    Copy-Item $Mannequins -Destination (Split-Path $Target) -Recurse -Force
    Write-Host "Mannequin copied to Content/Characters/Mannequins"
} else {
    Write-Warning "Mannequin not found in $Mannequins (install 'Templates and Feature Packs'). Characters will use the block bodies."
}

if (Test-Path (Join-Path $Project 'Content\Horror_Hospital')) {
    $Cmd = Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    $Script = (Join-Path $PSScriptRoot 'build_kit.py') -replace '\\', '/'
    & $Cmd $Uproject -run=pythonscript "-script=$Script" -unattended -nosplash -nullrhi | Select-String '\[KIT\]'
} else {
    Write-Warning "Horror Hospital not in Content/. Add it from Fab (Epic Launcher). The hospital will keep the procedural art."
}
