#include "CruiserAssetTools.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Misc/Paths.h"
#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#endif

bool UCruiserAssetTools::EnsureRiderSkeleton(USkeletalMesh* Mesh){
    if(!Mesh)return false;
    if(Mesh->GetSkeleton())return true;
#if WITH_EDITOR
    FString Name=Mesh->GetName()+TEXT("Skeleton");
    UPackage* Package=CreatePackage(*(TEXT("/Game/Characters/")+Name));
    USkeleton* Skeleton=NewObject<USkeleton>(Package,*Name,RF_Public|RF_Standalone);
    if(!Skeleton->MergeAllBonesToBoneTree(Mesh))return false;
    Mesh->SetSkeleton(Skeleton);Mesh->MarkPackageDirty();Package->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(Skeleton);
    return true;
#else
    return false;
#endif
}

bool UCruiserAssetTools::ConfigureGrassDetail(UStaticMesh* Mesh){
#if WITH_EDITOR
    if(!Mesh||Mesh->GetNumSourceModels()<1)return false;
    Mesh->Modify();Mesh->SetNumSourceModels(3);
    const FMeshBuildSettings BuildSettings=Mesh->GetSourceModel(0).BuildSettings;
    const float Ratios[]={1.f,.4f,.12f},Sizes[]={1.f,.035f,.012f};
    for(int i=0;i<3;++i){
        auto& Source=Mesh->GetSourceModel(i);Source.BuildSettings=BuildSettings;
        Source.ReductionSettings.PercentTriangles=Ratios[i];Source.ScreenSize=Sizes[i];
    }
    Mesh->SetAutoComputeLODScreenSize(false);Mesh->Build(false);Mesh->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}

UStaticMesh* UCruiserAssetTools::BakeStudioHead(USkeletalMesh* Source,const FString& PackagePath){
#if WITH_EDITOR
 if(!Source||!Source->GetResourceForRendering()||Source->GetResourceForRendering()->LODRenderData.IsEmpty())return nullptr;
 const auto& LOD=Source->GetResourceForRendering()->LODRenderData[0];
 const auto& VB=LOD.StaticVertexBuffers;const auto* Index=LOD.MultiSizeIndexContainer.GetIndexBuffer();
 for(const auto& Section:LOD.RenderSections){FString Name=Source->GetMaterials()[Section.MaterialIndex].MaterialSlotName.ToString();if(Name.Contains(TEXT("eyeLeft"))||Name.Contains(TEXT("eyeRight"))){FVector Center=FVector::ZeroVector;int Count=0;for(uint32 T=0;T<Section.NumTriangles*3;++T){Center+=FVector(VB.PositionVertexBuffer.VertexPosition(Index->Get(Section.BaseIndex+T)));++Count;}UE_LOG(LogTemp,Display,TEXT("HERO EYE %s %s"),*Name,*(Center/FMath::Max(1,Count)).ToString());}}
 FVector Pivot=FVector::ZeroVector;const auto& Ref=Source->GetRefSkeleton();int Head=Ref.FindBoneIndex(TEXT("head"));
 if(Head>=0){FTransform T=Ref.GetRefBonePose()[Head];for(int P=Ref.GetParentIndex(Head);P>=0;P=Ref.GetParentIndex(P))T=T*Ref.GetRefBonePose()[P];Pivot=T.GetLocation();}
 FBox NeckBounds(ForceInit);for(uint32 I=0;I<VB.PositionVertexBuffer.GetNumVertices();++I){FVector P=FVector(VB.PositionVertexBuffer.VertexPosition(I))-Pivot;if(P.Z>-9.5&&P.Z<-8.5)NeckBounds+=P;}
 UE_LOG(LogTemp,Display,TEXT("NECK SEAM bounds: %s / %s"),*NeckBounds.Min.ToString(),*NeckBounds.Max.ToString());
 if(auto* Existing=LoadObject<UStaticMesh>(nullptr,*(PackagePath+TEXT(".")+FPaths::GetBaseFilename(PackagePath))))return Existing;
 FMeshDescription Desc;FStaticMeshAttributes Attr(Desc);Attr.Register();
 auto Positions=Attr.GetVertexPositions();auto Normals=Attr.GetVertexInstanceNormals();auto Tangents=Attr.GetVertexInstanceTangents();auto Signs=Attr.GetVertexInstanceBinormalSigns();auto UV=Attr.GetVertexInstanceUVs();UV.SetNumChannels(1);
 // Clip the donor shoulder/chest at the neck, interpolating boundary attributes.
 // Keeping triangles that merely crossed the old cutoff left a visible flange.
 struct FCorner {FVector3f P,N,T;FVector2f UV;float Sign;};
 constexpr float NeckCut=-9.f;
 TArray<FPolygonGroupID> Groups;for(int I=0;I<Source->GetMaterials().Num();++I)Groups.Add(Desc.CreatePolygonGroup());
 for(int I=0;I<Groups.Num();++I)Attr.GetPolygonGroupMaterialSlotNames()[Groups[I]]=Source->GetMaterials()[I].MaterialSlotName;
 for(const auto& Section:LOD.RenderSections)for(uint32 Tri=0;Tri<Section.NumTriangles;++Tri){
  TArray<FCorner> Input,Clipped;
  for(int J=0;J<3;++J){uint32 I=Index->Get(Section.BaseIndex+Tri*3+J);
   FVector3f P=FVector3f(FVector(VB.PositionVertexBuffer.VertexPosition(I))-Pivot),N=FVector3f(VB.StaticMeshVertexBuffer.VertexTangentZ(I)),T=VB.StaticMeshVertexBuffer.VertexTangentX(I);
   // Retopologize the donor's trapezius flare into a neck-sized seam. The
   // transition ends below the jaw; the face and cranium remain unchanged.
   float A=FMath::Clamp((-4.f-P.Z)/5.f,0.f,1.f),Blend=A*A*(3-2*A),D=-6*A*(1-A)/5.f;
   float SX=1-.55f*Blend,SY=1-.32f*Blend,DX=-.55f*D*P.X,DY=D*(-.32f*P.Y+1.3872f);
   N=FVector3f(N.X/SX,N.Y/SY,N.Z-DX*N.X/SX-DY*N.Y/SY).GetSafeNormal();
   T=FVector3f(SX*T.X+DX*T.Z,SY*T.Y+DY*T.Z,T.Z).GetSafeNormal();
   P.X*=SX;P.Y=P.Y*SY+1.3872f*Blend;
   Input.Add({P,N,T,VB.StaticMeshVertexBuffer.GetVertexUV(I,0),VB.StaticMeshVertexBuffer.VertexTangentZ(I).W});
  }
  for(int J=0;J<3;++J){const auto& A=Input[J];const auto& B=Input[(J+1)%3];bool InA=A.P.Z>=NeckCut,InB=B.P.Z>=NeckCut;
   if(InA)Clipped.Add(A);
   if(InA!=InB){float Alpha=(NeckCut-A.P.Z)/(B.P.Z-A.P.Z);Clipped.Add({FMath::Lerp(A.P,B.P,Alpha),FMath::Lerp(A.N,B.N,Alpha).GetSafeNormal(),FMath::Lerp(A.T,B.T,Alpha).GetSafeNormal(),FMath::Lerp(A.UV,B.UV,Alpha),A.Sign});}
  }
  if(Clipped.Num()<3)continue;TArray<FVertexInstanceID> Corners;
  for(const auto& P:Clipped){auto V=Desc.CreateVertex();Positions[V]=P.P;auto C=Desc.CreateVertexInstance(V);Normals[C]=P.N;Tangents[C]=P.T;Signs[C]=P.Sign;UV.Set(C,0,P.UV);Corners.Add(C);}
  Desc.CreatePolygon(Groups[Section.MaterialIndex],Corners);
 }
 UPackage* Package=CreatePackage(*PackagePath);auto* Result=NewObject<UStaticMesh>(Package,*FPaths::GetBaseFilename(PackagePath),RF_Public|RF_Standalone);
 for(const auto& Slot:Source->GetMaterials())Result->GetStaticMaterials().Add(FStaticMaterial(nullptr,Slot.MaterialSlotName));
 UStaticMesh::FBuildMeshDescriptionsParams Params;Params.bBuildSimpleCollision=false;Params.bFastBuild=false;Params.bCommitMeshDescription=true;
 if(!Result->BuildFromMeshDescriptions({&Desc},Params))return nullptr;
 FAssetRegistryModule::AssetCreated(Result);Result->MarkPackageDirty();
 UE_LOG(LogTemp,Display,TEXT("STUDIO HEAD: %d vertices, %d triangles, pivot %s, bounds %s"),Desc.Vertices().Num(),Desc.Triangles().Num(),*Pivot.ToString(),*Result->GetBounds().BoxExtent.ToString());return Result;
#else
 return nullptr;
#endif
}

