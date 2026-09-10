#pragma once

#include "CoreMinimal.h"
#include "WorldGen/AstraeonEnvironmentTypes.h"
#include "WorldGen/AstraeonRegionTypes.h"
#include "AstraeonWorldProfileTypes.generated.h"

UENUM(BlueprintType)
enum class EAstraeonBiomeId : uint8
{
	RockyDesert UMETA(DisplayName = "Rocky Desert")
};

// Seeds para contenido que puede variar dentro de una región diseñada. Ninguna controla
// geografía, landing zone, POI narrativo ni camino crítico.
USTRUCT(BlueprintType)
struct ASTRAEON_API FAstraeonSecondaryContentSeeds
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	int32 EnvironmentSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	int32 ResourceSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	int32 FaunaSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	int32 WeatherSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	int32 EncounterSeed = 0;
};

USTRUCT(BlueprintType)
struct ASTRAEON_API FAstraeonPlanetProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	FName PlanetProfileId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	FAstraeonEnvironmentalSnapshot Environment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	TArray<EAstraeonBiomeId> Biomes;

	// The body itself (Phase 3). The radius is data, never a Scale; the relief seed belongs to
	// the planet, not to a session: every session walks the same Khepri.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	double RadiusCm = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	int32 BodySeed = 0;
};

USTRUCT(BlueprintType)
struct ASTRAEON_API FAstraeonRegionProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	FName RegionProfileId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	FName PlanetProfileId;

	// Región cuadrada: 250 m a cada lado del origen, equivalente a 500 × 500 m.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	float HalfExtentMeters = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	EAstraeonBiomeId Biome = EAstraeonBiomeId::RockyDesert;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	FVector2D LandingZoneMeters = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	FAstraeonSecondaryContentSeeds SecondaryContentSeeds;

	// Ejes authored de los corredores. La futura superficie y PCG los consumen para preservar
	// tránsito: nunca se reconstruyen a partir de la seed de contenido.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	TArray<FVector2D> DirectRouteWaypointsMeters;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	TArray<FVector2D> SafeRouteWaypointsMeters;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	float CriticalCorridorWidthMeters = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	float LandingClearRadiusMeters = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	float SignalClearRadiusMeters = 15.0f;

	// Recursos y POIs principales: coordenadas diseñadas, no resultados de una seed.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	TArray<FAstraeonResourceNode> FixedResources;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldProfile")
	TArray<FAstraeonPointOfInterest> FixedPointsOfInterest;
};
