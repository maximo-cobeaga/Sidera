#include "Tests/AstraeonPlanetStateSmoke.h"
#include "AstraeonGameInstance.h"
#include "AstraeonPlayerCharacter.h"
#include "AstraeonPlayerController.h"
#include "Creatures/AstraeonCreatureActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Planet/AstraeonPlanetRuntime.h"
#include "Planet/Gravity/AstraeonPlanetGravityComponent.h"
#include "Planet/State/AstraeonPlanetEntities.h"
#include "Planet/State/AstraeonRuntimeStateManager.h"

namespace
{
	const TCHAR* SlotName = TEXT("AstraeonPlanetStateSmoke");
	constexpr double AwayCm = 500000.0; // 5 km: far outside the 450 m keep ring.
}

AAstraeonPlanetStateSmoke::AAstraeonPlanetStateSmoke()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
}

void AAstraeonPlanetStateSmoke::Teleport(const FVector& Direction)
{
	auto* PC = GetWorld()->GetFirstPlayerController();
	auto* Character = PC ? Cast<AAstraeonPlayerCharacter>(PC->GetPawn()) : nullptr;
	auto* Planet = AAstraeonPlanetRuntime::FindActive(GetWorld());
	if (!Character || !Planet) return;
	Planet->PrepareCollision(Direction, true);
	Character->SetActorLocation(Planet->GetSurfacePointCm(Direction, 150.0), false, nullptr, ETeleportType::TeleportPhysics);
	Character->GetCharacterMovement()->StopMovementImmediately();
	Character->FindComponentByClass<UAstraeonPlanetGravityComponent>()->SetPlanetBody(Planet->GetActorLocation(), Planet->RadiusCm, float(Planet->GravityMS2));
	Planet->RefreshEntitiesNow();
}

void AAstraeonPlanetStateSmoke::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDone) return;
	Elapsed += DeltaSeconds; StepSeconds += DeltaSeconds;
	auto* PC = Cast<AAstraeonPlayerController>(GetWorld()->GetFirstPlayerController());
	auto* Character = PC ? Cast<AAstraeonPlayerCharacter>(PC->GetPawn()) : nullptr;
	auto* Planet = AAstraeonPlanetRuntime::FindActive(GetWorld());
	auto* Game = GetGameInstance<UAstraeonGameInstance>();
	if (!PC || !Character || !Planet || !Game)
	{
		if (Elapsed > 10.f) Finish(false, TEXT("Missing player, runtime or session"));
		return;
	}
	if (Elapsed > 120.f) { Finish(false, FString::Printf(TEXT("Timed out in step %d"), int32(Current))); return; }
	const auto Definition = Planet->GetDefinition();
	const FVector PlayerDirection = (Character->GetActorLocation() - Planet->GetActorLocation()).GetSafeNormal();

	switch (Current)
	{
	case EStep::Start:
		if (!Planet->bSpawnFauna) { Finish(false, TEXT("Fauna is off: run with -AstraeonPlanetFauna")); return; }
		PC->StartSelectedNewGame();
		Next(EStep::FindPrey);
		return;
	case EStep::FindPrey:
	{
		// The nearest creature the seed places within 2 km; the player goes to it.
		TArray<FAstraeonPlanetPatchAddress> Cells;
		FAstraeonPlanetEntities::CellsNear(Definition, PlayerDirection, 200000.0, Cells);
		double Best = TNumericLimits<double>::Max();
		TArray<FAstraeonPlanetEntitySpawn> Spawns;
		for (const auto& Cell : Cells)
		{
			FAstraeonPlanetEntities::CreaturesInCell(Definition, Cell, Spawns);
			for (const auto& Spawn : Spawns)
			{
				const double Arc = FMath::Acos(FMath::Clamp(FVector::DotProduct(Spawn.Direction, PlayerDirection), -1.0, 1.0));
				if (Arc < Best) { Best = Arc; PreyId = Spawn.EntityId; PreyDirection = Spawn.Direction; }
			}
		}
		if (PreyId.IsNone()) { Finish(false, TEXT("No creature within 2 km of the spawn")); return; }
		// Land 30 m from it, outside its alert radius, so it grazes instead of charging.
		const FVector Tangent = FVector::CrossProduct(PreyDirection, FVector(0, 0, 1)).GetSafeNormal();
		Teleport((PreyDirection + Tangent * (3000.0 / Planet->RadiusCm)).GetSafeNormal());
		UE_LOG(LogTemp, Display, TEXT("PlanetState: prey %s at %.0f m from the spawn"), *PreyId.ToString(), Best * Planet->RadiusCm / 100.0);
		Next(EStep::AwaitPrey);
		return;
	}
	case EStep::AwaitPrey:
		if (Planet->FindEntityActor(PreyId)) { Next(EStep::Hunt); return; }
		if (StepSeconds > 5.f) Finish(false, TEXT("The streamer never materialized the prey"));
		return;
	case EStep::Hunt:
	{
		// The weapon's own path: damage until dead, the kill's harvest, then the defeat record.
		AAstraeonCreatureActor* Prey = Planet->FindEntityActor(PreyId);
		if (!Prey) { Finish(false, TEXT("The prey vanished before the hunt")); return; }
		while (!Prey->IsDead()) Prey->ApplyWeaponDamage(1000.f);
		Game->RecordCreatureKill(Prey->GetCreatureProfile());
		if (!Game->RecordCreatureDefeat(Prey)) { Finish(false, TEXT("The defeat was not recorded")); return; }
		Prey->BeginDeathSequence();
		if (!Game->GetPlanetState()->IsDefeated(PreyId)) { Finish(false, TEXT("State does not hold the defeat")); return; }
		UE_LOG(LogTemp, Display, TEXT("PlanetState: %s defeated; nest clock %.0f s"), *PreyId.ToString(), Game->GetPlanetState()->Find(PreyId)->RemainingSeconds);
		Next(EStep::GoAway);
		return;
	}
	case EStep::GoAway:
		if (StepSeconds < 1.f) return; // Let the corpse fall first.
		{
			const FVector Tangent = FVector::CrossProduct(PreyDirection, FVector(1, 0, 0)).GetSafeNormal();
			Teleport((PreyDirection + Tangent * (AwayCm / Planet->RadiusCm)).GetSafeNormal());
		}
		Next(EStep::AwayCheck);
		return;
	case EStep::AwayCheck:
		if (StepSeconds < 1.5f) return;
		if (Planet->FindEntityActor(PreyId)) { Finish(false, TEXT("5 km away the prey's cell is still loaded")); return; }
		Next(EStep::ComeBack);
		return;
	case EStep::ComeBack:
		Teleport((PreyDirection + FVector::CrossProduct(PreyDirection, FVector(0, 0, 1)).GetSafeNormal() * (3000.0 / Planet->RadiusCm)).GetSafeNormal());
		Next(EStep::BackCheck);
		return;
	case EStep::BackCheck:
		if (StepSeconds < 1.5f) return;
		if (Planet->FindEntityActor(PreyId)) { Finish(false, TEXT("The cell reloaded and the prey came back to life")); return; }
		NeighboursSeenBack = Planet->GetEntityActorCount();
		UE_LOG(LogTemp, Display, TEXT("PlanetState: back at the kill site, prey absent, %d other creatures streamed in"), NeighboursSeenBack);
		Next(EStep::SaveWipeLoad);
		return;
	case EStep::SaveWipeLoad:
	{
		SavedDirection = PlayerDirection;
		if (!Game->SaveCurrentGame(SlotName, 0)) { Finish(false, TEXT("Planet session did not save")); return; }
		// Forget everything, go elsewhere: the load alone must bring both back.
		Game->GetPlanetState()->Reset();
		if (Game->GetPlanetState()->IsDefeated(PreyId)) { Finish(false, TEXT("Wipe did not wipe")); return; }
		Teleport((PreyDirection + FVector::CrossProduct(PreyDirection, FVector(1, 0, 0)).GetSafeNormal() * (AwayCm / Planet->RadiusCm)).GetSafeNormal());
		if (!Game->LoadSavedGame(SlotName, 0) || !PC->RestorePlanetLocation()) { Finish(false, TEXT("Planet session did not load onto this body")); return; }
		Next(EStep::LoadCheck);
		return;
	}
	case EStep::LoadCheck:
	{
		if (StepSeconds < 1.5f) return;
		const double OffCm = FMath::Acos(FMath::Clamp(FVector::DotProduct(PlayerDirection, SavedDirection), -1.0, 1.0)) * Planet->RadiusCm;
		UE_LOG(LogTemp, Display, TEXT("PlanetState: after load, player %.1f cm from the saved place, prey defeated=%d, creatures around=%d"),
			OffCm, Game->GetPlanetState()->IsDefeated(PreyId), Planet->GetEntityActorCount());
		if (OffCm > 200.0) { Finish(false, FString::Printf(TEXT("Loaded %.0f cm away from the saved place"), OffCm)); return; }
		if (!Game->GetPlanetState()->IsDefeated(PreyId) || Planet->FindEntityActor(PreyId)) { Finish(false, TEXT("The save lost the defeat")); return; }
		Next(EStep::Repopulate);
		return;
	}
	case EStep::Repopulate:
		// The nest clock is game design, not a leak: once it lapses the grazer returns.
		Game->AdvanceCreatureRespawns(300.f);
		Planet->RefreshEntitiesNow();
		Next(EStep::RepopulateCheck);
		return;
	case EStep::RepopulateCheck:
		if (StepSeconds < 1.f) return;
		if (!Planet->FindEntityActor(PreyId)) { Finish(false, TEXT("The nest never repopulated")); return; }
		UGameplayStatics::DeleteGameInSlot(SlotName, 0);
		Finish(true, FString::Printf(TEXT("OK prey=%s spawns=%d despawns=%d"), *PreyId.ToString(), Planet->GetEntitySpawnCount(), Planet->GetEntityDespawnCount()));
		return;
	}
}

void AAstraeonPlanetStateSmoke::Finish(bool bPassed, const FString& Reason)
{
	bDone = true;
	UE_LOG(LogTemp, Display, TEXT("PlanetState: RESULTADO=%s %s"), bPassed ? TEXT("OK") : TEXT("FALLO"), *Reason);
	FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
}
