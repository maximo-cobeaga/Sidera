#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstraeonPlayerCharacter.generated.h"

class AAstraeonRegionMarker;
class UAstraeonGameInstance;
class UAstraeonSuitComponent;
class UCameraComponent;
struct FHitResult;

UCLASS()
class ASTRAEON_API AAstraeonPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAstraeonPlayerCharacter();

	void Interact();

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Survival")
	TObjectPtr<UAstraeonSuitComponent> SuitComponent;

	// Last actor location where the character was confirmed to be standing on solid,
	// walkable ground (see RescueFromVoidIfNeeded). Used as a safety net so a fall
	// through missing/late collision - for example right after a SURFACE HATCH
	// teleport - can always be recovered from instead of falling forever.
	FVector LastSafeGroundLocationCm = FVector::ZeroVector;
	bool bHasSafeGroundLocation = false;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void StartJump();
	void StopJump();
	void ScanEnvironment();
	void CraftSignalResonator();
	AAstraeonRegionMarker* FindFocusedRegionMarker() const;
	AAstraeonRegionMarker* FindNearestRegionMarkerInReach(float RadiusCm) const;
	bool InteractWithRegionMarker(AAstraeonRegionMarker& Marker, UAstraeonGameInstance& AstraeonGameInstance, bool& bOutSignalSourceAttempted);
	bool DeployToSurface(UAstraeonGameInstance& AstraeonGameInstance);
	bool TraceForDeploymentFloor(const FVector& DeploymentLocationCm, FHitResult& OutHit) const;
	void MarkLocationAsSafeGround(const FVector& LocationCm);
	void RescueFromVoidIfNeeded();
};
