#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AstraeonGameInstance.h"
#include "AstraeonGameModeBase.h"
#include "AstraeonHUD.h"
#include "AstraeonPlayerCharacter.h"
#include "AstraeonPlayerController.h"
#include "Creatures/AstraeonCreatureActor.h"
#include "Exploration/AstraeonMapRevealLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Knowledge/AstraeonLogbookComponent.h"
#include "Survival/AstraeonSuitComponent.h"
#include "Persistence/AstraeonSaveGame.h"
#include "WorldGen/AstraeonRegionMaterializer.h"
#include "WorldGen/AstraeonRegionMarker.h"
#include "WorldGen/AstraeonWorldGenerator.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonEnvironmentDeterminismTest,
	"Astraeon.WorldGen.Environment.Determinism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonEnvironmentDeterminismTest::RunTest(const FString& Parameters)
{
	const FAstraeonEnvironmentalSnapshot First = UAstraeonWorldGenerator::GenerateEnvironment(12345);
	const FAstraeonEnvironmentalSnapshot Second = UAstraeonWorldGenerator::GenerateEnvironment(12345);

	TestEqual(TEXT("Gravity is deterministic"), First.GravityMS2, Second.GravityMS2);
	TestEqual(TEXT("Temperature is deterministic"), First.TemperatureKelvin, Second.TemperatureKelvin);
	TestEqual(TEXT("Pressure is deterministic"), First.PressureKPa, Second.PressureKPa);
	TestEqual(TEXT("Breathability is deterministic"), First.bBreathable, Second.bBreathable);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonRegionLayoutDeterminismTest,
	"Astraeon.WorldGen.Region.Determinism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonRegionLayoutDeterminismTest::RunTest(const FString& Parameters)
{
	const FAstraeonRegionLayout First = UAstraeonWorldGenerator::GenerateRegionLayout(12345);
	const FAstraeonRegionLayout Second = UAstraeonWorldGenerator::GenerateRegionLayout(12345);

	TestEqual(TEXT("Resource count is deterministic"), First.Resources.Num(), Second.Resources.Num());
	TestEqual(TEXT("POI count is deterministic"), First.PointsOfInterest.Num(), Second.PointsOfInterest.Num());
	TestEqual(TEXT("Signal location is deterministic"), First.PointsOfInterest[0].LocationMeters, Second.PointsOfInterest[0].LocationMeters);
	TestEqual(TEXT("Signature resource is deterministic"), First.Resources[2].ResourceId, Second.Resources[2].ResourceId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonRegionLayoutInvariantTest,
	"Astraeon.WorldGen.Region.Invariants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonRegionLayoutInvariantTest::RunTest(const FString& Parameters)
{
	for (int32 Seed = 0; Seed < 32; ++Seed)
	{
		const FAstraeonRegionLayout Layout = UAstraeonWorldGenerator::GenerateRegionLayout(Seed);
		TestEqual(TEXT("MVP has exactly three initial resources"), Layout.Resources.Num(), 3);
		TestEqual(TEXT("MVP has exactly three initial POIs"), Layout.PointsOfInterest.Num(), 3);
		TestTrue(TEXT("First resource is silicate fiber"), Layout.Resources[0].ResourceId == TEXT("silicate_fiber"));
		TestTrue(TEXT("Second resource is ferrite nodule"), Layout.Resources[1].ResourceId == TEXT("ferrite_nodule"));
		TestTrue(TEXT("Third resource is seed signature"), Layout.Resources[2].bSeedSignature);
		TestEqual(TEXT("First POI is signal source"), Layout.PointsOfInterest[0].Type, EAstraeonPointOfInterestType::SignalSource);

		for (const FAstraeonResourceNode& Resource : Layout.Resources)
		{
			TestTrue(TEXT("Resource remains inside region radius"), Resource.LocationMeters.Size() <= Layout.RegionRadiusMeters);
			TestTrue(TEXT("Resource quantity is positive"), Resource.Quantity > 0);
		}

		for (const FAstraeonPointOfInterest& PointOfInterest : Layout.PointsOfInterest)
		{
			TestTrue(TEXT("POI remains inside region radius"), PointOfInterest.LocationMeters.Size() <= Layout.RegionRadiusMeters);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonRegionMaterializerSpecsTest,
	"Astraeon.WorldGen.Region.MaterializerSpecs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonRegionMaterializerSpecsTest::RunTest(const FString& Parameters)
{
	const FAstraeonRegionLayout Layout = UAstraeonWorldGenerator::GenerateRegionLayout(2468);
	const TArray<FAstraeonRegionActorSpec> Specs = UAstraeonRegionMaterializer::BuildActorSpecs(Layout);

	TestEqual(TEXT("One actor spec per resource and POI"), Specs.Num(), Layout.Resources.Num() + Layout.PointsOfInterest.Num());
	TestEqual(TEXT("First specs are resources"), Specs[0].Kind, EAstraeonRegionActorKind::Resource);
	TestEqual(TEXT("Signal source spec is fourth after three resources"), Specs[3].ActorId, FName(TEXT("signal_source")));
	TestEqual(TEXT("Signal source is POI kind"), Specs[3].Kind, EAstraeonRegionActorKind::PointOfInterest);
	TestTrue(TEXT("Generated actor location uses centimeters"), FMath::Abs(Specs[0].LocationCm.X) > FMath::Abs(Layout.Resources[0].LocationMeters.X));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonItacaMaterializerSpecsTest,
	"Astraeon.WorldGen.Itaca.MaterializerSpecs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonItacaMaterializerSpecsTest::RunTest(const FString& Parameters)
{
	const TArray<FAstraeonRegionActorSpec> Specs = UAstraeonRegionMaterializer::BuildItacaActorSpecs();

	TestEqual(TEXT("Ítaca MVP has ARGOS console and surface hatch"), Specs.Num(), 2);
	TestEqual(TEXT("First Ítaca actor is ARGOS console"), Specs[0].ActorId, FName(TEXT("itaca_argos_console")));
	TestEqual(TEXT("Second Ítaca actor is surface hatch"), Specs[1].ActorId, FName(TEXT("itaca_surface_hatch")));
	TestEqual(TEXT("Surface hatch is interactable POI"), Specs[1].Kind, EAstraeonRegionActorKind::PointOfInterest);
	const FVector DeploymentLocationCm = UAstraeonRegionMaterializer::GetSurfaceDeploymentLocationCm();
	TestTrue(TEXT("Surface deployment target is above walkable runtime surface"), DeploymentLocationCm.Z >= 120.0f);
	TestTrue(TEXT("Surface deployment target is away from Ítaca hatch"), FVector2D(DeploymentLocationCm.X, DeploymentLocationCm.Y).Size() >= 1000.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonRegionMarkerApplySpecTest,
	"Astraeon.WorldGen.Region.MarkerApplySpec",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonRegionMarkerApplySpecTest::RunTest(const FString& Parameters)
{
	AAstraeonRegionMarker* Marker = NewObject<AAstraeonRegionMarker>();
	FAstraeonRegionActorSpec Spec;
	Spec.ActorId = TEXT("test_resource");
	Spec.Kind = EAstraeonRegionActorKind::Resource;
	Spec.LocationCm = FVector(100.0f, 200.0f, 60.0f);
	Spec.Scale = FVector(0.8f, 0.8f, 0.8f);

	Marker->ApplySpec(Spec);
	TestEqual(TEXT("Marker id is applied"), Marker->GetMarkerId(), FName(TEXT("test_resource")));
	TestEqual(TEXT("Marker kind is applied"), Marker->GetMarkerKind(), EAstraeonRegionActorKind::Resource);
	TestEqual(TEXT("Marker location is applied"), Marker->GetActorLocation(), Spec.LocationCm);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonRegionMarkerVisualIdentityTest,
	"Astraeon.WorldGen.Region.MarkerVisualIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonRegionMarkerVisualIdentityTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("ARGOS marker label is readable"), AAstraeonRegionMarker::BuildMarkerLabel(TEXT("itaca_argos_console"), EAstraeonRegionActorKind::PointOfInterest).ToString().Contains(TEXT("ARGOS")));
	TestTrue(TEXT("Surface hatch marker label is readable"), AAstraeonRegionMarker::BuildMarkerLabel(TEXT("itaca_surface_hatch"), EAstraeonRegionActorKind::PointOfInterest).ToString().Contains(TEXT("ESCOTILLA")));
	TestTrue(TEXT("Signal source marker label is readable"), AAstraeonRegionMarker::BuildMarkerLabel(TEXT("signal_source"), EAstraeonRegionActorKind::PointOfInterest).ToString().Contains(TEXT("SEÑAL")));
	TestTrue(TEXT("Resource marker label includes resource id"), AAstraeonRegionMarker::BuildMarkerLabel(TEXT("silicate_fiber"), EAstraeonRegionActorKind::Resource).ToString().Contains(TEXT("silicate_fiber")));
	TestNotEqual(TEXT("ARGOS and signal colors differ"), AAstraeonRegionMarker::BuildMarkerColor(TEXT("itaca_argos_console"), EAstraeonRegionActorKind::PointOfInterest), AAstraeonRegionMarker::BuildMarkerColor(TEXT("signal_source"), EAstraeonRegionActorKind::PointOfInterest));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonEnvironmentVariationTest,
	"Astraeon.WorldGen.Environment.SeedVariation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonEnvironmentVariationTest::RunTest(const FString& Parameters)
{
	const FAstraeonEnvironmentalSnapshot First = UAstraeonWorldGenerator::GenerateEnvironment(111);
	const FAstraeonEnvironmentalSnapshot Second = UAstraeonWorldGenerator::GenerateEnvironment(222);

	const bool bDifferent = !FMath::IsNearlyEqual(First.GravityMS2, Second.GravityMS2)
		|| !FMath::IsNearlyEqual(First.TemperatureKelvin, Second.TemperatureKelvin)
		|| !FMath::IsNearlyEqual(First.PressureKPa, Second.PressureKPa);

	TestTrue(TEXT("Different seeds produce at least one measurable environmental difference"), bDifferent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonEnvironmentInvariantTest,
	"Astraeon.WorldGen.Environment.Invariants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonEnvironmentInvariantTest::RunTest(const FString& Parameters)
{
	for (int32 Seed = 0; Seed < 32; ++Seed)
	{
		const FAstraeonEnvironmentalSnapshot Environment = UAstraeonWorldGenerator::GenerateEnvironment(Seed);
		TestTrue(TEXT("Gravity stays in MVP range"), Environment.GravityMS2 >= 3.2f && Environment.GravityMS2 <= 15.2f);
		TestTrue(TEXT("Temperature stays in MVP range"), Environment.TemperatureKelvin >= 220.0f && Environment.TemperatureKelvin <= 335.0f);
		TestTrue(TEXT("Pressure stays in MVP range"), Environment.PressureKPa >= 15.0f && Environment.PressureKPa <= 140.0f);
		TestTrue(TEXT("Risk stays normalized"), Environment.EnvironmentalRisk01 >= 0.0f && Environment.EnvironmentalRisk01 <= 1.0f);
		const float AtmosphereTotal = Environment.Atmosphere.Oxygen + Environment.Atmosphere.Nitrogen + Environment.Atmosphere.CarbonDioxide + Environment.Atmosphere.Argon;
		TestTrue(TEXT("Atmosphere is normalized"), FMath::IsNearlyEqual(AtmosphereTotal, 1.0f, 0.001f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonBreathabilityClassificationTest,
	"Astraeon.Science.Environment.Breathability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonBreathabilityClassificationTest::RunTest(const FString& Parameters)
{
	FAstraeonEnvironmentalSnapshot Environment;
	Environment.TemperatureKelvin = 293.15f;
	Environment.PressureKPa = 101.325f;
	Environment.Atmosphere.Oxygen = 0.21f;
	Environment.Atmosphere.Nitrogen = 0.78f;
	Environment.Atmosphere.Argon = 0.009f;
	Environment.Atmosphere.CarbonDioxide = 0.001f;

	TestTrue(TEXT("Earth-like simplified atmosphere is breathable"), UAstraeonWorldGenerator::IsBreathable(Environment));

	Environment.Atmosphere.Oxygen = 0.05f;
	Environment.Atmosphere.Nitrogen = 0.94f;
	TestFalse(TEXT("Low oxygen partial pressure is not breathable"), UAstraeonWorldGenerator::IsBreathable(Environment));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonCreatureAwarenessStateTest,
	"Astraeon.Creatures.Awareness.DistanceStates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonCreatureAwarenessStateTest::RunTest(const FString& Parameters)
{
	FAstraeonCreatureProfile Profile;
	Profile.AlertRadiusMeters = 45.0f;
	Profile.ThreatRadiusMeters = 14.0f;

	TestEqual(TEXT("Far creature patrols"), AAstraeonCreatureActor::EvaluateAwarenessState(80.0f, Profile), EAstraeonCreatureAwarenessState::Patrolling);
	TestEqual(TEXT("Recently near creature disengages"), AAstraeonCreatureActor::EvaluateAwarenessState(55.0f, Profile), EAstraeonCreatureAwarenessState::Disengaging);
	TestEqual(TEXT("Nearby creature alerts"), AAstraeonCreatureActor::EvaluateAwarenessState(30.0f, Profile), EAstraeonCreatureAwarenessState::Alert);
	TestEqual(TEXT("Very close creature threatens"), AAstraeonCreatureActor::EvaluateAwarenessState(8.0f, Profile), EAstraeonCreatureAwarenessState::Threatening);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonCreatureScanLogbookTest,
	"Astraeon.Creatures.Scanning.LogbookEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonCreatureScanLogbookTest::RunTest(const FString& Parameters)
{
	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	FAstraeonCreatureProfile Profile;
	Profile.SpeciesId = TEXT("umbra_grazer");
	Profile.DisplayName = FText::FromString(TEXT("Umbra Grazer"));

	TestFalse(TEXT("Creature scan fails before session"), GameInstance->RecordCreatureScan(Profile));
	GameInstance->StartNewGame(8080);
	TestTrue(TEXT("Creature scan succeeds during session"), GameInstance->RecordCreatureScan(Profile));
	TestTrue(TEXT("Creature scan adds a creature logbook entry"), GameInstance->GetRuntimeLogbookEntries().ContainsByPredicate([](const FAstraeonLogbookEntry& Entry)
	{
		return Entry.EntryId == TEXT("creature.umbra_grazer") && Entry.Certainty == EAstraeonDiscoveryCertainty::Observed;
	}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonCreaturePatrolFeedbackTest,
	"Astraeon.Creatures.Feedback.PatrolAndScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonCreaturePatrolFeedbackTest::RunTest(const FString& Parameters)
{
	const FVector InitialOffset = AAstraeonCreatureActor::ComputePatrolOffsetCm(0.0f, 5.0f);
	const FVector LaterOffset = AAstraeonCreatureActor::ComputePatrolOffsetCm(10.0f, 5.0f);
	TestTrue(TEXT("Patrol has visible radius"), FMath::IsNearlyEqual(InitialOffset.Size2D(), 500.0f, 0.1f));
	TestFalse(TEXT("Patrol offset changes over time"), InitialOffset.Equals(LaterOffset, 0.1f));

	const FVector PatrolScale = AAstraeonCreatureActor::ComputeStateVisualScale(EAstraeonCreatureAwarenessState::Patrolling);
	const FVector ThreatScale = AAstraeonCreatureActor::ComputeStateVisualScale(EAstraeonCreatureAwarenessState::Threatening);
	TestTrue(TEXT("Threatening state has stronger visible scale"), ThreatScale.X > PatrolScale.X && ThreatScale.Y > PatrolScale.Y);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonCreatureProfileApplyTest,
	"Astraeon.Creatures.Profile.Apply",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonCreatureProfileApplyTest::RunTest(const FString& Parameters)
{
	AAstraeonCreatureActor* Creature = NewObject<AAstraeonCreatureActor>();
	FAstraeonCreatureProfile Profile;
	Profile.SpeciesId = TEXT("test_species");
	Profile.AlertRadiusMeters = 30.0f;
	Profile.ThreatRadiusMeters = 10.0f;

	Creature->ConfigureCreature(Profile);
	Creature->UpdateAwarenessFromPlayerDistanceMeters(9.0f);

	TestEqual(TEXT("Creature profile species is applied"), Creature->GetCreatureProfile().SpeciesId, FName(TEXT("test_species")));
	TestEqual(TEXT("Creature enters threatening state"), Creature->GetAwarenessState(), EAstraeonCreatureAwarenessState::Threatening);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonArgosBriefingLogbookTest,
	"Astraeon.Narrative.Argos.BriefingLogbook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonArgosBriefingLogbookTest::RunTest(const FString& Parameters)
{
	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	GameInstance->RecordArgosBriefing();
	TestEqual(TEXT("No session means no ARGOS entry"), GameInstance->GetRuntimeLogbookEntries().Num(), 0);

	GameInstance->StartNewGame(8080);
	GameInstance->RecordArgosBriefing();
	TestTrue(TEXT("ARGOS briefing entry exists"), GameInstance->GetRuntimeLogbookEntries().ContainsByPredicate([](const FAstraeonLogbookEntry& Entry)
	{
		return Entry.EntryId == TEXT("argos.first_signal_briefing") && Entry.Certainty == EAstraeonDiscoveryCertainty::Confirmed;
	}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonItacaSurfaceDeploymentLogbookTest,
	"Astraeon.Narrative.Itaca.SurfaceDeploymentLogbook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonItacaSurfaceDeploymentLogbookTest::RunTest(const FString& Parameters)
{
	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	TestFalse(TEXT("No session means no surface deployment entry"), GameInstance->RecordSurfaceDeployment());

	GameInstance->StartNewGame(8081);
	TestTrue(TEXT("Initial hint asks for ARGOS briefing"), GameInstance->GetObjectiveHint().Contains(TEXT("ARGOS")));
	GameInstance->RecordArgosBriefing();
	TestTrue(TEXT("Briefing hint points to surface hatch"), GameInstance->GetObjectiveHint().Contains(TEXT("ESCOTILLA")));
	TestTrue(TEXT("Surface deployment can be recorded"), GameInstance->RecordSurfaceDeployment());
	TestTrue(TEXT("Surface deployment entry exists"), GameInstance->GetRuntimeLogbookEntries().ContainsByPredicate([](const FAstraeonLogbookEntry& Entry)
	{
		return Entry.EntryId == TEXT("itaca.surface_deployment") && Entry.Certainty == EAstraeonDiscoveryCertainty::Observed;
	}));
	TestTrue(TEXT("Deployment hint points to scanner"), GameInstance->GetObjectiveHint().Contains(TEXT("Click Izq")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonSignalResolutionObjectiveTest,
	"Astraeon.Narrative.Signal.ResolveObjective",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonSignalResolutionObjectiveTest::RunTest(const FString& Parameters)
{
	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	GameInstance->StartNewGame(7070);
	const FName SignatureResourceId = GameInstance->GetCurrentRegionLayout().Resources[2].ResourceId;

	TestEqual(TEXT("New game starts at measure objective"), GameInstance->GetObjectiveState(), EAstraeonObjectiveState::MeasureEnvironment);
	TestFalse(TEXT("Cannot resolve signal without resonator"), GameInstance->TryResolveSignalSource());

	GameInstance->ScanCurrentEnvironment();
	GameInstance->AddInventoryItem(TEXT("silicate_fiber"), 1);
	GameInstance->AddInventoryItem(TEXT("ferrite_nodule"), 1);
	GameInstance->AddInventoryItem(SignatureResourceId, 1);
	TestTrue(TEXT("Craft resonator succeeds"), GameInstance->CraftSignalResonator());
	TestEqual(TEXT("Crafting advances objective to reach signal"), GameInstance->GetObjectiveState(), EAstraeonObjectiveState::ReachSignalSource);
	TestTrue(TEXT("Signal resolves with resonator"), GameInstance->TryResolveSignalSource());
	TestTrue(TEXT("Signal objective is completed"), GameInstance->IsSignalResolved());
	TestTrue(TEXT("Final signal logbook entry exists"), GameInstance->GetRuntimeLogbookEntries().ContainsByPredicate([](const FAstraeonLogbookEntry& Entry)
	{
		return Entry.EntryId == TEXT("signal.first_source") && Entry.Certainty == EAstraeonDiscoveryCertainty::Confirmed;
	}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonCriticalPathFullFlowTest,
	"Astraeon.Functional.CriticalPath.FullFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonCriticalPathFullFlowTest::RunTest(const FString& Parameters)
{
	const FString SlotName(TEXT("AstraeonAutomationCriticalPath"));
	UGameplayStatics::DeleteGameInSlot(SlotName, 0);

	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	GameInstance->StartNewGame(123456);
	const FName SignatureResourceId = GameInstance->GetCurrentRegionLayout().Resources[2].ResourceId;

	GameInstance->RecordArgosBriefing();
	TestTrue(TEXT("ARGOS briefing is recorded"), GameInstance->GetRuntimeLogbookEntries().ContainsByPredicate([](const FAstraeonLogbookEntry& Entry)
	{
		return Entry.EntryId == TEXT("argos.first_signal_briefing");
	}));
	TestTrue(TEXT("Surface deployment is recorded"), GameInstance->RecordSurfaceDeployment());

	TestTrue(TEXT("Environment scan succeeds"), GameInstance->ScanCurrentEnvironment());
	TestEqual(TEXT("Environment scan advances objective"), GameInstance->GetObjectiveState(), EAstraeonObjectiveState::GatherResources);

	TestTrue(TEXT("Collect silicate"), GameInstance->AddInventoryItem(TEXT("silicate_fiber"), 1));
	TestTrue(TEXT("Collect ferrite"), GameInstance->AddInventoryItem(TEXT("ferrite_nodule"), 1));
	TestTrue(TEXT("Collect seed signature resource"), GameInstance->AddInventoryItem(SignatureResourceId, 1));
	TestEqual(TEXT("Three resource stacks advance objective to crafting"), GameInstance->GetObjectiveState(), EAstraeonObjectiveState::CraftSignalResonator);

	TestTrue(TEXT("Craft signal resonator"), GameInstance->CraftSignalResonator());
	TestEqual(TEXT("Crafting advances objective to signal source"), GameInstance->GetObjectiveState(), EAstraeonObjectiveState::ReachSignalSource);
	TestTrue(TEXT("Resolve first signal"), GameInstance->TryResolveSignalSource());
	TestTrue(TEXT("Critical path completes"), GameInstance->IsSignalResolved());
	TestEqual(TEXT("Resonator remains as completion tool evidence"), GameInstance->GetInventoryItemCount(TEXT("signal_resonator")), 1);

	TestTrue(TEXT("Completed critical path can be saved"), GameInstance->SaveCurrentGame(SlotName, 0));
	UAstraeonGameInstance* LoadedGame = NewObject<UAstraeonGameInstance>();
	TestTrue(TEXT("Completed critical path can be loaded"), LoadedGame->LoadSavedGame(SlotName, 0));
	TestTrue(TEXT("Loaded critical path remains complete"), LoadedGame->IsSignalResolved());
	TestEqual(TEXT("Loaded seed remains deterministic"), LoadedGame->GetCurrentWorldSeed(), 123456);
	TestTrue(TEXT("Loaded final signal logbook entry persists"), LoadedGame->GetRuntimeLogbookEntries().ContainsByPredicate([](const FAstraeonLogbookEntry& Entry)
	{
		return Entry.EntryId == TEXT("signal.first_source") && Entry.Certainty == EAstraeonDiscoveryCertainty::Confirmed;
	}));

	UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonCraftSignalResonatorTest,
	"Astraeon.Resources.Crafting.SignalResonator",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonCraftSignalResonatorTest::RunTest(const FString& Parameters)
{
	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	GameInstance->StartNewGame(6060);
	const FName SignatureResourceId = GameInstance->GetCurrentRegionLayout().Resources[2].ResourceId;

	TestFalse(TEXT("Cannot craft without ingredients"), GameInstance->CanCraftSignalResonator());
	GameInstance->AddInventoryItem(TEXT("silicate_fiber"), 1);
	GameInstance->AddInventoryItem(TEXT("ferrite_nodule"), 1);
	GameInstance->AddInventoryItem(SignatureResourceId, 1);

	TestTrue(TEXT("Can craft with required ingredients"), GameInstance->CanCraftSignalResonator());
	TestTrue(TEXT("Craft succeeds"), GameInstance->CraftSignalResonator());
	TestEqual(TEXT("Craft consumes silicate"), GameInstance->GetInventoryItemCount(TEXT("silicate_fiber")), 0);
	TestEqual(TEXT("Craft consumes ferrite"), GameInstance->GetInventoryItemCount(TEXT("ferrite_nodule")), 0);
	TestEqual(TEXT("Craft consumes signature resource"), GameInstance->GetInventoryItemCount(SignatureResourceId), 0);
	TestEqual(TEXT("Craft creates resonator"), GameInstance->GetInventoryItemCount(TEXT("signal_resonator")), 1);
	TestTrue(TEXT("Craft records recipe in logbook"), GameInstance->GetRuntimeLogbookEntries().ContainsByPredicate([](const FAstraeonLogbookEntry& Entry)
	{
		return Entry.EntryId == TEXT("recipe.signal_resonator") && Entry.Certainty == EAstraeonDiscoveryCertainty::Confirmed;
	}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonInventoryAddItemsTest,
	"Astraeon.Resources.Inventory.AddItems",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonInventoryAddItemsTest::RunTest(const FString& Parameters)
{
	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	TestFalse(TEXT("Cannot add inventory before session"), GameInstance->AddInventoryItem(TEXT("silicate_fiber"), 1));

	GameInstance->StartNewGame(9090);
	TestTrue(TEXT("Can add valid resource"), GameInstance->AddInventoryItem(TEXT("silicate_fiber"), 1));
	TestTrue(TEXT("Can stack same resource"), GameInstance->AddInventoryItem(TEXT("silicate_fiber"), 2));
	TestEqual(TEXT("Inventory stack count accumulates"), GameInstance->GetInventoryItemCount(TEXT("silicate_fiber")), 3);
	TestFalse(TEXT("Cannot add invalid quantity"), GameInstance->AddInventoryItem(TEXT("silicate_fiber"), 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonMapRevealCellTest,
	"Astraeon.Exploration.Map.RevealCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonMapRevealCellTest::RunTest(const FString& Parameters)
{
	FAstraeonRevealedMap Map;
	const FAstraeonMapCellId Origin = UAstraeonMapRevealLibrary::CellIdForLocationMeters(FVector2D(0.0f, 0.0f), Map.CellSizeMeters);
	const FAstraeonMapCellId Negative = UAstraeonMapRevealLibrary::CellIdForLocationMeters(FVector2D(-1.0f, -51.0f), Map.CellSizeMeters);

	TestEqual(TEXT("Origin maps to cell 0,0 X"), Origin.X, 0);
	TestEqual(TEXT("Origin maps to cell 0,0 Y"), Origin.Y, 0);
	TestEqual(TEXT("Negative coordinate floors X"), Negative.X, -1);
	TestEqual(TEXT("Negative coordinate floors Y"), Negative.Y, -2);
	TestTrue(TEXT("First reveal changes the map"), UAstraeonMapRevealLibrary::RevealCell(Map, Origin));
	TestFalse(TEXT("Repeated reveal does not duplicate"), UAstraeonMapRevealLibrary::RevealCell(Map, Origin));
	TestEqual(TEXT("Only one revealed cell"), Map.RevealedCells.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonMapRevealRadiusPersistenceTest,
	"Astraeon.Exploration.Map.RadiusAndPersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonMapRevealRadiusPersistenceTest::RunTest(const FString& Parameters)
{
	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	GameInstance->StartNewGame(1357);
	const int32 InitiallyRevealed = GameInstance->GetRevealedMap().RevealedCells.Num();
	TestEqual(TEXT("New game reveals the starting 3x3 area"), InitiallyRevealed, 9);

	const int32 NewlyRevealed = GameInstance->RevealMapAroundLocationMeters(FVector2D(150.0f, 0.0f), 1);
	TestTrue(TEXT("Revealing another area adds cells"), NewlyRevealed > 0);
	TestTrue(TEXT("Map keeps revealed cells"), GameInstance->GetRevealedMap().RevealedCells.Num() > InitiallyRevealed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonGameModeBootstrapClassesTest,
	"Astraeon.Session.Bootstrap.GameModeClasses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonGameModeBootstrapClassesTest::RunTest(const FString& Parameters)
{
	AAstraeonGameModeBase* GameMode = NewObject<AAstraeonGameModeBase>();
	TestTrue(TEXT("Default pawn is first-person Astraeon character"), GameMode->DefaultPawnClass.Get() == AAstraeonPlayerCharacter::StaticClass());
	TestTrue(TEXT("Player controller is Astraeon controller"), GameMode->PlayerControllerClass.Get() == AAstraeonPlayerController::StaticClass());
	TestTrue(TEXT("HUD class is Astraeon bootstrap HUD"), GameMode->HUDClass.Get() == AAstraeonHUD::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonMenuHUDLinesTest,
	"Astraeon.Session.Menu.HUDLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonMenuHUDLinesTest::RunTest(const FString& Parameters)
{
	AAstraeonPlayerController* PlayerController = NewObject<AAstraeonPlayerController>();
	PlayerController->IncreaseSelectedSeed();
	const TArray<FString> Lines = AAstraeonHUD::BuildMenuLines(PlayerController);

	TestTrue(TEXT("Menu is visible by default"), PlayerController->IsMenuVisible());
	TestTrue(TEXT("Menu shows editable selected seed"), Lines.ContainsByPredicate([](const FString& Line) { return Line.Contains(TEXT("Semilla seleccionada: 1002")); }));
	TestTrue(TEXT("Menu explains start input"), Lines.ContainsByPredicate([](const FString& Line) { return Line.Contains(TEXT("Intro: nueva partida")); }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonHUDStatusLinesTest,
	"Astraeon.Session.Bootstrap.HUDStatusLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonHUDStatusLinesTest::RunTest(const FString& Parameters)
{
	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	GameInstance->StartNewGame(5150);

	const TArray<FString> Lines = AAstraeonHUD::BuildStatusLines(GameInstance);
	TestTrue(TEXT("HUD exposes enough bootstrap lines"), Lines.Num() >= 7);
	TestTrue(TEXT("HUD shows seed"), Lines.ContainsByPredicate([](const FString& Line) { return Line.Contains(TEXT("Semilla: 5150")); }));
	TestTrue(TEXT("HUD shows pressure with explicit unit field"), Lines.ContainsByPredicate([](const FString& Line) { return Line.Contains(TEXT("Presión kPa")); }));
	TestTrue(TEXT("HUD shows map reveal count"), Lines.ContainsByPredicate([](const FString& Line) { return Line.Contains(TEXT("Celdas reveladas")); }));
	TestTrue(TEXT("HUD shows objective hint"), Lines.ContainsByPredicate([](const FString& Line) { return Line.Contains(TEXT("Pista:")); }));
	TestTrue(TEXT("HUD shows current feedback"), Lines.ContainsByPredicate([](const FString& Line) { return Line.Contains(TEXT("Estado:")); }));
	TestTrue(TEXT("HUD shows logbook count"), Lines.ContainsByPredicate([](const FString& Line) { return Line.Contains(TEXT("Bitácora: 1 entradas")); }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonObjectiveHintProgressionTest,
	"Astraeon.Session.Objectives.HintProgression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonObjectiveHintProgressionTest::RunTest(const FString& Parameters)
{
	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	GameInstance->StartNewGame(24601);
	const FName SignatureResourceId = GameInstance->GetCurrentRegionLayout().Resources[2].ResourceId;

	TestTrue(TEXT("Initial objective hints ARGOS"), GameInstance->GetObjectiveHint().Contains(TEXT("ARGOS")));
	GameInstance->RecordArgosBriefing();
	TestTrue(TEXT("Briefing objective hints surface hatch"), GameInstance->GetObjectiveHint().Contains(TEXT("ESCOTILLA")));
	GameInstance->RecordSurfaceDeployment();
	TestTrue(TEXT("Deployment objective hints scan"), GameInstance->GetObjectiveHint().Contains(TEXT("Escanea")));
	GameInstance->ScanCurrentEnvironment();
	TestTrue(TEXT("Resource objective hints collection"), GameInstance->GetObjectiveHint().Contains(TEXT("recoge")));
	GameInstance->AddInventoryItem(TEXT("silicate_fiber"), 1);
	GameInstance->AddInventoryItem(TEXT("ferrite_nodule"), 1);
	GameInstance->AddInventoryItem(SignatureResourceId, 1);
	TestTrue(TEXT("Crafting objective hints C input"), GameInstance->GetObjectiveHint().Contains(TEXT("Presiona C")));
	GameInstance->CraftSignalResonator();
	TestTrue(TEXT("Signal objective hints signal marker"), GameInstance->GetObjectiveHint().Contains(TEXT("SEÑAL")));
	GameInstance->TryResolveSignalSource();
	TestTrue(TEXT("Completed objective hints save"), GameInstance->GetObjectiveHint().Contains(TEXT("Guarda")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonNewGameSeedTest,
	"Astraeon.Session.NewGame.Seed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonNewGameSeedTest::RunTest(const FString& Parameters)
{
	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	GameInstance->StartNewGame(424242);

	TestTrue(TEXT("New game marks session as started"), GameInstance->HasStartedGame());
	TestEqual(TEXT("Requested seed is stored"), GameInstance->GetCurrentWorldSeed(), 424242);

	const FAstraeonEnvironmentalSnapshot Expected = UAstraeonWorldGenerator::GenerateEnvironment(424242);
	TestEqual(TEXT("Generated environment uses the requested seed"), GameInstance->GetCurrentEnvironment().WorldSeed, Expected.WorldSeed);
	TestEqual(TEXT("Generated pressure is deterministic"), GameInstance->GetCurrentEnvironment().PressureKPa, Expected.PressureKPa);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonNewGameLogbookTest,
	"Astraeon.Session.NewGame.InitialLogbook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonNewGameLogbookTest::RunTest(const FString& Parameters)
{
	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	GameInstance->StartNewGame(123);

	TestEqual(TEXT("New game creates one initial logbook entry"), GameInstance->GetRuntimeLogbookEntries().Num(), 1);
	TestEqual(TEXT("Initial entry certainty is measured"), GameInstance->GetRuntimeLogbookEntries()[0].Certainty, EAstraeonDiscoveryCertainty::Measured);
	TestEqual(TEXT("Default seed normalizes zero input"), UAstraeonGameInstance::NormalizeRequestedSeed(0), 1001);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonSuitGravityMobilityTest,
	"Astraeon.Survival.Suit.GravityMobility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonSuitGravityMobilityTest::RunTest(const FString& Parameters)
{
	const float LowGravityMultiplier = UAstraeonSuitComponent::ComputeGravitySpeedMultiplier(3.2f);
	const float EarthGravityMultiplier = UAstraeonSuitComponent::ComputeGravitySpeedMultiplier(9.81f);
	const float HighGravityMultiplier = UAstraeonSuitComponent::ComputeGravitySpeedMultiplier(15.2f);

	TestTrue(TEXT("Low gravity improves mobility within cap"), LowGravityMultiplier > EarthGravityMultiplier && LowGravityMultiplier <= 1.18f);
	TestTrue(TEXT("Earth gravity is neutral"), FMath::IsNearlyEqual(EarthGravityMultiplier, 1.0f));
	TestTrue(TEXT("High gravity reduces mobility within cap"), HighGravityMultiplier < EarthGravityMultiplier && HighGravityMultiplier >= 0.62f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonSuitHazardDamageTest,
	"Astraeon.Survival.Suit.HazardDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonSuitHazardDamageTest::RunTest(const FString& Parameters)
{
	UAstraeonSuitComponent* Suit = NewObject<UAstraeonSuitComponent>();
	Suit->ApplyHazardDamage(12.5f);
	TestEqual(TEXT("Direct hazard damage reduces health"), Suit->GetHealthPercent(), 87.5f);
	Suit->ApplyHazardDamage(-100.0f);
	TestEqual(TEXT("Negative direct damage is ignored"), Suit->GetHealthPercent(), 87.5f);
	Suit->ApplyHazardDamage(1000.0f);
	TestEqual(TEXT("Direct hazard damage clamps at zero"), Suit->GetHealthPercent(), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonSuitOxygenAndDamageTest,
	"Astraeon.Survival.Suit.OxygenAndDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonSuitOxygenAndDamageTest::RunTest(const FString& Parameters)
{
	FAstraeonEnvironmentalSnapshot SafeEnvironment;
	SafeEnvironment.bBreathable = true;
	SafeEnvironment.PressureKPa = 101.325f;
	SafeEnvironment.TemperatureKelvin = 293.15f;

	FAstraeonEnvironmentalSnapshot HostileEnvironment = SafeEnvironment;
	HostileEnvironment.bBreathable = false;
	HostileEnvironment.PressureKPa = 12.0f;
	HostileEnvironment.TemperatureKelvin = 225.0f;

	TestTrue(TEXT("Sealed suit consumes more oxygen in non-breathable atmosphere"),
		UAstraeonSuitComponent::ComputeOxygenConsumptionPercentPerSecond(HostileEnvironment) > UAstraeonSuitComponent::ComputeOxygenConsumptionPercentPerSecond(SafeEnvironment));
	TestEqual(TEXT("Safe environment causes no immediate damage"), UAstraeonSuitComponent::ComputeEnvironmentalDamagePercentPerSecond(SafeEnvironment), 0.0f);
	TestTrue(TEXT("Hostile pressure/temperature causes damage"), UAstraeonSuitComponent::ComputeEnvironmentalDamagePercentPerSecond(HostileEnvironment) > 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonEnvironmentScanUpdatesLogbookTest,
	"Astraeon.Scanning.Environment.UpdatesLogbook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonEnvironmentScanUpdatesLogbookTest::RunTest(const FString& Parameters)
{
	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	TestFalse(TEXT("Scan fails before a session exists"), GameInstance->ScanCurrentEnvironment());

	GameInstance->StartNewGame(321);
	TestEqual(TEXT("Initial entry starts measured"), GameInstance->GetRuntimeLogbookEntries()[0].Certainty, EAstraeonDiscoveryCertainty::Measured);
	TestTrue(TEXT("Scan succeeds during an active session"), GameInstance->ScanCurrentEnvironment());
	TestEqual(TEXT("Scan keeps one environment entry"), GameInstance->GetRuntimeLogbookEntries().Num(), 1);
	TestEqual(TEXT("Scan confirms the environment entry"), GameInstance->GetRuntimeLogbookEntries()[0].Certainty, EAstraeonDiscoveryCertainty::Confirmed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonSaveLoadRoundTripTest,
	"Astraeon.Persistence.SaveGame.RoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonSaveLoadRoundTripTest::RunTest(const FString& Parameters)
{
	const FString SlotName(TEXT("AstraeonAutomationRoundTrip"));
	UGameplayStatics::DeleteGameInSlot(SlotName, 0);

	UAstraeonGameInstance* SourceGame = NewObject<UAstraeonGameInstance>();
	SourceGame->StartNewGame(9001);

	TestTrue(TEXT("Started game can be saved"), SourceGame->SaveCurrentGame(SlotName, 0));

	UAstraeonGameInstance* LoadedGame = NewObject<UAstraeonGameInstance>();
	TestTrue(TEXT("Saved game can be loaded"), LoadedGame->LoadSavedGame(SlotName, 0));
	TestEqual(TEXT("Loaded seed matches source"), LoadedGame->GetCurrentWorldSeed(), SourceGame->GetCurrentWorldSeed());
	TestEqual(TEXT("Loaded logbook count matches source"), LoadedGame->GetRuntimeLogbookEntries().Num(), SourceGame->GetRuntimeLogbookEntries().Num());
	TestEqual(TEXT("Loaded environment pressure matches source"), LoadedGame->GetCurrentEnvironment().PressureKPa, SourceGame->GetCurrentEnvironment().PressureKPa);
	TestEqual(TEXT("Loaded region resource count matches source"), LoadedGame->GetCurrentRegionLayout().Resources.Num(), SourceGame->GetCurrentRegionLayout().Resources.Num());
	TestEqual(TEXT("Loaded signal location matches source"), LoadedGame->GetCurrentRegionLayout().PointsOfInterest[0].LocationMeters, SourceGame->GetCurrentRegionLayout().PointsOfInterest[0].LocationMeters);
	SourceGame->AddInventoryItem(TEXT("silicate_fiber"), 2);
	SourceGame->AddInventoryItem(TEXT("signal_resonator"), 1);
	TestTrue(TEXT("Started game can be saved after inventory update"), SourceGame->SaveCurrentGame(SlotName, 0));
	TestTrue(TEXT("Updated saved game can be loaded"), LoadedGame->LoadSavedGame(SlotName, 0));
	TestEqual(TEXT("Loaded revealed map count matches source"), LoadedGame->GetRevealedMap().RevealedCells.Num(), SourceGame->GetRevealedMap().RevealedCells.Num());
	TestEqual(TEXT("Loaded inventory matches source"), LoadedGame->GetInventoryItemCount(TEXT("silicate_fiber")), 2);
	TestEqual(TEXT("Loaded crafted item matches source"), LoadedGame->GetInventoryItemCount(TEXT("signal_resonator")), 1);
	TestEqual(TEXT("Loaded objective matches source"), LoadedGame->GetObjectiveState(), SourceGame->GetObjectiveState());

	UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonLogbookUpsertTest,
	"Astraeon.Knowledge.Logbook.Upsert",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonLogbookUpsertTest::RunTest(const FString& Parameters)
{
	UAstraeonLogbookComponent* Logbook = NewObject<UAstraeonLogbookComponent>();
	FAstraeonLogbookEntry Entry;
	Entry.EntryId = TEXT("environment.first_scan");
	Entry.Title = FText::FromString(TEXT("First Scan"));
	Entry.Summary = FText::FromString(TEXT("ARGOS records the first environmental measurement."));
	Entry.Certainty = EAstraeonDiscoveryCertainty::Measured;

	Logbook->UpsertEntry(Entry);
	TestTrue(TEXT("Entry exists after insert"), Logbook->HasEntry(Entry.EntryId));
	TestEqual(TEXT("One entry after insert"), Logbook->GetEntries().Num(), 1);

	Entry.Certainty = EAstraeonDiscoveryCertainty::Confirmed;
	Logbook->UpsertEntry(Entry);
	TestEqual(TEXT("Upsert replaces instead of duplicating"), Logbook->GetEntries().Num(), 1);
	TestEqual(TEXT("Upsert updates certainty"), Logbook->GetEntries()[0].Certainty, EAstraeonDiscoveryCertainty::Confirmed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonSaveGameShapeTest,
	"Astraeon.Persistence.SaveGame.Shape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonSaveGameShapeTest::RunTest(const FString& Parameters)
{
	UAstraeonSaveGame* SaveGame = NewObject<UAstraeonSaveGame>();
	SaveGame->WorldSeed = 777;
	SaveGame->GeneratorVersion = UAstraeonWorldGenerator::CurrentGeneratorVersion;
	SaveGame->Environment = UAstraeonWorldGenerator::GenerateEnvironment(SaveGame->WorldSeed);
	SaveGame->Inventory.Add(TEXT("silicate_fiber"), 2);

	FAstraeonLogbookEntry Entry;
	Entry.EntryId = TEXT("signal.origin");
	Entry.Certainty = EAstraeonDiscoveryCertainty::Observed;
	SaveGame->LogbookEntries.Add(Entry);

	TestEqual(TEXT("Seed is stored"), SaveGame->WorldSeed, 777);
	TestEqual(TEXT("Inventory count is stored"), SaveGame->Inventory[TEXT("silicate_fiber")], 2);
	TestEqual(TEXT("Logbook entries are stored"), SaveGame->LogbookEntries.Num(), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
