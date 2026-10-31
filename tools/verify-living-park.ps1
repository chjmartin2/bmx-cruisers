param([string]$Name='living-park')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$exe=Join-Path $root 'Build/Windows/BMXCruiser/Binaries/Win64/BMXCruiser.exe'
$replay=(Join-Path $root 'reference/extracted/course1-completion-replay.json').Replace('\','/')
$expertReplay=(Join-Path $root 'reference/extracted/course1-expert-steering-replay.json').Replace('\','/')
$tests=@(
 @{Name='cosmetics';Args=@('-VerifyCosmetics','-AutoQuit=16','-MenuPage=3','-RenderQuality=1080','-ResX=1920','-ResY=1080')},
 @{Name='skin-tones';Args=@('-VerifySkinTones','-AutoQuit=25','-MenuPage=3','-RenderQuality=1080','-ResX=1920','-ResY=1080')},
 @{Name='normal';Args=@("-ReplayFile=`"$replay`"",'-ReplayCaptureRow=135','-Difficulty=2','-Style=1','-Gender=0','-Outfit=0','-AutoQuit=28','-RenderQuality=1080','-ResX=1920','-ResY=1080','-ProfileCourse','-csvCaptureFrames=0','-csvCaptureOnStart')},
 @{Name='expert-4k';Args=@("-ReplayFile=`"$expertReplay`"",'-ReplayCaptureRow=135','-Difficulty=4','-Style=5','-Gender=1','-Outfit=1','-AutoQuit=23','-RenderQuality=2160','-ResX=3840','-ResY=2160')}
)
foreach($test in $tests){
 $log=(Join-Path $root "reference/extracted/$Name-$($test.Name).log").Replace('\','/')
 $dims=if($test.Name -eq 'expert-4k'){'3840x2160w'}else{'1920x1080w'}
 $args=@('-RenderOffscreen','-windowed','-ForceRes','-nosound','-unattended',"-ExecCmds=`"r.SetRes $dims,csv.Overlay 0`"","-abslog=`"$log`"")+$test.Args
 $p=Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru
 Wait-Process -Id $p.Id -Timeout 100
 $text=Get-Content -LiteralPath $log -Raw
 if($text -match 'Failed to compile Material|Fatal error|Assertion failed|COSMETIC INVARIANT.*FAILED|VERIFICATION FAILED|GPU crash detected|DXGI_ERROR_DEVICE|missing usage flag'){throw "$($test.Name) failed; see $log"}
 if($test.Name -eq 'cosmetics' -or $test.Name -eq 'skin-tones'){$expected=if($test.Name -eq 'skin-tones'){14}else{6};if(([regex]::Matches($text,'COSMETIC INVARIANT.*PASSED')).Count -ne $expected){throw "The $expected appearance invariant checks did not finish"}}
 elseif($text -notmatch 'VERIFICATION PASSED'){throw "$($test.Name) did not complete the route"}
 Write-Output "$($test.Name): PASSED"
}

