#include "CruiserWorld.h"
#include "CruiserHUD.h"
#include "Core/RunRules.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Components/InputComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/ConstructorHelpers.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/Engine.h"
#include "UnrealClient.h"
#include "ProfilingDebugging/CsvProfiler.h"
CSV_DEFINE_CATEGORY(BMXCruiser,true);

ACruiserWorld::ACruiserWorld() {
    Simulation=MakeUnique<BMX::OriginalSimulation>();
    PrimaryActorTick.bCanEverTick=true;
    Scene=CreateDefaultSubobject<USceneComponent>(TEXT("Park"));SetRootComponent(Scene);
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("ElevatedCamera"));Camera->SetupAttachment(Scene);
    Camera->ProjectionMode=ECameraProjectionMode::Perspective;Camera->FieldOfView=36;
    Camera->bConstrainAspectRatio=true;Camera->AspectRatio=16.f/9.f;
    GarageKey=CreateDefaultSubobject<UPointLightComponent>(TEXT("GarageSoftbox"));GarageKey->SetupAttachment(Scene);GarageKey->SetIntensity(200000);GarageKey->SetAttenuationRadius(700);GarageKey->SetSourceRadius(90);GarageKey->SetLightColor(FLinearColor(1,.94f,.86f));
    GarageRim=CreateDefaultSubobject<UPointLightComponent>(TEXT("GarageRim"));GarageRim->SetupAttachment(Scene);GarageRim->SetIntensity(40000);GarageRim->SetAttenuationRadius(600);GarageRim->SetSourceRadius(70);GarageRim->SetLightColor(FLinearColor(.68f,.81f,1));
    Rider=CreateDefaultSubobject<USceneComponent>(TEXT("Rider"));Rider->SetupAttachment(Scene);
    Terrain=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("OriginalCourse"));Terrain->SetupAttachment(Scene);
    Terrain->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VehicleRoot=CreateDefaultSubobject<USceneComponent>(TEXT("Vehicle"));VehicleRoot->SetupAttachment(Rider);
    Bicycle=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("BicycleWheels"));Bicycle->SetupAttachment(VehicleRoot);
    Bicycle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Music=CreateDefaultSubobject<UAudioComponent>(TEXT("StudioMusic"));Music->SetupAttachment(Scene);
    Music->bAutoActivate=false;Music->bAllowSpatialization=false;Music->bIsUISound=true;
    Rolling=CreateDefaultSubobject<UAudioComponent>(TEXT("TyresAndDrivetrain"));
    Ambience=CreateDefaultSubobject<UAudioComponent>(TEXT("ParkAmbience"));
    Effects=CreateDefaultSubobject<UAudioComponent>(TEXT("RideEffects"));
    CelebrationAudio=CreateDefaultSubobject<UAudioComponent>(TEXT("FireworkBursts"));
    for(auto* Audio:{Rolling.Get(),Ambience.Get(),Effects.Get(),CelebrationAudio.Get()}){Audio->SetupAttachment(Scene);Audio->bAutoActivate=false;Audio->bAllowSpatialization=false;Audio->bIsUISound=true;}
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    CubeMesh=Cube.Object;CylinderMesh=Cylinder.Object;SphereMesh=Sphere.Object;
}

UStaticMeshComponent* ACruiserWorld::Part(UStaticMesh* Mesh,UMaterialInterface* Material,FVector Location,FVector Scale,USceneComponent* Parent) {
    auto* Component=NewObject<UStaticMeshComponent>(this);
    AddInstanceComponent(Component);Component->SetupAttachment(Parent?Parent:Scene.Get());
    Component->SetStaticMesh(Mesh);for(int32 Slot=0;Slot<FMath::Max(1,Mesh->GetStaticMaterials().Num());++Slot)Component->SetMaterial(Slot,Material);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);Component->SetCanEverAffectNavigation(false);
    Component->SetRelativeLocation(Location);Component->SetRelativeScale3D(Scale);
    Component->RegisterComponent();return Component;
}

UStaticMeshComponent* ACruiserWorld::Rod(FVector A,FVector B,float Radius,UMaterialInterface* Material,USceneComponent* Parent) {
    auto* Component=Part(CylinderMesh,Material,(A+B)*.5,FVector(Radius/50,Radius/50,(B-A).Size()/100),Parent);
    Component->SetRelativeRotation(FRotationMatrix::MakeFromZ(B-A).Rotator());return Component;
}

UInstancedStaticMeshComponent* ACruiserWorld::Instances(UStaticMesh* Mesh,UMaterialInterface* Material) {
    auto* Component=NewObject<UInstancedStaticMeshComponent>(this);
    AddInstanceComponent(Component);Component->SetupAttachment(Scene);
    Component->SetStaticMesh(Mesh);for(int32 Slot=0;Slot<FMath::Max(1,Mesh->GetStaticMaterials().Num());++Slot)Component->SetMaterial(Slot,Material);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);Component->SetCanEverAffectNavigation(false);
    Component->RegisterComponent();return Component;
}

void ACruiserWorld::BeginPlay() {
    Super::BeginPlay();
    const TCHAR* LevelCourse=GetWorld()->URL.GetOption(TEXT("Course="),TEXT(""));
    if(*LevelCourse)CourseIndex=FCString::Atoi(LevelCourse);else FParse::Value(FCommandLine::Get(),TEXT("Course="),CourseIndex);
    CourseIndex=FMath::Clamp(CourseIndex,1,5);
    ScoreBase=FMath::Max(0,FCString::Atoi(GetWorld()->URL.GetOption(TEXT("ScoreBase="),TEXT("0"))));
    PlayingGame=FCString::Atoi(GetWorld()->URL.GetOption(TEXT("Game="),TEXT("0")))!=0||FParse::Param(FCommandLine::Get(),TEXT("VerifyCampaign"));
    ClearedCourses=FCString::Atoi(GetWorld()->URL.GetOption(TEXT("Cleared="),TEXT("0")));
    if(*LevelCourse)MenuPage=2;
    // Opt-in offline trailer capture. Ordinary play continues to use real time.
    if(FParse::Value(FCommandLine::Get(),TEXT("CaptureVideo="),VideoCaptureDirectory)){
        VideoCaptureDirectory=FPaths::ConvertRelativePathToFull(VideoCaptureDirectory);
        IFileManager::Get().MakeDirectory(*VideoCaptureDirectory,true);
        FApp::SetFixedDeltaTime(1.0/30.0);FApp::SetUseFixedTimeStep(true);
    }
    InitializeDisplay();
    TArray<uint8> RAM,IO;
    if(!FFileHelper::LoadFileToArray(RAM,*(CourseIndex==1?FPaths::ProjectContentDir()/TEXT("Data/original-ram.bin"):CourseFile(TEXT("-ram.bin"))))||
       !FFileHelper::LoadFileToArray(IO,*(CourseIndex==1?FPaths::ProjectContentDir()/TEXT("Data/original-io.bin"):CourseFile(TEXT("-io.bin"))))||
       !Simulation->Load(RAM.GetData(),RAM.Num(),IO.GetData(),IO.Num())) {
        Failure=TEXT("The selected course data could not be loaded.");return;
    }
    BestScores.Init(0,BMX::RideSettings::Count);CourseBestScores.Init(0,25);CampaignBestScores.Init(0,5);
    if(auto* Save=Cast<UCrusierSave>(UGameplayStatics::LoadGameFromSlot(TEXT("BMXCruiser"),0))){
        if(Save->BestScores.Num()==BMX::RideSettings::Count)BestScores=Save->BestScores;
        else BestScores[BMX::RideSettings::Normal]=Save->HighScore;
        // Old bests allowed retries and unfinished runs; only the new field
        // qualifies for the one-life board. Preserve course practice records.
        if(Save->CompletedRunBestScores.Num()==5)CampaignBestScores=Save->CompletedRunBestScores;
        if(Save->CourseBestScores.Num()==25)CourseBestScores=Save->CourseBestScores;
        else for(int I=0;I<5;++I)CourseBestScores[I]=BestScores[I];
        ViewTilt=FMath::IsFinite(Save->ViewTilt)?FMath::Clamp(Save->ViewTilt,30.f,75.f):40;
        ViewDistance=FMath::IsFinite(Save->ViewDistance)?FMath::Clamp(Save->ViewDistance,90.f,140.f):100;
        SkinTone=FMath::Clamp(Save->SkinTone,0,6);DifficultyIndex=FMath::Clamp(Save->Pace,0,4);Gender=FMath::Clamp(Save->Gender,0,1);Outfit=FMath::Clamp(Save->Outfit,0,1);
        HelmetColor=FMath::Clamp(Save->HelmetColor,0,7);BikeColor=FMath::Clamp(Save->BikeColor,0,7);VehicleStyle=FMath::Clamp(Save->VehicleStyle,0,5);
    }
    for(int I=0;I<5;++I)BestScores[I]=CourseBestScores[(CourseIndex-1)*5+I];
    FParse::Value(FCommandLine::Get(),TEXT("ViewTilt="),ViewTilt);ViewTilt=FMath::Clamp(ViewTilt,30.f,75.f);
    FParse::Value(FCommandLine::Get(),TEXT("ViewDistance="),ViewDistance);ViewDistance=FMath::Clamp(ViewDistance,90.f,140.f);
    FParse::Value(FCommandLine::Get(),TEXT("Difficulty="),DifficultyIndex);
    if(PlayingGame){DifficultyIndex=FCString::Atoi(GetWorld()->URL.GetOption(TEXT("RunPace="),*FString::FromInt(DifficultyIndex)));RunPace=DifficultyIndex;}
    DifficultyIndex=FMath::Clamp(DifficultyIndex,0,BMX::RideSettings::Count-1);HighScore=CampaignBestScores[DifficultyIndex];
    if(!BuildCourse()){Failure=TEXT("The park assets need to be prepared.");return;}
    FParse::Value(FCommandLine::Get(),TEXT("Style="),VehicleStyle);VehicleStyle=FMath::Clamp(VehicleStyle,0,5);
    FParse::Value(FCommandLine::Get(),TEXT("Gender="),Gender);Gender=FMath::Clamp(Gender,0,1);
    FParse::Value(FCommandLine::Get(),TEXT("Outfit="),Outfit);Outfit=FMath::Clamp(Outfit,0,1);
    FParse::Value(FCommandLine::Get(),TEXT("SkinTone="),SkinTone);SkinTone=FMath::Clamp(SkinTone,0,6);
    FParse::Value(FCommandLine::Get(),TEXT("MenuPage="),MenuPage);
    FParse::Value(FCommandLine::Get(),TEXT("PreviewOrbit="),PreviewOrbit);
    FParse::Value(FCommandLine::Get(),TEXT("PreviewZoom="),PreviewZoom);PreviewZoom=FMath::Clamp(PreviewZoom,.50f,1.15f);
    BuildRider();CreateLighting();
    GarageFloor=Part(CubeMesh,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Gear_Grip.M_Gear_Grip")),FVector::ZeroVector,FVector(16,16,.05f));GarageFloor->SetCastShadow(false);
    GarageBackdrop=Part(CubeMesh,Materials[10],FVector::ZeroVector,FVector(.1f,20,12));GarageBackdrop->SetCastShadow(false);
    Rider->SetRelativeLocation(FVector(Simulation->ForwardPixels()*7.5,Simulation->RightPixels()*7.5,2));
    Camera->SetRelativeLocation(FVector(-2300,0,2450));Camera->SetRelativeRotation(FRotator(-40,0,0));
    if(auto* PC=GetWorld()->GetFirstPlayerController()){
        PC->bShowMouseCursor=true;PC->bEnableClickEvents=true;PC->bAutoManageActiveCameraTarget=false;PC->SetViewTarget(this);
        PC->SetInputMode(FInputModeGameOnly());
        EnableInput(PC);
        // Explicit axis bindings register continuous gamepad input with PlayerInput.
        InputComponent->BindAxisKey(EKeys::Gamepad_LeftX);
        InputComponent->BindAxisKey(EKeys::Gamepad_LeftY);
        InputComponent->BindKey(EKeys::M,IE_Pressed,this,&ACruiserWorld::ToggleMusic);
        InputComponent->BindKey(EKeys::V,IE_Pressed,this,&ACruiserWorld::ToggleEffects);
    }
    UE_LOG(LogTemp,Display,TEXT("Course ready"));
    if(FParse::Param(FCommandLine::Get(),TEXT("VerifyRunMenus"))){
        bool Good=MenuActions().Contains(TEXT("game"))&&MenuActions().Contains(TEXT("courses"))&&MenuActions().Contains(TEXT("scores"));
        PlayingGame=true;Simulation->Start();Simulation->Crash();
        Good&=MenuActions().Contains(TEXT("game"))&&!MenuActions().Contains(TEXT("ride"))&&!MenuActions().Contains(TEXT("nextcourse"));
        PlayingGame=false;Good&=MenuActions().Contains(TEXT("ride"));
        Simulation->Reset();MenuPage=8;Good&=MenuActions().Num()==1&&MenuActions()[0]==TEXT("back");MenuPage=2;
        UE_LOG(LogTemp,Display,TEXT("ONE LIFE AND PRACTICE MENUS %s"),Good?TEXT("PASSED"):TEXT("FAILED"));
    }
    StartMusic();PreviousState=Simulation->GetState();
    if(FCString::Atoi(GetWorld()->URL.GetOption(TEXT("Start="),TEXT("0")))||FCString::Atoi(GetWorld()->URL.GetOption(TEXT("Practice="),TEXT("0"))))RestartRide();
    // Optional developer replay uses the same simulation and original collision.
    FString ReplayPath;
    FParse::Value(FCommandLine::Get(),TEXT("ReplayFile="),ReplayPath);
    FString ReplayDirectory;
    if(FParse::Value(FCommandLine::Get(),TEXT("CourseReplayDirectory="),ReplayDirectory))ReplayPath=ReplayDirectory/FString::Printf(TEXT("course%d-completion-replay.json"),CourseIndex);
    if(!ReplayPath.IsEmpty()){
        FString Json;TSharedPtr<FJsonObject> Replay;
        if(FFileHelper::LoadFileToString(Json,*ReplayPath)&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Replay)&&Replay.IsValid()){
            MenuPage=2;ReplayStepCycles=Replay->GetIntegerField(TEXT("cycles_per_input"));
            for(auto Value:Replay->GetArrayField(TEXT("inputs")))ReplayInputs.Add(Value->AsNumber());
            FParse::Value(FCommandLine::Get(),TEXT("ReplayCaptureRow="),ReplayCaptureRow);
            UE_LOG(LogTemp,Display,TEXT("Verification replay loaded: %d inputs"),ReplayInputs.Num());
        }
    }
    LastWallTime=FPlatformTime::Seconds();
}

FString ACruiserWorld::CourseFile(const FString& Suffix)const{
    return FPaths::ProjectContentDir()/FString::Printf(TEXT("Data/course%d%s"),CourseIndex,*Suffix);
}
void ACruiserWorld::SelectCourse(int32 Course,bool Start){
    if(Start&&(Course!=CourseIndex+1||!BMX::RunRules::CanContinue(PlayingGame,CourseIndex,ClearedCourses,Simulation->GetState()==BMX::RunState::Complete)))return;
    SaveBest();SavePreferences();
    UGameplayStatics::OpenLevel(this,TEXT("CourseOne"),true,FString::Printf(TEXT("Course=%d?Start=%d?Practice=%d?ScoreBase=%d?Game=%d?Cleared=%d?RunPace=%d"),FMath::Clamp(Course,1,5),Start?1:0,Start?0:1,Start?ScoreBase+Simulation->Score():0,Start&&PlayingGame?1:0,Start?CourseIndex:0,RunPace));
}
void ACruiserWorld::PlayGame(){
    SaveBest();SavePreferences();
    UGameplayStatics::OpenLevel(this,TEXT("CourseOne"),true,FString::Printf(TEXT("Course=1?Start=1?ScoreBase=0?Game=1?Cleared=0?RunPace=%d"),DifficultyIndex));
}
bool ACruiserWorld::BuildCourse() {
    const TCHAR* Names[]={TEXT("Grass"),TEXT("Asphalt"),TEXT("Water"),TEXT("Wood"),TEXT("Foliage"),TEXT("Metal"),TEXT("Rubber"),TEXT("Blue"),TEXT("Purple"),TEXT("Skin"),TEXT("Dark"),TEXT("Rock"),TEXT("White"),TEXT("Bank"),TEXT("Verge"),TEXT("FlowerWhite"),TEXT("FlowerYellow"),TEXT("FlowerPurple"),TEXT("FlowerRed")};
    for(const auto* Name:Names){
        FString Path=FString::Printf(TEXT("/Game/Materials/M_%s.M_%s"),Name,Name);
        auto* Material=LoadObject<UMaterialInterface>(nullptr,*Path);
        if(!Material)return false;Materials.Add(Material);
    }
    FString Text;
    if(!FFileHelper::LoadFileToString(Text,*(CourseFile(TEXT(".json")))))return false;
    TSharedPtr<FJsonObject> Data;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Data)||!Data.IsValid())return false;
    FinishScore=Data->GetIntegerField(TEXT("finish_scroll"));
    VisibleObstacles.Reset();WaterContacts.Reset();
    Simulation->SetOriginalCollision(FParse::Param(FCommandLine::Get(),TEXT("OriginalCollision")));
    TArray<FBox2D> FinishFootprints;
    auto Surface=[&](const TCHAR* Field,int Section,UMaterialInterface* Material,float Height){
        TArray<FVector> Vertices,Normals;TArray<int32> Indices;TArray<FVector2D> UV;
        for(const auto& Value:Data->GetArrayField(Field)){
            const auto& R=Value->AsArray();double X=R[0]->AsNumber(),Y=R[1]->AsNumber(),W=R[2]->AsNumber(),H=R[3]->AsNumber();
            double A=Y*7.5,B=(Y+H)*7.5,C=(X-160)*7.5,D=(X+W-160)*7.5;int32 Base=Vertices.Num();
            FinishFootprints.Add(FBox2D(FVector2D(A,C),FVector2D(B,D)).ExpandBy(35));
            // Source-pixel subdivisions align with the meadow grid and its
            // triangle diagonals. Large, flat checkers cut through the hills.
            int NX=FMath::Max(1,FMath::CeilToInt((B-A)/7.5)),NY=FMath::Max(1,FMath::CeilToInt((D-C)/7.5));
            for(int IX=0;IX<=NX;++IX)for(int IY=0;IY<=NY;++IY){
                float PX=FMath::Lerp(A,B,double(IX)/NX),PY=FMath::Lerp(C,D,double(IY)/NY);
                Vertices.Add(FVector(PX,PY,FenceGroundHeight(PX,PY)+Height));
                Normals.Add(FVector(FenceGroundHeight(PX-1,PY)-FenceGroundHeight(PX+1,PY),FenceGroundHeight(PX,PY-1)-FenceGroundHeight(PX,PY+1),2).GetSafeNormal());
                UV.Add(FVector2D(PX/200,PY/200));
                if(IX<NX&&IY<NY){int Q=Base+IX*(NY+1)+IY;Indices.Append({Q,Q+NY+2,Q+NY+1,Q,Q+1,Q+NY+2});}
            }
        }
        Terrain->CreateMeshSection_LinearColor(Section,Vertices,Indices,Normals,UV,TArray<FLinearColor>(),TArray<FProcMeshTangent>(),false);
        Terrain->SetMaterial(Section,Material);
        float MinClearance=MAX_flt;
        for(int I=0;I<Indices.Num();I+=3){const FVector P=(Vertices[Indices[I]]+Vertices[Indices[I+1]]+Vertices[Indices[I+2]])/3;MinClearance=FMath::Min(MinClearance,float(P.Z)-FenceGroundHeight(P.X,P.Y));}
        UE_LOG(LogTemp,Display,TEXT("FINISH TERRAIN section %d: %s, minimum clearance %.3f cm"),Section,MinClearance>=Height-.01f?TEXT("PASSED"):TEXT("FAILED"),MinClearance);
    };
    FString VisualText;TSharedPtr<FJsonObject> Visual;
    if(!FFileHelper::LoadFileToString(VisualText,*(CourseFile(TEXT("-visuals.json"))))||
       !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(VisualText),Visual)||!Visual.IsValid())return false;
    auto SmoothSurface=[&](const TCHAR* Field,int Section,UMaterialInterface* Material){
        const auto Mesh=Visual->GetObjectField(Field);
        TArray<FVector> Vertices,Normals;TArray<int32> Indices;TArray<FVector2D> UV;
        for(const auto& Value:Mesh->GetArrayField(TEXT("vertices"))){
            auto P=Value->AsArray();FVector V(P[0]->AsNumber(),P[1]->AsNumber(),P[2]->AsNumber());
            // Extend only the decorative outer underlay for the wider camera.
            // Interior water openings and the original playable terrain stay intact.
            if(FCString::Strcmp(Field,TEXT("grass_underlay"))==0){
                if(V.Y<=-2025)V.Y=-8000;else if(V.Y>=2025)V.Y=8000;
                if(V.X<=-750)V.X=-6000;else if(V.X>=(Data->GetIntegerField(TEXT("height"))*8+100)*7.5)V.X=(Data->GetIntegerField(TEXT("height"))*8+100)*7.5+6000;
            }
            Vertices.Add(V);Normals.Add(FVector::UpVector);UV.Add(FVector2D(V.X/200,V.Y/200));
        }
        for(const auto& Value:Mesh->GetArrayField(TEXT("indices")))Indices.Add(Value->AsNumber());
        TArray<FLinearColor> Colors;
        if(Mesh->HasField(TEXT("colors")))for(auto Value:Mesh->GetArrayField(TEXT("colors"))){auto C=Value->AsArray();Colors.Add(FLinearColor(C[0]->AsNumber(),C[1]->AsNumber(),C[2]->AsNumber(),C[3]->AsNumber()));}
        if(FCString::Strcmp(Field,TEXT("banks"))==0||FCString::Strcmp(Field,TEXT("grass"))==0){
            for(auto& Normal:Normals)Normal=FVector::ZeroVector;
            for(int i=0;i<Indices.Num();i+=3){
                int A=Indices[i],B=Indices[i+1],C=Indices[i+2];FVector N=FVector::CrossProduct(Vertices[C]-Vertices[A],Vertices[B]-Vertices[A]).GetSafeNormal();
                Normals[A]+=N;Normals[B]+=N;Normals[C]+=N;
            }
            for(auto& Normal:Normals)Normal.Normalize();
        }
        if(FCString::Strcmp(Field,TEXT("water"))==0){
            for(int I=0;I<Indices.Num();I+=3){FVector2D A(Vertices[Indices[I]]),B(Vertices[Indices[I+1]]),C(Vertices[Indices[I+2]]);FWaterTriangle T;T.A=A;T.B=B;T.C=C;T.Bounds=FBox2D(A,A);T.Bounds+=B;T.Bounds+=C;WaterContacts.Add(T);}
        }
        Terrain->CreateMeshSection_LinearColor(Section,Vertices,Indices,Normals,UV,Colors,TArray<FProcMeshTangent>(),false);
        Terrain->SetMaterial(Section,Material);
    };
    SmoothSurface(TEXT("grass"),0,Materials[0]);SmoothSurface(TEXT("road"),1,Materials[1]);
    SmoothSurface(TEXT("water"),2,Materials[2]);SmoothSurface(TEXT("waterbed"),5,Materials[13]);
    SmoothSurface(TEXT("banks"),6,Materials[13]);SmoothSurface(TEXT("verge"),7,Materials[14]);
    SmoothSurface(TEXT("grass_underlay"),8,Materials[0]);
    Surface(TEXT("finish_black_rects"),3,Materials[10],2);
    Surface(TEXT("finish_white_rects"),4,Materials[12],2);
    auto* ShrubMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Models/Shrub.Shrub"));
    auto* GrassMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Models/LawnGrass.LawnGrass"));
    if(!ShrubMesh||!GrassMesh)return false;
    UE_LOG(LogTemp,Display,TEXT("Shrub bounds: %s. Grass bounds: %s"),*ShrubMesh->GetBounds().BoxExtent.ToString(),*GrassMesh->GetBounds().BoxExtent.ToString());
    TArray<UStaticMesh*> ShrubVariants;TArray<UInstancedStaticMeshComponent*> ShrubInstances;
    for(int i=0;i<9;++i){
        FString Asset=FString::Printf(TEXT("/Game/Models/Shrub_%d.Shrub_%d"),i,i);
        if(auto* Variant=LoadObject<UStaticMesh>(nullptr,*Asset)){
            ShrubVariants.Add(Variant);auto* Group=Instances(Variant,Materials[4]);Group->SetCullDistances(8000,10000);ShrubInstances.Add(Group);
        }
    }
    auto* DenseMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_DenseFoliage.M_DenseFoliage"));
    for(int i=0;i<4;++i){
        FString Asset=FString::Printf(TEXT("/Game/Models/DenseShrub_%d.DenseShrub_%d"),i,i);
        if(auto* Variant=LoadObject<UStaticMesh>(nullptr,*Asset)){
            ShrubVariants.Add(Variant);auto* Group=Instances(Variant,DenseMaterial?DenseMaterial:Materials[4].Get());Group->SetCullDistances(8000,10000);ShrubInstances.Add(Group);
        }
    }
    if(ShrubVariants.IsEmpty()){ShrubVariants.Add(ShrubMesh);ShrubInstances.Add(Instances(ShrubMesh,Materials[4]));}
    TArray<UInstancedStaticMeshComponent*> Flowers;
    for(int i=15;i<19;++i){auto* Group=Instances(SphereMesh,Materials[i]);Group->SetCastShadow(false);Flowers.Add(Group);}
    auto* FlowerCenters=Instances(SphereMesh,Materials[16]);
    FlowerCenters->SetCastShadow(false);
    TArray<FVector> RockV,RockN;TArray<int32> RockI;TArray<FVector2D> RockUV;
    FRandomStream Random(1985);
    auto AddRock=[&](FVector Center,FVector Radius,bool Hazard=false){
        if(Hazard){FVisibleObstacle O;O.A=FVector2D(Center);O.Radius=FVector2D(Radius.X*.85,Radius.Y*.85);O.Ellipse=true;VisibleObstacles.Add(O);}
        int Base=RockV.Num();float Phase=Random.FRandRange(0,2*PI);
        for(int r=0;r<=8;++r)for(int a=0;a<=16;++a){
            float Polar=r*PI/8,T=a*PI/8;FVector N(FMath::Sin(Polar)*FMath::Cos(T),FMath::Sin(Polar)*FMath::Sin(T),FMath::Cos(Polar));
            float Rough=1+.12f*FMath::Sin(3*T+2*Polar+Phase)+.08f*FMath::Cos(5*T-Polar+Phase);
            RockV.Add(Center+N*Radius*Rough);RockN.Add((N/Radius).GetSafeNormal());RockUV.Add(FVector2D(a/16.f,r/8.f));
            if(r<8&&a<16){int Q=Base+r*17+a;RockI.Append({Q,Q+1,Q+17,Q+1,Q+18,Q+17});}
        }
    };
    for(const auto& Value:Data->GetArrayField(TEXT("shrubs"))){
        auto P=Value->AsObject();float X=P->GetNumberField(TEXT("y"))*60,Y=(P->GetNumberField(TEXT("x"))-20)*60;
        float W=P->GetNumberField(TEXT("width"))*60,D=P->GetNumberField(TEXT("depth"))*60;
        int Color=P->GetIntegerField(TEXT("color"));
        if(Color==1){
            AddRock(FVector(X,Y,25),FVector(D*.4,W*.4,45),true);
            continue;
        }
        FVisibleObstacle Bush;Bush.A=FVector2D(X,Y);Bush.Radius=FVector2D(D*.33,W*.33);Bush.Ellipse=true;VisibleObstacles.Add(Bush);
        int Variant=ShrubVariants.Num()>9&&Random.FRand()<.8?Random.RandRange(9,ShrubVariants.Num()-1):Random.RandRange(0,ShrubVariants.Num()-1);const auto SB=ShrubVariants[Variant]->GetBounds();
        FRotator Rotation(0,Random.FRandRange(0,360),0);float R=FMath::DegreesToRadians(Rotation.Yaw);
        float ExtX=FMath::Abs(FMath::Cos(R))*SB.BoxExtent.X+FMath::Abs(FMath::Sin(R))*SB.BoxExtent.Y;
        float ExtY=FMath::Abs(FMath::Sin(R))*SB.BoxExtent.X+FMath::Abs(FMath::Cos(R))*SB.BoxExtent.Y;
        float Horizontal=FMath::Min(D/(2*ExtX),W/(2*ExtY))*Random.FRandRange(.92,1.04);
        FVector Scale(Horizontal,Horizontal,FMath::Clamp(FMath::Min(W,D)*Random.FRandRange(.65,1.05),45.f,110.f)/FMath::Max(1.f,float(SB.BoxExtent.Z*2)));
        FVector Offset=Rotation.RotateVector(FVector(SB.Origin.X*Scale.X,SB.Origin.Y*Scale.Y,(SB.Origin.Z-SB.BoxExtent.Z)*Scale.Z));
        ShrubInstances[Variant]->AddInstance(FTransform(Rotation,FVector(X,Y,-2)-Offset,Scale));
        if(Variant>=9){
            for(int Branch=0;Branch<6;++Branch){
                FRotator Turn(0,Random.FRandRange(0,360),0);FVector BranchScale=Scale*Random.FRandRange(.58,.76);
                FVector BranchOffset=Turn.RotateVector(FVector(SB.Origin.X*BranchScale.X,SB.Origin.Y*BranchScale.Y,(SB.Origin.Z-SB.BoxExtent.Z)*BranchScale.Z));
                float Angle=Branch*2*PI/6;
                FVector Center(X+FMath::Cos(Angle)*D*.19,Y+FMath::Sin(Angle)*W*.19,-2);
                ShrubInstances[Variant]->AddInstance(FTransform(Turn,Center-BranchOffset,BranchScale));
            }
        }
        // Small flower clusters preserve the original scenery's colour accents.
        if(Random.FRand()<.72f){
            int Tint=Color==2?3:Color==4?2:Color==7?1:Random.RandRange(0,2);
            int Count=FMath::Clamp(int(W*D/160),14,90);
            float Height=SB.BoxExtent.Z*Scale.Z*2;
            for(int Bloom=0;Bloom<Count;++Bloom){
                float Theta=Random.FRandRange(0,2*PI),Radius=Random.FRandRange(.2,.86);
                FVector Center(X+FMath::Cos(Theta)*D*.36*Radius,Y+FMath::Sin(Theta)*W*.36*Radius,Height*(.58+.28*FMath::Sqrt(1-Radius*Radius)));
                FlowerCenters->AddInstance(FTransform(FRotator::ZeroRotator,Center,FVector(.018,.018,.012)));
                for(int Petal=0;Petal<5;++Petal){
                    float Angle=Petal*2*PI/5;
                    Flowers[Tint]->AddInstance(FTransform(FRotator(0,FMath::RadiansToDegrees(Angle),0),Center+FVector(FMath::Cos(Angle)*1.1,FMath::Sin(Angle)*1.1,.2),FVector(.027,.015,.009)));
                }
            }
        }
    }
    TArray<uint8> TerrainCells;
    FFileHelper::LoadFileToArray(TerrainCells,*(CourseFile(TEXT("-terrain.bin"))));
    const auto& Shore=Visual->GetObjectField(TEXT("banks"))->GetArrayField(TEXT("vertices"));
    for(int i=2;i<Shore.Num();i+=90){
        if(Random.FRand()<.25f)continue;
        auto P=Shore[i]->AsArray();float X=P[0]->AsNumber(),Y=P[1]->AsNumber();
        int PY=FMath::RoundToInt(X/7.5),PX=FMath::RoundToInt(Y/7.5+160);bool BesideTrail=false;
        for(int DY=-5;DY<=5&&!BesideTrail;++DY)for(int DX=-5;DX<=5;++DX){int Index=(PY+DY)*320+PX+DX;if(PX+DX>=0&&PX+DX<320&&TerrainCells.IsValidIndex(Index)&&TerrainCells[Index]==1){BesideTrail=true;break;}}
        if(BesideTrail)continue;
        for(int Stone=0;Stone<Random.RandRange(1,3);++Stone){
            float Size=Random.FRandRange(4,10);AddRock(FVector(X+Random.FRandRange(-6,6),Y+Random.FRandRange(-6,6),-2),FVector(Size,Size*Random.FRandRange(.65,1.2),Size*.7));
        }
    }
    // These four original character codes form the large white boulder. They
    // were previously misread as isolated asphalt and had no visible rock mesh.
    const auto& Codes=Data->GetArrayField(TEXT("codes"));
    for(int Row=0;Row<Codes.Num();++Row){auto Cells=Codes[Row]->AsArray();for(int Col=0;Col<Cells.Num();++Col)if(Cells[Col]->AsNumber()==70){
        float X=(Row+1)*60,Y=(Col+1-20)*60;AddRock(FVector(X,Y,FenceGroundHeight(X,Y)+30),FVector(55,55,65),true);
    }}
    auto* NaturalRocks=NewObject<UProceduralMeshComponent>(this);AddInstanceComponent(NaturalRocks);NaturalRocks->SetupAttachment(Scene);NaturalRocks->RegisterComponent();
    NaturalRocks->CreateMeshSection_LinearColor(0,RockV,RockI,RockN,RockUV,TArray<FLinearColor>(),TArray<FProcMeshTangent>(),false);NaturalRocks->SetMaterial(0,Materials[11]);
    auto* GrassMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_LawnGrass.M_LawnGrass"));
    auto* Grass=Instances(GrassMesh,GrassMaterial?GrassMaterial:Materials[4].Get());Grass->SetCullDistances(4600,6500);
    Grass->SetCastShadow(false);Grass->bAffectDistanceFieldLighting=false;
    const auto GB=GrassMesh->GetBounds();
    TArray<uint8> Meadow;FFileHelper::LoadFileToArray(Meadow,*((CourseIndex==1?FPaths::ProjectContentDir()/TEXT("Data/meadow-heights.bin"):CourseFile(TEXT("-meadow.bin")))));
    auto GrassHeight=[&](float PX,float PY){
        constexpr int Columns=136;int Rows=Meadow.Num()/int(sizeof(float))/Columns;
        if(Rows<2)return -2.f;
        float GX=FMath::Clamp((PX+110)/4,0.f,float(Columns-1.001)),GY=FMath::Clamp((PY+100)/4,0.f,float(Rows-1.001));int X=int(GX),Y=int(GY);
        auto At=[&](int C,int R){float Z;FMemory::Memcpy(&Z,Meadow.GetData()+(R*Columns+C)*sizeof(float),sizeof(float));return Z;};
        return FMath::Lerp(FMath::Lerp(At(X,Y),At(X+1,Y),GX-X),FMath::Lerp(At(X,Y+1),At(X+1,Y+1),GX-X),GY-Y);
    };
    for(int i=0;i<55000;++i){
        int PX=Random.RandRange(-90,409),PY=Random.RandRange(0,Data->GetIntegerField(TEXT("height"))*8-1);
        if(PX>=0&&PX<320&&(!TerrainCells.IsValidIndex(PY*320+PX)||TerrainCells[PY*320+PX]!=0))continue;
        bool OnFinish=false;for(const auto& Bounds:FinishFootprints)if(Bounds.IsInside(FVector2D(PY*7.5,(PX-160)*7.5))){OnFinish=true;break;}
        if(OnFinish)continue;
        float Size=Random.FRandRange(5,8)/FMath::Max(1.f,float(GB.BoxExtent.Z*2));FVector Scale(Size);
        FRotator Rotation(0,Random.FRand()*360,0);
        FVector Offset=Rotation.RotateVector(FVector(GB.Origin.X*Scale.X,GB.Origin.Y*Scale.Y,(GB.Origin.Z-GB.BoxExtent.Z)*Scale.Z));
        Grass->AddInstance(FTransform(Rotation,FVector(PY*7.5,(PX-160)*7.5,GrassHeight(PX,PY))-Offset,Scale));
    }
    DressCourse(Data,Visual);BuildLivingPark(Data,Visual);BuildTrailCharacter();
    WaterContactBuckets.Reset();ObstacleContactBuckets.Reset();
    for(int I=0;I<WaterContacts.Num();++I)for(int X=FMath::FloorToInt(WaterContacts[I].Bounds.Min.X/300);X<=FMath::FloorToInt(WaterContacts[I].Bounds.Max.X/300);++X)WaterContactBuckets.FindOrAdd(X).Add(I);
    for(int I=0;I<VisibleObstacles.Num();++I){const auto& O=VisibleObstacles[I];double Low=O.Ellipse?O.A.X-O.Radius.X:FMath::Min(O.A.X,O.B.X),High=O.Ellipse?O.A.X+O.Radius.X:FMath::Max(O.A.X,O.B.X);for(int X=FMath::FloorToInt((Low-18)/300);X<=FMath::FloorToInt((High+18)/300);++X)ObstacleContactBuckets.FindOrAdd(X).Add(I);}
    if(FParse::Param(FCommandLine::Get(),TEXT("BakeContacts"))){
        int32 Width=481,Height=FMath::CeilToInt((Data->GetIntegerField(TEXT("height"))*60+600)/5.f);
        TArray<uint8> Bytes;Bytes.SetNumUninitialized(8+Width*Height);FMemory::Memcpy(Bytes.GetData(),&Width,4);FMemory::Memcpy(Bytes.GetData()+4,&Height,4);
        for(int X=0;X<Height;++X)for(int Y=0;Y<Width;++Y)Bytes[8+X*Width+Y]=VisibleContact(FVector(X*5,Y*5-1200,0))?1:0;
        FString Folder;FParse::Value(FCommandLine::Get(),TEXT("ContactFolder="),Folder);if(Folder.IsEmpty())Folder=FPaths::ProjectSavedDir();
        FFileHelper::SaveArrayToFile(Bytes,*(Folder/FString::Printf(TEXT("course%d-contact.bin"),CourseIndex)));
        UE_LOG(LogTemp,Display,TEXT("CONTACT PROFILE BAKED course %d"),CourseIndex);
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("VerifyContacts"))){
        bool Good=VisibleObstacles.Num()>0&&WaterContacts.Num()>0;
        if(WaterContacts.Num()){const auto& T=WaterContacts[0];FVector2D P=(T.A+T.B+T.C)/3;Good&=VisibleContact(FVector(P.X,P.Y,0));}
        if(CourseIndex==3){
            FVector Rock(2820,60,0),Clear(2820,140,0);
            Good&=VisibleContact(Rock)&&!VisibleContact(Clear);
            Good&=SweepVisibleContact(FVector(2820,-20,0),Clear);
        }
        UE_LOG(LogTemp,Display,TEXT("VISIBLE CONTACTS course %d: %s; %d obstacles; %d water triangles"),CourseIndex,Good?TEXT("PASSED"):TEXT("FAILED"),VisibleObstacles.Num(),WaterContacts.Num());
        if(!Good)return false;
    }
    return true;
}

void ACruiserWorld::BuildRider(){BuildVehicle();BuildHumanRider();}

void ACruiserWorld::CreateLighting() {
    auto* SkyMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Sky.M_Sky"));
    if(SkyMaterial){auto* Dome=Part(SphereMesh,SkyMaterial,FVector(7000,0,0),FVector(4000));Dome->SetCastShadow(false);Dome->bAffectDistanceFieldLighting=false;}
    auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>();
    auto* SunComponent=CastChecked<UDirectionalLightComponent>(Sun->GetLightComponent());
    SunComponent->SetIntensity(95000);SunComponent->SetLightColor(FLinearColor(1,.98,.92));
    SunComponent->SetMobility(EComponentMobility::Movable);
    Sun->SetActorRotation(FRotator(-55,-35,0));
    SunComponent->SetAtmosphereSunLight(true);
    SunComponent->SetForwardShadingPriority(1);
    SunComponent->SetLightSourceAngle(3.2f);
    if(auto* Clouds=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_CloudShade.M_CloudShade")))SunComponent->SetLightFunctionMaterial(Clouds);
    auto* Fog=GetWorld()->SpawnActor<AExponentialHeightFog>();Fog->SetActorLocation(FVector(0,0,-300));
    Fog->GetComponent()->SetFogDensity(.004f);Fog->GetComponent()->SetFogHeightFalloff(.15f);Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(.55f,.66f,.70f));
    auto* Fill=GetWorld()->SpawnActor<ADirectionalLight>();
    auto* FillLight=CastChecked<UDirectionalLightComponent>(Fill->GetLightComponent());
    FillLight->SetMobility(EComponentMobility::Movable);Fill->SetActorRotation(FRotator(-60,-20,0));
    FillLight->SetIntensity(5500);FillLight->SetLightColor(FLinearColor(.80,.88,1));FillLight->SetCastShadows(false);
    FillLight->SetForwardShadingPriority(0);
    GetWorld()->SpawnActor<ASkyAtmosphere>();
    auto* Sky=GetWorld()->SpawnActor<ASkyLight>();Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sky->GetLightComponent()->SetRealTimeCaptureEnabled(true);Sky->GetLightComponent()->SetIntensity(1.6);
    auto* Post=GetWorld()->SpawnActor<APostProcessVolume>();Post->bUnbound=true;
    auto& P=Post->Settings;P.bOverride_AutoExposureMethod=true;P.AutoExposureMethod=AEM_Histogram;
    P.bOverride_AutoExposureMinBrightness=true;P.bOverride_AutoExposureMaxBrightness=true;P.AutoExposureMinBrightness=13.3;P.AutoExposureMaxBrightness=13.3;
    P.bOverride_MotionBlurAmount=true;P.MotionBlurAmount=0;P.bOverride_BloomIntensity=true;P.BloomIntensity=.12;
    P.bOverride_AmbientOcclusionIntensity=true;P.AmbientOcclusionIntensity=.55;
    P.bOverride_AmbientOcclusionRadius=true;P.AmbientOcclusionRadius=65;
    P.bOverride_ColorSaturation=true;P.ColorSaturation=FVector4(.94,.94,.94,1);
    P.bOverride_ColorContrast=true;P.ColorContrast=FVector4(1,1,1,1);
    SunComponent->ContactShadowLength=.04f;
}

void ACruiserWorld::SaveBest() {
    if(ReplayInputs.Num()||FParse::Param(FCommandLine::Get(),TEXT("VerifyCampaign")))return;
    BestScores[ActiveDifficulty]=FMath::Max(BestScores[ActiveDifficulty],Simulation->Score());
    CourseBestScores[(CourseIndex-1)*5+ActiveDifficulty]=BestScores[ActiveDifficulty];
    if(BMX::RunRules::Qualifies(PlayingGame,CourseIndex,ClearedCourses,Simulation->GetState()==BMX::RunState::Complete,ActiveDifficulty,RunPace))CampaignBestScores[ActiveDifficulty]=FMath::Max(CampaignBestScores[ActiveDifficulty],ScoreBase+Simulation->Score());
    HighScore=CampaignBestScores[DifficultyIndex];
    SavePreferences();
}

void ACruiserWorld::StartRide(){
    MenuPage=2;MenuFocus=0;
    if(Paused&&DifficultyIndex==ActiveDifficulty){Paused=false;return;}
    if(Simulation->GetState()!=BMX::RunState::Riding||Paused)RestartRide();
}

void ACruiserWorld::RestartRide(){
    if(PlayingGame&&Simulation->GetState()!=BMX::RunState::Ready){PlayGame();return;}
    SaveBest();CelebrationCaptured=false;CelebrationTime=-1;NextFireworkPop=0;HaveContactPosition=false;Simulation->Reset();Simulation->Start();Paused=false;DisplayScroll=0;ScrollView.Reset();
    ActiveDifficulty=DifficultyIndex;ActiveSpeed=BMX::RideSettings::Speed(ActiveDifficulty);RideWallStart=FPlatformTime::Seconds();LastWallTime=RideWallStart;
    Simulation->SetSteeringScale(BMX::RideSettings::SteeringScale(ActiveDifficulty));
    // Let the course camera, lighting history and streamed resources settle
    // before the race clock starts. The original simulation is held still.
    RideWarmupRemaining=FParse::Param(FCommandLine::Get(),TEXT("VerifyAnalog"))?0:1.5f;
    Rider->SetRelativeLocation(FVector(Simulation->ForwardPixels()*7.5,Simulation->RightPixels()*7.5,2));
    PlayEffect(TEXT("start"));
    UE_LOG(LogTemp,Display,TEXT("Course One ride started: %s, speed %.2fx"),DifficultyName(ActiveDifficulty),ActiveSpeed);
}

void ACruiserWorld::PauseRide(){if(Simulation->GetState()==BMX::RunState::Riding)Paused=!Paused;else MenuBack();}
void ACruiserWorld::ToggleMusic(){Muted=!Muted;UpdateAudio();}
void ACruiserWorld::ToggleEffects(){EffectsMuted=!EffectsMuted;UpdateAudio();}
const TCHAR* ACruiserWorld::DifficultyName(int32 Index){const TCHAR* Names[]={TEXT("CRUISE"),TEXT("RELAXED"),TEXT("NORMAL"),TEXT("FAST"),TEXT("EXPERT")};return Names[FMath::Clamp(Index,0,4)];}
void ACruiserWorld::SetDifficulty(int32 Index){if(!IsMenu()||PlayingGame)return;DifficultyIndex=FMath::Clamp(Index,0,4);HighScore=CampaignBestScores[DifficultyIndex];SavePreferences();PlayEffect(TEXT("select"));}
void ACruiserWorld::ReturnToTitle(){MenuPage=2;MenuFocus=0;CelebrationTime=-1;SaveBest();PlayingGame=false;ClearedCourses=0;ScoreBase=0;Simulation->Reset();ScrollView.Reset();DisplayScroll=0;Paused=false;PedalPhase=0;Rider->SetRelativeRotation(FRotator::ZeroRotator);Rider->SetRelativeLocation(FVector(Simulation->ForwardPixels()*7.5,Simulation->RightPixels()*7.5,2));}

void ACruiserWorld::AdvanceReplay(float DeltaSeconds){
    ReplayDelay+=DeltaSeconds;
    if(ReplayDelay<2)return;
    if(Simulation->GetState()==BMX::RunState::Ready){RestartRide();return;}
    ReplayBudget+=DeltaSeconds*ActiveSpeed*BMX::OriginalSimulation::ClockHz;
    while(ReplayBudget>=1&&ReplayIndex<ReplayInputs.Num()&&Simulation->GetState()==BMX::RunState::Riding){
        int Input=ReplayInputs[ReplayIndex];Simulation->SetInput(Input==-1||Input==4||Input==6,Input==1||Input==5||Input==7,Input==3||Input==6||Input==7,Input==2||Input==4||Input==5);
        uint64 Chunk=FMath::Min(uint64(ReplayBudget),uint64(ReplayStepCycles)-ReplayCycles);
        Simulation->TickCycles(Chunk);ReplayCycles+=Chunk;ReplayBudget-=Chunk;
        if(ReplayCycles==uint64(ReplayStepCycles)){++ReplayIndex;ReplayCycles=0;}
    }
    if(!ReplayCaptured&&ReplayCaptureRow>0&&Simulation->Rows()>=ReplayCaptureRow){
        ReplayCaptured=true;FScreenshotRequest::RequestScreenshot(TEXT("CourseOne-ride"),true,true);
    }
}

void ACruiserWorld::HandleInput() {
    auto* PC=GetWorld()->GetFirstPlayerController();if(!PC)return;
    if(FParse::Param(FCommandLine::Get(),TEXT("VerifyAnalog"))){
        if(Simulation->GetState()==BMX::RunState::Ready)RestartRide();
        int Phase=FMath::FloorToInt(VerificationTime*2);
        if(Phase!=AnalogTestPhase){
            UE_LOG(LogTemp,Display,TEXT("ANALOG PHASE %d x %d -> %d y %d -> %d axes %.3f %.3f"),AnalogTestPhase,AnalogStartX,Simulation->SpriteX(),AnalogStartY,Simulation->SpriteY(),PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX),PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY));
            if(AnalogTestPhase==0)AnalogTestGood&=Simulation->SpriteX()>AnalogStartX+5;
            if(AnalogTestPhase==1)AnalogTestGood&=Simulation->SpriteX()<AnalogStartX-5;
            if(AnalogTestPhase==2)AnalogTestGood&=Simulation->SpriteY()<AnalogStartY-5;
            if(AnalogTestPhase==3)AnalogTestGood&=Simulation->SpriteY()>AnalogStartY+5;
            if(AnalogTestPhase==4){AnalogTestGood&=FMath::Abs(PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX))<.05&&FMath::Abs(PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY))<.05;UE_LOG(LogTemp,Display,TEXT("ANALOG INPUT %s"),AnalogTestGood?TEXT("PASSED"):TEXT("FAILED"));FPlatformMisc::RequestExit(false);}
            AnalogTestPhase=Phase;AnalogStartX=Simulation->SpriteX();AnalogStartY=Simulation->SpriteY();
        }
        PC->InputKey(FInputKeyParams(EKeys::Gamepad_LeftX,Phase==0?.9:Phase==1?-.9:0.,GetWorld()->GetDeltaSeconds(),1,true));
        PC->InputKey(FInputKeyParams(EKeys::Gamepad_LeftY,Phase==2?.9:Phase==3?-.9:0.,GetWorld()->GetDeltaSeconds(),1,true));
    }
    auto Down=[&](FKey Key){return PC->IsInputKeyDown(Key);};
    auto Pressed=[&](FKey Key){return PC->IsInputKeyDown(Key)||PC->WasInputKeyJustPressed(Key);};
    float AX=PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX),AY=PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
    bool Menu=IsMenu()&&!ReplayInputs.Num();PC->bShowMouseCursor=Menu;
    if(Menu!=MenuInputActive){
        if(Menu){FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);PC->SetInputMode(Mode);}
        else PC->SetInputMode(FInputModeGameOnly());
        MenuInputActive=Menu;
    }
    auto Edge=[&](FKey Key){bool On=Pressed(Key),Was=HeldMenuKeys.Contains(Key);if(On)HeldMenuKeys.Add(Key);else HeldMenuKeys.Remove(Key);return On&&!Was;};
    const bool Up=Edge(EKeys::Up)|Edge(EKeys::Gamepad_DPad_Up),Dn=Edge(EKeys::Down)|Edge(EKeys::Tab)|Edge(EKeys::Gamepad_DPad_Down);
    const bool Left=Edge(EKeys::Left)|Edge(EKeys::Gamepad_DPad_Left),Right=Edge(EKeys::Right)|Edge(EKeys::Gamepad_DPad_Right);
    const bool Accept=Edge(EKeys::Enter)|Edge(EKeys::SpaceBar)|Edge(EKeys::Gamepad_FaceButton_Bottom);
    const bool Back=Edge(EKeys::Escape)|Edge(EKeys::Gamepad_FaceButton_Right),Pause=Edge(EKeys::P),Restart=Edge(EKeys::R);
    if(Menu){if(Up)MenuMove(-1);if(Dn)MenuMove(1);if(Left)MenuAdjust(-1);if(Right)MenuAdjust(1);if(Accept)MenuAccept();if(Back)MenuBack();if(Restart&&Simulation->GetState()!=BMX::RunState::Ready)RestartRide();}
    else {if(Back||Pause)PauseRide();if(Restart)RestartRide();}
    if(Edge(EKeys::C)&&!ReplayInputs.Num()){
        if(MenuPage==6)MenuBack();
        else {if(Simulation->GetState()==BMX::RunState::Riding)Paused=true;MenuPage=6;MenuFocus=0;}
    }
    if(MenuPage==6||Simulation->GetState()==BMX::RunState::Riding){
        if(Edge(EKeys::PageUp))AdjustRideView(5,0);
        if(Edge(EKeys::PageDown))AdjustRideView(-5,0);
        if(Edge(EKeys::Equals)||Edge(EKeys::Add))AdjustRideView(0,-5);
        if(Edge(EKeys::Hyphen)||Edge(EKeys::Subtract))AdjustRideView(0,5);
        if(Edge(EKeys::Home))AdjustRideView(0,0,true);
    }
    if(ReplayInputs.IsEmpty())Simulation->SetInput(Down(EKeys::Left)||Down(EKeys::A)||Down(EKeys::Gamepad_DPad_Left)||AX<-.35,
        Down(EKeys::Right)||Down(EKeys::D)||Down(EKeys::Gamepad_DPad_Right)||AX>.35,
        Down(EKeys::Up)||Down(EKeys::W)||Down(EKeys::Gamepad_DPad_Up)||AY>.35,
        Down(EKeys::Down)||Down(EKeys::S)||Down(EKeys::Gamepad_DPad_Down)||AY<-.35);
    bool HD=Pressed(EKeys::One),UHD=Pressed(EKeys::Two),FS=Pressed(EKeys::F11);
    if(HD&&!Previous1080)SetResolution(false);
    if(UHD&&!Previous4K)SetResolution(true);
    if(FS&&!PreviousFullscreen)ToggleFullscreen();
    Previous1080=HD;Previous4K=UHD;PreviousFullscreen=FS;
    bool Screenshot=Pressed(EKeys::F9);
    if(Screenshot&&!PreviousScreenshot)FScreenshotRequest::RequestScreenshot(TEXT("BMXCruiser"),true,true);
    PreviousScreenshot=Screenshot;
}

void ACruiserWorld::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);
    if(!Failure.IsEmpty())return;
    HandleInput();
    UpdateDisplayQuality();
    const double WallNow=FPlatformTime::Seconds();
    const double Elapsed=!VideoCaptureDirectory.IsEmpty()?DeltaSeconds:(LastWallTime>0?WallNow-LastWallTime:0);LastWallTime=WallNow;
    if(!ReplayInputs.Num()&&!FParse::Param(FCommandLine::Get(),TEXT("RenderOffscreen"))&&Simulation->GetState()==BMX::RunState::Riding&&!Paused&&GetWorld()->GetGameViewport()&&GetWorld()->GetGameViewport()->Viewport&&!GetWorld()->GetGameViewport()->Viewport->HasFocus())Paused=true;
    const double Before=Simulation->Cycles()/BMX::OriginalSimulation::ClockHz;
    const bool Warming=RideWarmupRemaining>0;
    if(Warming&&!Paused){RideWarmupRemaining=FMath::Max(0.f,RideWarmupRemaining-DeltaSeconds);if(RideWarmupRemaining==0){RideWallStart=WallNow;UE_LOG(LogTemp,Display,TEXT("Course %d warm-up complete; race clock starts"),CourseIndex);}}
    if(!Paused&&!Warming){if(ReplayInputs.Num())AdvanceReplay(Elapsed);else Simulation->Advance(Elapsed*ActiveSpeed);}
    auto State=Simulation->GetState();
    const double Now=Simulation->Cycles()/BMX::OriginalSimulation::ClockHz;
    const float PreviousScroll=DisplayScroll;
    DisplayScroll=ScrollView.Update(Now,Simulation->LastScrollCycle()/BMX::OriginalSimulation::ClockHz,Simulation->Rows(),Now-Before);
    CSV_CUSTOM_STAT(BMXCruiser,VisualForwardSpeed,Now>Before?(DisplayScroll-PreviousScroll)/(Now-Before):0,ECsvCustomStatOp::Set);
    float Offset=Simulation->ForwardPixels()*7.5-Simulation->Rows()*60;
    FVector Position=Rider->GetRelativeLocation();
    float Blend=1-FMath::Exp(-28*DeltaSeconds);
    Position.X=DisplayScroll+FMath::Lerp(float(Position.X-PreviousScroll),Offset,Blend);
    Position.Y=FMath::Lerp(float(Position.Y),float(Simulation->RightPixels()*7.5),Blend);
    Position.Z=(State==BMX::RunState::Ready&&MenuPage!=6?42:2)+.45f*FMath::Sin(float(Now)*23)+.25f*FMath::Sin(float(Now)*37);Rider->SetRelativeLocation(Position);
    if(!FParse::Param(FCommandLine::Get(),TEXT("OriginalCollision"))&&!FParse::Param(FCommandLine::Get(),TEXT("VerifyAnalog"))&&!Paused&&(State==BMX::RunState::Riding||State==BMX::RunState::Complete)){
        if(SweepVisibleContact(HaveContactPosition?PreviousContactPosition:Position,Position)){UE_LOG(LogTemp,Display,TEXT("VISIBLE CONTACT at row %d position %.1f %.1f source %.1f %.1f"),Simulation->Rows(),Position.X,Position.Y,Simulation->ForwardPixels()*7.5,Simulation->RightPixels()*7.5);Simulation->Crash();}
        PreviousContactPosition=Position;HaveContactPosition=true;State=Simulation->GetState();
    }
    if(State!=PreviousState&&(State==BMX::RunState::Crashed||State==BMX::RunState::Complete)){
        SaveBest();FString Effect=State==BMX::RunState::Complete?TEXT("finish"):TEXT("crash");
        if(State==BMX::RunState::Crashed){
            TArray<uint8> Cells;FFileHelper::LoadFileToArray(Cells,*(CourseFile(TEXT("-terrain.bin"))));
            int PX=FMath::RoundToInt(Simulation->RightPixels()+160),PY=FMath::RoundToInt(Simulation->ForwardPixels());
            if(PX>=0&&PX<320&&Cells.IsValidIndex(PY*320+PX)&&Cells[PY*320+PX]==2)Effect=TEXT("splash");
        }
        PlayEffect(Effect);MenuFocus=0;if(State==BMX::RunState::Complete)CelebrationTime=0;
        UE_LOG(LogTemp,Display,TEXT("Ride ended: state %d, score %d, source seconds %.6f, wall seconds %.6f, speed %.2fx"),int(State),Simulation->Score(),Simulation->Cycles()/BMX::OriginalSimulation::ClockHz,WallNow-RideWallStart,ActiveSpeed);
        if(ReplayInputs.Num()){
            UE_LOG(LogTemp,Display,TEXT("VERIFICATION %s"),State==BMX::RunState::Complete&&Simulation->Score()==FinishScore?TEXT("PASSED"):TEXT("FAILED"));
            if(FParse::Param(FCommandLine::Get(),TEXT("ProfileCourse"))&&FCsvProfiler::IsCapturing())FCsvProfiler::Get()->EndCapture();
            FScreenshotRequest::RequestScreenshot(TEXT("CourseOne-finish"),true,true);
        }
    }
    PreviousState=State;

    float Lean=Simulation->SpriteFrame()>=0xdc&&Simulation->SpriteFrame()<0xde?-12:Simulation->SpriteFrame()>=0xde&&Simulation->SpriteFrame()<0xe0?12:0;
    FRotator Heading(0,Lean,State==BMX::RunState::Crashed?65:-Lean*.3f);
    Rider->SetRelativeRotation(FQuat::Slerp(Rider->GetRelativeRotation().Quaternion(),Heading.Quaternion(),1-FMath::Exp(-18*DeltaSeconds)));
    bool InGarage=State==BMX::RunState::Ready&&MenuPage!=6&&!ReplayInputs.Num();
    Camera->PostProcessSettings.bOverride_AutoExposureBias=InGarage;Camera->PostProcessSettings.AutoExposureBias=InGarage?1.2f:0.f;
    GarageKey->SetVisibility(InGarage);GarageRim->SetVisibility(InGarage);if(GarageFloor){GarageFloor->SetVisibility(InGarage);GarageBackdrop->SetVisibility(InGarage);}
    GarageKey->SetRelativeLocation(Rider->GetRelativeLocation()+FVector(185,-160,215));GarageRim->SetRelativeLocation(Rider->GetRelativeLocation()+FVector(-125,140,240));
    if(InGarage){
        if(!VideoCaptureDirectory.IsEmpty()&&FParse::Param(FCommandLine::Get(),TEXT("TrailerOrbit")))PreviewOrbit+=DeltaSeconds*8;
        if(auto* PC=GetWorld()->GetFirstPlayerController()){float Turn=(PC->IsInputKeyDown(EKeys::E)?1:0)-(PC->IsInputKeyDown(EKeys::Q)?1:0);PreviewOrbit+=Turn*DeltaSeconds*45;}
        if(auto* PC=GetWorld()->GetFirstPlayerController()){if(PC->WasInputKeyJustPressed(EKeys::Equals))PreviewZoom=FMath::Max(.66f,PreviewZoom-.05f);if(PC->WasInputKeyJustPressed(EKeys::Hyphen))PreviewZoom=FMath::Min(1.15f,PreviewZoom+.05f);}
        FVector View=FRotator(0,PreviewOrbit,0).RotateVector(FVector(470,-500,300))*PreviewZoom;
        if(GarageFloor){GarageFloor->SetRelativeLocation(Rider->GetRelativeLocation()+FVector(0,0,-4));GarageBackdrop->SetRelativeLocation(Rider->GetRelativeLocation()-View.GetSafeNormal2D()*410+FVector(0,0,330));GarageBackdrop->SetRelativeRotation(FRotationMatrix::MakeFromX(View.GetSafeNormal2D()).Rotator());}
        FVector Right=FRotationMatrix::MakeFromX(-View).GetUnitAxis(EAxis::Y);
        Camera->FieldOfView=46;Camera->SetRelativeLocation(Rider->GetRelativeLocation()+View);
        Camera->SetRelativeRotation((Rider->GetRelativeLocation()+FVector(0,0,87)-Right*145-Camera->GetRelativeLocation()).Rotation());
    }else{
        // Follow the scrolling course, not the rider. Original joystick movement
        // now travels visibly within the frame, including at the front/back limits.
        // The wider, centered view contains the full 320-pixel gameplay width.
        const float DefaultRadius=2450/FMath::Sin(FMath::DegreesToRadians(40.f));
        const float Focus=DefaultRadius*FMath::Cos(FMath::DegreesToRadians(40.f))-2300;
        float Radius=DefaultRadius*ViewDistance/100,Angle=FMath::DegreesToRadians(ViewTilt);
        Camera->FieldOfView=52;Camera->SetRelativeRotation(FRotator(-ViewTilt,0,0));
        Camera->SetRelativeLocation(FVector(DisplayScroll+Focus-Radius*FMath::Cos(Angle),0,Radius*FMath::Sin(Angle)));
    }
    if(State==BMX::RunState::Riding&&!Paused){
        PedalPhase+=(Now-Before)*8;
    }
    PoseHumanRider();
    UpdateLivingPark(DeltaSeconds);UpdateAudio();VerifyAppearance(DeltaSeconds);
    if(FParse::Param(FCommandLine::Get(),TEXT("CaptureMenu"))&&!MenuCaptured){MenuCaptureDelay+=DeltaSeconds;if(MenuCaptureDelay>3){MenuCaptured=true;FScreenshotRequest::RequestScreenshot(TEXT("BMXCruiser-title"),true,true);}}
    if(!VideoCaptureDirectory.IsEmpty()){
        FScreenshotRequest::RequestScreenshot(VideoCaptureDirectory/FString::Printf(TEXT("frame-%06d.png"),VideoCaptureFrame++),true,false);
    }
}

