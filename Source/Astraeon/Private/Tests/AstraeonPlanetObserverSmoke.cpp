#include "Tests/AstraeonPlanetObserverSmoke.h"
#include "AstraeonPlayerCharacter.h"
#include "AstraeonPlayerController.h"
#include "Engine/World.h"
#include "HAL/PlatformMisc.h"
#include "Planet/AstraeonPlanetRuntime.h"
#include "Planet/Gravity/AstraeonPlanetGravityComponent.h"

namespace
{
	constexpr float MenuSeconds = 3.f, ForwardEnd = 8.f, ClimbEnd = 10.f, DiveEnd = 20.f;
	constexpr double FloorToleranceCm = 150.0; // The observer floor is 200 cm; allow one frame of dive.
}

AAstraeonPlanetObserverSmoke::AAstraeonPlanetObserverSmoke()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
}

void AAstraeonPlanetObserverSmoke::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDone) return;
	Elapsed += DeltaSeconds;
	auto* PC = Cast<AAstraeonPlayerController>(GetWorld()->GetFirstPlayerController());
	auto* Character = PC ? Cast<AAstraeonPlayerCharacter>(PC->GetPawn()) : nullptr;
	const auto* Planet = AAstraeonPlanetRuntime::FindActive(GetWorld());
	auto* Gravity = Character ? Character->FindComponentByClass<UAstraeonPlanetGravityComponent>() : nullptr;
	if (!Character || !Planet || !Gravity)
	{
		if (Elapsed > 10.f) Finish(false, TEXT("Missing player, runtime or gravity"));
		return;
	}
	if (Planet->bNearCollision) { Finish(false, TEXT("This map has near collision; the observer is for TL_12")); return; }

	const FVector Center = Planet->GetActorLocation();
	const FVector Up = (Character->GetActorLocation() - Center).GetSafeNormal();
	const double AboveGround = FVector::Dist(Character->GetActorLocation(), Center)
		- FVector::Dist(Planet->GetSurfacePointCm(Up, 0.0), Center);
	if (Elapsed > 1.f)
	{
		if (!Character->IsObserverFlight()) { Finish(false, TEXT("Observer flight never engaged")); return; }
		LowestAboveGroundCm = FMath::Min(LowestAboveGroundCm, AboveGround);
		if (AboveGround < FloorToleranceCm)
		{
			Finish(false, FString::Printf(TEXT("Fell to %.0f cm above ground at t=%.2f s"), AboveGround, Elapsed));
			return;
		}
	}
	if (Elapsed < MenuSeconds) return; // The menu phase is exactly what failed for the owner.
	if (!bStarted) { PC->StartSelectedNewGame(); bStarted = true; ForwardStart = Character->GetActorLocation(); return; }

	if (Elapsed < ForwardEnd)
	{
		Character->AddMovementInput(Gravity->GetViewDirection(), 1.f);
		ForwardDistanceCm = FVector::Dist(Character->GetActorLocation(), ForwardStart);
		ClimbStartCm = AboveGround;
	}
	else if (Elapsed < ClimbEnd)
	{
		Character->AddMovementInput(Gravity->GetUpVector(), 1.f);
		ClimbedCm = AboveGround - ClimbStartCm;
	}
	else if (Elapsed < DiveEnd)
	{
		Character->AddMovementInput(-Gravity->GetUpVector(), 1.f);
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("PlanetObserver: avance=%.0f m subida=%.0f m altura final=%.0f cm minima=%.0f cm"),
			ForwardDistanceCm / 100.0, ClimbedCm / 100.0, AboveGround, LowestAboveGroundCm);
		FString Failures;
		if (ForwardDistanceCm < 5000.0) Failures += TEXT("apenas avanzo; ");
		if (ClimbedCm < 2000.0) Failures += TEXT("no subio; ");
		if (AboveGround > 400.0) Failures += TEXT("el descenso no llego al piso de 2 m; ");
		Finish(Failures.IsEmpty(), Failures.IsEmpty() ? TEXT("OK") : Failures);
	}
}

void AAstraeonPlanetObserverSmoke::Finish(bool bPassed, const FString& Reason)
{
	bDone = true;
	UE_LOG(LogTemp, Display, TEXT("PlanetObserver: RESULTADO=%s %s"), bPassed ? TEXT("OK") : TEXT("FALLO"), *Reason);
	FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
}
