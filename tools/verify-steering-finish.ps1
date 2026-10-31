param([string[]]$Cases=@('normal','expert','cruise'))
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$exe=Join-Path $root 'Build/Windows/BMXCruiser/Binaries/Win64/BMXCruiser.exe'
$runs=@(
 @{Name='normal';Pace=2;Replay='course1-completion-replay.json';Quit=28;Size='1920x1080w';X=1920;Y=1080},
 @{Name='expert';Pace=4;Replay='course1-expert-steering-replay.json';Quit=23;Size='3840x2160w';X=3840;Y=2160},
 @{Name='cruise';Pace=0;Replay='course1-cruise-steering-replay.json';Quit=40;Size='1920x1080w';X=1920;Y=1080}
)
foreach($case in $runs | Where-Object {$_.Name -in $Cases}){
 $log=(Join-Path $root "reference/extracted/steering-finish-$($case.Name).log").Replace('\','/')
 $replay=(Join-Path $root "reference/extracted/$($case.Replay)").Replace('\','/')
 $start=Get-Date
 $args=@('-RenderOffscreen','-windowed','-ForceRes','-nosound','-unattended',"-ExecCmds=`"r.SetRes $($case.Size)`"","-abslog=`"$log`"","-ReplayFile=`"$replay`"",'-ReplayCaptureRow=205',"-Difficulty=$($case.Pace)","-AutoQuit=$($case.Quit)","-RenderQuality=$($case.Y)","-ResX=$($case.X)","-ResY=$($case.Y)",'-Style=1','-Gender=0','-Outfit=1','-ViewTilt=55','-ViewDistance=100')
 $p=Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru
 Wait-Process -Id $p.Id -Timeout 100
 $content=Get-Content -LiteralPath $log -Raw
 if($content -match 'VERIFICATION FAILED|FINISH TERRAIN.*FAILED|Fatal error|GPU crash detected|DXGI_ERROR_DEVICE|missing usage flag'){throw "Failed: $($case.Name)"}
 if($content -notmatch 'VERIFICATION PASSED' -or ([regex]::Matches($content,'FINISH TERRAIN.*PASSED')).Count -ne 2){throw "Incomplete: $($case.Name)"}
 $shot=Get-ChildItem (Join-Path $root 'Build/Windows/BMXCruiser/Saved/Screenshots/Windows') -Filter 'CourseOne-ride*.png' | Where-Object {$_.LastWriteTime -gt $start} | Sort-Object LastWriteTime -Descending | Select-Object -First 1
 if(!$shot){throw 'Missing finish-line screenshot'}
 Copy-Item -LiteralPath $shot.FullName -Destination (Join-Path $root "preview/finish-terrain-$($case.Name).png") -Force
 Write-Output "$($case.Name): steering route, original collisions, finish terrain PASSED"
}
