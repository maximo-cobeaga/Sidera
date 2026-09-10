#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AstraeonGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Persistence/AstraeonSaveGame.h"
#include "Persistence/AstraeonSaveMigration.h"
#include "Planet/State/AstraeonPlanetEntities.h"
#include "Planet/State/AstraeonRuntimeStateManager.h"
#include "Planet/Surface/AstraeonPlanetSurface.h"
#include "WorldGen/AstraeonWorldProfiles.h"

namespace AstraeonPlanetStateTests
{
	FAstraeonPlanetDefinition Planet(double Radius = 5000000.0, int32 Seed = 4242)
	{
		FAstraeonPlanetDefinition P;
		P.BodyId = TEXT("state_lab"); P.RadiusCm = Radius; P.MassKg = 1.e16; P.SurfaceGravityMS2 = 9.81;
		P.WorldSeed = Seed; P.BodySeed = Seed;
		return P;
	}
	// All creatures the seed places within `RadiusCm` of `Direction`.
	TArray<FAstraeonPlanetEntitySpawn> Near(const FAstraeonPlanetDefinition& P, const FVector& Direction, double RadiusCm)
	{
		TArray<FAstraeonPlanetPatchAddress> Cells;
		FAstraeonPlanetEntities::CellsNear(P, Direction, RadiusCm, Cells);
		TArray<FAstraeonPlanetEntitySpawn> All, InCell;
		for (const auto& Cell : Cells) { FAstraeonPlanetEntities::CreaturesInCell(P, Cell, InCell); All.Append(InCell); }
		return All;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPlanetEntitiesTest, "Astraeon.Planet.Entities.DeterministicPlacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonPlanetEntitiesTest::RunTest(const FString& Parameters)
{
	using namespace AstraeonPlanetStateTests;
	for (double Radius : {20000.0, 5000000.0, 50000000.0})
	{
		const auto P = Planet(Radius);
		const uint8 Lod = FAstraeonPlanetEntities::CellLod(P);
		TestTrue(TEXT("Cells are at most 1 km wide"), 0.5 * PI * Radius / double(int64(1) << Lod) <= FAstraeonPlanetEntities::CellSpanCm);
		const FVector Here = FVector(0.3, -0.5, 0.8).GetSafeNormal();
		// A 5 km cap: dozens of cells, and the whole planet on the 200 m bench.
		const double Cap = FMath::Min(Radius, 500000.0);
		const auto First = Near(P, Here, Cap);
		// Loading other cells in between must not change what this area holds.
		Near(P, -Here, Cap);
		const auto Again = Near(P, Here, Cap);
		if (!TestEqual(TEXT("Same count on reload"), Again.Num(), First.Num())) return false;
		TSet<FName> Ids;
		for (int32 I = 0; I < First.Num(); ++I)
		{
			TestTrue(TEXT("Same id on reload"), First[I].EntityId == Again[I].EntityId);
			TestTrue(TEXT("Same place on reload"), First[I].Direction == Again[I].Direction);
			FAstraeonPlanetPatchAddress Cell;
			FAstraeonPlanetEntities::CellAt(P, First[I].Direction, Cell);
			TestTrue(TEXT("An entity lies inside the cell that owns it"), Cell == First[I].Cell);
			TestTrue(TEXT("No creature on a mountain"), FAstraeonPlanetSurface::SampleMountainHeightCm(P, First[I].Direction) <= FAstraeonPlanetEntities::MaxSpawnMountainCm);
			bool bAlreadySeen = false;
			Ids.Add(First[I].EntityId, &bAlreadySeen);
			TestFalse(TEXT("Ids are unique"), bAlreadySeen);
		}
		TestTrue(FString::Printf(TEXT("The area is not empty (radius %.0f)"), Radius), First.Num() > 0);
		// Another planet seed is another fauna.
		const auto Other = Near(Planet(Radius, 777), Here, Cap);
		bool bDiffers = Other.Num() != First.Num();
		for (int32 I = 0; !bDiffers && I < Other.Num(); ++I) bDiffers = !(Other[I].Direction == First[I].Direction);
		TestTrue(TEXT("A different seed places different creatures"), bDiffers);
		AddInfo(FString::Printf(TEXT("PlanetEntities radius_cm=%.0f cell_lod=%d creatures=%d"), Radius, Lod, First.Num()));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPlanetStateTest, "Astraeon.Planet.State.DefeatSurvivesUnloadReloadAndSave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonPlanetStateTest::RunTest(const FString& Parameters)
{
	using namespace AstraeonPlanetStateTests;
	const auto P = Planet();
	const auto Creatures = Near(P, FVector(1, 1, 1).GetSafeNormal(), 500000.0);
	if (!TestTrue(TEXT("There is a creature to hunt"), Creatures.Num() > 1)) return false;
	const FAstraeonPlanetEntitySpawn Prey = Creatures[0];
	UAstraeonRuntimeStateManager* State = NewObject<UAstraeonRuntimeStateManager>();
	TestFalse(TEXT("A nameless entity is not recorded"), State->RecordCreatureDefeat(P, NAME_None, Prey.Direction, 0.0, 240.f));
	TestFalse(TEXT("A place-less entity is not recorded"), State->RecordCreatureDefeat(P, Prey.EntityId, FVector::ZeroVector, 0.0, 240.f));
	TestTrue(TEXT("The defeat is recorded"), State->RecordCreatureDefeat(P, Prey.EntityId, Prey.Direction, 0.0, 240.f));
	TestTrue(TEXT("It is defeated"), State->IsDefeated(Prey.EntityId));
	TestFalse(TEXT("Its neighbour is not"), State->IsDefeated(Creatures[1].EntityId));
	TestEqual(TEXT("Its cell holds exactly its delta"), State->GetDeltasInCells(P.BodyId, {Prey.Cell}).Num(), 1);

	// Streaming: what a cell materializes is the seed minus the deltas, however often it reloads.
	const auto Materialize = [&]()
	{
		TArray<FAstraeonPlanetEntitySpawn> Spawns;
		FAstraeonPlanetEntities::CreaturesInCell(P, Prey.Cell, Spawns);
		return Spawns.ContainsByPredicate([&](const FAstraeonPlanetEntitySpawn& S) { return S.EntityId == Prey.EntityId && !State->IsDefeated(S.EntityId); });
	};
	for (int32 Reload = 0; Reload < 3; ++Reload) TestFalse(TEXT("Unload and reload do not revive it"), Materialize());

	State->Advance(100.f);
	TestTrue(TEXT("Still defeated before its nest clock runs out"), State->IsDefeated(Prey.EntityId));

	// Save and load through the real save object, serialized to bytes and back.
	UAstraeonSaveGame* Save = NewObject<UAstraeonSaveGame>();
	Save->SaveGameVersion = FAstraeonSaveMigration::CurrentVersion;
	Save->PlanetDeltas = State->GetDeltas();
	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("Save serializes"), UGameplayStatics::SaveGameToMemory(Save, Bytes))) return false;
	const UAstraeonSaveGame* Loaded = Cast<UAstraeonSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("Save deserializes"), Loaded)) return false;
	UAstraeonRuntimeStateManager* Restored = NewObject<UAstraeonRuntimeStateManager>();
	Restored->Restore(Loaded->PlanetDeltas);
	TestTrue(TEXT("Defeated after save and load"), Restored->IsDefeated(Prey.EntityId));
	TestTrue(TEXT("The spatial index is rebuilt from the save alone"), Restored->GetDeltasInCells(P.BodyId, {Prey.Cell}).Num() == 1);
	if (const FAstraeonPlanetDelta* Delta = Restored->Find(Prey.EntityId))
	{
		TestTrue(TEXT("The clock survives the save"), FMath::IsNearlyEqual(Delta->RemainingSeconds, 140.f, 0.01f));
		TestTrue(TEXT("The place survives the save"), Delta->Direction.Equals(Prey.Direction, 1.e-12));
	}

	const TArray<FName> Lapsed = Restored->Advance(150.f);
	TestTrue(TEXT("The nest repopulates when its clock runs out"), Lapsed.Contains(Prey.EntityId) && !Restored->IsDefeated(Prey.EntityId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonSaveV3Test, "Astraeon.Persistence.SaveGame.V3PlanetaryLocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonSaveV3Test::RunTest(const FString& Parameters)
{
	using FLegacy = FAstraeonLegacyFlatProjection;
	// v2 -> v3: the flat fields become body + direction + altitude + heading, exactly invertible.
	UAstraeonSaveGame* Legacy = NewObject<UAstraeonSaveGame>();
	Legacy->SaveGameVersion = 2;
	Legacy->ContentSeed = 555; Legacy->WorldSeed = 555;
	Legacy->RegionLayout = UAstraeonWorldProfiles::BuildFixedRegionLayout(555);
	Legacy->PlayerTransform = FTransform(FRotator(0, 90, 0), FVector(12000, -8000, 300));
	Legacy->ItacaOriginCm = FVector(500, 700, 120);
	const FAstraeonPointOfInterest* Nest = Legacy->RegionLayout.PointsOfInterest.FindByPredicate(
		[](const FAstraeonPointOfInterest& P) { return P.Type == EAstraeonPointOfInterestType::CreatureSpawn; });
	if (!TestNotNull(TEXT("The fixed region has a nest"), Nest)) return false;
	Legacy->CreatureRespawnTimers.Add(Nest->PointId, 100.f);
	Legacy->CreatureRespawnTimers.Add(TEXT("zz_unknown_nest"), 50.f);
	if (!TestTrue(TEXT("v2 upgrades"), FAstraeonSaveMigration::Upgrade(*Legacy))) return false;
	TestEqual(TEXT("Written as v3"), Legacy->SaveGameVersion, 3);
	TestEqual(TEXT("On the legacy body"), Legacy->PlanetBodyId, FLegacy::BodyId());
	TestTrue(TEXT("Player location maps back to the flat point"), FLegacy::ToFlat(Legacy->PlayerDirection, Legacy->PlayerAltitudeCm).Equals(FVector(12000, -8000, 300), 0.01));
	TestTrue(TEXT("Arc distance from the anchor is the flat distance"),
		FMath::IsNearlyEqual(FMath::Acos(Legacy->PlayerDirection.Z) * FLegacy::RadiusCm, FVector2D(12000, -8000).Size(), 0.001));
	TestTrue(TEXT("Heading lies on the tangent plane"), FMath::Abs(FVector::DotProduct(Legacy->PlayerForwardTangent, Legacy->PlayerDirection)) < 1.e-9);
	TestTrue(TEXT("Heading keeps its bearing (flat +Y)"), FVector::DotProduct(Legacy->PlayerForwardTangent,
		FQuat::FindBetweenNormals(FVector(0, 0, 1), Legacy->PlayerDirection).RotateVector(FVector(0, 1, 0))) > 0.999);
	TestTrue(TEXT("Itaca maps back to its flat origin"), FLegacy::ToFlat(Legacy->ItacaDirection, Legacy->ItacaAltitudeCm).Equals(FVector(500, 700, 120), 0.01));
	if (TestEqual(TEXT("Both nest clocks became deltas"), Legacy->PlanetDeltas.Num(), 2))
	{
		const FAstraeonPlanetDelta& Known = Legacy->PlanetDeltas[0];
		TestEqual(TEXT("Stable id order"), Known.EntityId, Nest->PointId);
		TestTrue(TEXT("A known nest keeps its place"), FLegacy::ToFlat(Known.Direction, Known.AltitudeCm).Equals(FVector(Nest->LocationMeters * 100.0, 0.0), 0.01));
		TestTrue(TEXT("The clock is kept"), FMath::IsNearlyEqual(Known.RemainingSeconds, 100.f));
		TestTrue(TEXT("An unknown nest stays at the anchor"), Legacy->PlanetDeltas[1].Direction.Equals(FVector(0, 0, 1), 1.e-12));
	}
	// v1 -> v3 keeps the v1 -> v2 rule on the way.
	UAstraeonSaveGame* VeryOld = NewObject<UAstraeonSaveGame>();
	VeryOld->SaveGameVersion = 1; VeryOld->WorldSeed = 777;
	TestTrue(TEXT("v1 upgrades"), FAstraeonSaveMigration::Upgrade(*VeryOld));
	TestTrue(TEXT("v1 keeps its legacy ids and seed"), VeryOld->PlanetProfileId == FName(TEXT("legacy_generated_planet")) && VeryOld->ContentSeed == 777);
	UAstraeonSaveGame* Future = NewObject<UAstraeonSaveGame>();
	Future->SaveGameVersion = 99;
	TestFalse(TEXT("A save from a future version is refused, not guessed"), FAstraeonSaveMigration::Upgrade(*Future));

	// Full session round trip in the flat build, through the real slot: Itaca and the nest
	// clock come back although v3 stores neither as a flat field.
	const FString Slot(TEXT("AstraeonAutomationSaveV3"));
	UGameplayStatics::DeleteGameInSlot(Slot, 0);
	UAstraeonGameInstance* Source = NewObject<UAstraeonGameInstance>();
	Source->StartNewGame(1001);
	Source->SetItacaOriginCm(FVector(12000, -8000, 150));
	Source->RecordCreatureDeath(Nest->PointId);
	if (!TestTrue(TEXT("Session saves"), Source->SaveCurrentGame(Slot, 0))) return false;
	UAstraeonGameInstance* Target = NewObject<UAstraeonGameInstance>();
	TestTrue(TEXT("Session loads"), Target->LoadSavedGame(Slot, 0));
	TestTrue(TEXT("Itaca origin round-trips through the planetary location"), Target->GetItacaOriginCm().Equals(Source->GetItacaOriginCm(), 0.01));
	TestFalse(TEXT("A hunted nest stays empty after load"), Target->IsCreatureSpawnPopulated(Nest->PointId));
	UGameplayStatics::DeleteGameInSlot(Slot, 0);
	return true;
}

#endif
