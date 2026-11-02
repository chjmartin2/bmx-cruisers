#include "CruiserWorld.h"
#include "Core/DisplayLayout.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "GenericPlatform/GenericApplication.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ConfigCacheIni.h"
#include "UnrealClient.h"
#include "Kismet/GameplayStatics.h"

void ACruiserWorld::ApplyGraphicsPreset(){
 auto Set=[](const TCHAR* Name,int Value){if(auto* V=IConsoleManager::Get().FindConsoleVariable(Name))V->Set(Value,ECVF_SetByCode);};
 Set(TEXT("r.DynamicGlobalIlluminationMethod"),0);Set(TEXT("r.ReflectionMethod"),0);Set(TEXT("r.Shadow.Virtual.Enable"),0);
 Set(TEXT("r.SSR.Quality"),GraphicsPreset==2?2:0);Set(TEXT("r.AmbientOcclusionLevels"),GraphicsPreset==2?1:0);
 Set(TEXT("r.ShadowQuality"),GraphicsPreset==0?0:3);Set(TEXT("r.Shadow.CSM.MaxCascades"),GraphicsPreset==2?3:1);
 Set(TEXT("r.Shadow.MaxResolution"),GraphicsPreset==2?2048:512);Set(TEXT("r.Streaming.PoolSize"),GraphicsPreset==0?384:GraphicsPreset==1?512:1024);
 Set(TEXT("r.Streaming.MipBias"),0);Set(TEXT("r.MaxAnisotropy"),GraphicsPreset==0?4:8);
 Set(TEXT("r.Tonemapper.Sharpen"),GraphicsPreset==0?1:0);
 Set(TEXT("r.VolumetricFog"),0);Set(TEXT("r.MotionBlurQuality"),0);Set(TEXT("r.BloomQuality"),GraphicsPreset==0?1:3);
 Set(TEXT("t.MaxFPS"),60);
 DecorationInterval=GraphicsPreset==0?1.f/20:GraphicsPreset==1?1.f/30:0;
 UE_LOG(LogTemp,Display,TEXT("GRAPHICS PRESET %d: no Lumen, ray tracing or virtual shadows; decoration %.0f Hz"),GraphicsPreset,DecorationInterval>0?1/DecorationInterval:60);
}

void ACruiserWorld::SetGraphicsPreset(int32 Index){
 GraphicsPreset=FMath::Clamp(Index,0,2);
 GConfig->SetInt(TEXT("BMXCruiser.Display"),TEXT("GraphicsPreset"),GraphicsPreset,GGameUserSettingsIni);
 GConfig->SetInt(TEXT("BMXCruiser.Display"),TEXT("PerformanceVersion"),1,GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);
 // This setting is offered on the start-screen settings page; rebuilding the
 // selected course applies its decoration budget without touching run scores.
 UGameplayStatics::OpenLevel(this,TEXT("CourseOne"),true,FString::Printf(TEXT("Course=%d?MenuPage=5"),CourseIndex));
}

void ACruiserWorld::InitializeDisplay(){
    int Version=0;GConfig->GetInt(TEXT("BMXCruiser.Display"),TEXT("PerformanceVersion"),Version,GGameUserSettingsIni);
    if(Version>=1)GConfig->GetInt(TEXT("BMXCruiser.Display"),TEXT("GraphicsPreset"),GraphicsPreset,GGameUserSettingsIni);
    FParse::Value(FCommandLine::Get(),TEXT("GraphicsPreset="),GraphicsPreset);GraphicsPreset=FMath::Clamp(GraphicsPreset,0,2);
    ApplyGraphicsPreset();
    GConfig->GetBool(TEXT("BMXCruiser.Display"),TEXT("UHDQuality"),UHDQuality,GGameUserSettingsIni);
    int Quality=0;if(FParse::Value(FCommandLine::Get(),TEXT("RenderQuality="),Quality))UHDQuality=Quality>=2160;
    if(GIsEditor||FParse::Param(FCommandLine::Get(),TEXT("RenderOffscreen"))){DisplayReady=true;return;}
    auto* Settings=UGameUserSettings::GetGameUserSettings();
    bool Fullscreen=Settings->GetFullscreenMode()!=EWindowMode::Windowed;
    if(FParse::Param(FCommandLine::Get(),TEXT("windowed")))Fullscreen=false;
    if(FParse::Param(FCommandLine::Get(),TEXT("fullscreen")))Fullscreen=true;
    DesiredFullscreen=Fullscreen;ApplyDisplayMode(Fullscreen);
}

void ACruiserWorld::ApplyDisplayMode(bool Fullscreen){
    DesiredFullscreen=Fullscreen;
    if(GIsEditor)return;
    auto* Viewport=GetWorld()->GetGameViewport();if(!Viewport)return;
    auto Window=Viewport->GetWindow();if(!Window.IsValid())return;
    Window->SetTitle(FText::FromString(TEXT("BMX Cruisers")));
    FDisplayMetrics Metrics;FDisplayMetrics::RebuildDisplayMetrics(Metrics);
    if(Metrics.MonitorInfo.IsEmpty())return;
    FVector2D Position=Window->GetPositionInScreen(),Size=Window->GetClientSizeInScreen();
    int Selected=0;double BestOverlap=-1;
    for(int i=0;i<Metrics.MonitorInfo.Num();++i){
        const auto& R=Metrics.MonitorInfo[i].DisplayRect;
        double Overlap=FMath::Max(0.,FMath::Min(double(R.Right),Position.X+Size.X)-FMath::Max(double(R.Left),Position.X))*FMath::Max(0.,FMath::Min(double(R.Bottom),Position.Y+Size.Y)-FMath::Max(double(R.Top),Position.Y));
        if(Overlap>BestOverlap||(Overlap==BestOverlap&&Metrics.MonitorInfo[i].bIsPrimary)){BestOverlap=Overlap;Selected=i;}
    }
    const auto& Monitor=Metrics.MonitorInfo[Selected];const auto& Work=Monitor.WorkArea;
    const double DPI=FMath::Max(1.,Monitor.DPI/96.);
    auto Fit=BMX::FitWindow(Work.Right-Work.Left,Work.Bottom-Work.Top,DPI);
    FIntPoint Client=Fullscreen?FIntPoint(Monitor.DisplayRect.Right-Monitor.DisplayRect.Left,Monitor.DisplayRect.Bottom-Monitor.DisplayRect.Top):FIntPoint(Fit.Width,Fit.Height);
    FIntPoint TopLeft=Fullscreen?FIntPoint(Monitor.DisplayRect.Left,Monitor.DisplayRect.Top):FIntPoint(Work.Left+(Work.Right-Work.Left-Client.X)/2,Work.Top+(Work.Bottom-Work.Top-Client.Y)/2);
    auto* Settings=UGameUserSettings::GetGameUserSettings();
    Settings->SetDisplayProperties(Monitor.ID,Selected);
    Settings->SetFullscreenMode(Fullscreen?EWindowMode::WindowedFullscreen:EWindowMode::Windowed);
    Settings->SetScreenResolution(Client);Settings->SetWindowPosition(TopLeft.X,TopLeft.Y);
    Settings->ApplyResolutionSettings(false);Settings->ConfirmVideoMode();Settings->SaveSettings();
    Window->MoveWindowTo(FVector2D(TopLeft));
    DisplayReady=true;
    LastDisplaySize=FIntPoint::ZeroValue;
    UE_LOG(LogTemp,Display,TEXT("Display mode: %s, monitor %d rect [%d,%d,%d,%d], work [%d,%d,%d,%d], client %dx%d, DPI %.2f"),Fullscreen?TEXT("single-monitor borderless"):TEXT("fitted window"),Selected,Monitor.DisplayRect.Left,Monitor.DisplayRect.Top,Monitor.DisplayRect.Right,Monitor.DisplayRect.Bottom,Work.Left,Work.Top,Work.Right,Work.Bottom,Client.X,Client.Y,DPI);
}

void ACruiserWorld::SetResolution(bool UHD){
    UHDQuality=UHD;LastRenderPercentage=-1;
    GConfig->SetBool(TEXT("BMXCruiser.Display"),TEXT("UHDQuality"),UHDQuality,GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);
    UpdateDisplayQuality();
}

void ACruiserWorld::ToggleFullscreen(){ApplyDisplayMode(UGameUserSettings::GetGameUserSettings()->GetFullscreenMode()==EWindowMode::Windowed);}

void ACruiserWorld::UpdateDisplayQuality(){
    if(!DisplayReady)ApplyDisplayMode(DesiredFullscreen);
    auto* Client=GetWorld()->GetGameViewport();if(!Client||!Client->Viewport)return;
    FIntPoint Size=Client->Viewport->GetSizeXY();if(Size.X<=0||Size.Y<=0)return;
    const float ViewHeight=FMath::Max(1.f,FMath::Min(float(Size.Y),Size.X*9.f/16));
    const float Target=GraphicsPreset==0?900.f:GraphicsPreset==1?1080.f:UHDQuality?2160.f:1080.f;
    float Percent=FMath::Clamp(100*Target/ViewHeight,25.f,GraphicsPreset==2?200.f:100.f);
    if(Size==LastDisplaySize&&FMath::IsNearlyEqual(Percent,LastRenderPercentage))return;
    auto& Console=IConsoleManager::Get();
    if(auto* CVar=Console.FindConsoleVariable(TEXT("r.SecondaryScreenPercentage.GameViewport")))CVar->Set(100.f,ECVF_SetByCode);
    if(auto* CVar=Console.FindConsoleVariable(TEXT("r.DynamicRes.OperationMode")))CVar->Set(0,ECVF_SetByCode);
    if(auto* CVar=Console.FindConsoleVariable(TEXT("r.ScreenPercentage")))CVar->Set(Percent,ECVF_SetByCode);
    // Temporal upscalers accept up to 200%; smaller windows use full-resolution
    // temporal AA followed by spatial downsampling for the larger scene buffer.
    if(auto* CVar=Console.FindConsoleVariable(TEXT("r.AntiAliasingMethod")))CVar->Set(2,ECVF_SetByCode);
    if(auto* CVar=Console.FindConsoleVariable(TEXT("r.TemporalAA.Upsampling")))CVar->Set(1,ECVF_SetByCode);
    LastDisplaySize=Size;LastRenderPercentage=Percent;
    UE_LOG(LogTemp,Display,TEXT("Display quality: preset %d target %.0fp, viewport %dx%d, scene scale %.2f%%, window unchanged"),GraphicsPreset,Target,Size.X,Size.Y,Percent);
}
