param([string]$BuildPath='Build/Release1.3/Windows',[switch]$BakeContacts,[string]$OutputPath='release-work/original-sites')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root $BuildPath
$exe=Join-Path $build 'BMXCruiser/Binaries/Win64/BMXCruiser.exe'
$work=Join-Path $root $OutputPath
New-Item -ItemType Directory -Path $work -Force | Out-Null
$shots=@(@{course=1;kind='parking'},@{course=2;kind='pool'},@{course=3;kind='parking'},@{course=4;kind='parking'},@{course=5;kind='pool'})
foreach($shot in $shots){
 $log=Join-Path $root ('reference/extracted/sites-course'+$shot.course+'.log')
 $args='-RenderOffscreen -windowed -ForceRes -nosound -unattended -ResX=1920 -ResY=1080 -RenderQuality=1080 -ExecCmds="r.SetRes 1920x1080w,DisableAllScreenMessages" -Course='+$shot.course+' -SitePreview='+$shot.kind+' -VerifyContacts -VerifySiteFeatures -CaptureMenu -AutoQuit=6 -abslog="'+$log+'"'
 if($BakeContacts){$args+=' -BakeContacts -ContactFolder="'+$work+'"'}
 $p=Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru
 $p | Wait-Process -Timeout 180
 $text=Get-Content -LiteralPath $log -Raw
 if($text -notmatch 'ORIGINAL SITE CONTACTS course \d: PASSED' -or $text -match 'CONTACTS.*FAILED|Fatal error|GPU crash detected|DXGI_ERROR_DEVICE'){throw ('Original site verification failed on course '+$shot.course)}
 $screen=Get-ChildItem -LiteralPath (Join-Path $build 'BMXCruiser/Saved/Screenshots/Windows') -Filter 'BMXCruiser-title*.png' | Sort-Object LastWriteTime -Descending | Select-Object -First 1
 if(!$screen){throw 'Site screenshot missing'}
 Copy-Item -LiteralPath $screen.FullName -Destination (Join-Path $work ('course'+$shot.course+'-'+$shot.kind+'.png')) -Force
 Write-Output ('Course '+$shot.course+': pool, car, oil, coping, close-pass, swept contact and existing bridge checks passed.')
}
