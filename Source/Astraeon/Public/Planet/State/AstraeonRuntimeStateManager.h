#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Planet/AstraeonPlanetDefinition.h"
#include "Planet/Patches/AstraeonPlanetPatchAddress.h"
#include "Planet/State/AstraeonPlanetStateTypes.h"
#include "AstraeonRuntimeStateManager.generated.h"

// The mutable layer of every planet: what the player changed on top of the seed. The seed
// rebuilds the world; this rebuilds the difference. Streaming never owns state: a patch or a
// cell can unload and reload, and what happened there is read back from here.
UCLASS()
class ASTRAEON_API UAstraeonRuntimeStateManager : public UObject
{
	GENERATED_BODY()
public:
	// Returns false for an invalid planet, id or direction: nothing is recorded half-way.
	bool RecordCreatureDefeat(const FAstraeonPlanetDefinition& Planet, FName EntityId, const FVector& Direction,
		double AltitudeCm, float RespawnSeconds);
	bool IsDefeated(FName EntityId) const;
	const FAstraeonPlanetDelta* Find(FName EntityId) const;
	// Counts timed deltas down; returns the ids whose delta lapsed and was removed.
	TArray<FName> Advance(float DeltaSeconds);
	// Deltas of `BodyId` in the given fixed-level cells, in stable id order.
	TArray<FAstraeonPlanetDelta> GetDeltasInCells(FName BodyId, const TArray<FAstraeonPlanetPatchAddress>& Cells) const;
	const TArray<FAstraeonPlanetDelta>& GetDeltas() const { return Deltas; }
	void Restore(const TArray<FAstraeonPlanetDelta>& Saved);
	void Reset();
private:
	void RebuildIndex();
	UPROPERTY() TArray<FAstraeonPlanetDelta> Deltas;
	TMap<FName, int32> ById;
	TMap<FString, TArray<int32>> ByCell;
};
