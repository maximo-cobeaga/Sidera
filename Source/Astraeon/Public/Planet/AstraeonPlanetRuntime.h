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

// Renders through the quadtree patch manager. Collision exists only in a ring of finest-level
// patches around the player, built on workers from the same builder as the rendered patches.
// The six fixed faces are only the image shown until the first patch cover is ready.
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
	// Where a new session puts the player. A direction, not a location: the radius is data.
	UPROPERTY(EditAnywhere, Category="Planet") FVector SpawnDirection = FVector(0, 0, 1);
	UFUNCTION(BlueprintCallable, Category="Planet") bool Rebuild();
	UFUNCTION(BlueprintPure, Category="Planet") FVector GetSurfacePointCm(FVector Direction, double AltitudeCm=0.0) const;
	static AAstraeonPlanetRuntime* FindActive(const UWorld* World);
	FAstraeonPlanetDefinition GetDefinition() const;
	// Builds the whole collision ring around `Direction` now, on the game thread. For teleports:
	// normal play builds ahead of the player on workers.
	bool PrepareCollision(const FVector& Direction, bool bForce=false);
	int32 GetCollisionTriangleCount() const { return CollisionTriangles; }
	// Collision patches built since play began. Correlating this with locomotion cuts is what
	// separated "the clip is wrong" from "something stops the character" in Phase 1.
	int32 GetCollisionRebuildCount() const { return CollisionRebuilds; }
	int32 GetCollisionPatchCount() const { return CollisionLive.Num(); }
	// Patches built on the game thread because the one under the player was not ready.
	// Expected at start and after teleports; anywhere else it means the ring is too small.
	int32 GetCollisionEmergencyBuilds() const { return CollisionEmergencyBuilds; }
	// Frames in which the patch under the player had no collision. Must stay zero.
	int32 GetCollisionMissingFrames() const { return CollisionMissingFrames; }
	bool IsUsingPatches() const { return Patches.IsValid(); }
	const FAstraeonPlanetPatchManager* GetPatchManager() const { return Patches.Get(); }
	// Frames in which the patch under the player was not the finest one on screen: the ground
	// the player sees and the ground the player stands on differ there.
	int32 GetGroundMismatchFrames() const { return GroundMismatchFrames; }
	int32 GetGroundCheckedFrames() const { return GroundCheckedFrames; }
	int32 GetPatchComponentCount() const { return Backend.IsValid() ? Backend->GetComponentCount() : 0; }
	// Game-thread cost of selection + scheduling + uploads + collision, per frame.
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
	using FAddress = FAstraeonPlanetPatchAddress;
	void UpdatePatches(float DeltaSeconds);
	void UpdateCollision(const FVector& PawnBodyCm);
	bool BuildCollisionNow(const FAddress& Address);
	void CommitCollision(const FAstraeonPlanetPatchBuildResult& Mesh);
	void RemoveCollision(const FAddress& Address);
	void ResetCollision();
	FAstraeonPlanetPatchBuildOptions CollisionOptions() const;
	FVector ObserverBodyCm() const;
	UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> Faces;
	TArray<FAstraeonCubeSphereMesh> FaceData;
	// Every collision component ever created; the live ones by address, the rest reusable.
	UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> CollisionComponents;
	TMap<FAddress, UProceduralMeshComponent*> CollisionLive;
	TArray<UProceduralMeshComponent*> CollisionFree;
	TMap<FAddress, uint64> CollisionPending;
	TUniquePtr<FAstraeonPlanetStreamingManager> CollisionStreaming;
	TArray<FAddress> CollisionWant;
	TArray<FAddress> CollisionKeep;
	FVector CollisionRingCenter = FVector::ZeroVector;
	FVector LastPawnDirection = FVector(0, 0, 1);
	int32 CollisionTriangles = 0;
	int32 CollisionRebuilds = 0;
	int32 CollisionEmergencyBuilds = 0;
	int32 CollisionMissingFrames = 0;
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
