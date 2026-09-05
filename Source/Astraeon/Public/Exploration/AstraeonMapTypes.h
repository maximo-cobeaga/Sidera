#pragma once

#include "CoreMinimal.h"
#include "AstraeonMapTypes.generated.h"

USTRUCT(BlueprintType)
struct ASTRAEON_API FAstraeonMapCellId
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Map")
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Map")
	int32 Y = 0;

	bool operator==(const FAstraeonMapCellId& Other) const
	{
		return X == Other.X && Y == Other.Y;
	}
};

USTRUCT(BlueprintType)
struct ASTRAEON_API FAstraeonRevealedMap
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Map")
	float CellSizeMeters = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Map")
	TArray<FAstraeonMapCellId> RevealedCells;
};

FORCEINLINE uint32 GetTypeHash(const FAstraeonMapCellId& CellId)
{
	return HashCombine(GetTypeHash(CellId.X), GetTypeHash(CellId.Y));
}
