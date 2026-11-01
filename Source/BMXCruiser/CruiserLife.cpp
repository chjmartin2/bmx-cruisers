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
 FFileHelper::LoadFileToArray(CourseCells,*(CourseFile(TEXT("-terrain.bin"))));
 const int H=Data->GetIntegerField(TEXT("height"))*8;FRandomStream R(31985);
 auto Cell=[&](int X,int Y){return X>=0&&X<320&&Y>=0&&Y<H&&CourseCells.IsValidIndex(Y*320+X)?CourseCells[Y*320+X]:uint8(255);};
 auto* Reeds=Instances(CylinderMesh,Materials[4]);auto* Heads=Instances(SphereMesh,Materials[3]);auto* Pebbles=Instances(SphereMesh,Materials[11]);auto* Leaves=Instances(SphereMesh,Materials[4]);auto* Wood=Instances(CylinderMesh,Materials[3]);
 auto RodInstance=[&](UInstancedStaticMeshComponent* M,FVector A,FVector B,float Radius){M->AddInstance(FTransform(FRotationMatrix::MakeFromZ(B-A).Rotator(),(A+B)*.5,FVector(Radius/50,Radius/50,(B-A).Size()/100)));};
 // Sample water-edge cells rather than adding hazards to any original opening.
 for(int Y=8;Y<H-8;Y+=4)for(int X=8;X<312;X+=4){
  if(Cell(X,Y)!=2)continue;
  bool Shore=Cell(X-4,Y)==0||Cell(X+4,Y)==0||Cell(X,Y-4)==0||Cell(X,Y+4)==0;
  if(!Shore&&Cell(X-7,Y)==2&&Cell(X+7,Y)==2&&Cell(X,Y-7)==2&&Cell(X,Y+7)==2&&R.FRand()<.014){
   FVector P(Y*7.5,(X-160)*7.5,-8);WaterCenters.Add(P);
   if(R.FRand()<.4)for(int N=0;N<4;++N)Leaves->AddInstance(FTransform(FRotator(0,R.FRand()*360,0),P+FVector(R.FRandRange(-23,23),R.FRandRange(-23,23),.4),FVector(.18,.15,.009)));
  }
 }
 // Dress the continuous curve, not the old square water cells.
 for(auto Value:Visual->GetArrayField(TEXT("shoreline_samples"))){
  if(R.FRand()>.38f)continue;auto C=Value->AsArray();FVector P(C[0]->AsNumber(),C[1]->AsNumber(),-8);
  for(int N=0;N<R.RandRange(3,7);++N){FVector A=P+FVector(R.FRandRange(-9,9),R.FRandRange(-9,9),0),B=A+FVector(R.FRandRange(-8,8),R.FRandRange(-8,8),R.FRandRange(24,62));RodInstance(Reeds,A,B,.6);if(N%2==0)Heads->AddInstance(FTransform(FRotator::ZeroRotator,B,FVector(.022,.022,.10)));}
  if(R.FRand()<.25f)Pebbles->AddInstance(FTransform(FRotator(0,R.FRand()*360,0),P,FVector(.15,.21,.09)));
  if(R.FRand()<.06f)RodInstance(Wood,P,P+FVector(60,24,5),4);
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
 CrashDroplets=Instances(SphereMesh,Materials[12]);CrashDroplets->SetCastShadow(false);CrashDroplets->SetVisibility(false);
 for(int N=0;N<64;++N)CrashDroplets->AddInstance(FTransform(FVector(0,0,-200)));
 Fireworks=Instances(CylinderMesh,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_FireworkStreak.M_FireworkStreak")));Fireworks->SetCastShadow(false);Fireworks->SetNumCustomDataFloats(4);
 for(int N=0;N<FireworkBursts*(FireworkRays*FireworkTrail+1);++N)Fireworks->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,0,-200),FVector::ZeroVector));Fireworks->SetVisibility(false);
 UE_LOG(LogTemp,Display,TEXT("Living park ready: %d ripple sites, %d wildlife; marked original X limits 24..321"),WaterCenters.Num(),Wildlife.Num());
}
void ACruiserWorld::UpdateLivingPark(float Dt){
 CelebrationSparks.Reset();
 EnvironmentTime+=Dt;float T=EnvironmentTime;
 for(int N=0;N<NatureAudio.Num();++N){NatureAudio[N]->SetVolumeMultiplier(EffectsMuted?0:.28);if(NaturePCM.Num()&&NatureStreams[N]->GetAvailableAudioByteCount()<96000)NatureStreams[N]->QueueAudio(NaturePCM.GetData(),NaturePCM.Num());}
 for(int N=0;N<Wildlife.Num();++N){bool Duck=N<6&&WaterCenters.Num();FVector P=WildlifeOrigins[N];float Phase=T*(Duck?.28:.9)+N;
  P+=FVector(FMath::Sin(Phase)* (Duck?12:170),FMath::Cos(Phase*.7)*(Duck?10:90),Duck?FMath::Sin(T*2+N)*.7:FMath::Sin(Phase*2)*14);Wildlife[N]->SetRelativeLocation(P);Wildlife[N]->SetRelativeRotation(FRotator(0,Duck?FMath::Sin(Phase)*35:90+FMath::RadiansToDegrees(Phase),0));
  for(int S=0;S<2;++S)Wings[N*2+S]->SetRelativeRotation(FRotator(0,0,Duck?0:(S?1:-1)*FMath::Sin(T*8+N)*35));
 }
 if(Ripples){
  const int PreviousVertexCount=RippleVertices.Num();
  RippleVertices.Reset();RippleIndices.Reset();RippleNormals.Reset();RippleUV.Reset();RippleColors.Reset();
  float X=Rider->GetRelativeLocation().X;
  for(int N=0;N<WaterCenters.Num();++N){FVector P=WaterCenters[N];if(FMath::Abs(P.X-X)>2400)continue;
   for(int Ring=0;Ring<3;++Ring){float Age=FMath::Fmod(T*.23+N*.319+Ring/3.f,1.f),R=5+Age*40,W=.25f+.7f*(1-Age);int Base=RippleVertices.Num();
    for(int A=0;A<=48;++A)for(int Edge=0;Edge<2;++Edge){float Angle=A*2*PI/48,Rad=R+(Edge?W:-W);RippleVertices.Add(P+FVector(FMath::Cos(Angle)*Rad,FMath::Sin(Angle)*Rad,.16+Ring*.03));RippleNormals.Add(FVector::UpVector);RippleUV.Add(FVector2D(A/48.f,Edge));RippleColors.Add(FLinearColor(.1,.19,.19,1-Age));if(A<48&&Edge==0){int Q=Base+A*2;RippleIndices.Append({Q,Q+2,Q+1,Q+1,Q+2,Q+3});}}
   }
  }
  if(CrashWasWater&&CrashTime>=0&&CrashTime<2){
   for(int Ring=0;Ring<3;++Ring){float Age=CrashTime-Ring*.13f;if(Age<0)continue;
    float Radius=15+Age*120,Width=3*(1-Age/2);int Base=RippleVertices.Num();
    for(int A=0;A<=64;++A)for(int Edge=0;Edge<2;++Edge){float Angle=A*2*PI/64,Rad=Radius+(Edge?Width:-Width);RippleVertices.Add(CrashPosition+FVector(FMath::Cos(Angle)*Rad,FMath::Sin(Angle)*Rad,-9));RippleNormals.Add(FVector::UpVector);RippleUV.Add(FVector2D(A/64.f,Edge));RippleColors.Add(FLinearColor(.65,.85,1,FMath::Max(0.f,1-Age/2)));if(A<64&&Edge==0){int Q=Base+A*2;RippleIndices.Append({Q,Q+2,Q+1,Q+1,Q+2,Q+3});}}
   }
  }
  if(PreviousVertexCount==RippleVertices.Num()&&Ripples->GetNumSections()>0)
   Ripples->UpdateMeshSection_LinearColor(0,RippleVertices,RippleNormals,RippleUV,RippleColors,TArray<FProcMeshTangent>());
  else Ripples->CreateMeshSection_LinearColor(0,RippleVertices,RippleIndices,RippleNormals,RippleUV,RippleColors,TArray<FProcMeshTangent>(),false);
 }
 bool Riding=Simulation->GetState()==BMX::RunState::Riding&&!Paused;
 if(Dust){for(int N=0;N<40;++N){float Age=FMath::Fmod(T*1.5+N*.173,1.f);FVector P=Rider->GetRelativeLocation()+FVector(-50-Age*100,FMath::Sin(N*71.f)*15,3+Age*14);float Size=Riding?(1-Age)*.025:0;
  if(CrashTime>=0&&CrashTime<1.35f&&!CrashWasWater){float A=N*2.39996f,R=CrashTime*(45+(N%7)*13);P=CrashPosition+FVector(FMath::Cos(A)*R,FMath::Sin(A)*R,7+CrashTime*25);Size=(.075f+(N%5)*.012f)*FMath::Sin(PI*FMath::Clamp((CrashTime+.08f)/1.43f,0.f,1.f));}
  Dust->UpdateInstanceTransform(N,FTransform(FRotator::ZeroRotator,P,FVector(Size)),false,false);}Dust->MarkRenderStateDirty();}
 if(CrashDroplets){bool Splash=CrashWasWater&&CrashTime>=0&&CrashTime<1.5f;CrashDroplets->SetVisibility(Splash);
  if(Splash){for(int N=0;N<64;++N){float Age=FMath::Max(0.f,CrashTime-(N%4)*.025f),A=N*2.39996f,Speed=60+(N%9)*12;FVector P=CrashPosition+FVector(FMath::Cos(A)*Speed*Age,FMath::Sin(A)*Speed*Age,8+(170+(N%7)*20)*Age-240*Age*Age);float Size=P.Z>CrashPosition.Z-8?.028f*(1-Age/1.5f):0;CrashDroplets->UpdateInstanceTransform(N,FTransform(FRotator(0,0,A*40),P,FVector(Size,Size,Size*2.3f)),false,false);}CrashDroplets->MarkRenderStateDirty();}
 }
 if(CelebrationTime>=0)CelebrationTime+=Dt;
 if(Fireworks&&Fireworks->IsVisible()!=(CelebrationTime>=0))Fireworks->SetVisibility(CelebrationTime>=0);
 if(Fireworks&&CelebrationTime>=0){
  const FLinearColor Palette[]={FLinearColor(1,.002f,.0004f),FLinearColor(.001f,1,.003f),FLinearColor(1,.32f,.004f),FLinearColor(.004f,.13f,1),FLinearColor(.001f,.8f,1),FLinearColor(1,.004f,.18f),FLinearColor(1,.82f,.38f)};
  auto Streak=[&](int Index,FVector A,FVector B,float Radius,FLinearColor C,float Brightness){FVector D=B-A;FRotator Rotation=D.IsNearlyZero()?FRotator::ZeroRotator:FRotationMatrix::MakeFromZ(D).Rotator();Fireworks->UpdateInstanceTransform(Index,FTransform(Rotation,(A+B)*.5,FVector(Radius/50,Radius/50,D.Size()/100)),false,false);Fireworks->SetCustomDataValue(Index,0,C.R);Fireworks->SetCustomDataValue(Index,1,C.G);Fireworks->SetCustomDataValue(Index,2,C.B);Fireworks->SetCustomDataValue(Index,3,Brightness*(C.G>.7f&&C.B>.5f?1.f:.12f));
   // Keep the colored core vivid after daylight tone mapping; world-space
   // emissive meshes supply bloom, while the final pass preserves the hue.
   if(Radius>0&&Brightness>50&&FMath::Min(A.Z,B.Z)>FenceGroundHeight((A.X+B.X)*.5f,(A.Y+B.Y)*.5f)+2)CelebrationSparks.Add({Fireworks->GetComponentTransform().TransformPosition(A),Fireworks->GetComponentTransform().TransformPosition(B),C,Radius,FMath::Clamp(Brightness/5000,0.f,1.f)});
  };
  for(int Burst=0;Burst<FireworkBursts;++Burst){
   float Age=FMath::Fmod(CelebrationTime+Burst*.47f,5.2f)-.7f;
   FVector Center=Rider->GetRelativeLocation()+FVector(-300+(Burst%3)*130,(Burst%2?1:-1)*(550+(Burst%3)*230),950+(Burst%2)*70);
   int Rocket=FireworkBursts*FireworkRays*FireworkTrail+Burst;
   FVector RocketHead=Center-FVector(0,0,-Age*850);
   Streak(Rocket,Age<0?RocketHead-FVector(0,0,115):Center-FVector(0,0,8),Age<0?RocketHead:Center+FVector(0,0,8),Age<0?3.f:Age<.16f?22*(1-Age/.16f):0.f,FLinearColor(1,.82f,.4f),Age<0?22000:150000);
   for(int Ray=0;Ray<FireworkRays;++Ray){
    float A=Ray*2.39996f+Burst*.6f,B=FMath::Acos(1-2*(Ray+.5f)/FireworkRays);
    FVector Direction(FMath::Cos(A)*FMath::Sin(B),FMath::Sin(A)*FMath::Sin(B),FMath::Cos(B));
    float Speed=650+(Burst%3)*90+FMath::Sin(Ray*7.1f)*90;
    auto Point=[&](float T){T=FMath::Max(0.f,T);return Center+Direction*(Speed/.40f)*(1-FMath::Exp(-.40f*T))+FVector(0,0,-65*T*T);};
    for(int Trail=0;Trail<FireworkTrail;++Trail){
     float SparkAge=Age-Trail*.045f,Tail=Age>1.9f?.015f:.055f;
     float Fade=FMath::Pow(FMath::Clamp(1-SparkAge/3.2f,0.f,1.f),1.25f)*(1-Trail*.17f);
     if(Age<0||SparkAge<0||SparkAge>3.2f||(Age>1.8f&&FMath::Sin(Age*43+Ray*13.7f+Trail)<-.25f))Fade=0;
     FLinearColor C=Ray%9==0||Trail==0?FLinearColor(1,.92f,.72f):Palette[Burst%UE_ARRAY_COUNT(Palette)];
     float Radius=(Age>1.9f?2.8f:4.2f)*(1-Trail*.14f)*(Fade>0?1:0);
     float Glow=Trail==0||Ray%9==0?65000:((Burst%7)==3?26000:(Burst%7)==0?14000:18000);
     Streak((Burst*FireworkRays+Ray)*FireworkTrail+Trail,Point(SparkAge-Tail),Point(SparkAge),Trail==0?Radius*.35f:Radius,C,Fade*Glow);
    }
   }
  }
  Fireworks->MarkRenderStateDirty();
 }
}
