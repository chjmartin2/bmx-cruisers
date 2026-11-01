#include "CruiserWorld.h"
#include "Core/RunRules.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

const TCHAR* ACruiserWorld::VehicleName(int32 I){const TCHAR* N[]={TEXT("CRUISER"),TEXT("BMX"),TEXT("SCOOTER"),TEXT("ROLLER BLADES"),TEXT("ROLLER SKATES"),TEXT("WHEELCHAIR")};return N[FMath::Clamp(I,0,5)];}
const TCHAR* ACruiserWorld::ColorName(int32 I){const TCHAR* N[]={TEXT("COBALT"),TEXT("CRIMSON"),TEXT("SUNSHINE"),TEXT("FOREST"),TEXT("VIOLET"),TEXT("PEARL"),TEXT("CHARCOAL"),TEXT("TANGERINE")};return N[FMath::Clamp(I,0,7)];}
const TCHAR* ACruiserWorld::SkinName(int32 I){const TCHAR* N[]={TEXT("PORCELAIN"),TEXT("FAIR"),TEXT("SAND"),TEXT("HONEY"),TEXT("BRONZE"),TEXT("UMBER"),TEXT("DEEP")};return N[FMath::Clamp(I,0,6)];}
FLinearColor ACruiserWorld::SkinColor(int32 I){const FLinearColor N[]={FLinearColor(.72f,.49f,.39f),FLinearColor(.58f,.34f,.24f),FLinearColor(.43f,.235f,.135f),FLinearColor(.31f,.16f,.075f),FLinearColor(.20f,.085f,.038f),FLinearColor(.105f,.039f,.017f),FLinearColor(.047f,.017f,.008f)};return N[FMath::Clamp(I,0,6)];}
TArray<FName> ACruiserWorld::MenuActions()const{
    if(MenuPage==7)return {TEXT("course1"),TEXT("course2"),TEXT("course3"),TEXT("course4"),TEXT("course5"),TEXT("back")};
    if(MenuPage==8)return {TEXT("back")};
    if(MenuPage==6)return {TEXT("viewtilt"),TEXT("viewdistance"),TEXT("viewreset"),TEXT("back")};
    if(PlayingGame&&Simulation->GetState()==BMX::RunState::Complete)return CourseIndex<5?TArray<FName>{TEXT("nextcourse"),TEXT("title")}:TArray<FName>{TEXT("game"),TEXT("scores"),TEXT("courses"),TEXT("title")};
    if(PlayingGame&&Simulation->GetState()==BMX::RunState::Crashed)return {TEXT("game"),TEXT("courses"),TEXT("scores"),TEXT("title")};
    if(Simulation->GetState()==BMX::RunState::Complete)return CourseIndex<5?TArray<FName>{TEXT("nextcourse"),TEXT("ride"),TEXT("courses"),TEXT("title")}:TArray<FName>{TEXT("ride"),TEXT("courses"),TEXT("title")};
    if(Simulation->GetState()!=BMX::RunState::Ready)return PlayingGame?TArray<FName>{TEXT("ride"),TEXT("view"),TEXT("title"),TEXT("music"),TEXT("effects"),TEXT("fullscreen")}:TArray<FName>{TEXT("ride"),TEXT("pace"),TEXT("view"),TEXT("title"),TEXT("music"),TEXT("effects"),TEXT("fullscreen")};
    if(MenuPage<2)return {TEXT("continue")};
    if(MenuPage==3)return {TEXT("gender"),TEXT("skin"),TEXT("outfit"),TEXT("helmet"),TEXT("bikecolor"),TEXT("style"),TEXT("back")};
    if(MenuPage==4)return {TEXT("back")};
    if(MenuPage==5)return {TEXT("music"),TEXT("effects"),TEXT("quality"),TEXT("fullscreen"),TEXT("view"),TEXT("back")};
    return {TEXT("game"),TEXT("courses"),TEXT("scores"),TEXT("pace"),TEXT("customize"),TEXT("settings"),TEXT("intro"),TEXT("credits"),TEXT("quit")};
}
FString ACruiserWorld::MenuLabel(FName A)const{
    if(A==TEXT("game"))return TEXT("PLAY GAME / ONE LIFE");
    if(A==TEXT("scores"))return TEXT("HIGH SCORE BOARD");
    if(A==TEXT("courses"))return TEXT("PRACTICE COURSES");
    if(A==TEXT("nextcourse"))return FString::Printf(TEXT("CONTINUE TO COURSE %d"),CourseIndex+1);
    if(A.ToString().StartsWith(TEXT("course")))return FString::Printf(TEXT("COURSE %s"),*A.ToString().Right(1));
    if(A==TEXT("back")&&MenuPage==6)return Simulation->GetState()==BMX::RunState::Ready?TEXT("BACK TO SETTINGS"):TEXT("BACK TO PAUSE MENU");
    if(A==TEXT("view"))return TEXT("CAMERA VIEW");
    if(A==TEXT("viewtilt"))return FString::Printf(TEXT("TILT     <  %.0f DEGREES  >"),ViewTilt);
    if(A==TEXT("viewdistance"))return FString::Printf(TEXT("DISTANCE     <  %.0f%%  >"),ViewDistance);
    if(A==TEXT("viewreset"))return TEXT("RESET DEFAULT VIEW");
    if(A==TEXT("ride"))return Simulation->GetState()==BMX::RunState::Ready?FString::Printf(TEXT("RIDE COURSE %d"),CourseIndex):Paused&&ActiveDifficulty==DifficultyIndex?TEXT("RESUME RIDE"):TEXT("RIDE AGAIN");
    if(A==TEXT("pace"))return FString::Printf(TEXT("PACE     <  %s  /  %.0f%%  >"),DifficultyName(DifficultyIndex),100*BMX::RideSettings::Speed(DifficultyIndex));
    if(A==TEXT("gender"))return FString::Printf(TEXT("CHARACTER     <  %s  >"),Gender?TEXT("GIRL"):TEXT("BOY"));
    if(A==TEXT("skin"))return FString::Printf(TEXT("SKIN TONE     <  %s  >"),SkinName(SkinTone));
    if(A==TEXT("settings"))return TEXT("PICTURE & SOUND");
    if(A==TEXT("outfit"))return FString::Printf(TEXT("CLOTHING     <  %s  >"),Outfit?TEXT("FULL EXTREME"):TEXT("CASUAL"));
    if(A==TEXT("helmet"))return FString::Printf(TEXT("HELMET     <  %s  >"),ColorName(HelmetColor));
    if(A==TEXT("bikecolor"))return FString::Printf(TEXT("VEHICLE COLOR     <  %s  >"),ColorName(BikeColor));
    if(A==TEXT("style"))return FString::Printf(TEXT("RIDE STYLE     <  %s  >"),VehicleName(VehicleStyle));
    if(A==TEXT("music"))return Muted?TEXT("GUITAR THEME     OFF"):TEXT("GUITAR THEME     ON");
    if(A==TEXT("effects"))return EffectsMuted?TEXT("NATURE & RIDE SOUND     OFF"):TEXT("NATURE & RIDE SOUND     ON");
    if(A==TEXT("quality"))return UHDQuality?TEXT("PICTURE DETAIL     4K"):TEXT("PICTURE DETAIL     1080p");
    if(A==TEXT("fullscreen"))return TEXT("TOGGLE FULLSCREEN     F11");
    if(A==TEXT("customize"))return TEXT("CUSTOMIZE CHARACTER");
    if(A==TEXT("intro"))return TEXT("HOW TO PLAY");
    if(A==TEXT("title"))return TEXT("BACK TO START");
    if(A==TEXT("credits"))return TEXT("CREDITS");
    if(A==TEXT("quit"))return TEXT("QUIT TO DESKTOP");
    if(A==TEXT("continue"))return TEXT("PRESS ENTER TO CONTINUE");
    return TEXT("BACK TO MAIN MENU");
}
void ACruiserWorld::MenuMove(int32 D){auto A=MenuActions();MenuFocus=(MenuFocus+D+A.Num())%A.Num();PlayEffect(TEXT("select"));}
void ACruiserWorld::MenuAdjust(int32 D){auto A=MenuActions();MenuFocus=FMath::Clamp(MenuFocus,0,A.Num()-1);FName N=A[MenuFocus];if(N==TEXT("pace")||N==TEXT("gender")||N==TEXT("skin")||N==TEXT("outfit")||N==TEXT("helmet")||N==TEXT("bikecolor")||N==TEXT("style")||N==TEXT("quality")||N==TEXT("viewtilt")||N==TEXT("viewdistance"))MenuAction(N,D);}
void ACruiserWorld::MenuAccept(){auto A=MenuActions();MenuAction(A[FMath::Clamp(MenuFocus,0,A.Num()-1)]);}
void ACruiserWorld::MenuBack(){if(MenuPage==6){MenuPage=Simulation->GetState()==BMX::RunState::Ready?5:2;MenuFocus=0;return;}if(Simulation->GetState()!=BMX::RunState::Ready){if(Paused)Paused=false;else ReturnToTitle();}else {MenuPage=2;MenuFocus=0;}}
void ACruiserWorld::MenuAction(FName A,int32 D){
    if(!IsMenu())return;
    if(A==TEXT("continue")){MenuPage=MenuPage==0?1:2;MenuFocus=0;}
    else if(A==TEXT("game"))PlayGame();
    else if(A==TEXT("scores")){ReturnToTitle();MenuPage=8;MenuFocus=0;}
    else if(A==TEXT("courses")){if(Simulation->GetState()!=BMX::RunState::Ready)ReturnToTitle();MenuPage=7;MenuFocus=CourseIndex-1;}
    else if(A==TEXT("nextcourse"))SelectCourse(CourseIndex+1,true);
    else if(A.ToString().StartsWith(TEXT("course")))SelectCourse(FCString::Atoi(*A.ToString().Right(1)));
    else if(A==TEXT("ride"))StartRide();
    else if(A==TEXT("title"))ReturnToTitle();
    else if(A==TEXT("pace"))SetDifficulty((DifficultyIndex+D+5)%5);
    else if(A==TEXT("customize")){MenuPage=3;MenuFocus=0;}
    else if(A==TEXT("view")){MenuPage=6;MenuFocus=0;}
    else if(A==TEXT("viewtilt"))AdjustRideView(D*5,0);
    else if(A==TEXT("viewdistance"))AdjustRideView(0,D*5);
    else if(A==TEXT("viewreset"))AdjustRideView(0,0,true);
    else if(A==TEXT("settings")){MenuPage=5;MenuFocus=0;}
    else if(A==TEXT("intro")){MenuPage=1;MenuFocus=0;}
    else if(A==TEXT("credits")){MenuPage=4;MenuFocus=0;}
    else if(A==TEXT("back")){MenuBack();SavePreferences();}
    else if(A==TEXT("music"))ToggleMusic();
    else if(A==TEXT("effects"))ToggleEffects();
    else if(A==TEXT("quality"))SetResolution(!UHDQuality);
    else if(A==TEXT("fullscreen"))ToggleFullscreen();
    else if(A==TEXT("quit")){if(auto* PC=GetWorld()->GetFirstPlayerController())PC->ConsoleCommand(TEXT("quit"));}
    else {
        if(A==TEXT("gender"))Gender=(Gender+1)%2;
        if(A==TEXT("skin"))SkinTone=(SkinTone+D+7)%7;
        if(A==TEXT("outfit"))Outfit=(Outfit+1)%2;
        if(A==TEXT("helmet"))HelmetColor=(HelmetColor+D+8)%8;
        if(A==TEXT("bikecolor"))BikeColor=(BikeColor+D+8)%8;
        if(A==TEXT("style"))VehicleStyle=(VehicleStyle+D+6)%6;
        RebuildAppearance();SavePreferences();
    }
    PlayEffect(TEXT("select"));
}
void ACruiserWorld::SavePreferences(){
    if(ReplayInputs.Num()||FParse::Param(FCommandLine::Get(),TEXT("VerifyCampaign")))return;
    auto* Save=Cast<UCrusierSave>(UGameplayStatics::CreateSaveGameObject(UCrusierSave::StaticClass()));
    Save->CourseBestScores=CourseBestScores;Save->CampaignBestScores=CampaignBestScores;
    Save->CompletedRunBestScores=CampaignBestScores;
    Save->HighScore=CourseBestScores[BMX::RideSettings::Normal];Save->BestScores.Reset();for(int I=0;I<5;++I)Save->BestScores.Add(CourseBestScores[I]);
    Save->ViewTilt=ViewTilt;Save->ViewDistance=ViewDistance;
    Save->SkinTone=SkinTone;Save->Pace=DifficultyIndex;Save->Gender=Gender;Save->Outfit=Outfit;Save->HelmetColor=HelmetColor;Save->BikeColor=BikeColor;Save->VehicleStyle=VehicleStyle;
    UGameplayStatics::SaveGameToSlot(Save,TEXT("BMXCruiser"),0);
}

// Development-only capture/invariant checks run the real actor and all skeletal
// assets. Cosmetic rebuilds must not change even one byte of simulation memory.
void ACruiserWorld::VerifyAppearance(float Dt){
 VerificationTime+=Dt;
 if(FParse::Param(FCommandLine::Get(),TEXT("VerifyCampaign"))&&Simulation->GetState()==BMX::RunState::Complete&&CelebrationTime>3){
  if(CourseIndex<5){SelectCourse(CourseIndex+1,true);return;}
  const bool Qualified=BMX::RunRules::Qualifies(PlayingGame,CourseIndex,ClearedCourses,true,ActiveDifficulty,RunPace)&&ScoreBase+Simulation->Score()==1960;
  UE_LOG(LogTemp,Display,TEXT("ONE LIFE RUN QUALIFIED %s; cleared %d, total %d"),Qualified?TEXT("PASSED"):TEXT("FAILED"),ClearedCourses,ScoreBase+Simulation->Score());
  UE_LOG(LogTemp,Display,TEXT("ALL FIVE COURSES VERIFIED"));FPlatformMisc::RequestExit(false);return;
 }
 if(ReplayInputs.Num()&&CelebrationTime>1.7f&&!CelebrationCaptured){CelebrationCaptured=true;FScreenshotRequest::RequestScreenshot(TEXT("Cruisers-fireworks"),true,true);}
 float QuitAfter=0;if(FParse::Value(FCommandLine::Get(),TEXT("AutoQuit="),QuitAfter)&&VerificationTime>QuitAfter)FPlatformMisc::RequestExit(false);
 const bool SkinCheck=FParse::Param(FCommandLine::Get(),TEXT("VerifySkinTones"));
 if(!SkinCheck&&!FParse::Param(FCommandLine::Get(),TEXT("VerifyCosmetics")))return;
 const float Interval=SkinCheck?1.5f:2.f;
 int Step=FMath::FloorToInt((VerificationTime-2)/Interval);
 if(Step<0||Step>=(SkinCheck?14:6))return;
 if(Step!=VerificationStyle){
  VerificationStyle=Step;VerificationCapture=false;
  auto Memory=Simulation->Memory();auto Cycles=Simulation->Cycles();auto State=Simulation->GetState();
  VehicleStyle=SkinCheck?1:Step;Gender=Step%2;Outfit=1;SkinTone=SkinCheck?Step/2:Step%7;HelmetColor=SkinCheck?0:Step;BikeColor=SkinCheck?0:Step;MenuPage=3;
  RebuildAppearance();
  bool Same=Memory==Simulation->Memory()&&Cycles==Simulation->Cycles()&&State==Simulation->GetState();
  UE_LOG(LogTemp,Display,TEXT("COSMETIC INVARIANT style %d gender %d outfit %d skin %d: %s"),VehicleStyle,Gender,Outfit,SkinTone,Same?TEXT("PASSED"):TEXT("FAILED"));
 }
 if(!VerificationCapture&&FMath::Fmod(VerificationTime-2,Interval)>.8f){VerificationCapture=true;FScreenshotRequest::RequestScreenshot(SkinCheck?FString::Printf(TEXT("Cruisers-skin-%d-%d"),SkinTone,Gender):FString::Printf(TEXT("Cruisers-style-%d"),Step),true,true);}
}

void ACruiserWorld::AdjustRideView(float TiltDelta,float DistanceDelta,bool Reset){
 ViewTilt=Reset?40:FMath::Clamp(ViewTilt+TiltDelta,30.f,75.f);
 ViewDistance=Reset?100:FMath::Clamp(ViewDistance+DistanceDelta,90.f,140.f);
 ViewNoticeUntil=GetWorld()->GetTimeSeconds()+2.5f;
 SavePreferences();
}
