param([string]$EnginePath = '', [switch]$Package, [switch]$PrepareAssets)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'BMXCruiser.uproject'
if (!$EnginePath) {
    $candidates = @('C:/Program Files/Epic Games/UE_5.8','C:/Program Files/Epic Games/UE_5.7','D:/Epic Games/UE_5.8','D:/Epic Games/UE_5.7')
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath (Join-Path $candidate 'Engine/Build/BatchFiles/Build.bat')) { $EnginePath = $candidate; break }
    }
}
if (!$EnginePath) { throw 'Unreal Engine 5 is not installed yet. Install it in Epic Launcher, then pass -EnginePath if using a custom location.' }
$buildTool = Join-Path $EnginePath 'Engine/Build/BatchFiles/Build.bat'
& $buildTool BMXCruiserEditor Win64 Development "-Project=$projectFile" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw 'The native Unreal project did not compile. Review the build output.' }
if (!(Test-Path -LiteralPath (Join-Path $projectRoot 'Content/Characters/HeroHeadV3.uasset'))) {
    $editor = Join-Path $EnginePath 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
    $restore = (Join-Path $PSScriptRoot 'restore_engine_assets.py').Replace('\','/')
    & $editor $projectFile '-EnablePlugins=MetaHumanCharacter,MetaHuman' -run=pythonscript "-script=$restore" -unattended -nosplash -NullRHI -stdout -FullStdOutLogOutput
    if ($LASTEXITCODE -ne 0) { throw 'The installed MetaHuman templates could not be restored.' }
}
$assetMarker = Join-Path $projectRoot 'Content/Data/assets-ready.json'
if ($PrepareAssets -or !(Test-Path -LiteralPath $assetMarker)) {
    if (Test-Path -LiteralPath $assetMarker) { Remove-Item -LiteralPath $assetMarker }
    $editor = Join-Path $EnginePath 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
    $assetScript = (Join-Path $PSScriptRoot 'prepare_unreal_assets.py').Replace('\','/')
    & $editor $projectFile -run=pythonscript "-script=$assetScript" -unattended -nosplash -NullRHI -stdout -FullStdOutLogOutput
    if ($LASTEXITCODE -ne 0 -or !(Test-Path -LiteralPath $assetMarker)) { throw 'Unreal asset preparation did not complete.' }
}
if ($Package) {
    $automation = Join-Path $EnginePath 'Engine/Build/BatchFiles/RunUAT.bat'
    $destination = Join-Path $projectRoot 'Build/Windows'
    & $automation BuildCookRun "-project=$projectFile" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$destination" -utf8output
    if ($LASTEXITCODE -ne 0) { throw 'The Windows packaging step failed.' }
    Copy-Item -LiteralPath (Join-Path $projectRoot 'assets/CREDITS.md') -Destination (Join-Path $destination 'CREDITS.txt') -Force
    Copy-Item -LiteralPath (Join-Path $projectRoot 'assets/source/rocketbox/LICENSE.md') -Destination (Join-Path $destination 'LICENSE-Rocketbox.txt') -Force
    Copy-Item -LiteralPath (Join-Path $EnginePath 'Engine/Source/ThirdParty/Licenses/ROBOTO_License.txt') -Destination (Join-Path $destination 'LICENSE-Roboto.txt') -Force
    Write-Output "Windows build prepared in $destination"
}
