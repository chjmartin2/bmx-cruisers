#pragma once
#include "CoreMinimal.h"

struct FCruiserMeshChunk {
 TArray<FVector> Vertices,Normals;
 TArray<FVector2D> UV;
 TArray<FLinearColor> Colors;
 TArray<int32> Indices;
};
using FCruiserSceneCache=TMap<FString,TArray<FCruiserMeshChunk>>;
bool LoadCruiserSceneCache(const FString& Path,FCruiserSceneCache& Out);
