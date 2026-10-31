#include "CruiserHUD.h"
#include "CruiserWorld.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "CanvasItem.h"
#include "Fonts/SlateFontInfo.h"
#include "Fonts/CompositeFont.h"
#include "Misc/Paths.h"
#include "Core/DisplayLayout.h"
namespace { ACruiserWorld* Ride(UWorld* W){for(TActorIterator<ACruiserWorld>I(W);I;++I)return *I;return nullptr;} }
void ACruiserHUD::DrawHUD(){
 Super::DrawHUD();if(!Canvas)return;auto* G=Ride(GetWorld());if(!G)return;
 const BMX::HUDLayout L(Canvas->SizeX,Canvas->SizeY,Canvas->GetDPIScale());const float S=L.Scale,X=L.X,Y=L.Y;
 const FLinearColor Paper(.95f,.94f,.86f),Yellow(.94f,.91f,.07f),Red(.91f,.075f,.035f),Blue(.045f,.12f,.25f),Ink(.008f,.014f,.029f,.97f),Dim(.54f,.61f,.66f);
 float Time=GetWorld()->GetTimeSeconds();
 auto Rect=[&](float A,float B,float W,float H,FLinearColor C){DrawRect(C,X+A*S,Y+B*S,W*S,H*S);};
 if(!RaceTypeface){RaceTypeface=NewObject<UFont>(this);RaceTypeface->FontCacheType=EFontCacheType::Runtime;RaceTypeface->GetMutableInternalCompositeFont()=FCompositeFont(TEXT("Race"),FPaths::ProjectContentDir()/TEXT("UI/Fonts/Roboto-BoldCondensedItalic.ttf"),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);}
 if(!BodyTypeface){BodyTypeface=NewObject<UFont>(this);BodyTypeface->FontCacheType=EFontCacheType::Runtime;BodyTypeface->GetMutableInternalCompositeFont()=FCompositeFont(TEXT("Body"),FPaths::ProjectContentDir()/TEXT("UI/Fonts/Roboto-Medium.ttf"),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);}
 auto Text=[&](FString T,float A,float B,float Z,FLinearColor C,bool Bold=false){
  FSlateFontInfo F(Bold?RaceTypeface.Get():BodyTypeface.Get(),FMath::Max(8.f,14*Z*S),Bold?FName(TEXT("Race")):FName(TEXT("Body")));
  FCanvasTextItem I(FVector2D(X+A*S,Y+B*S),FText::FromString(T),F,C);I.EnableShadow(FLinearColor(0,0,0,.45),FVector2D(1,1)*S);Canvas->DrawItem(I);
 };
 auto Line=[&](FVector2D A,FVector2D B,FLinearColor C,float W){FCanvasLineItem I(FVector2D(X,Y)+A*S,FVector2D(X,Y)+B*S);I.SetColor(C);I.LineThickness=W*S;Canvas->DrawItem(I);};
 auto Tri=[&](FVector2D A,FVector2D B,FVector2D C,FLinearColor Col){FCanvasTriangleItem I(FVector2D(X,Y)+A*S,FVector2D(X,Y)+B*S,FVector2D(X,Y)+C*S,GWhiteTexture);I.SetColor(Col);I.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(I);};
 auto Plate=[&](float A,float B,float W,float H,FLinearColor C,float Cut=18){Tri(FVector2D(A,B),FVector2D(A+W,B),FVector2D(A+W-Cut,B+H),C);Tri(FVector2D(A,B),FVector2D(A+W-Cut,B+H),FVector2D(A,B+H),C);};
 auto Art=[&](const TCHAR* Path,float A,float B,float W,float H,float Alpha=1){auto* T=LoadObject<UTexture2D>(nullptr,Path);if(T)DrawTexture(T,X+A*S,Y+B*S,W*S,H*S,0,0,1,1,FLinearColor(1,1,1,Alpha),BLEND_Translucent);};
 auto Actions=G->MenuActions();float MX=-1,MY=-1;if(PlayerOwner)PlayerOwner->GetMousePosition(MX,MY);MX=L.DesignX(MX);MY=L.DesignY(MY);
 auto Button=[&](int32 I,float A,float B,float W,float H){FName N=Actions[I];bool Selected=G->MenuFocus==I,Hover=MX>=A&&MX<=A+W&&MY>=B&&MY<=B+H;
  if(Selected){Plate(A+7,B+7,W,H,Red);Plate(A,B,W,H,Yellow);for(int J=0;J<4;++J)Line(FVector2D(A+W-100+J*15,B+H-13),FVector2D(A+W-85+J*15,B+13),FLinearColor(.07,.09,.08,.2),3);}
  else {Plate(A,B,W,H,Hover?FLinearColor(.10,.16,.24,.97):FLinearColor(.027,.044,.075,.93));Line(FVector2D(A,B+H),FVector2D(A+W-19,B+H),FLinearColor(.18,.25,.32,.8),1);}
  FLinearColor Fore=Selected?Ink:Paper;
  Text(FString::Printf(TEXT("%02d"),I+1),A+15,B+H*.24f,1.45,Selected?FLinearColor(.35,.32,.02):Red,true);
  Text(G->MenuLabel(N),A+64,B+H*.25f,1.35,Fore,true);
  AddHitBox(FVector2D(L.PixelX(A),L.PixelY(B)),FVector2D(W*S*L.DPI,H*S*L.DPI),N,true);
 };
 auto Panel=[&](float Width){
  Plate(0,0,Width+34,1080,Ink,74);Art(TEXT("/Game/UI/Splash.Splash"),-180,0,960,540,.065);
  for(int I=0;I<22;++I){float A=I*43;Line(FVector2D(A,1080),FVector2D(Width*.5f+(A-Width*.5f)*.32f,710),FLinearColor(.55,.06,.055,.18),1);}
  for(int I=0;I<10;++I){float H=710+370*FMath::Pow(I/9.f,1.7f);Line(FVector2D(0,H),FVector2D(Width,H),FLinearColor(.55,.06,.055,.18),1);}
  Rect(Width-3,0,3,1080,Red);Rect(Width-15,70,7,120,Yellow);
  for(int I=0;I<6;++I)Plate(38+I*22,1008,13,25,I<3?Red:Yellow,9);
 };
 const auto State=G->Simulation->GetState();const bool Ready=State==BMX::RunState::Ready;
 if(Ready&&G->MenuPage==0){
  Art(TEXT("/Game/UI/Splash.Splash"),0,0,1920,1080);Rect(0,922,1920,158,FLinearColor(.005,.008,.02,.84));
  Text(TEXT("A CLASSIC CHALLENGE. BUILT TO RIDE AGAIN."),70,949,1.55,Paper,true);Button(0,1130,956,720,70);Text(TEXT("COURSE ONE / COUNTRY LANES"),70,1001,.92,Yellow);
 }else if(Ready&&(G->MenuPage==1||G->MenuPage==4)){
  Rect(0,0,1920,1080,Ink);Art(TEXT("/Game/UI/Splash.Splash"),1030,-40,1050,590,.13);
  Art(TEXT("/Game/UI/Logo.Logo"),70,24,390,218);Plate(70,265,1765,7,Red,7);
  Text(G->MenuPage==1?TEXT("RIDER FIELD GUIDE"):TEXT("THE ORIGINAL / THE CREW"),70,301,3.2,Yellow,true);
  if(G->MenuPage==1){
   Text(TEXT("READ THE LINE. FIND THE GAP."),70,395,2.1,Paper,true);
   Text(TEXT("Water, bushes, rocks and fences. Eight original screens. One clean ride."),70,458,1.5,Paper);
   Plate(70,545,1765,205,FLinearColor(.035,.055,.09,.96));
   Text(TEXT("ARROWS / WASD"),105,579,2,Yellow,true);Text(TEXT("MOVE & STEER"),105,637,1.15,Dim);
   Text(TEXT("P / ESC"),700,579,2,Yellow,true);Text(TEXT("PAUSE / RETURN"),700,637,1.15,Dim);
   Text(TEXT("R"),1240,579,2,Yellow,true);Text(TEXT("RESTART THE RIDE"),1240,637,1.15,Dim);
   Text(TEXT("Normal matches the C64. Other paces change course speed; steering stays responsive."),70,790,1.3,Paper);
   Text(TEXT("Every character and ride style shares the same course, speed and collision rules."),70,838,1.3,Dim);
  }else{
   Text(TEXT("BMX RACERS"),70,402,2.3,Paper,true);
   Text(TEXT("Original game programming: David Darling & Richard Darling"),70,468,1.5,Paper);
   Text(TEXT("Original publisher: Mastertronic. Original melody retained and rearranged."),70,521,1.3,Dim);
   Text(TEXT("BMX CRUISERS / COURSE ONE"),70,608,2,Yellow,true);
   Text(TEXT("Riders: Microsoft Rocketbox / MIT. Scenery: Poly Haven & ambientCG / CC0."),70,672,1.3,Paper);
   Text(TEXT("Equipment meshes, trail dressing, animation and presentation authored for this game."),70,720,1.3,Paper);
   Text(TEXT("Cover-inspired artwork adapted using AI. Full source credits and licenses included."),70,774,1.3,Dim);
  }
  Button(0,70,945,1765,73);
 }else if(G->MenuPage==6&&G->IsMenu()){
  Panel(650);Art(TEXT("/Game/UI/Logo.Logo"),52,15,380,212);
  Text(TEXT("SET YOUR VIEW"),55,268,2.2,Yellow,true);
  Text(TEXT("LIVE COURSE PREVIEW"),58,315,.9,Dim);
  for(int I=0;I<Actions.Num();++I)Button(I,55,390+I*90,545,65);
  Text(TEXT("LOWER TILT: LOOK FARTHER AHEAD"),55,805,.92,Paper);
  Text(TEXT("HIGHER TILT: LOOK MORE DIRECTLY DOWN"),55,844,.86,Paper);
  Text(TEXT("PGUP / PGDN  TILT     + / -  ZOOM"),55,925,.9,Yellow);
  Text(TEXT("HOME  RESET     ESC  BACK"),55,967,.9,Paper);
  Text(TEXT("ARROWS  SELECT / ADJUST    ENTER  CONFIRM"),55,1045,.78,Paper);
 }else if(Ready){
  bool Workshop=G->MenuPage==3,Settings=G->MenuPage==5;const float Width=Workshop?710:760;Panel(Width);
  Art(TEXT("/Game/UI/Logo.Logo"),52,15,440,246);
  Text(Workshop?TEXT("RIDER GARAGE"):Settings?TEXT("TUNE THE EXPERIENCE"):TEXT("01 / COUNTRY LANES"),55,268,2.2,Yellow,true);
  Text(Workshop?TEXT("YOUR LOOK. YOUR RIDE."):Settings?TEXT("PICTURE, SOUND & DISPLAY"):TEXT("EIGHT SCREENS. EVERY GAP COUNTS."),58,315,.9,Dim);
  const float Top=369,H=Workshop?65:Settings?72:66,Gap=Workshop?15:Settings?26:16;
  for(int I=0;I<Actions.Num();++I)Button(I,55,Top+I*(H+Gap),Width-105,H);
  if(Workshop){
   Text(TEXT("SKIN PALETTE"),57,947,.86,Dim);
   for(int I=0;I<7;++I){float A=57+I*76;Plate(A,976,60,18,G->SkinColor(I),4);if(I==G->SkinTone)Line(FVector2D(A,1000),FVector2D(A+55,1000),Yellow,3);}
   Plate(935,916,850,112,Ink);Plate(935,916,850,5,Red,5);Text(G->VehicleName(G->VehicleStyle),972,931,2.1,Yellow,true);
   Text(G->VehicleStyle==3?TEXT("HARD SHELL / ALLOY FRAME / 80 mm INLINE WHEELS"):G->VehicleStyle==4?TEXT("STITCHED HIGH TOP / TWIN TRUCKS / TOE STOP"):G->VehicleStyle==2?TEXT("110 mm WHEELS / EXTRUDED DECK / DOUBLE CLAMP"):TEXT("BUILT FOR THE ORIGINAL COURSE. STYLED BY YOU."),974,974,.86,Paper);
   Text(TEXT("Q / E ROTATE    + / - ZOOM    COSMETIC CHOICES"),974,1003,.75,Dim);
  }else{
   Plate(1130,851,660,147,Ink);Text(TEXT("READY TO RIDE"),1165,878,2.7,Yellow,true);Text(FString::Printf(TEXT("%s / BEST %06d"),G->VehicleName(G->VehicleStyle),G->HighScore),1168,941,1.1,Paper);
   if(!Settings)Text(TEXT("COURSE PACE CHANGES. STEERING STAYS NORMAL."),58,970,.80,Paper);
  }
  Text(TEXT("ARROWS / TAB  SELECT    ENTER  CONFIRM    ESC  BACK"),55,1045,.83,Paper);
 }else{
  Plate(0,0,480,69,Ink);Plate(742,0,450,69,Ink);Plate(1450,0,470,69,Ink,0);Rect(0,67,1920,2,Red);
  Text(FString::Printf(TEXT("SCORE  %06d"),G->Simulation->Score()),30,15,1.9,Paper,true);Text(TEXT("COURSE 01"),828,15,1.9,Yellow,true);Text(FString::Printf(TEXT("HI  %06d"),FMath::Max(G->HighScore,G->Simulation->Score())),1580,15,1.9,Paper,true);
  Rect(0,1041,1920,39,Ink);Text(TEXT("ARROWS / WASD  STEER     P / ESC  PAUSE     R  RESTART     C  CAMERA     PGUP / PGDN  TILT     + / -  ZOOM"),30,1051,.83,Dim);Text(G->DifficultyName(G->ActiveDifficulty),1720,1051,.88,Yellow,true);
  if(!G->IsMenu()&&Time<G->ViewNoticeUntil){Plate(630,87,700,58,Ink);Text(FString::Printf(TEXT("VIEW  %.0f DEGREES / %.0f%%     HOME: RESET"),G->ViewTilt,G->ViewDistance),660,103,1.1,Yellow,true);}
  if(G->IsMenu()){
   bool Finished=State==BMX::RunState::Complete;float Top=Finished?345:200;Plate(480,Top,970,Finished?675:723,Ink,32);Plate(480,Top,970,7,Red,7);
   Text(G->Paused?TEXT("TAKE A BREATHER"):Finished?TEXT("COURSE CONQUERED!"):TEXT("BACK IN THE SADDLE"),530,Top+38,3,Yellow,true);
   Text(Finished?TEXT("Every gap cleared. Take your victory lap."):TEXT("Find your pace. Then find your line."),534,Top+104,1.2,Dim);
   for(int I=0;I<Actions.Num();++I)Button(I,530,Top+164+I*(Finished?68:78),870,Finished?55:62);
  }
 }
 if(!G->Failure.IsEmpty()){Rect(200,400,1520,160,Ink);Text(G->Failure,240,448,1.5,Paper);}
}
void ACruiserHUD::NotifyHitBoxClick(FName N){Super::NotifyHitBoxClick(N);if(auto* G=Ride(GetWorld()))G->MenuAction(N);}
