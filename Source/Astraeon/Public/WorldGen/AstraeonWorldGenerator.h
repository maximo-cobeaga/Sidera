#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WorldGen/AstraeonEnvironmentTypes.h"
#include "WorldGen/AstraeonRegionTypes.h"
#include "AstraeonWorldGenerator.generated.h"

UCLASS()
class ASTRAEON_API UAstraeonWorldGenerator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentGeneratorVersion = 1;

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static int32 DeriveSeed(int32 RootSeed, const FString& Context);

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static FAstraeonEnvironmentalSnapshot GenerateEnvironment(int32 WorldSeed);

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static FAstraeonRegionLayout GenerateRegionLayout(int32 WorldSeed);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Science")
	static bool IsBreathable(const FAstraeonEnvironmentalSnapshot& Environment);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Science")
	static float ComputeEnvironmentalRisk01(const FAstraeonEnvironmentalSnapshot& Environment);
};
