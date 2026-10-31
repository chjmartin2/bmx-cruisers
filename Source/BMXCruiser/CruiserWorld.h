#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "Core/OriginalSimulation.h"
#include "Core/ScrollPresentation.h"
#include "Core/RideSettings.h"
#include "CruiserWorld.generated.h"

class UCameraComponent;
class UProceduralMeshComponent;
class UStaticMeshComponent;
class UAudioComponent;
class USoundWaveProcedural;
class UMaterialInterface;
class UInstancedStaticMeshComponent;
class UPoseableMeshComponent;

UCLASS()
class UCrusierSave : public USaveGame {
    GENERATED_BODY()
public:
    UPROPERTY() int32 HighScore=0;
    UPROPERTY() TArray<int32> BestScores;
    UPROPERTY() int32 Pace=2;
    UPROPERTY() int32 Gender=0;
    UPROPERTY() int32 Outfit=0;
    UPROPERTY() int32 HelmetColor=0;
    UPROPERTY() int32 BikeColor=0;
    UPROPERTY() int32 VehicleStyle=1;
    UPROPERTY() int32 SkinTone=2;
    UPROPERTY() float ViewTilt=40;
    UPROPERTY() float ViewDistance=100;
};

UCLASS()
class BMXCRUISER_API ACruiserWorld : public AActor {
    GENERATED_BODY()
public:
    ACruiserWorld();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    TUniquePtr<BMX::OriginalSimulation> Simulation;
    int32 HighScore=0;
    bool Paused=false, Muted=false;
    bool EffectsMuted=false;
    int32 DifficultyIndex=BMX::RideSettings::Normal,ActiveDifficulty=BMX::RideSettings::Normal;
    TArray<int32> BestScores;
    double ActiveSpeed=1;
    bool IsMenu()const{return Paused||Simulation->GetState()!=BMX::RunState::Riding;}
    void StartRide();
    void RestartRide();
    void ReturnToTitle();
    void PauseRide();
    void ToggleMusic();
    void ToggleEffects();
    void SetDifficulty(int32 Index);
    void SetResolution(bool UHD);
    void ToggleFullscreen();
    bool UHDQuality=false;
    static const TCHAR* DifficultyName(int32 Index);
    FString Failure;
    // Cosmetic settings are deliberately separate from OriginalSimulation.
    int32 Gender=0,Outfit=0,HelmetColor=0,BikeColor=0,VehicleStyle=1,SkinTone=2;
    float PreviewOrbit=0,PreviewZoom=.90f;
    float ViewTilt=40,ViewDistance=100,ViewNoticeUntil=0;
    void AdjustRideView(float TiltDelta,float DistanceDelta,bool Reset=false);
    int32 MenuPage=0,MenuFocus=0; // splash, introduction, main, customize, credits
    void MenuMove(int32 Direction);
    void MenuAdjust(int32 Direction);
    void MenuAccept();
    void MenuBack();
    void MenuAction(FName Action,int32 Direction=1);
    TArray<FName> MenuActions() const;
    FString MenuLabel(FName Action) const;
    static const TCHAR* VehicleName(int32 Index);
    static const TCHAR* ColorName(int32 Index);
    static const TCHAR* SkinName(int32 Index);
    static FLinearColor SkinColor(int32 Index);
    void SavePreferences();
    float CelebrationTime=-1;
private:
    UPROPERTY() TObjectPtr<USceneComponent> Scene;
    UPROPERTY() TObjectPtr<USceneComponent> Rider;
    UPROPERTY() TObjectPtr<UCameraComponent> Camera;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> GarageFloor;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> GarageBackdrop;
    UPROPERTY() TObjectPtr<class UPointLightComponent> GarageKey;
    UPROPERTY() TObjectPtr<class UPointLightComponent> GarageRim;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Terrain;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Bicycle;
    UPROPERTY() TObjectPtr<USceneComponent> VehicleRoot;
    UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> BikePaint;
    UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> HelmetPaint;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> LifeParts;
    UPROPERTY() TArray<TObjectPtr<USceneComponent>> Wildlife;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Wings;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Dust;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Fireworks;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Ripples;
    UPROPERTY() TArray<TObjectPtr<UAudioComponent>> NatureAudio;
    UPROPERTY() TArray<TObjectPtr<USoundWaveProcedural>> NatureStreams;
    TArray<uint8> NaturePCM;
    TArray<FVector> WaterCenters;
    TArray<FVector> WildlifeOrigins;
    TArray<uint8> CourseCells;
    TArray<FVector> RippleVertices,RippleNormals;
    TArray<int32> RippleIndices;
    TArray<FVector2D> RippleUV;
    TArray<FLinearColor> RippleColors;
    float EnvironmentTime=0;
    float VerificationTime=0;
    int32 VerificationStyle=-1;
    bool VerificationCapture=false,CelebrationCaptured=false;
    void VerifyAppearance(float DeltaSeconds);
    void BuildLivingPark(const TSharedPtr<class FJsonObject>& Data,const TSharedPtr<class FJsonObject>& Visual);
    void UpdateLivingPark(float DeltaSeconds);
    void RebuildAppearance();
    void BuildVehicle();
    void BuildWorkshopEquipment();
    void BuildTrailCharacter();
    UPROPERTY() TArray<TObjectPtr<USceneComponent>> SkateRoots;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> SkateWheels;
    UPROPERTY() TObjectPtr<UAudioComponent> Music;
    UPROPERTY() TObjectPtr<USoundWaveProcedural> MusicStream;
    UPROPERTY() TObjectPtr<UAudioComponent> Rolling;
    UPROPERTY() TObjectPtr<UAudioComponent> Ambience;
    UPROPERTY() TObjectPtr<UAudioComponent> Effects;
    UPROPERTY() TObjectPtr<UAudioComponent> CelebrationAudio;
    UPROPERTY() TObjectPtr<USoundWaveProcedural> CelebrationStream;
    TArray<uint8> PopPCM;
    float NextFireworkPop=0;
    UPROPERTY() TObjectPtr<USoundWaveProcedural> RollingStream;
    UPROPERTY() TObjectPtr<USoundWaveProcedural> AmbienceStream;
    UPROPERTY() TObjectPtr<USoundWaveProcedural> EffectsStream;
    UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> CylinderMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> SphereMesh;
    UPROPERTY() TArray<TObjectPtr<UMaterialInterface>> Materials;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Pedals;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> CrankArms;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> WheelSpokes;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> WheelDetail;
    TArray<FTransform> WheelDetailReference;
    TArray<FVector> WheelDetailHubs;
    TArray<float> WheelDetailRadii;
    UPROPERTY() TObjectPtr<UPoseableMeshComponent> Human;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> HeroHead;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> NeckBridge;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Helmet;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> RiderGuards;
    TArray<FTransform> HumanReference;
    TArray<FTransform> HumanLocalReference;
    TArray<FName> HumanBoneNames;
    TArray<int32> HumanParents;
    FQuat HumanFacing;
    float HumanScale=1;
    TArray<uint8> MusicPCM;
    TArray<uint8> RollingPCM,AmbiencePCM;
    double LastWallTime=0,RideWallStart=0;
    FString VideoCaptureDirectory;
    int32 VideoCaptureFrame=0;
    float MenuCaptureDelay=0;
    bool MenuCaptured=false,PreviousMenuLeft=false,PreviousMenuRight=false;
    bool MenuInputActive=false;
    TSet<FKey> HeldMenuKeys;
    FIntPoint LastDisplaySize=FIntPoint::ZeroValue;
    bool DisplayReady=false,DesiredFullscreen=false;
    float LastRenderPercentage=-1;
    void InitializeDisplay();
    void ApplyDisplayMode(bool Fullscreen);
    void UpdateDisplayQuality();
    bool Previous1080=false,Previous4K=false,PreviousFullscreen=false;
    bool PreviousScreenshot=false;
    TArray<int32> ReplayInputs;
    int32 ReplayIndex=0,ReplayStepCycles=0,ReplayCaptureRow=0;
    uint64 ReplayCycles=0;
    double ReplayBudget=0;
    float ReplayDelay=0;
    bool ReplayCaptured=false;
    float PedalPhase=0;
    float DisplayScroll=0;
    BMX::ScrollPresentation ScrollView;
    float LastPosePhase=-1;
    BMX::RunState PreviousState=BMX::RunState::Invalid;
    bool BuildCourse();
    float FenceGroundHeight(float X,float Y) const;
    void DressCourse(const TSharedPtr<class FJsonObject>& Data,const TSharedPtr<class FJsonObject>& Visual);
    void BuildRider();
    void BuildHumanRider();
    void PoseHumanRider();
    void CreateLighting();
    void SaveBest();
    void StartMusic();
    void UpdateAudio();
    void PlayEffect(const FString& Name);
    void HandleInput();
    void AdvanceReplay(float DeltaSeconds);
    UStaticMeshComponent* Part(UStaticMesh* Mesh,UMaterialInterface* Material,FVector Location,FVector Scale,USceneComponent* Parent=nullptr);
    UStaticMeshComponent* Rod(FVector A,FVector B,float Radius,UMaterialInterface* Material,USceneComponent* Parent=nullptr);
    UInstancedStaticMeshComponent* Instances(UStaticMesh* Mesh,UMaterialInterface* Material);
};
