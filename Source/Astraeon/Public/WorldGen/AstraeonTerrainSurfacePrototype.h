#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldGen/AstraeonTerrainField.h"
#include "AstraeonTerrainSurfacePrototype.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

// Actor aislado de T01. No reemplaza el campo actual de cubos: permite medir una superficie
// continua con la misma seed y el mismo contrato antes de integrarla al recorrido principal.
UCLASS()
class ASTRAEON_API AAstraeonTerrainSurfacePrototype : public AActor
{
	GENERATED_BODY()

public:
	AAstraeonTerrainSurfacePrototype();

	void BuildPrototype(const FAstraeonTerrainSurfaceContext& Context);
	static float GetPrototypeSpacingCm();
	static void BuildMeshData(const FAstraeonTerrainSurfaceContext& Context, float SpacingCm,
		TArray<FVector>& OutVertices, TArray<int32>& OutTriangles, TArray<FVector>& OutNormals,
		TArray<FVector2D>& OutUVs);

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	int32 GetVertexCount() const { return VertexCount; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	int32 GetTriangleCount() const { return TriangleCount; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|WorldGen")
	TObjectPtr<UProceduralMeshComponent> SurfaceMesh;

	UPROPERTY(VisibleAnywhere, Category = "Astraeon|WorldGen")
	int32 VertexCount = 0;

	UPROPERTY(VisibleAnywhere, Category = "Astraeon|WorldGen")
	int32 TriangleCount = 0;
};
