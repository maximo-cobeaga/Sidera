#pragma once

#include "CoreMinimal.h"
#include "Creatures/AstraeonCreatureTypes.h"
#include "Engine/GameInstance.h"
#include "Exploration/AstraeonMapTypes.h"
#include "Knowledge/AstraeonLogbookTypes.h"
#include "Persistence/AstraeonSaveGame.h"
#include "WorldGen/AstraeonEnvironmentTypes.h"
#include "WorldGen/AstraeonRegionTypes.h"
#include "AstraeonGameInstance.generated.h"

UENUM(BlueprintType)
enum class EAstraeonObjectiveState : uint8
{
	MeasureEnvironment UMETA(DisplayName = "Measure Environment"),
	GatherResources UMETA(DisplayName = "Gather Resources"),
	CraftSignalResonator UMETA(DisplayName = "Craft Signal Resonator"),
	ReachSignalSource UMETA(DisplayName = "Reach Signal Source"),
	Completed UMETA(DisplayName = "Completed")
};

UCLASS()
class ASTRAEON_API UAstraeonGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Session")
	void StartNewGame(int32 RequestedWorldSeed = 0);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Session")
	int32 GetCurrentWorldSeed() const { return CurrentWorldSeed; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Session")
	const FAstraeonEnvironmentalSnapshot& GetCurrentEnvironment() const { return CurrentEnvironment; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Session")
	const FAstraeonRegionLayout& GetCurrentRegionLayout() const { return CurrentRegionLayout; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Session")
	const TArray<FAstraeonLogbookEntry>& GetRuntimeLogbookEntries() const { return RuntimeLogbookEntries; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Map")
	const FAstraeonRevealedMap& GetRevealedMap() const { return RevealedMap; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Inventory")
	const TMap<FName, int32>& GetInventory() const { return Inventory; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Inventory")
	int32 GetInventoryItemCount(FName ItemId) const;

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Inventory")
	bool AddInventoryItem(FName ItemId, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Inventory")
	bool ConsumeInventoryItem(FName ItemId, int32 Quantity = 1);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Crafting")
	bool CanCraftSignalResonator() const;

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Crafting")
	bool CraftSignalResonator();

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Narrative")
	void RecordArgosBriefing();

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Narrative")
	bool RecordSurfaceDeployment();

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Map")
	int32 RevealMapAroundLocationMeters(FVector2D LocationMeters, int32 RadiusCells = 1);

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Scanning")
	bool ScanCurrentEnvironment();

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Scanning")
	bool RecordCreatureScan(const FAstraeonCreatureProfile& CreatureProfile);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Session")
	bool HasStartedGame() const { return bHasStartedGame; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Objectives")
	EAstraeonObjectiveState GetObjectiveState() const { return ObjectiveState; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Objectives")
	FString GetObjectiveHint() const;

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Feedback")
	void SetLastFeedbackMessage(const FString& Message);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Feedback")
	FString GetLastFeedbackMessage() const { return LastFeedbackMessage; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Objectives")
	bool IsSignalResolved() const { return ObjectiveState == EAstraeonObjectiveState::Completed; }

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Objectives")
	bool TryResolveSignalSource();

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Persistence")
	bool SaveCurrentGame(const FString& SlotName = TEXT("AstraeonAutosave"), int32 UserIndex = 0) const;

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Persistence")
	bool LoadSavedGame(const FString& SlotName = TEXT("AstraeonAutosave"), int32 UserIndex = 0);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Persistence")
	UAstraeonSaveGame* CreateSaveSnapshot() const;

	static int32 NormalizeRequestedSeed(int32 RequestedWorldSeed);

private:
	void UpsertRuntimeLogbookEntry(const FAstraeonLogbookEntry& Entry);
	bool HasRuntimeLogbookEntry(FName EntryId) const;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Session")
	bool bHasStartedGame = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Session")
	int32 CurrentWorldSeed = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Session")
	FAstraeonEnvironmentalSnapshot CurrentEnvironment;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Session")
	FAstraeonRegionLayout CurrentRegionLayout;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Map")
	FAstraeonRevealedMap RevealedMap;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Inventory")
	TMap<FName, int32> Inventory;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Objectives")
	EAstraeonObjectiveState ObjectiveState = EAstraeonObjectiveState::MeasureEnvironment;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Feedback")
	FString LastFeedbackMessage;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Session")
	TArray<FAstraeonLogbookEntry> RuntimeLogbookEntries;
};
