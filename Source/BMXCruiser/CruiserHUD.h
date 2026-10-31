#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CruiserHUD.generated.h"

UCLASS()
class BMXCRUISER_API ACruiserHUD : public AHUD {
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<class UFont> RaceTypeface;
    UPROPERTY() TObjectPtr<class UFont> BodyTypeface;
    virtual void DrawHUD() override;
    virtual void NotifyHitBoxClick(FName BoxName) override;
};
