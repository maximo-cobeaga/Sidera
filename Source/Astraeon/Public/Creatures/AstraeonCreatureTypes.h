#pragma once

#include "CoreMinimal.h"
#include "AstraeonCreatureTypes.generated.h"

UENUM(BlueprintType)
enum class EAstraeonCreatureAwarenessState : uint8
{
	Patrolling UMETA(DisplayName = "Patrolling"),
	Alert UMETA(DisplayName = "Alert"),
	Threatening UMETA(DisplayName = "Threatening"),
	Disengaging UMETA(DisplayName = "Disengaging")
};

USTRUCT(BlueprintType)
struct ASTRAEON_API FAstraeonCreatureProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Creatures")
	FName SpeciesId = TEXT("umbra_grazer");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Creatures")
	FText DisplayName = FText::FromString(TEXT("Umbra Grazer"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Creatures")
	float AlertRadiusMeters = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Creatures")
	float ThreatRadiusMeters = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Creatures")
	float PatrolRadiusMeters = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Creatures")
	float ThreatDamagePercentPerSecond = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Creatures")
	float MaxHealth = 100.0f;

	// Lo que deja al morir. Matar tiene que rendir algo o no compite con esquivar.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Creatures")
	FName HarvestItemId = TEXT("biomass_sample");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Creatures")
	int32 HarvestQuantity = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Creatures")
	FText ScannerSummary = FText::FromString(TEXT("A cautious surface organism. It reacts to proximity before direct contact."));
};
