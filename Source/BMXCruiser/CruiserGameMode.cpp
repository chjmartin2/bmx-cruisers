#include "CruiserGameMode.h"
#include "CruiserWorld.h"
#include "CruiserHUD.h"
#include "Engine/World.h"

ACruiserGameMode::ACruiserGameMode() {
    DefaultPawnClass=nullptr;
    HUDClass=ACruiserHUD::StaticClass();
}
void ACruiserGameMode::BeginPlay() {
    Super::BeginPlay();
    GetWorld()->SpawnActor<ACruiserWorld>();
}
