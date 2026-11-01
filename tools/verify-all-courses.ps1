param([switch]$UHD)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$exe=Join-Path $root 'Build/Windows/BMXCruiser/Binaries/Win64/BMXCruiser.exe'
$save=Join-Path $root 'Build/Windows/BMXCruiser/Saved/SaveGames/BMXCruiser.sav'
$backup=Join-Path $root 'release-work/campaign-preferences.sav'
if(Test-Path -LiteralPath $save){Copy-Item -LiteralPath $save -Destination $backup -Force}
$log=Join-Path $root 'reference/extracted/campaign-verification.log'
$size=if($UHD){'3840x2160w'}else{'1920x1080w'}
$width=if($UHD){3840}else{1920};$height=if($UHD){2160}else{1080}
try{
 $arguments='-RenderOffscreen -windowed -ForceRes -nosound -unattended -VerifyCampaign -Course=1 -Difficulty=2 -ReplayCaptureRow=100 -CourseReplayDirectory="'+(Join-Path $root 'reference/extracted')+'" -AutoQuit=100 -ExecCmds="r.SetRes '+$size+'" -ResX='+$width+' -ResY='+$height+' -RenderQuality='+$height+' -abslog="'+$log+'"'
 $p=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
 $p | Wait-Process -Timeout 600
 $text=Get-Content -LiteralPath $log -Raw
 if($text -notmatch 'ALL FIVE COURSES VERIFIED' -or ([regex]::Matches($text,'VERIFICATION PASSED')).Count -ne 5 -or $text -match 'VERIFICATION FAILED|FINISH TERRAIN.*FAILED|Fatal error|GPU crash detected|DXGI_ERROR_DEVICE'){throw 'Campaign verification failed; inspect campaign-verification.log'}
 Write-Output 'All five packaged courses, native collisions, finish terrain and next-course transitions PASSED'
}finally{
 if(Test-Path -LiteralPath $backup){Copy-Item -LiteralPath $backup -Destination $save -Force}
}
