#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Planet/AstraeonPlanetDefinition.h"
#include "Planet/Surface/AstraeonCubeSphereMesh.h"
#include "AstraeonPlanetRuntime.generated.h"
class UProceduralMeshComponent;
class UMaterialInterface;

// Phase 1 fixed-resolution proof. No quadtree, massive content or full-planet collision.
UCLASS()
class ASTRAEON_API AAstraeonPlanetRuntime : public AActor
{
	GENERATED_BODY()
public:
	AAstraeonPlanetRuntime();
	UPROPERTY(EditAnywhere, Category="Planet") double RadiusCm = 20000.0;
	UPROPERTY(EditAnywhere, Category="Planet") int32 BodySeed = 4242;
	UPROPERTY(EditAnywhere, Category="Planet") int32 FaceQuads = 32;
	UPROPERTY(EditAnywhere, Category="Planet") double GravityMS2 = 9.81;
	UPROPERTY(EditAnywhere, Category="Planet") TObjectPtr<UMaterialInterface> SurfaceMaterial;
	UFUNCTION(BlueprintCallable, Category="Planet") bool Rebuild();
	UFUNCTION(BlueprintPure, Category="Planet") FVector GetSurfacePointCm(FVector Direction, double AltitudeCm=0.0) const;
	static AAstraeonPlanetRuntime* FindActive(const UWorld* World);
	FAstraeonPlanetDefinition GetDefinition() const;
	bool PrepareCollision(const FVector& Direction, bool bForce=false);
	int32 GetCollisionTriangleCount() const { return CollisionTriangles; }
	// Cuantas veces se recreo la seccion de colision. Correlacionar esto con los cortes de
	// animacion es lo que separa "el clip esta mal" de "algo para al personaje".
	int32 GetCollisionRebuildCount() const { return CollisionRebuilds; }
	virtual void Tick(float DeltaSeconds) override;
protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
private:
	UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> Faces;
	// Dos secciones de colision que se alternan. Con una sola habia que moverla y recocer su
	// cuerpo fisico en el sitio, y el jugador se quedaba sin suelo mientras tanto.
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> NearCollision;
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> NearCollisionRelay;
	int32 ActiveCollisionBuffer = 0;
	TArray<FAstraeonCubeSphereMesh> FaceData;
	FVector CollisionDirection = FVector::ZeroVector;
	int32 CollisionTriangles = 0;
	int32 CollisionRebuilds = 0;
};
