#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstraeonPlanetPatchLODSmoke.generated.h"

class ACameraActor;

/**
 * TL_12: a scripted camera flies a fixed route over a 50 km planet with no collision.
 * Terrain-following over a cube corner, climb to 300 km, descent, terrain-following along a
 * cube edge, then a still hold. The pawn is frozen and hidden: nothing here walks.
 *
 * Fails on a hole or overlap on screen, a failed build, unbounded components, or a route that
 * does not settle once the camera stops. Everything else is measured and logged, not judged:
 * the thresholds come after the first measurement, not before.
 *
 * Uso: Astraeon.exe /Game/Maps/TL_12_PatchLOD -game -AstraeonSmokePatchLOD
 */
UCLASS()
class ASTRAEON_API AAstraeonPlanetPatchLODSmoke : public AActor
{
	GENERATED_BODY()
public:
	AAstraeonPlanetPatchLODSmoke();
	virtual void Tick(float DeltaSeconds) override;
private:
	struct FKey { float Seconds; FVector Direction; double AboveGroundCm; };
	void Place(float Seconds, float DeltaSeconds);
	void Audit();
	void Finish(bool bPassed, const FString& Reason);

	TArray<FKey> Route;
	UPROPERTY() TObjectPtr<ACameraActor> Camera;
	FVector LastForward = FVector::ZeroVector;
	FVector LastPosition = FVector::ZeroVector;
	float Elapsed = 0.f;
	float Startup = 0.f;
	bool bStarted = false;
	bool bFlying = false;
	bool bDone = false;
	int32 LastRelays = -1;
	int32 Audits = 0;
	int32 MaxVisible = 0;
	int32 MaxComponents = 0;
	int32 MaxDelta = 0;
	int32 CurrentDelta = 0;
	int32 FramesDeltaTwo = 0;
	int32 Frames = 0;
	int32 LowFrames = 0;
	int32 LowFramesFinest = 0;
	int32 WorstLowLod = MAX_int32;
	double MaxSpeedCmS = 0.0;
	int32 NextShot = 0;
};
