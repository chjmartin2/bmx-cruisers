#include "CruiserWorld.h"
#include "Camera/CameraComponent.h"
#include "ProceduralMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace {
struct FSiteMesh{TArray<FVector> V,N;TArray<int32>I;TArray<FVector2D>UV;TArray<FLinearColor>C;};
FBox2D Bounds(const TSharedPtr<FJsonObject>& O){auto B=O->GetArrayField(TEXT("bounds"));return FBox2D(FVector2D(B[1]->AsNumber()*7.5,(B[0]->AsNumber()-160)*7.5),FVector2D(B[3]->AsNumber()*7.5,(B[2]->AsNumber()-160)*7.5));}
void Quad(FSiteMesh& G,FVector A,FVector B,FVector C,FVector D,FVector N,FLinearColor Tint){
 if(FVector::DotProduct(FVector::CrossProduct(C-A,B-A),N)<0)Swap(B,D);
 int K=G.V.Num();for(auto P:{A,B,C,D}){G.V.Add(P);G.N.Add(N);G.C.Add(Tint);}
 G.UV.Append({FVector2D(0,0),FVector2D((B-A).Size()/200,0),FVector2D((B-A).Size()/200,(C-B).Size()/200),FVector2D(0,(C-B).Size()/200)});
 G.I.Append({K,K+1,K+2,K,K+2,K+3});
}
void Box(FSiteMesh& G,FVector P,FVector H,FLinearColor C){
 auto At=[&](float X,float Y,float Z){return P+FVector(X*H.X,Y*H.Y,Z*H.Z);};
 for(float S:{-1.f,1.f}){
  Quad(G,At(S,-1,-1),At(S,1,-1),At(S,1,1),At(S,-1,1),FVector(S,0,0),C);
  Quad(G,At(-1,S,-1),At(1,S,-1),At(1,S,1),At(-1,S,1),FVector(0,S,0),C);
  Quad(G,At(-1,-1,S),At(1,-1,S),At(1,1,S),At(-1,1,S),FVector(0,0,S),C);
 }
}
void Tube(FSiteMesh& G,FVector A,FVector B,float Radius,FLinearColor Tint){
 FVector Axis=(B-A).GetSafeNormal(),U=FVector::CrossProduct(Axis,FVector::UpVector).GetSafeNormal();if(U.IsNearlyZero())U=FVector::RightVector;
 FVector V=FVector::CrossProduct(Axis,U);int K=G.V.Num();constexpr int Sides=12;
 for(int R=0;R<2;++R)for(int S=0;S<=Sides;++S){float T=S*2*PI/Sides;FVector N=U*FMath::Cos(T)+V*FMath::Sin(T);G.V.Add((R?B:A)+N*Radius);G.N.Add(N);G.UV.Add(FVector2D(R*(B-A).Size()/100,S/float(Sides)));G.C.Add(Tint);}
 for(int S=0;S<Sides;++S){int Q=K+S;G.I.Append({Q,Q+Sides+1,Q+1,Q+1,Q+Sides+1,Q+Sides+2});}
}
void Wheel(FSiteMesh& G,FVector P,float Radius,float Thickness,FLinearColor Tint){
 int K=G.V.Num();constexpr int Rings=32,Sides=12;
 for(int R=0;R<=Rings;++R)for(int S=0;S<=Sides;++S){float A=R*2*PI/Rings,T=S*2*PI/Sides;FVector Radial(FMath::Cos(A),0,FMath::Sin(A)),N=Radial*FMath::Cos(T)+FVector(0,FMath::Sin(T),0);G.V.Add(P+Radial*Radius+N*Thickness);G.N.Add(N);G.UV.Add(FVector2D(R/float(Rings),S/float(Sides)));G.C.Add(Tint);}
 for(int R=0;R<Rings;++R)for(int S=0;S<Sides;++S){int Q=K+R*(Sides+1)+S;G.I.Append({Q,Q+Sides+1,Q+1,Q+1,Q+Sides+1,Q+Sides+2});}
}
}

void ACruiserWorld::BuildOriginalSites(const TSharedPtr<FJsonObject>& Visual){
 SiteExclusions.Reset();SitePreviewTargets.Reset();
 const auto Features=Visual->GetObjectField(TEXT("site_features"));
 TMap<FString,FSiteMesh> Meshes;
 // Populate before retaining references: TMap insertion may relocate values.
 for(const TCHAR* Name:{TEXT("CarPaint"),TEXT("CarGlass"),TEXT("Rubber"),TEXT("Metal"),TEXT("Dark"),TEXT("PoolTiles"),TEXT("ParkingPaint"),TEXT("OilSlick")})Meshes.Add(Name);
 auto& Paint=Meshes.FindOrAdd(TEXT("CarPaint"));auto& Glass=Meshes.FindOrAdd(TEXT("CarGlass"));
 auto& Rubber=Meshes.FindOrAdd(TEXT("Rubber"));auto& Chrome=Meshes.FindOrAdd(TEXT("Metal"));
 auto& Dark=Meshes.FindOrAdd(TEXT("Dark"));auto& Tiles=Meshes.FindOrAdd(TEXT("PoolTiles"));
 auto& Lines=Meshes.FindOrAdd(TEXT("ParkingPaint"));auto& Oil=Meshes.FindOrAdd(TEXT("OilSlick"));
 for(auto Value:Features->GetArrayField(TEXT("parking_lots"))){auto B=Bounds(Value->AsObject());SiteExclusions.Add(B);if(!SitePreviewTargets.Contains(TEXT("parking")))SitePreviewTargets.Add(TEXT("parking"),FVector(B.GetCenter(),0));}
 for(auto Value:Features->GetArrayField(TEXT("parking_markings"))){auto R=Value->AsArray();float X0=R[1]->AsNumber()*7.5,X1=R[3]->AsNumber()*7.5,Y0=(R[0]->AsNumber()-160)*7.5,Y1=(R[2]->AsNumber()-160)*7.5;
  Quad(Lines,FVector(X0,Y0,.32),FVector(X1,Y0,.32),FVector(X1,Y1,.32),FVector(X0,Y1,.32),FVector::UpVector,FLinearColor(.76,.75,.64));
 }
 for(auto Value:Features->GetArrayField(TEXT("pools"))){
  auto B=Bounds(Value->AsObject());SiteExclusions.Add(B);if(!SitePreviewTargets.Contains(TEXT("pool")))SitePreviewTargets.Add(TEXT("pool"),FVector(B.GetCenter(),0));
  float X0=B.Min.X+60,X1=B.Max.X-60,Y0=B.Min.Y+60,Y1=B.Max.Y-60;
  FLinearColor Aqua(.13,.43,.52),Coping(.72,.74,.69);
  Quad(Tiles,FVector(X0,Y0,-95),FVector(X1,Y0,-95),FVector(X1,Y1,-95),FVector(X0,Y1,-95),FVector::UpVector,Aqua);
  for(float X:{X0,X1})Quad(Tiles,FVector(X,Y0,-95),FVector(X,Y1,-95),FVector(X,Y1,0),FVector(X,Y0,0),FVector(X==X0?1:-1,0,0),Aqua);
  for(float Y:{Y0,Y1})Quad(Tiles,FVector(X0,Y,-95),FVector(X1,Y,-95),FVector(X1,Y,0),FVector(X0,Y,0),FVector(0,Y==Y0?1:-1,0),Aqua);
  // The original gray step/ladder notch is on the west (low-screen-X) side.
  float StepX=(X0+X1)*.5;Box(Tiles,FVector(StepX,Y0+45,-4),FVector(30,45,4),Aqua);
  for(float X:{StepX-27,StepX+27}){
   Tube(Chrome,FVector(X,Y0-25,0),FVector(X,Y0-25,47),2.3,FLinearColor::White);
   FVector Prev(X,Y0-25,47);
   for(int S=1;S<=12;++S){float A=S*PI/12;FVector Next(X,Y0-10-15*FMath::Cos(A),47+15*FMath::Sin(A));Tube(Chrome,Prev,Next,2.3,FLinearColor::White);Prev=Next;}
   Tube(Chrome,Prev,FVector(X,Y0+5,-55),2.3,FLinearColor::White);
  }
  for(float Z:{-48.f,-28.f,-8.f,12.f})Tube(Chrome,FVector(StepX-27,Y0+5,Z),FVector(StepX+27,Y0+5,Z),2.2,FLinearColor::White);
  // A fine raised coping lip, within the original 8-pixel tiled surround.
  for(float X:{X0-3,X1+3})Box(Tiles,FVector(X,(Y0+Y1)*.5,1),FVector(3,(Y1-Y0)*.5+6,1),Coping);
  for(float Y:{Y0-3,Y1+3})Box(Tiles,FVector((X0+X1)*.5,Y,1),FVector((X1-X0)*.5,3,1),Coping);
 }
 for(auto Value:Features->GetArrayField(TEXT("cars"))){
  auto O=Value->AsObject();auto B=Bounds(O);FVector P(B.GetCenter(),0);float L=B.GetSize().X-6,W=B.GetSize().Y-6;
  FLinearColor C=O->GetStringField(TEXT("color"))==TEXT("red")?FLinearColor(.43,.018,.025):FLinearColor(.018,.065,.38);
  FVisibleObstacle Contact;Contact.Box=true;Contact.A=FVector2D(P);Contact.Radius=FVector2D(L*.5,W*.5);VisibleObstacles.Add(Contact);
  auto V=[&](double X,double Y,double Z){return P+FVector(X*L,Y*W,Z);};
  // Beveled longitudinal body panels, hood, hatch, pillars and glazed cabin.
  constexpr int Sides=8;const float Stations[]={-.50f,-.46f,-.32f,.30f,.46f,.50f};
  const FVector2D Ring[]={FVector2D(-.37,43),FVector2D(-.47,48),FVector2D(-.47,65),FVector2D(-.40,73),FVector2D(.40,73),FVector2D(.47,65),FVector2D(.47,48),FVector2D(.37,43)};
  for(int R=0;R<5;++R)for(int S=0;S<Sides;++S){int T=(S+1)%Sides;float A=R==0?.88f:1,Bb=R==4?.88f:1;
   auto Q=[&](int K,int J,float Shrink){return V(Stations[K],Ring[J].X*Shrink,Ring[J].Y-(Shrink<1?7:0));};
   FVector N=FVector::CrossProduct(Q(R+1,T,Bb)-Q(R,S,A),Q(R,T,A)-Q(R,S,A)).GetSafeNormal();
   Quad(Paint,Q(R,S,A),Q(R,T,A),Q(R+1,T,Bb),Q(R+1,S,Bb),N,C);
  }
  for(float End:{-.50f,.50f})for(int S=0;S<Sides;++S){
   FVector A=V(End,0,56),CapB=V(End,Ring[S].X*.88f,Ring[S].Y-7),D=V(End,Ring[(S+1)%Sides].X*.88f,Ring[(S+1)%Sides].Y-7),N(End>0?1:-1,0,0);
   if(FVector::DotProduct(FVector::CrossProduct(D-A,CapB-A),N)<0)Swap(CapB,D);
   int K=Paint.V.Num();for(auto Q:{A,CapB,D}){Paint.V.Add(Q);Paint.N.Add(N);Paint.UV.Add(FVector2D(Q.Y/200,Q.Z/200));Paint.C.Add(C);}Paint.I.Append({K,K+1,K+2});
  }
  for(float S:{-1.f,1.f}){
   Box(Dark,V(0,S*.47,32),FVector(L*.22,2,3),FLinearColor::White);
   Quad(Glass,V(-.39,S*.405,72),V(.28,S*.405,72),V(.10,S*.315,112),V(-.22,S*.315,112),FVector(0,S,.15),FLinearColor::White);
   Tube(Paint,V(-.22,S*.315,112),V(-.39,S*.405,72),2,C);Tube(Paint,V(.10,S*.315,112),V(.28,S*.405,72),2,C);
   Tube(Dark,V(-.03,S*.36,72),V(-.03,S*.315,112),1.4,FLinearColor::White);
   Tube(Chrome,V(-.10,S*.472,64),V(-.04,S*.472,64),.8,FLinearColor::White);
   Box(Paint,V(.17,S*.48,79),FVector(6,W*.025,3.5),C);
   for(float X:{-.31f,.31f}){
    FVector WheelP=V(X,S*(.5-8/W),26);Wheel(Rubber,WheelP,20,6,FLinearColor::White);
    FVector Hub=WheelP+FVector(0,S*6,0);Wheel(Chrome,Hub,12,2,FLinearColor::White);
    for(int Spoke=0;Spoke<6;++Spoke){float A=Spoke*PI/3;Tube(Chrome,Hub,Hub+FVector(FMath::Cos(A)*12,0,FMath::Sin(A)*12),1.1,FLinearColor::White);}
   }
   // Door shut line and sill; kept inside the same top-down footprint.
   Tube(Dark,V(-.28,S*.472,48),V(-.28,S*.472,69),.38,FLinearColor::White);
   Tube(Dark,V(.20,S*.472,48),V(.20,S*.472,69),.38,FLinearColor::White);
   for(float End:{-.50f,.50f})Box(Chrome,V(End,0,48),FVector(1.2,W*.36,2),FLinearColor::White);
   Box(Lines,V(.493,S*.28,62),FVector(1.5,W*.11,4),FLinearColor(.9,.88,.70));
   Box(Paint,V(-.493,S*.28,61),FVector(1.5,W*.11,4),FLinearColor(.6,.006,.003));
  }
  Quad(Paint,V(-.22,-.315,112),V(.10,-.315,112),V(.10,.315,112),V(-.22,.315,112),FVector::UpVector,C);
  Quad(Glass,V(.10,-.315,111),V(.28,-.405,73),V(.28,.405,73),V(.10,.315,111),FVector(.6,0,.5),FLinearColor::White);
  Quad(Glass,V(-.39,-.405,73),V(-.22,-.315,111),V(-.22,.315,111),V(-.39,.405,73),FVector(-.6,0,.5),FLinearColor::White);
  for(float End:{-.50f,.50f}){Box(Dark,V(End,0,55),FVector(1.3,W*.20,3),FLinearColor::White);Box(Lines,V(End*1.006,0,49),FVector(1.5,13,3.2),FLinearColor(.82,.82,.72));}
 }
 for(auto Value:Features->GetArrayField(TEXT("oil_slicks"))){
  auto B=Bounds(Value->AsObject());FVector P(B.GetCenter(),.40);float RX=B.GetSize().X*.48,RY=B.GetSize().Y*.48;
  FVisibleObstacle Contact;Contact.Ellipse=true;Contact.A=FVector2D(P);Contact.Radius=FVector2D(RX*.87,RY*.87);VisibleObstacles.Add(Contact);
  int K=Oil.V.Num();Oil.V.Add(P);Oil.N.Add(FVector::UpVector);Oil.UV.Add(FVector2D(.5));Oil.C.Add(FLinearColor::White);
  for(int I=0;I<=64;++I){float T=I*2*PI/64,R=1+.035f*FMath::Sin(T*5)+.025f*FMath::Cos(T*9);Oil.V.Add(P+FVector(FMath::Cos(T)*RX*R,FMath::Sin(T)*RY*R,0));Oil.N.Add(FVector::UpVector);Oil.UV.Add(FVector2D(FMath::Cos(T)*.5+.5,FMath::Sin(T)*.5+.5));Oil.C.Add(FLinearColor::White);if(I<64)Oil.I.Append({K,K+I+2,K+I+1});}
 }
 for(auto& Entry:Meshes){auto& G=Entry.Value;if(G.I.IsEmpty())continue;
  auto* M=NewObject<UProceduralMeshComponent>(this);AddInstanceComponent(M);M->SetupAttachment(Scene);M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetCastShadow(Entry.Key!=TEXT("OilSlick")&&Entry.Key!=TEXT("ParkingPaint"));M->RegisterComponent();
  M->CreateMeshSection_LinearColor(0,G.V,G.I,G.N,G.UV,G.C,TArray<FProcMeshTangent>(),false);M->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Materials/M_%s.M_%s"),*Entry.Key,*Entry.Key)));M->SetCullDistance(9000);
 }
 UE_LOG(LogTemp,Display,TEXT("ORIGINAL SITES course %d: %d pools, %d parking areas, %d cars, %d oil slicks"),CourseIndex,Features->GetArrayField(TEXT("pools")).Num(),Features->GetArrayField(TEXT("parking_lots")).Num(),Features->GetArrayField(TEXT("cars")).Num(),Features->GetArrayField(TEXT("oil_slicks")).Num());
}

bool ACruiserWorld::IsOriginalSite(float X,float Y)const{for(const auto& B:SiteExclusions)if(B.IsInsideOrOn(FVector2D(X,Y)))return true;return false;}

bool ACruiserWorld::VerifyOriginalSites(const TSharedPtr<FJsonObject>& Visual)const{
 const auto F=Visual->GetObjectField(TEXT("site_features"));bool Good=true;int Tests=0;
 for(const TCHAR* Key:{TEXT("cars"),TEXT("oil_slicks")})for(auto V:F->GetArrayField(Key)){auto B=Bounds(V->AsObject());bool Water=false;Good&=VisibleContact(FVector(B.GetCenter(),0),&Water)&&!Water;++Tests;}
 for(auto V:F->GetArrayField(TEXT("pools"))){auto B=Bounds(V->AsObject());bool Water=false;Good&=VisibleContact(FVector(B.GetCenter(),0),&Water)&&Water;++Tests;
  Good&=!VisibleContact(FVector(B.GetCenter().X,B.Min.Y+30,0));++Tests;
 }
 for(auto V:F->GetArrayField(TEXT("cars"))){auto B=Bounds(V->AsObject());Good&=SweepVisibleContact(FVector(B.Min.X-100,B.GetCenter().Y,0),FVector(B.GetCenter(),0));++Tests;Good&=!VisibleContact(FVector(B.GetCenter().X,B.Max.Y+40,0));++Tests;}
 UE_LOG(LogTemp,Display,TEXT("ORIGINAL SITE CONTACTS course %d: %s; %d car, oil, pool, coping and swept-contact probes"),CourseIndex,Good?TEXT("PASSED"):TEXT("FAILED"),Tests);return Good;
}

void ACruiserWorld::PreviewOriginalSite(){
 FString Kind;if(!FParse::Value(FCommandLine::Get(),TEXT("SitePreview="),Kind))return;
 const FVector* Target=SitePreviewTargets.Find(Kind);if(!Target)return;
 Rider->SetVisibility(false,true);Camera->FieldOfView=44;
 FVector View=*Target+FVector(-1350,-350,1800);Camera->SetRelativeLocation(View);Camera->SetRelativeRotation((*Target-View).Rotation());
}
