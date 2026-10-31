#include "CruiserWorld.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

void ACruiserWorld::BuildWorkshopEquipment(){
 auto Load=[&](const TCHAR* Name){return LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Equipment/%s.%s"),Name,Name));};
 auto Add=[&](const TCHAR* Name,FVector P,USceneComponent* Parent){
  auto* Mesh=Load(Name);if(!Mesh){Failure=TEXT("The detailed equipment assets need preparation.");return (UStaticMeshComponent*)nullptr;}
  auto* PartMesh=Part(Mesh,Materials[10],P,FVector(1),Parent);
  for(int I=0;I<Mesh->GetStaticMaterials().Num();++I){FString Slot=Mesh->GetStaticMaterials()[I].MaterialSlotName.ToString();
   if(Slot.Contains(TEXT("Paint")))PartMesh->SetMaterial(I,BikePaint);
   else {FString Mat=FString::Printf(TEXT("/Game/Materials/M_Gear_%s.M_Gear_%s"),*Slot,*Slot);if(auto* Material=LoadObject<UMaterialInterface>(nullptr,*Mat))PartMesh->SetMaterial(I,Material);}
  }
  return PartMesh;
 };
 if(VehicleStyle==2){
  Add(TEXT("ScooterFrameV2"),FVector::ZeroVector,VehicleRoot);
  for(FVector P:{FVector(-35,0,7),FVector(43,0,7)})if(auto* Wheel=Add(TEXT("ScooterWheelV2"),P,VehicleRoot)){
   WheelDetail.Add(Wheel);WheelDetailReference.Add(Wheel->GetRelativeTransform());WheelDetailHubs.Add(P);WheelDetailRadii.Add(5.5f);
  }
 }else{
  for(int Side=0;Side<2;++Side){
   auto* BootRoot=NewObject<USceneComponent>(this);AddInstanceComponent(BootRoot);BootRoot->SetupAttachment(VehicleRoot);BootRoot->RegisterComponent();SkateRoots.Add(BootRoot);
   BootRoot->SetRelativeLocation(FVector(-7,Side?17:-17,0));
   Add(VehicleStyle==3?TEXT("InlineBootV2"):TEXT("QuadBootV2"),FVector::ZeroVector,BootRoot);
   if(VehicleStyle==3){for(float X:{-12.f,-3.f,6.f,15.f})if(auto* Wheel=Add(TEXT("InlineWheelV2"),FVector(X,0,4.2),BootRoot))SkateWheels.Add(Wheel);}
   else for(float X:{-9.f,11.f})for(float Y:{-7.4f,7.4f})if(auto* Wheel=Add(TEXT("QuadWheelV2"),FVector(X,Y,3.2),BootRoot))SkateWheels.Add(Wheel);
  }
 }
}
