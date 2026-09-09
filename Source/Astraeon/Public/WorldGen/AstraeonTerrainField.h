#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstraeonTerrainField.generated.h"

class UInstancedStaticMeshComponent;

// Contrato de consulta de la superficie final. Todas las coordenadas son centímetros
// Unreal y la seed es la seed efectiva ya normalizada por la sesión. El contexto contiene
// las correcciones locales que convierten el relieve base en una superficie jugable.
struct FAstraeonTerrainSurfaceContext
{
	FName RegionProfileId;
	int32 WorldSeed = 0;
	FVector2D CenterCm = FVector2D::ZeroVector;
	TArray<FVector2D> GroundFlatSpotsCm;
	TArray<FVector2D> MountainKeepOutCm;
	float ItacaPadHeightCm = 20.0f;
};

struct FAstraeonTerrainSurfaceSample
{
	float HeightCm = 0.0f;
	FVector Normal = FVector::UpVector;
	bool bIsValid = false;
};

// Relieve determinista por seed. La altura es una función pura de (seed, x, y): se puede
// consultar sin que el terreno exista todavía, que es lo que va a necesitar la generación
// de planetas para colocar cosas sobre el suelo antes de construirlo.
UCLASS()
class ASTRAEON_API AAstraeonTerrainField : public AActor
{
	GENERATED_BODY()

public:
	AAstraeonTerrainField();

	// GroundFlatSpotsCm aplana el suelo con una rampa suave, y está pensado sólo para
	// estructuras rígidas que no pueden seguir el relieve (Ítaca). MountainKeepOutCm impide
	// que una montaña nazca encima de cualquier punto jugable. Los marcadores sueltos no
	// necesitan ninguno de los dos: se apoyan sobre el terreno consultando su altura.
	// ItacaPadHeightCm aplana las baldosas que pisan la huella de la estancia. Sin eso los
	// bloques de relieve, que nacen en Z=0 y suben, atraviesan el piso de Ítaca y aparecen
	// como suelo rocoso dentro de la nave y como paredes a su alrededor.
	void BuildTerrain(int32 WorldSeed, const FVector2D& CenterCm, const TArray<FVector2D>& GroundFlatSpotsCm, const TArray<FVector2D>& MountainKeepOutCm, float ItacaPadHeightCm);

	// Altura total del terreno: suelo + montañas.
	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static float GetHeightCm(int32 WorldSeed, float XCm, float YCm);

	// Sólo la capa de suelo. Es la que debe ser siempre caminable; las montañas son
	// barreras deliberadas y no cumplen el límite de escalón.
	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static float GetGroundHeightCm(int32 WorldSeed, float XCm, float YCm);

	// Cota de la plataforma bajo Ítaca. Es la única fuente de verdad: el inicio de partida,
	// el aterrizaje y la generación de relieve tienen que coincidir o la estancia queda
	// enterrada o flotando.
	static float GetItacaPadHeightCm(int32 WorldSeed, float XCm, float YCm);

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static float GetMountainHeightCm(int32 WorldSeed, float XCm, float YCm);

	// Consulta de la superficie que materializará el prototipo continuo. Incluye claros,
	// exclusiones de montañas y la plataforma bajo Ítaca; no depende de actores del mundo.
	static FAstraeonTerrainSurfaceSample SampleSurface(const FAstraeonTerrainSurfaceContext& Context, const FVector2D& PointCm);
	static float GetFieldRadiusCm();

	// 0 en el centro de un claro, 1 fuera de su radio, con transición suave.
	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static float ComputeClearanceScale(const FVector2D& PointCm, const TArray<FVector2D>& FlatSpotsCm, float RadiusCm);

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static float GetMountainClearanceCm();

	// Altura del suelo ya nivelada por los llanos. La usan tanto la construcción del
	// terreno como los tests, para que no puedan divergir.
	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static float GetClearedGroundHeightCm(int32 WorldSeed, const FVector2D& PointCm, const TArray<FVector2D>& FlatSpotsCm);

	// Comprobación explícita de radio. Existe porque intentar deducirla de la rampa suave
	// dejó la exclusión de montañas sin efecto y una montaña tapó la salida de la nave.
	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static bool IsWithinAnySpot(const FVector2D& PointCm, const TArray<FVector2D>& SpotsCm, float RadiusCm);

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static float GetTileSizeCm();

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static float GetFlatSpotRadiusCm();

	// Escalón máximo que el personaje puede subir. El relieve debe respetarlo entre tiles
	// vecinos o las colinas se vuelven muros.
	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static float GetMaxWalkableStepCm();

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	int32 GetTileCount() const;

private:
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|WorldGen")
	TObjectPtr<UInstancedStaticMeshComponent> TerrainTiles;
};
