param([string]$Output = 'release-work/frames')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$outputRoot=Join-Path $root $Output
$exe=Join-Path $root 'Build/Windows/BMXCruiser/Binaries/Win64/BMXCruiser.exe'
$save=Join-Path $root 'Build/Windows/BMXCruiser/Saved/SaveGames/BMXCruiser.sav'
$backup=Join-Path $root 'release-work/trailer-user-preferences.sav'
New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
if(Test-Path -LiteralPath $save){Copy-Item -LiteralPath $save -Destination $backup -Force}
$shots=@(
 @{Name='title';Seconds=6;Extra='-MenuPage=0 -Style=1 -Gender=0 -Outfit=1 -SkinTone=2'},
 @{Name='ride';Seconds=29;Extra='-Difficulty=2 -Style=1 -Gender=0 -Outfit=1 -SkinTone=2 -ViewTilt=40 -ViewDistance=100 -ReplayFile="'+(Join-Path $root 'reference/extracted/course1-completion-replay.json')+'"'},
 @{Name='garage-bmx';Seconds=7;Extra='-MenuPage=3 -Style=1 -Gender=0 -Outfit=1 -SkinTone=2 -PreviewOrbit=135 -PreviewZoom=0.9 -TrailerOrbit'},
 @{Name='garage-inline';Seconds=7;Extra='-MenuPage=3 -Style=3 -Gender=1 -Outfit=0 -SkinTone=4 -PreviewOrbit=135 -PreviewZoom=0.9 -TrailerOrbit'},
 @{Name='garage-wheelchair';Seconds=7;Extra='-MenuPage=3 -Style=5 -Gender=0 -Outfit=0 -SkinTone=5 -PreviewOrbit=135 -PreviewZoom=1 -TrailerOrbit'}
)
try {
 foreach($shot in $shots){
  $dir=Join-Path $outputRoot $shot.Name
  New-Item -ItemType Directory -Path $dir -Force | Out-Null
  $log=Join-Path $root ('reference/extracted/trailer-'+$shot.Name+'.log')
  $arguments='-RenderOffscreen -windowed -ForceRes -nosound -unattended -ResX=1920 -ResY=1080 -RenderQuality=1080 -ExecCmds="r.SetRes 1920x1080w,DisableAllScreenMessages" -CaptureVideo="'+$dir+'" -AutoQuit='+$shot.Seconds+' -abslog="'+$log+'" '+$shot.Extra
  $p=Start-Process -FilePath $exe -ArgumentList $arguments -WorkingDirectory $root -WindowStyle Hidden -PassThru
  $p | Wait-Process -Timeout 600
  $frames=@(Get-ChildItem -LiteralPath $dir -Filter 'frame-*.png')
  if($frames.Count -lt ($shot.Seconds*30-3)){throw "Incomplete capture: $($shot.Name), $($frames.Count) frames"}
  if($shot.Name -eq 'ride' -and !(Select-String -LiteralPath $log -SimpleMatch 'VERIFICATION PASSED' -Quiet)){throw 'Trailer replay failed'}
  Write-Output "$($shot.Name): $($frames.Count) frames captured"
 }
} finally {
 if(Test-Path -LiteralPath $backup){Copy-Item -LiteralPath $backup -Destination $save -Force}
}
