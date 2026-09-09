#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WorldGen/AstraeonRegionTypes.h"
#include "WorldGen/AstraeonTerrainTraversal.h"
#include "AstraeonRegionMaterializer.generated.h"

UENUM(BlueprintType)
enum class EAstraeonRegionActorKind : uint8
{
	Resource UMETA(DisplayName = "Resource"),
	PointOfInterest UMETA(DisplayName = "Point Of Interest")
};

USTRUCT(BlueprintType)
struct ASTRAEON_API FAstraeonRegionActorSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	FName ActorId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	EAstraeonRegionActorKind Kind = EAstraeonRegionActorKind::Resource;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	FVector LocationCm = FVector::ZeroVector;

	// Se propaga desde FAstraeonResourceNode para que el marcador sepa que hace falta una
	// herramienta antes de tocarlo.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	FName RequiredToolId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	FVector Scale = FVector(1.0f, 1.0f, 1.0f);
};

UCLASS()
class ASTRAEON_API UAstraeonRegionMaterializer : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static TArray<FAstraeonRegionActorSpec> BuildActorSpecs(const FAstraeonRegionLayout& Layout);

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	// OriginCm reubica la estancia completa: Ítaca es la nave y cambia de sitio al aterrizar.
	static TArray<FAstraeonRegionActorSpec> BuildItacaActorSpecs(const FVector& OriginCm = FVector::ZeroVector);

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	// El punto al que baja la ESCOTILLA cuelga de la nave, no del origen del mundo: si
	// Ítaca aterriza en otro lado, desplegar debe dejarte junto a la nave y no a un
	// kilómetro de distancia.
	static FVector GetSurfaceDeploymentLocationCm(const FVector& ItacaOriginCm = FVector::ZeroVector);

	// Única fuente del contexto de superficie. La materialización y la validación de la
	// seed tienen que mirar exactamente el mismo terreno: si divergen, se valida una
	// región y se construye otra. No son UFUNCTION porque el contexto es un struct plano.
	static FAstraeonTerrainSurfaceContext BuildSurfaceContext(int32 TerrainSeed, const FVector& ItacaOriginCm,
		const FAstraeonRegionLayout& Layout);

	// Todo lo que el jugador tiene que poder alcanzar a pie desde la escotilla.
	static TArray<FAstraeonTraversalGoal> BuildTraversalGoals(const FAstraeonRegionLayout& Layout);
};
