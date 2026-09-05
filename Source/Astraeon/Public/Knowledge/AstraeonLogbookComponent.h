#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Knowledge/AstraeonLogbookTypes.h"
#include "AstraeonLogbookComponent.generated.h"

UCLASS(ClassGroup = (Astraeon), meta = (BlueprintSpawnableComponent))
class ASTRAEON_API UAstraeonLogbookComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Knowledge")
	void UpsertEntry(const FAstraeonLogbookEntry& Entry);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Knowledge")
	bool HasEntry(FName EntryId) const;

	UFUNCTION(BlueprintPure, Category = "Astraeon|Knowledge")
	const TArray<FAstraeonLogbookEntry>& GetEntries() const { return Entries; }

private:
	UPROPERTY(SaveGame)
	TArray<FAstraeonLogbookEntry> Entries;
};
