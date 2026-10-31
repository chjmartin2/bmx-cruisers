#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CruiserGameMode.generated.h"

UCLASS()
class BMXCRUISER_API ACruiserGameMode : public AGameModeBase {
    GENERATED_BODY()
public:
    ACruiserGameMode();
    virtual void BeginPlay() override;
};
