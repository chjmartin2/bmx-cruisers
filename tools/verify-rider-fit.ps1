param([string]$Name='rider-fit')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$exe=Join-Path $root 'Build/Windows/BMXCruiser/Binaries/Win64/BMXCruiser.exe'
$screens=Join-Path $root 'Build/Windows/BMXCruiser/Saved/Screenshots/Windows'
$cases=@(
 @{Label='boy-casual-front';Gender=0;Outfit=0;Style=1;Orbit=0},
 @{Label='boy-extreme-front';Gender=0;Outfit=1;Style=1;Orbit=0},
 @{Label='girl-extreme-side';Gender=1;Outfit=1;Style=2;Orbit=-45},
 @{Label='girl-casual-rear';Gender=1;Outfit=0;Style=5;Orbit=180}
)
foreach($case in $cases){
 $log=(Join-Path $root "reference/extracted/$Name-$($case.Label).log").Replace('\','/')
 $start=Get-Date
 $args=@('-RenderOffscreen','-windowed','-ForceRes','-nosound','-unattended','-ExecCmds="r.SetRes 3840x2160w"',"-abslog=`"$log`"",'-MenuPage=3','-CaptureMenu','-AutoQuit=7','-RenderQuality=2160','-ResX=3840','-ResY=2160','-SkinTone=2',"-Style=$($case.Style)","-Gender=$($case.Gender)","-Outfit=$($case.Outfit)","-PreviewOrbit=$($case.Orbit)",'-PreviewZoom=.72')
 $p=Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru
 Wait-Process -Id $p.Id -Timeout 100
 $content=Get-Content -LiteralPath $log -Raw
 if($content -match 'Failed to compile Material|Fatal error|Assertion failed|Failed to find object|missing usage flag|GPU crash detected|DXGI_ERROR_DEVICE'){throw "Rider fit failed: $($case.Label)"}
 $capture=Get-ChildItem -LiteralPath $screens -Filter 'BMXCruiser-title*.png' | Where-Object {$_.LastWriteTime -gt $start} | Sort-Object LastWriteTime -Descending | Select-Object -First 1
 if(!$capture){throw "No capture for $($case.Label)"}
 Copy-Item -LiteralPath $capture.FullName -Destination (Join-Path $root "preview/$Name-$($case.Label).png") -Force
 Write-Output "$($case.Label): captured"
}
