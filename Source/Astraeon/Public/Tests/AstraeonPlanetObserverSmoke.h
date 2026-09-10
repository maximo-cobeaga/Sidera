#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstraeonPlanetObserverSmoke.generated.h"

/**
 * Regression for the owner's report of 2026-09-10: TL_12 opened with Play fell through the
 * planet at once, because a collisionless planet left the player walking on nothing.
 * Menu 3 s, then the session: forward, up, and a long dive that must stop above the ground.
 *
 * Uso: Astraeon.exe /Game/Maps/TL_12_PatchLOD -game -AstraeonSmokePlanetObserver
 */
UCLASS()
class ASTRAEON_API AAstraeonPlanetObserverSmoke : public AActor
{
	GENERATED_BODY()
public:
	AAstraeonPlanetObserverSmoke();
	virtual void Tick(float DeltaSeconds) override;
private:
	void Finish(bool bPassed, const FString& Reason);
	float Elapsed = 0.f;
	bool bStarted = false;
	bool bDone = false;
	double LowestAboveGroundCm = TNumericLimits<double>::Max();
	FVector ForwardStart = FVector::ZeroVector;
	double ForwardDistanceCm = 0.0;
	double ClimbStartCm = 0.0;
	double ClimbedCm = 0.0;
};
