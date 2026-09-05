#pragma once

#include "CoreMinimal.h"
#include "AstraeonRegionTypes.generated.h"

UENUM(BlueprintType)
enum class EAstraeonPointOfInterestType : uint8
{
	SignalSource UMETA(DisplayName = "Signal Source"),
	MinorAnomaly UMETA(DisplayName = "Minor Anomaly"),
	CreatureSpawn UMETA(DisplayName = "Creature Spawn")
};

USTRUCT(BlueprintType)
struct ASTRAEON_API FAstraeonResourceNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	FName ResourceId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	FVector2D LocationMeters = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	bool bSeedSignature = false;
};

USTRUCT(BlueprintType)
struct ASTRAEON_API FAstraeonPointOfInterest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	FName PointId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	EAstraeonPointOfInterestType Type = EAstraeonPointOfInterestType::MinorAnomaly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	FVector2D LocationMeters = FVector2D::ZeroVector;
};

USTRUCT(BlueprintType)
struct ASTRAEON_API FAstraeonRegionLayout
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	int32 WorldSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	int32 GeneratorVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	float RegionRadiusMeters = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	TArray<FAstraeonResourceNode> Resources;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	TArray<FAstraeonPointOfInterest> PointsOfInterest;
};
