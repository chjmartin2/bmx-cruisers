param([string]$Only='')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$work=Join-Path $root 'release-work/trailer-1.2'
$exe=Join-Path $root 'Build/TrailerCapture/Windows/BMXCruiser/Binaries/Win64/BMXCruiser.exe'
$ffmpeg='C:/dev/ffmpeg/bin/ffmpeg.exe'
$probe='C:/dev/ffmpeg/bin/ffprobe.exe'
New-Item -ItemType Directory -Path (Join-Path $work 'sources') -Force | Out-Null
$clean=Join-Path $root 'tests/replays/visible/course1-trailer-replay.json'
$shots=@(
 @{name='course1';start=5;end=17;extra='-Course=1 -VerifyCampaign -ReplayFile="'+$clean+'" -Outfit=1 -HelmetColor=1 -BikeColor=1'},
 @{name='course2';start=4.3;end=9.5;extra='-Course=2 -ReplayFile="'+(Join-Path $root 'tests/replays/visible/course2-completion-replay.json')+'" -Outfit=1 -HelmetColor=2 -BikeColor=2'},
 @{name='course3';start=4.3;end=9;extra='-Course=3 -ReplayFile="'+(Join-Path $root 'tests/replays/visible/course3-completion-replay.json')+'" -Outfit=0 -HelmetColor=1 -BikeColor=1'},
 @{name='course4';start=4.3;end=9;extra='-Course=4 -ReplayFile="'+(Join-Path $root 'tests/replays/visible/course4-completion-replay.json')+'" -Gender=1 -SkinTone=4 -Outfit=1 -HelmetColor=2 -BikeColor=2'},
 @{name='course5';start=4.3;end=9;extra='-Course=5 -ReplayFile="'+(Join-Path $root 'tests/replays/visible/course5-completion-replay.json')+'" -SkinTone=5 -Outfit=1 -HelmetColor=1 -BikeColor=1'},
 @{name='practice-menu';start=.7;end=3.7;extra='-MenuPage=7 -MenuFocus=2'},
 @{name='pace-menu';start=.7;end=3.7;extra='-MenuPage=2 -MenuFocus=3 -TrailerController'},
 @{name='practice-retry';start=5.5;end=13.5;extra='-Course=3 -ReplayFile="'+(Join-Path $root 'tests/replays/trailer-crash-straight.json')+'" -TrailerRetry -Outfit=0 -HelmetColor=1 -BikeColor=1'},
 @{name='garage-cruiser';start=1;end=4.6;extra='-MenuPage=3 -Style=0 -Gender=1 -Outfit=0 -SkinTone=4 -HelmetColor=5 -BikeColor=4 -PreviewOrbit=133 -PreviewZoom=.88 -TrailerOrbit'},
 @{name='garage-scooter';start=1;end=4.6;extra='-MenuPage=3 -Style=2 -Gender=0 -Outfit=1 -SkinTone=1 -HelmetColor=2 -BikeColor=2 -PreviewOrbit=140 -PreviewZoom=.87 -TrailerOrbit'},
 @{name='garage-inline';start=1;end=4.6;extra='-MenuPage=3 -Style=3 -Gender=1 -Outfit=0 -SkinTone=4 -HelmetColor=4 -BikeColor=4 -PreviewOrbit=135 -PreviewZoom=.88 -TrailerOrbit'},
 @{name='garage-quad';start=1;end=4.6;extra='-MenuPage=3 -Style=4 -Gender=0 -Outfit=1 -SkinTone=5 -HelmetColor=1 -BikeColor=1 -PreviewOrbit=132 -PreviewZoom=.88 -TrailerOrbit'},
 @{name='garage-chair';start=1;end=4.6;extra='-MenuPage=3 -Style=5 -Gender=1 -Outfit=0 -SkinTone=6 -HelmetColor=3 -BikeColor=3 -PreviewOrbit=140 -PreviewZoom=.83 -TrailerOrbit'},
 @{name='garage-bmx';start=1;end=4.6;extra='-MenuPage=3 -Style=1 -Gender=0 -Outfit=1 -SkinTone=2 -HelmetColor=1 -BikeColor=1 -PreviewOrbit=135 -PreviewZoom=.88 -TrailerOrbit'},
 @{name='controls-camera';start=4.5;end=12;extra='-Course=1 -ReplayFile="'+$clean+'" -TrailerController -TrailerCamera -ViewDistance=110 -Outfit=1 -HelmetColor=1 -BikeColor=1'},
 @{name='finish';start=0;end=32;extra='-Course=1 -ReplayFile="'+$clean+'" -CaptureFinishOnly -TrailerFinish -Outfit=1 -HelmetColor=1 -BikeColor=1'},
 @{name='finish-approach';start=22.8;end=25;extra='-Course=1 -ReplayFile="'+$clean+'" -TrailerFinish -Outfit=1 -HelmetColor=1 -BikeColor=1'},
 @{name='title';start=.5;end=4.5;extra='-MenuPage=0 -Outfit=1 -HelmetColor=1 -BikeColor=1'}
)
foreach($shot in $shots){
 if($Only -and $shot.name -ne $Only){continue}
 $video=Join-Path $work ('sources/'+$shot.name+'.mp4')
 if(Test-Path -LiteralPath $video){Write-Output ('Already captured: '+$shot.name);continue}
 $frames=Join-Path $work ('frames/'+$shot.name)
 if(Test-Path -LiteralPath $frames){throw ('Inspect the existing incomplete capture before retrying: '+$frames)}
 New-Item -ItemType Directory -Path $frames -Force | Out-Null
 $log=Join-Path $root ('reference/extracted/trailer12-'+$shot.name+'.log')
 $args='-RenderOffscreen -windowed -ForceRes -nosound -unattended -ResX=3840 -ResY=2160 -RenderQuality=2160 -ExecCmds="r.SetRes 3840x2160w,r.ScreenPercentage 100,DisableAllScreenMessages" -Difficulty=2 -CaptureVideo="'+$frames+'" -CaptureStart='+$shot.start+' -AutoQuit='+$shot.end+' -abslog="'+$log+'" '+$shot.extra
 $process=Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru
 $process | Wait-Process -Timeout 600
 $images=@(Get-ChildItem -LiteralPath $frames -Filter 'frame-*.jpg')
 $minimum=if($shot.name -eq 'finish'){120}else{[int](($shot.end-$shot.start)*30)-4}
 if($images.Count -lt $minimum){throw ('Incomplete '+$shot.name+': '+$images.Count+' frames')}
 $text=Get-Content -LiteralPath $log -Raw
 if($text -match 'Fatal error|GPU crash detected|DXGI_ERROR_DEVICE'){throw ('Render failed: '+$shot.name)}
 if($shot.name -eq 'finish' -and $text -notmatch 'VERIFICATION PASSED'){throw 'Finish route did not complete with collisions enabled'}
 & $ffmpeg -hide_banner -loglevel warning -y -framerate 30 -i (Join-Path $frames 'frame-%06d.jpg') -vf 'scale=iw:ih:in_range=pc:out_range=tv:out_color_matrix=bt709,format=yuv420p' -c:v h264_nvenc -preset p6 -rc vbr -cq 17 -b:v 0 -spatial-aq 1 -temporal-aq 1 -color_primaries bt709 -color_trc bt709 -colorspace bt709 -movflags +faststart -an $video
 if($LASTEXITCODE -ne 0){throw ('Source encoding failed: '+$shot.name)}
 $info=& $probe -v error -select_streams v:0 -show_entries stream=width,height,nb_frames -of json $video | ConvertFrom-Json
 if($info.streams[0].width -ne 3840 -or $info.streams[0].height -ne 2160){throw 'Capture was not native 4K'}
 $proof=Join-Path $work ('sources/'+$shot.name+'-proof.jpg')
 Copy-Item -LiteralPath $images[[int]($images.Count/2)].FullName -Destination $proof
 # Remove only this verified shot's raw files, after its native 4K source exists.
 $resolved=(Resolve-Path -LiteralPath $frames).Path
 $boundary=(Join-Path $root 'release-work/trailer-1.2/frames').Replace('/','\')+'\'
 if(!$resolved.StartsWith($boundary,[StringComparison]::OrdinalIgnoreCase)){throw 'Unexpected capture cleanup path'}
 Remove-Item -LiteralPath $resolved -Recurse -Force
 Write-Output ($shot.name+': '+$images.Count+' native 4K frames encoded')
}
