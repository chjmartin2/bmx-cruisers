#include "CruiserWorld.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Dom/JsonObject.h"
namespace {struct FTrailMesh{TArray<FVector> V,N;TArray<int32>I;TArray<FVector2D>UV;TArray<FLinearColor>C;};}
void ACruiserWorld::BuildTrailCharacter(){
 FRandomStream Random(4021985);TMap<int32,FTrailMesh> Chunks;int H=CourseCells.Num()/320;
 auto Road=[&](int X,int Y){return X>=0&&X<320&&Y>=0&&Y<H&&CourseCells[Y*320+X]==1;};
 auto Point=[&](FTrailMesh& G,FVector P,FLinearColor C){int I=G.V.Num();G.V.Add(P);G.N.Add(FVector::UpVector);G.UV.Add(FVector2D(P.X/80,P.Y/80));G.C.Add(C);return I;};
 auto Patch=[&](float PX,float PY,float RX,float RY,FLinearColor Color){
  auto& G=Chunks.FindOrAdd(FMath::FloorToInt(PY*7.5f/1200));FVector P(PY*7.5,(PX-160)*7.5,.18);int Center=Point(G,P,Color);int B=G.V.Num();
  float Phase=Random.FRand()*6.28f;for(int A=0;A<=16;++A){float T=A*PI/8,Uneven=.75f+.25f*FMath::Sin(T*5+Phase);FVector Q=P+FVector(FMath::Cos(T)*RX*Uneven,FMath::Sin(T)*RY*Uneven,0);FLinearColor Edge=Color;Edge.A=0;Point(G,Q,Edge);if(A<16)G.I.Append({Center,B+A,B+A+1});}
 };
 auto Crack=[&](FVector2D A,FVector2D B,float Width,FLinearColor C){
  if(!Road(FMath::RoundToInt(B.Y/7.5+160),FMath::RoundToInt(B.X/7.5)))return;
  auto& G=Chunks.FindOrAdd(FMath::FloorToInt(A.X/1200));FVector2D D=(B-A).GetSafeNormal(),N(-D.Y,D.X);int Base=G.V.Num();
  for(auto Q:{A+N*Width,A-N*Width,B-N*Width,B+N*Width})Point(G,FVector(Q.X,Q.Y,.24),C);G.I.Append({Base,Base+2,Base+1,Base,Base+3,Base+2});
 };
 for(int PY=8;PY<H-8;PY+=3){
  for(int PX=2;PX<317;){if(!Road(PX,PY)){++PX;continue;}int Left=PX;while(PX<318&&Road(PX,PY))++PX;int Right=PX,Width=Right-Left;if(Width<5)continue;
   float Mid=(Left+Right)*.5f;int Zone=(PY/210)%4;
   FLinearColor TrailDust=Zone==1?FLinearColor(.23,.175,.11,.27):Zone==2?FLinearColor(.29,.285,.21,.22):Zone==3?FLinearColor(.34,.33,.28,.22):FLinearColor(.19,.19,.17,.22);
   // Dust collects along unsealed edges and two drifting lines of bicycle wear.
   Patch(Left+1.6f,PY,Random.FRandRange(12,35),7,TrailDust);Patch(Right-1.6f,PY,Random.FRandRange(12,35),7,TrailDust);
   if(PY%9==2){float Drift=FMath::Sin(PY*.017f)*Width*.13f;for(float Side:{-1.f,1.f})Patch(Mid+Drift+Side*Width*.16f,PY,42,3.7f,FLinearColor(.30,.28,.235,.12));}
   if(Random.FRand()<.07){float X=Random.FRandRange(Left+2,Right-2);Patch(X,PY,Random.FRandRange(18,45),FMath::Min(22.f,Width*1.5f),FLinearColor(.055,.059,.058,.38));}
   if(Random.FRand()<.11){
    FVector2D A(PY*7.5,(Random.FRandRange(Left+2,Right-2)-160)*7.5);float Angle=Random.FRandRange(-2,2);int Count=Random.RandRange(3,7);
    for(int N=0;N<Count;++N){FVector2D B=A+FVector2D(Random.FRandRange(7,17),Random.FRandRange(-12,12)+Angle*4);Crack(A,B,.30f,FLinearColor(.035,.038,.037,.52));if(N==2)Crack(B,B+FVector2D(-8,13),.20f,FLinearColor(.04,.043,.04,.42));A=B;}
   }
  }
 }
 auto* Mat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_TrailWear.M_TrailWear"));
 for(auto& Chunk:Chunks){auto& G=Chunk.Value;auto* M=NewObject<UProceduralMeshComponent>(this);AddInstanceComponent(M);M->SetupAttachment(Scene);M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetCastShadow(false);M->RegisterComponent();M->CreateMeshSection_LinearColor(0,G.V,G.I,G.N,G.UV,G.C,TArray<FProcMeshTangent>(),false);M->SetMaterial(0,Mat);M->SetCullDistance(6000);}
 UE_LOG(LogTemp,Display,TEXT("Authored trail wear: %d unique course chunks; original surface and collision unchanged"),Chunks.Num());
}
