#include "CruiserWorld.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "ProceduralMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"

namespace {
FVector BendJoint(FVector Root,FVector End,float Upper,float Lower,FVector Preference){
    FVector Direction=(End-Root).GetSafeNormal();
    float Distance=FMath::Clamp(float((End-Root).Size()),FMath::Abs(Upper-Lower)+.1f,Upper+Lower-.1f);
    float Along=(Upper*Upper-Lower*Lower+Distance*Distance)/(2*Distance);
    float Height=FMath::Sqrt(FMath::Max(0.f,Upper*Upper-Along*Along));
    FVector Bend=(Preference-Direction*FVector::DotProduct(Preference,Direction)).GetSafeNormal();
    return Root+Direction*Along+Bend*Height;
}
}

void ACruiserWorld::BuildHumanRider(){
    auto* Mesh=LoadObject<USkeletalMesh>(nullptr,Gender?TEXT("/Game/Characters/YoungGirlV2.YoungGirlV2"):TEXT("/Game/Characters/YoungRiderV2.YoungRiderV2"));
    if(!Mesh||!Mesh->GetSkeleton()){Failure=TEXT("The rider asset and skeleton need preparation.");return;}
    Human=NewObject<UPoseableMeshComponent>(this);AddInstanceComponent(Human);Human->SetupAttachment(Rider);
    Human->SetSkinnedAssetAndUpdate(Mesh);Human->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto* HeroAsset=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Characters/HeroHeadV3.HeroHeadV3"));
    for(int Slot=0;Slot<Mesh->GetMaterials().Num();++Slot){
        FString Name=Mesh->GetMaterials()[Slot].MaterialSlotName.ToString().ToLower();
        FString Region=Name.Contains(TEXT("head"))?TEXT("head"):Name.Contains(TEXT("opacity"))?TEXT("opacity"):TEXT("body");
        FString Path=FString::Printf(TEXT("/Game/Materials/M_%s_%s.M_%s_%s"),Gender?TEXT("Girl"):TEXT("Rider"),*Region,Gender?TEXT("Girl"):TEXT("Rider"),*Region);
        if(auto* Material=LoadObject<UMaterialInterface>(nullptr,*Path)){auto* Instance=UMaterialInstanceDynamic::Create(Material,this);Instance->SetScalarParameterValue(TEXT("Extreme"),Outfit);Instance->SetVectorParameterValue(TEXT("SkinTint"),SkinColor(SkinTone));Instance->SetScalarParameterValue(TEXT("HideShoes"),VehicleStyle==3||VehicleStyle==4?1.f:0.f);Human->SetMaterial(Slot,Instance);}
    }
    if(HeroAsset){
        for(int Slot=0;Slot<Mesh->GetMaterials().Num();++Slot)if(Mesh->GetMaterials()[Slot].MaterialSlotName.ToString().ToLower().Contains(TEXT("head")))Human->SetMaterial(Slot,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_HeroHidden.M_HeroHidden")));
        HeroHead=Part(HeroAsset,Materials[9],FVector::ZeroVector,FVector(Gender?.93f:.97f),Rider);
        for(int Slot=0;Slot<HeroAsset->GetStaticMaterials().Num();++Slot){
            FString Name=HeroAsset->GetStaticMaterials()[Slot].MaterialSlotName.ToString().ToLower();
            if(Name.StartsWith(TEXT("head"))){FString Path=Gender?TEXT("/Game/Materials/M_HeroSkinFemale.M_HeroSkinFemale"):TEXT("/Game/Materials/M_HeroSkinMale.M_HeroSkinMale");auto* Material=LoadObject<UMaterialInterface>(nullptr,*Path);auto* Skin=UMaterialInstanceDynamic::Create(Material,this);Skin->SetVectorParameterValue(TEXT("SkinTint"),SkinColor(SkinTone));HeroHead->SetMaterial(Slot,Skin);}
            else HeroHead->SetMaterial(Slot,HeroAsset->GetStaticMaterials()[Slot].MaterialInterface);
        }
        HeroHead->SetRelativeRotation(FRotator(0,-90,0));
    }
    auto* NeckBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_NeckSkin.M_NeckSkin"));
    if(NeckBase){
        auto* Skin=UMaterialInstanceDynamic::Create(NeckBase,this);Skin->SetVectorParameterValue(TEXT("SkinTint"),SkinColor(SkinTone));
        NeckBridge=NewObject<UProceduralMeshComponent>(this);AddInstanceComponent(NeckBridge);NeckBridge->SetupAttachment(Rider);
        NeckBridge->SetCollisionEnabled(ECollisionEnabled::NoCollision);NeckBridge->SetMaterial(0,Skin);NeckBridge->RegisterComponent();
    }
    Human->bCastDynamicShadow=true;Human->SetCastShadow(true);Human->SetBoundsScale(3);
    Human->RegisterComponent();
    Human->AddTickPrerequisiteActor(this);
    const auto& Skeleton=Mesh->GetRefSkeleton();
    HumanLocalReference=Skeleton.GetRefBonePose();HumanReference.SetNum(HumanLocalReference.Num());
    for(int i=0;i<HumanLocalReference.Num();++i){
        int Parent=Skeleton.GetParentIndex(i);HumanParents.Add(Parent);HumanBoneNames.Add(Skeleton.GetBoneName(i));
        HumanReference[i]=Parent>=0?HumanLocalReference[i]*HumanReference[Parent]:HumanLocalReference[i];
    }
    auto Position=[&](const TCHAR* Name){int I=HumanBoneNames.IndexOfByKey(FName(Name));return I>=0?HumanReference[I].GetLocation():FVector::ZeroVector;};
    FVector Right=Position(TEXT("Bip01-R-Thigh"))-Position(TEXT("Bip01-L-Thigh"));
    HumanFacing=FRotator(0,90-Right.Rotation().Yaw,0).Quaternion();
    HumanScale=170.f/FMath::Max(1.f,float(Mesh->GetImportedBounds().BoxExtent.Z*2));Human->SetRelativeScale3D(FVector(HumanScale));
    FString Rig;
    for(int i=0;i<HumanBoneNames.Num();++i)Rig+=FString::Printf(TEXT("%s: %s\n"),*HumanBoneNames[i].ToString(),*HumanReference[i].GetLocation().ToString());
    FFileHelper::SaveStringToFile(Rig,*(FPaths::ProjectSavedDir()/TEXT("rider-reference-pose.txt")));
    UE_LOG(LogTemp,Display,TEXT("Young rider: %d bones, scale %.3f, facing %.1f degrees"),HumanBoneNames.Num(),HumanScale,HumanFacing.Rotator().Yaw);

    // A fitted helmet shell, with a dark rim and ventilation strips.
    Helmet=NewObject<UProceduralMeshComponent>(this);AddInstanceComponent(Helmet);Helmet->SetupAttachment(Rider);
    Helmet->SetCollisionEnabled(ECollisionEnabled::NoCollision);Helmet->RegisterComponent();
    auto Equip=[&](const TCHAR* Asset,USceneComponent* Parent){
        auto* M=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Equipment/%s.%s"),Asset,Asset));if(!M)return (UStaticMeshComponent*)nullptr;
        auto* C=Part(M,Materials[10],FVector::ZeroVector,FVector(1),Parent);
        for(int I=0;I<M->GetStaticMaterials().Num();++I){FString Slot=M->GetStaticMaterials()[I].MaterialSlotName.ToString();if(Slot.Contains(TEXT("Paint")))C->SetMaterial(I,HelmetPaint);else if(auto* Mat=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Materials/M_Gear_%s.M_Gear_%s"),*Slot,*Slot)))C->SetMaterial(I,Mat);}
        return C;
    };
    Equip(Outfit?TEXT("HelmetExtremeV4"):TEXT("HelmetCasualV3"),Helmet);
    Helmet->SetRelativeScale3D(FVector(Gender?.96f:1.f));
    if(Outfit)for(const TCHAR* Bone:{TEXT("Bip01-L-Calf"),TEXT("Bip01-R-Calf"),TEXT("Bip01-L-Forearm"),TEXT("Bip01-R-Forearm")}){
        if(auto* Guard=Equip(TEXT("GuardV2"),Rider))RiderGuards.Add(Guard);
    }
    PoseHumanRider();
}

void ACruiserWorld::PoseHumanRider(){
    if(!Human||HumanReference.IsEmpty())return;
    if(FMath::IsNearlyEqual(PedalPhase,LastPosePhase,.00001f))return;
    LastPosePhase=PedalPhase;
    TMap<FName,FVector> Positions;TMap<FName,FQuat> Rotations;
    auto Index=[&](FName Name){return HumanBoneNames.IndexOfByKey(Name);};
    auto Ref=[&](FName Name){int I=Index(Name);return I>=0?HumanReference[I].GetLocation():FVector::ZeroVector;};
    auto Place=[&](FName Name,FVector Position){Positions.Add(Name,Position/HumanScale);};
    auto Aim=[&](FName Bone,FName Child,FVector Position,FVector End){
        int I=Index(Bone);if(I<0)return;
        FVector From=HumanFacing.RotateVector(Ref(Child)-Ref(Bone)).GetSafeNormal();
        FQuat Swing=FQuat::FindBetweenNormals(From,(End-Position).GetSafeNormal());
        Rotations.Add(Bone,(Swing*HumanFacing*HumanReference[I].GetRotation()).GetNormalized());Place(Bone,Position);
    };
    auto Upright=[&](FName Bone,FVector Position){int I=Index(Bone);if(I>=0){Place(Bone,Position);Rotations.Add(Bone,HumanFacing*HumanReference[I].GetRotation());}};
    const bool Moving=Simulation->GetState()==BMX::RunState::Riding&&!Paused;
    float Stride=Moving?FMath::Sin(PedalPhase*.6f):0;
    float Bob=FMath::Sin(PedalPhase*2)*.35f;
    FVector Pelvis(-18,0,88+Bob),Waist(-13,0,101+Bob),Spine1(-5,0,113+Bob),Spine2(7,0,124+Bob),Neck(24,0,137+Bob),Head(29,0,146+Bob);
    const bool Standing=VehicleStyle>=2&&VehicleStyle<=4,Chair=VehicleStyle==5;
    if(Standing){Pelvis=FVector(-12,0,98+Bob);Waist=FVector(-4,0,111+Bob);Spine1=FVector(4,0,123+Bob);Spine2=FVector(10,0,134+Bob);Neck=FVector(20,0,148+Bob);Head=FVector(22,0,157+Bob);}
    if(Chair){Pelvis=FVector(-18,0,64+Bob);Waist=FVector(-21,0,77+Bob);Spine1=FVector(-22,0,89+Bob);Spine2=FVector(-20,0,100+Bob);Neck=FVector(-15,0,114+Bob);Head=FVector(-12,0,124+Bob);}
    Upright(TEXT("Bip01-Pelvis"),Pelvis);
    Aim(TEXT("Bip01-Spine"),TEXT("Bip01-Spine1"),Waist,Spine1);
    Aim(TEXT("Bip01-Spine1"),TEXT("Bip01-Spine2"),Spine1,Spine2);
    Aim(TEXT("Bip01-Spine2"),TEXT("Bip01-Neck"),Spine2,Neck);
    Upright(TEXT("Bip01-Neck"),Neck);Upright(TEXT("Bip01-Head"),Head);
    for(int Side=0;Side<2;++Side){
        float Sign=Side==0?-1:1,Phase=PedalPhase+Side*PI;
        if(Pedals.IsValidIndex(Side)&&CrankArms.IsValidIndex(Side)){
            FVector Center(-5,Sign*15,29),Pedal(-5+14*FMath::Cos(Phase),Sign*19,29+14*FMath::Sin(Phase));
            Pedals[Side]->SetRelativeLocation(Pedal);
            CrankArms[Side]->SetRelativeLocation((Center+Pedal)*.5);
            CrankArms[Side]->SetRelativeRotation(FRotationMatrix::MakeFromZ(Pedal-Center).Rotator());
        }
        FString Prefix=Side==0?TEXT("Bip01-L-"):TEXT("Bip01-R-");
        auto Name=[&](const TCHAR* Part){return FName(*(Prefix+Part));};
        auto Length=[&](FName A,FName B){return float((Ref(B)-Ref(A)).Size())*HumanScale;};
        FVector Hip(-18,Sign*9.5f,87+Bob),Ankle(-14+14*FMath::Cos(Phase),Sign*17,38+14*FMath::Sin(Phase));
        if(Standing){Hip=Pelvis+FVector(0,Sign*9.5f,-1);Ankle=VehicleStyle==2?FVector(Side?(-21-Stride*22):5,Side?8:-3,Side?(20+FMath::Max(0.f,Stride)*6):21):FVector(-7+Sign*Stride*14,Sign*(16+FMath::Abs(Stride)*7),24);}
        if(Chair){Hip=Pelvis+FVector(0,Sign*9.5f,-1);Ankle=FVector(17,Sign*16,24);}
        FVector Knee=BendJoint(Hip,Ankle,Length(Name(TEXT("Thigh")),Name(TEXT("Calf"))),Length(Name(TEXT("Calf")),Name(TEXT("Foot"))),FVector(1,Sign*.1,0));
        Aim(Name(TEXT("Thigh")),Name(TEXT("Calf")),Hip,Knee);Aim(Name(TEXT("Calf")),Name(TEXT("Foot")),Knee,Ankle);
        Aim(Name(TEXT("Foot")),Name(TEXT("Toe0")),Ankle,Ankle+FVector(17,0,Standing?-1:-5));
        if(SkateRoots.IsValidIndex(Side)){
            SkateRoots[Side]->SetRelativeLocation(Ankle+FVector(0,0,-23));
            SkateRoots[Side]->SetRelativeRotation(FRotator(0,Sign*(5+Stride*4),Sign*Stride*4));
        }
        FVector Shoulder(7,Sign*18,124+Bob),Hand(39,Sign*23,101);
        if(Standing){Shoulder=Spine2+FVector(0,Sign*18,0);Hand=VehicleStyle==2?FVector(23,Sign*24,109):FVector(11+Sign*Stride*25,Sign*25,Moving?94:88);}
        if(Chair){Shoulder=Spine2+FVector(0,Sign*18,0);Hand=FVector(-14+22*FMath::Cos(Phase),Sign*35,56+16*FMath::Sin(Phase));}
        FVector Elbow=BendJoint(Shoulder,Hand,Length(Name(TEXT("UpperArm")),Name(TEXT("Forearm"))),Length(Name(TEXT("Forearm")),Name(TEXT("Hand"))),FVector(-.2,Sign,-.5));
        Aim(Name(TEXT("UpperArm")),Name(TEXT("Forearm")),Shoulder,Elbow);Aim(Name(TEXT("Forearm")),Name(TEXT("Hand")),Elbow,Hand);
        Aim(Name(TEXT("Hand")),Name(TEXT("Finger1")),Hand,Hand+FVector(4,0,VehicleStyle==3||VehicleStyle==4?-3:-2));
        if(RiderGuards.Num()==4){
            auto FitGuard=[&](int I,FVector Joint,FVector End,FVector Outward){
                FVector Along=(End-Joint).GetSafeNormal();
                RiderGuards[I]->SetRelativeLocation(Joint-Along*3+Outward*1.2f);
                RiderGuards[I]->SetRelativeRotation(FRotationMatrix::MakeFromXZ(Along,Outward).Rotator());
            };
            FitGuard(Side,Knee,Ankle,FVector::ForwardVector);
            FitGuard(Side+2,Elbow,Hand,FVector(0,Sign,0));
        }
    }
    TArray<FTransform> Pose;Pose.SetNum(HumanReference.Num());
    FVector RootOffset=Pelvis/HumanScale-HumanFacing.RotateVector(Ref(TEXT("Bip01-Pelvis")));
    FTransform Global(HumanFacing,RootOffset);
    for(int i=0;i<Pose.Num();++i){
        int Parent=HumanParents[i];FName Name=HumanBoneNames[i];
        Pose[i]=Parent>=0?HumanLocalReference[i]*Pose[Parent]:HumanLocalReference[i]*Global;
        if(auto* P=Positions.Find(Name))Pose[i].SetLocation(*P);
        if(auto* Q=Rotations.Find(Name))Pose[i].SetRotation(Q->GetNormalized());
        Human->BoneSpaceTransforms[i]=Parent>=0?Pose[i].GetRelativeTransform(Pose[Parent]):Pose[i];
    }
    Human->MarkRefreshTransformDirty();
    for(int i=0;i<WheelDetail.Num();++i){
        const FTransform& WheelRef=WheelDetailReference[i];FVector Hub=WheelDetailHubs[i];
        FQuat Turn(FVector::RightVector,PedalPhase*75/FMath::Max(3.f,WheelDetailRadii[i]));
        FTransform WheelPose=WheelRef;WheelPose.SetLocation(Hub+Turn.RotateVector(WheelRef.GetLocation()-Hub));WheelPose.SetRotation(Turn*WheelRef.GetRotation());WheelDetail[i]->SetRelativeTransform(WheelPose);
    }
    for(auto Wheel:SkateWheels)Wheel->SetRelativeRotation(FRotator(FMath::RadiansToDegrees(PedalPhase*14),0,0));
    if(NeckBridge){
        // A tapered anatomical neck overlaps the clipped head and enters below
        // the shirt collar. Both ends follow this frame's actual torso/head pose.
        TArray<FVector> V,N;TArray<int32> Indices;TArray<FVector2D> UV;TArray<FLinearColor> Colors;
        constexpr int Rings=9,Sides=48;float Scale=Gender?.93f:.97f;
        FVector Base=Spine2+FVector(4,0,1),Top=Head+FVector(1,0,3-8.5f*Scale);
        for(int R=0;R<Rings;++R){float T=float(R)/(Rings-1);FVector Center=FMath::Lerp(Base,Top,T);
            float Flare=FMath::Square(1-T),RX=(4.6f+4.2f*Flare)*Scale,RY=(4.95f+5.f*Flare)*Scale;
            for(int S=0;S<=Sides;++S){float A=S*2*PI/Sides;float C=FMath::Cos(A),D=FMath::Sin(A);
                // Subtle paired neck tendons, rather than a featureless cylinder.
                float Tendon=.12f*FMath::Sin(T*PI)*FMath::Pow(FMath::Abs(FMath::Sin(2*A)),8);
                FVector Around(-RX*D,RY*C,0),Along=Top-Base+FVector(-8.4f*(1-T)*Scale*C,-10.f*(1-T)*Scale*D,0);
                V.Add(Center+FVector((RX+Tendon)*C,(RY+Tendon)*D,0));N.Add(FVector::CrossProduct(Around,Along).GetSafeNormal());UV.Add(FVector2D(float(S)/Sides,T));Colors.Add(FLinearColor::White);
                if(R<Rings-1&&S<Sides){int Q=R*(Sides+1)+S;Indices.Append({Q,Q+1,Q+Sides+2,Q,Q+Sides+2,Q+Sides+1});}
            }
        }
        if(!NeckBridge->GetProcMeshSection(0))NeckBridge->CreateMeshSection_LinearColor(0,V,Indices,N,UV,Colors,{},false);
        else NeckBridge->UpdateMeshSection_LinearColor(0,V,N,UV,Colors,{});
    }
    if(HeroHead)HeroHead->SetRelativeLocation(Head+FVector(1,0,3));
    if(Helmet)Helmet->SetRelativeLocation(Head+FVector(1,0,12));
    // Extra protective gear follows the actual posed joints.
    for(int Side=0;Side<2;++Side){FString P=Side==0?TEXT("Bip01-L-"):TEXT("Bip01-R-");
        for(const TCHAR* Finger:{TEXT("Finger0"),TEXT("Finger1"),TEXT("Finger2"),TEXT("Finger3"),TEXT("Finger4")}){
            for(const TCHAR* Joint:{TEXT("1"),TEXT("2")}){FName B(*(P+Finger+Joint));int J=Index(B);if(J>=0)Human->BoneSpaceTransforms[J].ConcatenateRotation(FRotator(VehicleStyle==3||VehicleStyle==4?12:42,0,0).Quaternion());}
        }
    }

}

