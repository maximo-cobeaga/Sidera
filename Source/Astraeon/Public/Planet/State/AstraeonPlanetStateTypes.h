#pragma once

#include "CoreMinimal.h"
#include "AstraeonPlanetStateTypes.generated.h"

// Values are part of the save format. Append only.
UENUM(BlueprintType)
enum class EAstraeonPlanetDeltaKind : uint8
{
	CreatureDefeated = 0,
};

// One persistent change to a planet. Indexed by where it happened on the body —direction,
// altitude and the fixed-level cell— never by a world transform or a rendered patch.
USTRUCT(BlueprintType)
struct ASTRAEON_API FAstraeonPlanetDelta
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Planet") FName EntityId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Planet") FName BodyId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Planet") EAstraeonPlanetDeltaKind Kind = EAstraeonPlanetDeltaKind::CreatureDefeated;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Planet") FVector Direction = FVector::ZeroVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Planet") double AltitudeCm = 0.0;
	// Fixed-level cell, stored so the spatial index can be rebuilt from a save alone.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Planet") int32 CellFace = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Planet") int32 CellLod = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Planet") int32 CellX = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Planet") int32 CellY = 0;
	// Game time until it lapses (a nest repopulates). Negative: permanent.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astraeon|Planet") float RemainingSeconds = -1.0f;
};
