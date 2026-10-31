#include "CruiserWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
namespace {
FLinearColor Paint(int32 I){const FLinearColor C[]={FLinearColor(.015,.12,.52),FLinearColor(.6,.018,.025),FLinearColor(.85,.57,.015),FLinearColor(.018,.19,.058),FLinearColor(.28,.025,.48),FLinearColor(.77,.80,.73),FLinearColor(.018,.024,.03),FLinearColor(.93,.14,.012)};return C[FMath::Clamp(I,0,7)];}
}
void ACruiserWorld::RebuildAppearance(){
 TArray<USceneComponent*> PartsToRemove;Rider->GetChildrenComponents(true,PartsToRemove);
 for(int I=PartsToRemove.Num()-1;I>=0;--I)if(PartsToRemove[I]!=VehicleRoot&&PartsToRemove[I]!=Bicycle)PartsToRemove[I]->DestroyComponent();
 RiderGuards.Reset();SkateRoots.Reset();SkateWheels.Reset();Bicycle->ClearAllMeshSections();Human=nullptr;HeroHead=nullptr;NeckBridge=nullptr;Helmet=nullptr;Pedals.Reset();CrankArms.Reset();WheelSpokes.Reset();WheelDetail.Reset();WheelDetailReference.Reset();WheelDetailHubs.Reset();WheelDetailRadii.Reset();HumanReference.Reset();HumanParents.Reset();HumanBoneNames.Reset();HumanLocalReference.Reset();LastPosePhase=-1;
 BuildRider();UE_LOG(LogTemp,Display,TEXT("COSMETIC style=%d gender=%d outfit=%d skin=%d; simulation unchanged at %llu cycles"),VehicleStyle,Gender,Outfit,SkinTone,Simulation->Cycles());
}
void ACruiserWorld::BuildVehicle(){
 auto* Steel=Materials[5].Get();auto* Rubber=Materials[6].Get();auto* Dark=Materials[10].Get();
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Paint.M_Paint"));
 BikePaint=UMaterialInstanceDynamic::Create(Base?Base:Materials[7].Get(),this);BikePaint->SetVectorParameterValue(TEXT("Tint"),Paint(BikeColor));
 auto* HelmetBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_HelmetComposite.M_HelmetComposite"));
 HelmetPaint=UMaterialInstanceDynamic::Create(HelmetBase?HelmetBase:Base?Base:Materials[7].Get(),this);HelmetPaint->SetVectorParameterValue(TEXT("Tint"),Paint(HelmetColor));
 if(VehicleStyle>=2&&VehicleStyle<=4){BuildWorkshopEquipment();return;}
 auto* Blue=BikePaint.Get();auto* Root=VehicleRoot.Get();
 TArray<FVector> V,N;TArray<int32> I;TArray<FVector2D> UV;TArray<FLinearColor>C;
 auto Torus=[&](FVector Center,float Radius,float Tube,bool Horizontal,int Material){
  const int B=V.Num();for(int A=0;A<=64;++A)for(int T=0;T<=10;++T){float U=A*2*PI/64,P=T*2*PI/10,R=Radius+Tube*FMath::Cos(P);FVector Pt(R*FMath::Cos(U),Tube*FMath::Sin(P),R*FMath::Sin(U)),No(FMath::Cos(U)*FMath::Cos(P),FMath::Sin(P),FMath::Sin(U)*FMath::Cos(P));if(Horizontal){Swap(Pt.Y,Pt.Z);Swap(No.Y,No.Z);}V.Add(Center+Pt);N.Add(No);UV.Add(FVector2D(A/64.f,T/10.f));C.Add(FLinearColor::White);if(A<64&&T<10){int Q=B+A*11+T;I.Append({Q,Q+11,Q+1,Q+1,Q+11,Q+12});}}
 };
 auto Wheel=[&](FVector Hub,float R,float Width,bool Animate){
  TArray<USceneComponent*> Before;Root->GetChildrenComponents(false,Before);
  Torus(Hub,R,Width,false,0);Torus(Hub,R-Width*1.1f,.65,false,0);
  Rod(Hub+FVector(0,-6,0),Hub+FVector(0,6,0),2,Steel,Root);
  for(int A=0;A<32;++A){float T=A*2*PI/32;auto* Spoke=Rod(Hub+FVector(0,A%2?2:-2,0),Hub+FVector((R-2)*FMath::Cos(T),0,(R-2)*FMath::Sin(T)),.12,Steel,Root);(void)Spoke;}
  // Machined brake rotor, axle bolts and tyre tread blocks.
  for(int A=0;A<12;++A){float T=A*2*PI/12;Rod(Hub+FVector(6*FMath::Cos(T),4,6*FMath::Sin(T)),Hub+FVector(8*FMath::Cos(T+.2),4,8*FMath::Sin(T+.2)),.48,Steel,Root);}
  for(int A=0;A<48;++A){float T=A*2*PI/48;auto* Tread=Part(CubeMesh,Rubber,Hub+FVector((R+Width*.8)*FMath::Cos(T),0,(R+Width*.8)*FMath::Sin(T)),FVector(.035,Width*.018,.012),Root);Tread->SetRelativeRotation(FRotator(-FMath::RadiansToDegrees(T),0,0));}
  TArray<USceneComponent*> After;Root->GetChildrenComponents(false,After);
  for(auto* P:After)if(!Before.Contains(P))if(auto* Mesh=Cast<UStaticMeshComponent>(P)){WheelDetail.Add(Mesh);WheelDetailReference.Add(Mesh->GetRelativeTransform());WheelDetailHubs.Add(Hub);WheelDetailRadii.Add(R);}
 };
 if(VehicleStyle<2){
  const bool Cruiser=VehicleStyle==0;float R=Cruiser?29:25;
  FVector Rear(-50,0,27),Front(50,0,27),Crank(-5,0,29),Seat(-15,0,72),Head(35,0,76);
  Wheel(Rear,R,2.6,true);Wheel(Front,R,2.6,true);
  // Paired rear stays and front fork, welded frame, headset and seat collar.
  for(float Side:{-4.f,4.f}){Rod(Rear+FVector(0,Side,0),Crank+FVector(0,Side,0),1.3,Blue,Root);Rod(Rear+FVector(0,Side,0),Seat,1.3,Blue,Root);Rod(Head+FVector(0,Side,0),Front+FVector(0,Side,0),1.75,Blue,Root);}
  if(Cruiser){FVector Mid(7,0,58);Rod(Seat,Mid,2.5,Blue,Root);Rod(Mid,Head,2.5,Blue,Root);}else Rod(Seat,Head,2.25,Blue,Root);
  Rod(Crank,Seat,2.4,Blue,Root);Rod(Crank,Head,2.5,Blue,Root);
  Rod(Seat,Seat+FVector(0,0,8),1.6,Steel,Root);Part(SphereMesh,Dark,Seat+FVector(-5,0,10),FVector(.28,.17,.063),Root);
  Rod(Head,Head+FVector(8,0,22),1.8,Steel,Root);
  for(int Side:{-1,1}){Rod(FVector(43,0,98),FVector(Cruiser?34:43,Side*25,98),1.5,Steel,Root);Rod(FVector(43,Side*16,98),FVector(43,Side*26,98),2.1,Rubber,Root);Rod(FVector(45,Side*18,97),FVector(50,Side*17,93),.6,Steel,Root);Rod(FVector(43,Side*16,97),FVector(34,Side*5,68),.28,Dark,Root);Rod(FVector(34,Side*5,68),Front+FVector(0,Side*5,7),.28,Dark,Root);
   Pedals.Add(Part(CubeMesh,Dark,FVector(-5,Side*19,29),FVector(.18,.10,.026),Root));CrankArms.Add(Rod(Crank+FVector(0,Side*15,0),FVector(9,Side*19,29),1,Steel,Root));}
  for(float Z:{23.f,35.f})Rod(FVector(-50,4,27),FVector(-5,4,Z),.42,Dark,Root);
  Torus(Crank+FVector(0,4,0),9,.6,false,0);
  for(int A=0;A<8;++A){float T=A*PI/4;Rod(Crank+FVector(0,4,0),Crank+FVector(9*FMath::Cos(T),4,9*FMath::Sin(T)),.45,Steel,Root);}
  for(float X:{-13.f,26.f})Part(CubeMesh,Materials[12],FVector(X,-2,60),FVector(.1,.025,.025),Root);
 }else if(VehicleStyle==2){
  Wheel(FVector(-40,0,12),10,2,false);Wheel(FVector(50,0,12),10,2,false);
  Part(CubeMesh,Blue,FVector(-5,0,12),FVector(.88,.23,.065),Root);Part(CubeMesh,Rubber,FVector(-7,0,16),FVector(.71,.19,.018),Root);
  Rod(FVector(50,0,12),FVector(41,0,111),2.4,Blue,Root);Rod(FVector(41,-25,111),FVector(41,25,111),1.6,Steel,Root);
  for(int D:{-1,1})Rod(FVector(41,D*15,111),FVector(41,D*26,111),2.1,Rubber,Root);
 }else if(VehicleStyle<5){
  for(float Side:{-17.f,17.f}){Part(SphereMesh,Dark,FVector(-2,Side,13),FVector(.32,.12,.18),Root);Part(CubeMesh,Blue,FVector(-3,Side,6),FVector(.34,.14,.04),Root);
   if(VehicleStyle==3){for(int W=0;W<4;++W)Wheel(FVector(-15+W*9,Side,4),3.2,1.1,false);}
   else for(float X:{-13.f,8.f})for(float Y:{-6.f,6.f})Wheel(FVector(X,Side+Y,4),3.2,1.4,false);
  }
 }else{
  for(float Side:{-31.f,31.f}){Wheel(FVector(-23,Side,34),30,2.5,false);Torus(FVector(-23,Side*1.12f,34),27,.9,false,0);Rod(FVector(-23,Side,34),FVector(10,Side*.65,45),2,Blue,Root);Wheel(FVector(33,Side*.7,9),7,1.6,false);Rod(FVector(10,Side*.65,45),FVector(33,Side*.7,9),2,Blue,Root);Rod(FVector(-32,Side*.7,47),FVector(-32,Side*.7,96),2,Blue,Root);Rod(FVector(-8,Side*.72,61),FVector(20,Side*.72,61),2,Dark,Root);Rod(FVector(5,Side*.65,45),FVector(30,Side*.65,15),1.7,Steel,Root);}
  Part(CubeMesh,Dark,FVector(-9,0,49),FVector(.46,.47,.065),Root);Part(CubeMesh,Dark,FVector(-34,0,76),FVector(.055,.47,.46),Root);Part(CubeMesh,Blue,FVector(30,0,14),FVector(.23,.44,.04),Root);
 }
 Bicycle->CreateMeshSection_LinearColor(0,V,I,N,UV,C,TArray<FProcMeshTangent>(),false);Bicycle->SetMaterial(0,Rubber);
}

