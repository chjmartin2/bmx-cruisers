#include "CruiserWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "ProceduralMeshComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Sound/SoundAttenuation.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Camera/CameraComponent.h"

void ACruiserWorld::BuildLivingPark(const TSharedPtr<FJsonObject>& Data,const TSharedPtr<FJsonObject>& Visual){
 FFileHelper::LoadFileToArray(CourseCells,*(FPaths::ProjectContentDir()/TEXT("Data/course1-terrain.bin")));
 const int H=Data->GetIntegerField(TEXT("height"))*8;FRandomStream R(31985);
 auto Cell=[&](int X,int Y){return X>=0&&X<320&&Y>=0&&Y<H&&CourseCells.IsValidIndex(Y*320+X)?CourseCells[Y*320+X]:uint8(255);};
 auto* Reeds=Instances(CylinderMesh,Materials[4]);auto* Heads=Instances(SphereMesh,Materials[3]);auto* Pebbles=Instances(SphereMesh,Materials[11]);auto* Leaves=Instances(SphereMesh,Materials[4]);auto* Wood=Instances(CylinderMesh,Materials[3]);
 auto RodInstance=[&](UInstancedStaticMeshComponent* M,FVector A,FVector B,float Radius){M->AddInstance(FTransform(FRotationMatrix::MakeFromZ(B-A).Rotator(),(A+B)*.5,FVector(Radius/50,Radius/50,(B-A).Size()/100)));};
 // Sample water-edge cells rather than adding hazards to any original opening.
 for(int Y=8;Y<H-8;Y+=4)for(int X=8;X<312;X+=4){
  if(Cell(X,Y)!=2)continue;
  bool Shore=Cell(X-4,Y)==0||Cell(X+4,Y)==0||Cell(X,Y-4)==0||Cell(X,Y+4)==0;
  if(Shore&&R.FRand()<.36){FVector P(Y*7.5,(X-160)*7.5,-8);
   for(int N=0;N<R.RandRange(3,7);++N){FVector A=P+FVector(R.FRandRange(-9,9),R.FRandRange(-9,9),0),B=A+FVector(R.FRandRange(-8,8),R.FRandRange(-8,8),R.FRandRange(24,62));RodInstance(Reeds,A,B,.6);if(N%2==0)Heads->AddInstance(FTransform(FRotator(0,0,0),B,FVector(.022,.022,.10)));}
   if(R.FRand()<.35)Pebbles->AddInstance(FTransform(FRotator(R.FRand()*30,R.FRand()*360,0),P+FVector(0,0,-4),FVector(.15,.21,.09)));
   if(R.FRand()<.08)RodInstance(Wood,P,P+FVector(60,24,5),4);
  }
  if(!Shore&&Cell(X-7,Y)==2&&Cell(X+7,Y)==2&&Cell(X,Y-7)==2&&Cell(X,Y+7)==2&&R.FRand()<.014){
   FVector P(Y*7.5,(X-160)*7.5,-8);WaterCenters.Add(P);
   if(R.FRand()<.4)for(int N=0;N<4;++N)Leaves->AddInstance(FTransform(FRotator(0,R.FRand()*360,0),P+FVector(R.FRandRange(-23,23),R.FRandRange(-23,23),.4),FVector(.18,.15,.009)));
  }
 }
 // The original movement clamps sprite X to 24..321. These verge stones and
 // low rope posts mark those limits; no new collision or invisible wall exists.
 auto* Stakes=Instances(CylinderMesh,Materials[3]);auto* Rope=Instances(CylinderMesh,Materials[13]);auto* Caps=Instances(SphereMesh,Materials[12]);
 for(float Side:{-1150.f,1157.5f}){
  TArray<FVector> Attachments;
  for(float X=-300;X<H*7.5+600;X+=260){
   float Ground=FenceGroundHeight(X,Side),Z=Ground+R.FRandRange(30,43);
   RodInstance(Stakes,FVector(X,Side,Ground-1),FVector(X,Side,Z),3.3);
   Caps->AddInstance(FTransform(FRotator::ZeroRotator,FVector(X,Side,Z),FVector(.075,.075,.035)));
   Attachments.Add(FVector(X,Side,Z-7));
  }
  // Join the real attachment heights and follow intervening rises. Keep the
  // sagging rope clear of the meadow even between two low-ground posts.
  for(int I=1;I<Attachments.Num();++I){
   FVector Previous=Attachments[I-1];
   for(int Step=1;Step<=8;++Step){
    float T=Step/8.f;FVector P=FMath::Lerp(Attachments[I-1],Attachments[I],T);
    P.Z=FMath::Max(P.Z-10*FMath::Sin(PI*T),double(FenceGroundHeight(P.X,P.Y)+10));
    RodInstance(Rope,Previous,P,.85);Previous=P;
   }
  }
 }
 // Irregular fallen foliage and twigs enrich the verge, without altering a path.
 auto* Litter=Instances(SphereMesh,Materials[13]);Litter->SetCastShadow(false);
 for(int N=0;N<2400;++N){int X=R.RandRange(0,319),Y=R.RandRange(0,H-1);if(Cell(X,Y)!=0)continue;
  FVector P(Y*7.5,(X-160)*7.5,1);Litter->AddInstance(FTransform(FRotator(0,R.FRand()*360,0),P,FVector(R.FRandRange(.025,.07),.025,.005)));
  if(N%9==0)RodInstance(Wood,P,P+FVector(R.FRandRange(5,20),R.FRandRange(-10,10),1),.55);
 }
 // A distant tree line provides scale and depth beyond the playable meadow.
 auto* Trunks=Instances(CylinderMesh,Materials[3]);auto* Crowns=Instances(SphereMesh,Materials[4]);
 for(int N=0;N<180;++N){float X=R.FRandRange(-1300,H*7.5+1600),Y=(N%2?-1:1)*R.FRandRange(2050,3300),Z=R.FRandRange(260,520);RodInstance(Trunks,FVector(X,Y,-4),FVector(X,Y,Z),R.FRandRange(8,15));
  for(int C=0;C<6;++C)Crowns->AddInstance(FTransform(FRotator(R.FRand()*25,R.FRand()*360,0),FVector(X+R.FRandRange(-100,100),Y+R.FRandRange(-100,100),Z+R.FRandRange(-40,130)),FVector(R.FRandRange(1.6,3),R.FRandRange(1.6,3),R.FRandRange(1.9,3.6))));
 }
 // Small bird flocks fly well above the course. Ducks remain on broad water.
 for(int N=0;N<18;++N){auto* Root=NewObject<USceneComponent>(this);AddInstanceComponent(Root);Root->SetupAttachment(Scene);Root->RegisterComponent();
  bool Duck=N<6&&WaterCenters.Num();FVector P=Duck?WaterCenters[(N*7)%WaterCenters.Num()]+FVector(0,0,10):FVector(R.FRandRange(300,H*7.5),R.FRandRange(-900,900),R.FRandRange(200,460));Root->SetRelativeLocation(P);Wildlife.Add(Root);WildlifeOrigins.Add(P);
  Part(SphereMesh,Duck?Materials[13].Get():Materials[10].Get(),FVector::ZeroVector,FVector(Duck?.19:.1,Duck?.12:.05,Duck?.12:.05),Root);
  Part(SphereMesh,Duck?Materials[4].Get():Materials[10].Get(),FVector(9,0,Duck?10:2),FVector(.065,.06,.07),Root);
  Part(CubeMesh,Materials[16],FVector(15,0,Duck?10:2),FVector(.055,.045,.02),Root);
  for(int S:{-1,1}){auto* Wing=Part(SphereMesh,Materials[10],FVector(-2,S*8,0),FVector(.12,.15,.014),Root);Wings.Add(Wing);}
 }
 // Localized water and birds: attenuation is tied to park positions, not UI.
 FFileHelper::LoadFileToArray(NaturePCM,*(FPaths::ProjectContentDir()/TEXT("Audio/waterside.pcm")));
 for(int N=0;N<FMath::Min(12,WaterCenters.Num());++N){auto* A=NewObject<UAudioComponent>(this);AddInstanceComponent(A);A->SetupAttachment(Scene);A->SetRelativeLocation(WaterCenters[N*WaterCenters.Num()/FMath::Min(12,WaterCenters.Num())]);A->bAutoActivate=false;A->bAllowSpatialization=true;A->bOverrideAttenuation=true;A->AttenuationOverrides.bAttenuate=true;A->AttenuationOverrides.bSpatialize=true;A->AttenuationOverrides.AttenuationShapeExtents=FVector(100);A->AttenuationOverrides.FalloffDistance=1200;
  auto* S=NewObject<USoundWaveProcedural>(A);S->SetSampleRate(48000);S->NumChannels=2;S->Duration=INDEFINITELY_LOOPING_DURATION;S->QueueAudio(NaturePCM.GetData(),NaturePCM.Num());NatureAudio.Add(A);NatureStreams.Add(S);A->SetSound(S);A->SetVolumeMultiplier(.22);A->RegisterComponent();A->Play();
 }
 Ripples=NewObject<UProceduralMeshComponent>(this);AddInstanceComponent(Ripples);Ripples->SetupAttachment(Scene);Ripples->SetCollisionEnabled(ECollisionEnabled::NoCollision);Ripples->RegisterComponent();Ripples->SetCastShadow(false);
 Ripples->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Ripple.M_Ripple")));
 Dust=Instances(SphereMesh,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Dust.M_Dust")));Dust->SetCastShadow(false);for(int N=0;N<40;++N)Dust->AddInstance(FTransform(FVector(0,0,-200)));
 Fireworks=Instances(SphereMesh,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Firework.M_Firework")));Fireworks->SetCastShadow(false);for(int N=0;N<420;++N)Fireworks->AddInstance(FTransform(FVector(0,0,-200)));
 UE_LOG(LogTemp,Display,TEXT("Living park ready: %d ripple sites, %d wildlife; marked original X limits 24..321"),WaterCenters.Num(),Wildlife.Num());
}
void ACruiserWorld::UpdateLivingPark(float Dt){
 EnvironmentTime+=Dt;float T=EnvironmentTime;
 for(int N=0;N<NatureAudio.Num();++N){NatureAudio[N]->SetVolumeMultiplier(EffectsMuted?0:.28);if(NaturePCM.Num()&&NatureStreams[N]->GetAvailableAudioByteCount()<96000)NatureStreams[N]->QueueAudio(NaturePCM.GetData(),NaturePCM.Num());}
 for(int N=0;N<Wildlife.Num();++N){bool Duck=N<6&&WaterCenters.Num();FVector P=WildlifeOrigins[N];float Phase=T*(Duck?.28:.9)+N;
  P+=FVector(FMath::Sin(Phase)* (Duck?12:170),FMath::Cos(Phase*.7)*(Duck?10:90),Duck?FMath::Sin(T*2+N)*.7:FMath::Sin(Phase*2)*14);Wildlife[N]->SetRelativeLocation(P);Wildlife[N]->SetRelativeRotation(FRotator(0,Duck?FMath::Sin(Phase)*35:90+FMath::RadiansToDegrees(Phase),0));
  for(int S=0;S<2;++S)Wings[N*2+S]->SetRelativeRotation(FRotator(0,0,Duck?0:(S?1:-1)*FMath::Sin(T*8+N)*35));
 }
 if(Ripples){
  RippleVertices.Reset();RippleIndices.Reset();RippleNormals.Reset();RippleUV.Reset();RippleColors.Reset();
  float X=Rider->GetRelativeLocation().X;
  for(int N=0;N<WaterCenters.Num();++N){FVector P=WaterCenters[N];if(FMath::Abs(P.X-X)>2400)continue;
   for(int Ring=0;Ring<3;++Ring){float Age=FMath::Fmod(T*.23+N*.319+Ring/3.f,1.f),R=5+Age*40,W=.25f+.7f*(1-Age);int Base=RippleVertices.Num();
    for(int A=0;A<=48;++A)for(int Edge=0;Edge<2;++Edge){float Angle=A*2*PI/48,Rad=R+(Edge?W:-W);RippleVertices.Add(P+FVector(FMath::Cos(Angle)*Rad,FMath::Sin(Angle)*Rad,.16+Ring*.03));RippleNormals.Add(FVector::UpVector);RippleUV.Add(FVector2D(A/48.f,Edge));RippleColors.Add(FLinearColor(.1,.19,.19,1-Age));if(A<48&&Edge==0){int Q=Base+A*2;RippleIndices.Append({Q,Q+2,Q+1,Q+1,Q+2,Q+3});}}
   }
  }
  Ripples->CreateMeshSection_LinearColor(0,RippleVertices,RippleIndices,RippleNormals,RippleUV,RippleColors,TArray<FProcMeshTangent>(),false);
 }
 bool Riding=Simulation->GetState()==BMX::RunState::Riding&&!Paused;
 if(Dust){for(int N=0;N<40;++N){float Age=FMath::Fmod(T*1.5+N*.173,1.f);FVector P=Rider->GetRelativeLocation()+FVector(-50-Age*100,FMath::Sin(N*71.f)*15,3+Age*14);float Size=Riding?(1-Age)*.025:0;Dust->UpdateInstanceTransform(N,FTransform(FRotator::ZeroRotator,P,FVector(Size)),false,false);}Dust->MarkRenderStateDirty();}
 if(CelebrationTime>=0)CelebrationTime+=Dt;
 if(Fireworks&&CelebrationTime>=0){for(int N=0;N<420;++N){int Burst=N/70,Ray=N%70;float Age=FMath::Fmod(CelebrationTime+Burst*.47f,3.2f),A=Ray*2.39996f,B=FMath::Acos(1-2*(Ray+.5f)/70);FVector Direction(FMath::Cos(A)*FMath::Sin(B),FMath::Sin(A)*FMath::Sin(B),FMath::Cos(B));
  FVector Center=Rider->GetRelativeLocation()+FVector(-150+(Burst%3)*160,(Burst%2?1:-1)*(390+Burst*44),200+(Burst%3)*60);FVector P=Center+Direction*(90+Burst*12)*Age+FVector(0,0,-55*Age*Age);float Size=Age<2.5?.038f*(1-Age/3):0;Fireworks->UpdateInstanceTransform(N,FTransform(FRotationMatrix::MakeFromZ(Direction).Rotator(),P,FVector(Size*.48f,Size*.48f,Size*2.1f)),false,false);}Fireworks->MarkRenderStateDirty();}
 else if(Fireworks&&Simulation->GetState()==BMX::RunState::Riding){for(int N=0;N<420;++N)Fireworks->UpdateInstanceTransform(N,FTransform(FVector(0,0,-200)),false,false);Fireworks->MarkRenderStateDirty();}
}
