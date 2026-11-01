param([string]$BuildPath='Build/BridgeRails/Windows',[switch]$OnlyBridge)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$package=Join-Path $root $BuildPath
$exe=Join-Path $package 'BMXCruiser/Binaries/Win64/BMXCruiser.exe'
$replay=Join-Path $root 'release-work/crash-straight-replay.json'
New-Item -ItemType Directory -Path (Split-Path $replay) -Force | Out-Null
# No steering: hit the real Course Three boulder or the Course One shoreline.
@{cycles_per_input=49262;inputs=@(0)*1200} | ConvertTo-Json -Compress | Set-Content -LiteralPath $replay -Encoding ASCII
$profiles=@('SaveGames/BMXCruiser.sav','Config/Windows/GameUserSettings.ini')
$backups=@{}
foreach($relative in $profiles){
 $path=Join-Path $package ('BMXCruiser/Saved/'+$relative)
 if(Test-Path -LiteralPath $path){$backups[$path]=[IO.File]::ReadAllBytes($path)}else{$backups[$path]=$null}
}
$cases=@(
 @{name='crash-boulder';course=3;pace=2;width=1920;height=1080;quit=15;kind='impact';campaign=$false},
 @{name='crash-cruise';course=1;pace=0;width=1920;height=1080;quit=22;kind='splash';campaign=$true},
 @{name='crash-expert-4k';course=1;pace=4;width=3840;height=2160;quit=14;kind='splash';campaign=$true},
 @{name='crash-bridge-rail';course=1;pace=2;width=1920;height=1080;quit=22;kind='impact';campaign=$true;replay='tests/replays/bridge-rail-strike.json'}
)
if($OnlyBridge){$cases=@($cases | Where-Object {$_.name -eq 'crash-bridge-rail'})}
try{
 foreach($case in $cases){
  $log=Join-Path $root ('reference/extracted/'+$case.name+'.log')
  $caseReplay=if($case.replay){Join-Path $root $case.replay}else{$replay}
  $arguments='-RenderOffscreen -windowed -ForceRes -nosound -unattended -VerifyCrashPresentation -VerifyContacts -Course='+$case.course+' -Difficulty='+$case.pace+' -ReplayFile="'+$caseReplay+'" -AutoQuit='+$case.quit+' -ResX='+$case.width+' -ResY='+$case.height+' -RenderQuality='+$case.height+' -abslog="'+$log+'"'
  if($case.campaign){$arguments+=' -VerifyCampaign'}
  $process=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
  $process | Wait-Process -Timeout 180
  $text=Get-Content -LiteralPath $log -Raw
  if($text -notmatch 'CRASH HOLD AND INPUT GUARD PASSED' -or $text -notmatch ('CRASH PRESENTATION AND RESULT RULES PASSED: '+$case.kind) -or $text -match 'CRASH .*FAILED|Fatal error|GPU crash detected|DXGI_ERROR_DEVICE'){
   throw ($case.name+' failed; inspect '+$log)
  }
  if($case.replay -and ($text -notmatch 'BRIDGE GUARDRAIL CONTACTS course 1: PASSED' -or $text -notmatch 'CRASH PRESENTATION AND RESULT RULES PASSED: impact; hold [0-9.]+; score 127')){throw 'Expected bridge rail collision was not reached'}
  $screens=Join-Path $package 'BMXCruiser/Saved/Screenshots/Windows'
  $image=Get-ChildItem -LiteralPath $screens -Filter 'Crash-reaction*.png' | Sort-Object LastWriteTime | Select-Object -Last 1
  if(!$image){throw 'Crash screenshot was not written'}
  Copy-Item -LiteralPath $image.FullName -Destination (Join-Path $root ('release-work/'+$case.name+'.png')) -Force
  Write-Output ($case.name+': visible '+$case.kind+', 1.8-second hold, input lock, frozen score and result rules PASSED')
 }
}finally{
 foreach($path in $backups.Keys){
  if($null -ne $backups[$path]){[IO.File]::WriteAllBytes($path,$backups[$path])}
  elseif(Test-Path -LiteralPath $path){Remove-Item -LiteralPath $path}
 }
}
