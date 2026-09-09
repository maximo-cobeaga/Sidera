#pragma once

#include "CoreMinimal.h"
#include "AstraeonBuildingTypes.generated.h"

UENUM(BlueprintType)
enum class EAstraeonStructureType : uint8
{
	Wall UMETA(DisplayName = "Muro"),
	Floor UMETA(DisplayName = "Plataforma"),
	Pillar UMETA(DisplayName = "Pilar")
};

// Una construcción colocada. Se guarda como dato, no como actor: la región se remateraliza
// cada vez que Ítaca aterriza, así que lo construido tiene que poder reconstruirse desde el
// SaveGame en vez de depender de que el actor sobreviva.
USTRUCT(BlueprintType)
struct ASTRAEON_API FAstraeonPlacedStructure
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Building")
	EAstraeonStructureType Type = EAstraeonStructureType::Wall;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Building")
	FVector LocationCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Building")
	FRotator Rotation = FRotator::ZeroRotator;
};
