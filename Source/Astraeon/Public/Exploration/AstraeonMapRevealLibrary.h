#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Exploration/AstraeonMapTypes.h"
#include "AstraeonMapRevealLibrary.generated.h"

UCLASS()
class ASTRAEON_API UAstraeonMapRevealLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Astraeon|Map")
	static FAstraeonMapCellId CellIdForLocationMeters(FVector2D LocationMeters, float CellSizeMeters = 50.0f);

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Map")
	static bool RevealCell(UPARAM(ref) FAstraeonRevealedMap& RevealedMap, FAstraeonMapCellId CellId);

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Map")
	static int32 RevealRadius(UPARAM(ref) FAstraeonRevealedMap& RevealedMap, FVector2D CenterMeters, int32 RadiusCells = 1);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Map")
	static bool IsCellRevealed(const FAstraeonRevealedMap& RevealedMap, FAstraeonMapCellId CellId);
};
