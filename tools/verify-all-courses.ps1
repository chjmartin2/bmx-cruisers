param([switch]$UHD,[string]$BuildPath='',[string]$ReplayDirectory='tests/replays/visible',[switch]$OriginalCollision)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
if(!$BuildPath){foreach($candidate in @('Build/Release1.1Final/Windows','Build/Release1.1Game/Windows','Build/Windows/Windows','Build/Windows')){if(Test-Path -LiteralPath (Join-Path $root ($candidate+'/BMXCruiser.exe'))){$BuildPath=$candidate;break}}}
$exe=Join-Path (Join-Path $root $BuildPath) 'BMXCruiser/Binaries/Win64/BMXCruiser.exe'
$save=Join-Path (Join-Path $root $BuildPath) 'BMXCruiser/Saved/SaveGames/BMXCruiser.sav'
$backup=Join-Path $root 'release-work/campaign-preferences.sav'
$hadSave=Test-Path -LiteralPath $save
if($hadSave){Copy-Item -LiteralPath $save -Destination $backup -Force}
$log=Join-Path $root 'reference/extracted/campaign-verification.log'
$size=if($UHD){'3840x2160w'}else{'1920x1080w'}
$width=if($UHD){3840}else{1920};$height=if($UHD){2160}else{1080}
try{
 $arguments='-RenderOffscreen -windowed -ForceRes -nosound -unattended -VerifyCampaign -Course=1 -Difficulty=2 -ReplayCaptureRow=100 -CourseReplayDirectory="'+(Join-Path $root $ReplayDirectory)+'" -AutoQuit=100 -ExecCmds="r.SetRes '+$size+'" -ResX='+$width+' -ResY='+$height+' -RenderQuality='+$height+' -abslog="'+$log+'"'
 if($OriginalCollision){$arguments+=' -OriginalCollision'}
 $p=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
 $p | Wait-Process -Timeout 600
 $text=Get-Content -LiteralPath $log -Raw
 if($text -notmatch 'ALL FIVE COURSES VERIFIED' -or $text -notmatch 'ONE LIFE RUN QUALIFIED PASSED' -or ([regex]::Matches($text,'VERIFICATION PASSED')).Count -ne 5 -or $text -match 'VERIFICATION FAILED|FINISH TERRAIN.*FAILED|Fatal error|GPU crash detected|DXGI_ERROR_DEVICE'){throw 'Campaign verification failed; inspect campaign-verification.log'}
 Write-Output 'All five packaged courses, collisions, finish terrain and next-course transitions PASSED'
}finally{
 if($hadSave -and (Test-Path -LiteralPath $backup)){Copy-Item -LiteralPath $backup -Destination $save -Force}
}
