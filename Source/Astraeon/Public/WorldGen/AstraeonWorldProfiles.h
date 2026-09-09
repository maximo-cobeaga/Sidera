#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WorldGen/AstraeonWorldProfileTypes.h"
#include "AstraeonWorldProfiles.generated.h"

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
};
