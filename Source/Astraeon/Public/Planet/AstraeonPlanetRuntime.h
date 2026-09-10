#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Planet/AstraeonPlanetDefinition.h"
#include "Planet/LOD/AstraeonPlanetLODManager.h"
#include "Planet/Patches/AstraeonPlanetPatchManager.h"
#include "Planet/Patches/AstraeonPlanetProceduralPatchBackend.h"
#include "Planet/Surface/AstraeonCubeSphereMesh.h"
#include "AstraeonPlanetRuntime.generated.h"
class UProceduralMeshComponent;
class UMaterialInterface;

// Phase 2: render through the quadtree patch manager; collision stays the Phase 1 near patch
// until P2.4, rebuilt on the finest patch grid so it matches the rendered triangles exactly.
// `-AstraeonPlanetLegacyFaces` restores the six fixed faces for comparison until P2.4 closes.
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
	// Off in LOD-only labs (TL_12): nothing stands on the surface, so no near collision is built.
	UPROPERTY(EditAnywhere, Category="Planet") bool bNearCollision = true;
	UFUNCTION(BlueprintCallable, Category="Planet") bool Rebuild();
	UFUNCTION(BlueprintPure, Category="Planet") FVector GetSurfacePointCm(FVector Direction, double AltitudeCm=0.0) const;
	static AAstraeonPlanetRuntime* FindActive(const UWorld* World);
	FAstraeonPlanetDefinition GetDefinition() const;
	bool PrepareCollision(const FVector& Direction, bool bForce=false);
	int32 GetCollisionTriangleCount() const { return CollisionTriangles; }
	// Cuantas veces se recreo la seccion de colision. Correlacionar esto con los cortes de
	// animacion es lo que separa "el clip esta mal" de "algo para al personaje".
	int32 GetCollisionRebuildCount() const { return CollisionRebuilds; }
	bool IsUsingPatches() const { return Patches.IsValid(); }
	const FAstraeonPlanetPatchManager* GetPatchManager() const { return Patches.Get(); }
	// Frames in which the patch under the player was not the finest one on screen: the ground
	// the player sees and the ground the player stands on differ there.
	int32 GetGroundMismatchFrames() const { return GroundMismatchFrames; }
	int32 GetGroundCheckedFrames() const { return GroundCheckedFrames; }
	int32 GetPatchComponentCount() const { return Backend.IsValid() ? Backend->GetComponentCount() : 0; }
	// Game-thread cost of selection + scheduling + uploads, per frame.
	static constexpr double PatchWorkBudgetMs = 2.0;
	double GetMaxPatchWorkMs() const { return MaxPatchWorkMs; }
	double GetMeanPatchWorkMs() const { return PatchWorkFrames>0 ? SumPatchWorkMs/PatchWorkFrames : 0.0; }
	int32 GetPatchWorkFramesOverBudget() const { return PatchWorkFramesOverBudget; }
	virtual void Tick(float DeltaSeconds) override;
protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	void UpdatePatches(float DeltaSeconds);
	FVector ObserverBodyCm() const;
	UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> Faces;
	// Dos secciones de colision que se alternan. Con una sola habia que moverla y recocer su
	// cuerpo fisico en el sitio, y el jugador se quedaba sin suelo mientras tanto.
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> NearCollision;
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> NearCollisionRelay;
	int32 ActiveCollisionBuffer = 0;
	TArray<FAstraeonCubeSphereMesh> FaceData;
	// Grid of FaceData. Equals FaceQuads with legacy faces; with patches it is the finest patch
	// grid, so collision triangles are the rendered ones. Radius and cadence stay on FaceQuads.
	int32 CollisionQuads = 0;
	FVector CollisionDirection = FVector::ZeroVector;
	int32 CollisionTriangles = 0;
	int32 CollisionRebuilds = 0;
	FAstraeonPlanetLODSettings LODSettings;
	TUniquePtr<FAstraeonPlanetStreamingManager> Streaming;
	TUniquePtr<FAstraeonPlanetProceduralPatchBackend> Backend;
	TUniquePtr<FAstraeonPlanetPatchManager> Patches;
	float SinceSelection = 0.f;
	FVector ObserverAtSelection = FVector::ZeroVector;
	bool bFacesRetired = false;
	int32 GroundMismatchFrames = 0;
	int32 GroundCheckedFrames = 0;
	double MaxPatchWorkMs = 0.0;
	double SumPatchWorkMs = 0.0;
	int32 PatchWorkFrames = 0;
	int32 PatchWorkFramesOverBudget = 0;
};
