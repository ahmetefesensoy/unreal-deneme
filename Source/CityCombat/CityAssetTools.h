#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CityAssetTools.generated.h"

UCLASS()
class CITYCOMBAT_API UCityAssetTools : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="City Editor") static class UStaticMesh* BuildRifleMesh();
    UFUNCTION(BlueprintCallable, Category="City Editor") static bool BuildArenaNavigation(class UWorld* World, FVector Center=FVector(0,0,250), FVector Size=FVector(11000,10000,2000));
};
