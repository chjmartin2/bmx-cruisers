#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CruiserAssetTools.generated.h"
class USkeletalMesh;
class UStaticMesh;

UCLASS()
class BMXCRUISER_API UCruiserAssetTools : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="BMX Cruiser|Asset Preparation")
    static bool EnsureRiderSkeleton(USkeletalMesh* Mesh);
    UFUNCTION(BlueprintCallable,Category="BMX Cruiser|Asset Preparation")
    static bool ConfigureGrassDetail(UStaticMesh* Mesh);
    UFUNCTION(BlueprintCallable,Category="BMX Cruiser|Asset Preparation")
    static UStaticMesh* BakeStudioHead(USkeletalMesh* Mesh,const FString& PackagePath);
};
