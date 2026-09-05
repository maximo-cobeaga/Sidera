#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Exploration/AstraeonMapTypes.h"
#include "Knowledge/AstraeonLogbookTypes.h"
#include "WorldGen/AstraeonEnvironmentTypes.h"
#include "WorldGen/AstraeonRegionTypes.h"
#include "AstraeonSaveGame.generated.h"

UENUM(BlueprintType)
enum class EAstraeonSavedObjectiveState : uint8
{
	MeasureEnvironment UMETA(DisplayName = "Measure Environment"),
	GatherResources UMETA(DisplayName = "Gather Resources"),
	CraftSignalResonator UMETA(DisplayName = "Craft Signal Resonator"),
	ReachSignalSource UMETA(DisplayName = "Reach Signal Source"),
	Completed UMETA(DisplayName = "Completed")
};

UCLASS()
class ASTRAEON_API UAstraeonSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Persistence")
	int32 SaveGameVersion = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Persistence")
	int32 WorldSeed = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Persistence")
	int32 GeneratorVersion = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Persistence")
	FTransform PlayerTransform = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Persistence")
	FAstraeonEnvironmentalSnapshot Environment;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Persistence")
	FAstraeonRegionLayout RegionLayout;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Persistence")
	FAstraeonRevealedMap RevealedMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Persistence")
	TMap<FName, int32> Inventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Persistence")
	EAstraeonSavedObjectiveState ObjectiveState = EAstraeonSavedObjectiveState::MeasureEnvironment;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Persistence")
	TArray<FAstraeonLogbookEntry> LogbookEntries;
};
