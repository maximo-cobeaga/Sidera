#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstraeonPlanetStateSmoke.generated.h"

/**
 * Phase 2 gate, in a live world: "a defeated creature stays defeated after its patch unloads and
 * reloads". Hunt the nearest planetary creature through the weapon's own path, walk 5 km away
 * so its cell unloads, come back, save, wipe the state, load and restore the player: it must
 * still be gone. Then its nest clock runs out and it must come back.
 *
 * Uso: Astraeon.exe /Game/Maps/TL_13_CollisionRing -game -AstraeonSmokePlanetState -AstraeonPlanetFauna
 */
UCLASS()
class ASTRAEON_API AAstraeonPlanetStateSmoke : public AActor
{
	GENERATED_BODY()
public:
	AAstraeonPlanetStateSmoke();
	virtual void Tick(float DeltaSeconds) override;
private:
	enum class EStep : uint8 { Start, FindPrey, AwaitPrey, Hunt, GoAway, AwayCheck, ComeBack, BackCheck, SaveWipeLoad, LoadCheck, Repopulate, RepopulateCheck };
	void Teleport(const FVector& Direction);
	void Next(EStep Step) { Current = Step; StepSeconds = 0.f; }
	void Finish(bool bPassed, const FString& Reason);
	EStep Current = EStep::Start;
	float StepSeconds = 0.f;
	float Elapsed = 0.f;
	bool bDone = false;
	FName PreyId;
	FVector PreyDirection = FVector::ZeroVector;
	FVector SavedDirection = FVector::ZeroVector;
	int32 NeighboursSeenBack = 0;
};
