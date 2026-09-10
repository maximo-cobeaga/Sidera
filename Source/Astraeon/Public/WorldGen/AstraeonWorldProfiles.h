#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WorldGen/AstraeonWorldProfileTypes.h"
#include "Planet/AstraeonPlanetDefinition.h"
#include "AstraeonWorldProfiles.generated.h"

struct FAstraeonPlanetRegionSurface;

// Registro C++ textual de perfiles del MVP. Es deliberadamente pequeño: sustituye datos
// inventados por seed sin introducir DataAssets binarios antes de estabilizar Region A.
UCLASS()
class ASTRAEON_API UAstraeonWorldProfiles : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static FName GetMvpPlanetProfileId();
	static FName GetRegionAProfileId();
	static FAstraeonPlanetProfile GetMvpPlanetProfile();
	static FAstraeonRegionProfile GetRegionAProfile();
	static FAstraeonRegionLayout BuildFixedRegionLayout(int32 ContentSeed);
	static bool TryGetRegionProfile(FName RegionProfileId, FAstraeonRegionProfile& OutProfile);
	static bool IsPointInsideRegion(const FAstraeonRegionProfile& Profile, const FVector2D& PointMeters);

	// Phase 3: Khepri as a body, and Region A as a place on it. False for a profile without a body.
	static bool TryGetPlanetDefinition(FName PlanetProfileId, FAstraeonPlanetDefinition& OutDefinition);
	// Region A is fixed by design, so its place is too: resolved once from the region's own id
	// against its plan, never from a session's content seed. Deterministic and traversable.
	static bool ResolvePlanetRegion(FName RegionProfileId, FAstraeonPlanetRegionSurface& OutRegion);
};
