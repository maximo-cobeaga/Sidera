#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AstraeonGameInstance.h"
#include "AstraeonGameModeBase.h"
#include "AstraeonHUD.h"
#include "AstraeonPlayerCharacter.h"
#include "AstraeonPlayerController.h"
#include "Creatures/AstraeonCreatureActor.h"
#include "Building/AstraeonBuiltStructure.h"
#include "WorldGen/AstraeonTerrainField.h"
#include "Exploration/AstraeonMapRevealLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Knowledge/AstraeonLogbookComponent.h"
#include "Survival/AstraeonSuitComponent.h"
#include "Persistence/AstraeonSaveGame.h"
#include "WorldGen/AstraeonRegionMaterializer.h"
#include "WorldGen/AstraeonRegionMarker.h"
#include "WorldGen/AstraeonWorldGenerator.h"
#include "WorldGen/AstraeonWorldProfiles.h"

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
		// Los tres primeros recursos son el recorrido crítico y van siempre en ese orden:
		// el resto son vetas profundas opcionales que exigen herramienta.
		TestTrue(TEXT("Layout keeps the three critical path resources"), Layout.Resources.Num() >= 3);
		// Antes eran tres POIs fijos. Desde que la región reparte varios puntos de aparición
		// de criatura, el número dejó de ser la invariante: lo que se conserva es la
		// composición, una señal y una anomalía más los puntos de aparición.
		int32 SignalCount = 0;
		int32 AnomalyCount = 0;
		int32 CreatureSpawnCount = 0;
		for (const FAstraeonPointOfInterest& Point : Layout.PointsOfInterest)
		{
			SignalCount += Point.Type == EAstraeonPointOfInterestType::SignalSource ? 1 : 0;
			AnomalyCount += Point.Type == EAstraeonPointOfInterestType::MinorAnomaly ? 1 : 0;
			CreatureSpawnCount += Point.Type == EAstraeonPointOfInterestType::CreatureSpawn ? 1 : 0;
		}
		TestEqual(TEXT("Exactly one signal source"), SignalCount, 1);
		TestEqual(TEXT("Exactly one minor anomaly"), AnomalyCount, 1);
		TestTrue(TEXT("At least one creature spawn point"), CreatureSpawnCount >= 1);
		TestEqual(TEXT("Every POI is one of the three known kinds"),
			SignalCount + AnomalyCount + CreatureSpawnCount, Layout.PointsOfInterest.Num());
		TestTrue(TEXT("First resource is silicate fiber"), Layout.Resources[0].ResourceId == TEXT("silicate_fiber"));
		TestTrue(TEXT("Second resource is ferrite nodule"), Layout.Resources[1].ResourceId == TEXT("ferrite_nodule"));
		TestTrue(TEXT("Third resource is seed signature"), Layout.Resources[2].bSeedSignature);
		for (int32 Index = 0; Index < 3; ++Index)
		{
			TestTrue(TEXT("Critical path resources need no tool"), Layout.Resources[Index].RequiredToolId.IsNone());
		}
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
	// Los POIs van después de todos los recursos, cuya cantidad ahora depende de cuántas
	// vetas profundas genere la seed.
	const int32 FirstPointOfInterestIndex = Layout.Resources.Num();
	TestEqual(TEXT("Signal source spec follows the resources"), Specs[FirstPointOfInterestIndex].ActorId, FName(TEXT("signal_source")));
	TestEqual(TEXT("Signal source is POI kind"), Specs[FirstPointOfInterestIndex].Kind, EAstraeonRegionActorKind::PointOfInterest);
	TestTrue(TEXT("Generated actor location uses centimeters"), FMath::Abs(Specs[0].LocationCm.X) > FMath::Abs(Layout.Resources[0].LocationMeters.X));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonItacaMaterializerSpecsTest,
	"Astraeon.WorldGen.Itaca.MaterializerSpecs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonItacaMaterializerSpecsTest::RunTest(const FString& Parameters)
{
	const TArray<FAstraeonRegionActorSpec> Specs = UAstraeonRegionMaterializer::BuildItacaActorSpecs();

	TestEqual(TEXT("Ítaca has ARGOS, pilot console, fabricator and surface hatch"), Specs.Num(), 4);
	TestEqual(TEXT("First Ítaca actor is ARGOS console"), Specs[0].ActorId, FName(TEXT("itaca_argos_console")));
	TestEqual(TEXT("Second Ítaca actor is the pilot console"), Specs[1].ActorId, FName(TEXT("itaca_pilot_console")));
	TestEqual(TEXT("Third Ítaca actor is the fabricator"), Specs[2].ActorId, FName(TEXT("itaca_fabricator")));
	TestEqual(TEXT("Fourth Ítaca actor is surface hatch"), Specs[3].ActorId, FName(TEXT("itaca_surface_hatch")));
	TestEqual(TEXT("Surface hatch is interactable POI"), Specs[3].Kind, EAstraeonRegionActorKind::PointOfInterest);
	TestTrue(TEXT("Surface hatch has a forgiving temporary hitbox height"), Specs[3].Scale.Z >= 1.0f);
	const FVector DeploymentLocationCm = UAstraeonRegionMaterializer::GetSurfaceDeploymentLocationCm();
	TestTrue(TEXT("Surface deployment target is above walkable runtime surface"), DeploymentLocationCm.Z >= 120.0f);
	// Desplegar deja al jugador afuera de la escotilla, no en una plataforma lejana.
	TestTrue(TEXT("Surface deployment target is outside the hatch"), DeploymentLocationCm.X > Specs[3].LocationCm.X);
	TestTrue(TEXT("Surface deployment target stays beside the ship"), FVector2D(DeploymentLocationCm.X, DeploymentLocationCm.Y).Size() < 1500.0f);

	// Ítaca es la nave: al aterrizar en otro punto, toda la estancia se remateraliza allí
	// en vez de quedar clavada en el origen del mundo.
	const FVector LandedOrigin(4000.0f, -2500.0f, 0.0f);
	const TArray<FAstraeonRegionActorSpec> RelocatedSpecs = UAstraeonRegionMaterializer::BuildItacaActorSpecs(LandedOrigin);
	TestEqual(TEXT("Relocating Ítaca keeps every station"), RelocatedSpecs.Num(), Specs.Num());
	for (int32 Index = 0; Index < RelocatedSpecs.Num(); ++Index)
	{
		TestEqual(TEXT("Station keeps its identity after relocating"), RelocatedSpecs[Index].ActorId, Specs[Index].ActorId);
		TestEqual(TEXT("Station moves rigidly with the ship"), RelocatedSpecs[Index].LocationCm, Specs[Index].LocationCm + LandedOrigin);
	}

	// El destino de la ESCOTILLA cuelga de la nave. Cuando era absoluto, desplegar después
	// de aterrizar lejos teletransportaba al jugador de vuelta cerca del origen del mundo.
	const FVector RelocatedDeployment = UAstraeonRegionMaterializer::GetSurfaceDeploymentLocationCm(LandedOrigin);
	TestEqual(TEXT("Deployment target follows the ship"), RelocatedDeployment, DeploymentLocationCm + LandedOrigin);
	const FVector RelocatedHatch = RelocatedSpecs[3].LocationCm;
	TestTrue(TEXT("Deployment target stays near the relocated hatch"),
		FVector2D(RelocatedDeployment.X - RelocatedHatch.X, RelocatedDeployment.Y - RelocatedHatch.Y).Size()
		< FVector2D(RelocatedDeployment.X, RelocatedDeployment.Y).Size());
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

	// El id real que produce UAstraeonWorldGenerator es minor_geologic_anomaly. Cuando la
	// etiqueta se comparaba contra minor_anomaly, el jugador veía el id crudo en blanco.
	const FAstraeonRegionLayout AnomalyLayout = UAstraeonWorldGenerator::GenerateRegionLayout(777);
	const FAstraeonPointOfInterest* AnomalyPoint = AnomalyLayout.PointsOfInterest.FindByPredicate([](const FAstraeonPointOfInterest& Point)
	{
		return Point.Type == EAstraeonPointOfInterestType::MinorAnomaly;
	});
	TestNotNull(TEXT("Generated layout contains a minor anomaly"), AnomalyPoint);
	if (AnomalyPoint)
	{
		TestTrue(TEXT("Anomaly marker label is readable"), AAstraeonRegionMarker::BuildMarkerLabel(AnomalyPoint->PointId, EAstraeonRegionActorKind::PointOfInterest).ToString().Contains(TEXT("ANOMALÍA")));
		TestNotEqual(TEXT("Anomaly marker has a dedicated color"), AAstraeonRegionMarker::BuildMarkerColor(AnomalyPoint->PointId, EAstraeonRegionActorKind::PointOfInterest), FColor::White);
	}
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

	// El estado ya no se comunica escalando la malla: cada estado tiene su propio clip.
	// Lo que sí sigue siendo un contrato es que patrullar alterne pastar y caminar, o el
	// animal vuelve a leerse como un carrusel que nunca se detiene.
	TestTrue(TEXT("Patrol starts by grazing"), AAstraeonCreatureActor::IsGrazingAtPhase(0.0f));
	TestFalse(TEXT("Patrol then walks"), AAstraeonCreatureActor::IsGrazingAtPhase(6.0f));
	TestTrue(TEXT("The grazing cycle repeats"), AAstraeonCreatureActor::IsGrazingAtPhase(21.0f));
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
	TestTrue(TEXT("HUD identifies content variation"), Lines.ContainsByPredicate([](const FString& Line) { return Line.Contains(TEXT("Variación: 5150")); }));
	TestTrue(TEXT("HUD identifies the fixed Region A profile"), Lines.ContainsByPredicate([](const FString& Line) { return Line.Contains(TEXT("Región: region_first_signal_basin")); }));
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
	TestEqual(TEXT("Requested content seed is stored"), GameInstance->GetCurrentContentSeed(), 424242);
	TestEqual(TEXT("New game selects the fixed MVP planet"), GameInstance->GetCurrentPlanetProfileId(), UAstraeonWorldProfiles::GetMvpPlanetProfileId());
	TestEqual(TEXT("New game selects the fixed Region A"), GameInstance->GetCurrentRegionProfileId(), UAstraeonWorldProfiles::GetRegionAProfileId());

	const FAstraeonEnvironmentalSnapshot Expected = UAstraeonWorldProfiles::GetMvpPlanetProfile().Environment;
	TestEqual(TEXT("Environment retains content seed only as metadata"), GameInstance->GetCurrentEnvironment().WorldSeed, 424242);
	TestEqual(TEXT("Authored pressure does not derive from content seed"), GameInstance->GetCurrentEnvironment().PressureKPa, Expected.PressureKPa);
	TestEqual(TEXT("Terrain seed follows the selected content seed"), GameInstance->GetCurrentTerrainSeed(), 424242);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonFabricatorRecipesTest,
	"Astraeon.Crafting.Fabricator.Recipes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonFabricatorRecipesTest::RunTest(const FString& Parameters)
{
	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	GameInstance->StartNewGame(2024);

	const TArray<FAstraeonCraftingRecipe> Recipes = GameInstance->GetCraftingRecipes();
	// Se afirma que las recetas imprescindibles existen, no cuántas hay: el catálogo crece
	// con cada sistema nuevo y un número fijo obliga a tocar el test sin motivo.
	auto HasRecipe = [&Recipes](const TCHAR* RecipeId)
	{
		return Recipes.ContainsByPredicate([RecipeId](const FAstraeonCraftingRecipe& Recipe)
		{
			return Recipe.RecipeId == FName(RecipeId);
		});
	};

	TestTrue(TEXT("The resonator is craftable"), HasRecipe(TEXT("signal_resonator")));
	TestTrue(TEXT("The three protection modules are craftable"),
		HasRecipe(TEXT("module_respirator")) && HasRecipe(TEXT("module_thermal_shield")) && HasRecipe(TEXT("module_pressure_seal")));
	TestTrue(TEXT("Weapon and drill are craftable"),
		HasRecipe(TEXT("weapon_pulse_cutter")) && HasRecipe(TEXT("tool_core_drill")));
	TestTrue(TEXT("The building chain is craftable"),
		HasRecipe(TEXT("regolith_brick")) && HasRecipe(TEXT("tool_build_hammer")) && HasRecipe(TEXT("tool_demolition_maul")));

	// Sin recursos no se puede fabricar nada.
	FString Reason;
	TestFalse(TEXT("Empty inventory cannot craft"), GameInstance->CanCraftRecipe(Recipes[0], Reason));
	TestFalse(TEXT("A refusal always explains itself"), Reason.IsEmpty());

	const FName SignatureResourceId = GameInstance->GetCurrentRegionLayout().Resources[2].ResourceId;
	GameInstance->AddInventoryItem(TEXT("silicate_fiber"), 1);
	GameInstance->AddInventoryItem(TEXT("ferrite_nodule"), 1);
	GameInstance->AddInventoryItem(SignatureResourceId, 1);

	// Con lo justo para el resonador, la mesa protege el recorrido crítico y no deja
	// gastar esos insumos en accesorios.
	const FAstraeonCraftingRecipe* Respirator = Recipes.FindByPredicate([](const FAstraeonCraftingRecipe& Recipe)
	{
		return Recipe.RecipeId == TEXT("module_respirator");
	});
	TestNotNull(TEXT("Respirator recipe exists"), Respirator);
	if (Respirator)
	{
		TestFalse(TEXT("Resonator inputs are reserved"), GameInstance->CanCraftRecipe(*Respirator, Reason));
		TestTrue(TEXT("The refusal names the reservation"), Reason.Contains(TEXT("reservado")));
		TestFalse(TEXT("Crafting a reserved accessory is refused"), GameInstance->CraftRecipe(TEXT("module_respirator")));
	}

	// El resonador sí puede fabricarse, y libera la reserva.
	TestTrue(TEXT("Resonator can be crafted"), GameInstance->CraftRecipe(TEXT("signal_resonator")));
	TestTrue(TEXT("Resonator is in the inventory"), GameInstance->GetInventoryItemCount(TEXT("signal_resonator")) > 0);

	GameInstance->AddInventoryItem(TEXT("silicate_fiber"), 1);
	TestTrue(TEXT("Accessories are craftable once the resonator exists"), GameInstance->CraftRecipe(TEXT("module_respirator")));
	TestTrue(TEXT("Crafted module lands in the inventory"),
		GameInstance->GetInventoryItemCount(UAstraeonGameInstance::GetProtectionItemId(EAstraeonProtectionModule::Respirator)) > 0);

	// Equipar exige haberlo fabricado.
	TestTrue(TEXT("A crafted module can be equipped"), GameInstance->EquipProtection(EAstraeonProtectionModule::Respirator));
	TestFalse(TEXT("A module that was never built cannot be equipped"), GameInstance->EquipProtection(EAstraeonProtectionModule::PressureSeal));
	TestEqual(TEXT("A refused equip keeps the previous module"), GameInstance->GetEquippedProtection(), EAstraeonProtectionModule::Respirator);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonHandAndHungerTest,
	"Astraeon.Survival.Hand.HotbarAndHunger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonHandAndHungerTest::RunTest(const FString& Parameters)
{
	const FString SlotName(TEXT("AstraeonAutomationHand"));
	UGameplayStatics::DeleteGameInSlot(SlotName, 0);

	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	GameInstance->StartNewGame(8899);

	TestEqual(TEXT("A new expedition starts fed"), GameInstance->GetHungerPercent(), 100.0f);
	TestFalse(TEXT("A fed survivor is not starving"), GameInstance->IsStarving());
	TestTrue(TEXT("Hands start empty"), GameInstance->GetHandItemId().IsNone());
	TestTrue(TEXT("An empty inventory has no hotbar"), GameInstance->GetHotbarItems().IsEmpty());

	// Sólo herramientas, armas y comida se llevan en la mano; la materia prima no.
	TestTrue(TEXT("Tools are handheld"), UAstraeonGameInstance::IsHandheldItem(TEXT("tool_core_drill")));
	TestTrue(TEXT("Weapons are handheld"), UAstraeonGameInstance::IsHandheldItem(TEXT("weapon_pulse_cutter")));
	TestTrue(TEXT("Rations are handheld"), UAstraeonGameInstance::IsHandheldItem(UAstraeonGameInstance::GetRationItemId()));
	TestFalse(TEXT("Raw materials are not handheld"), UAstraeonGameInstance::IsHandheldItem(TEXT("silicate_fiber")));

	GameInstance->AddInventoryItem(TEXT("tool_core_drill"), 1);
	GameInstance->AddInventoryItem(TEXT("silicate_fiber"), 5);
	TestEqual(TEXT("Only handheld items reach the hotbar"), GameInstance->GetHotbarItems().Num(), 1);

	TestFalse(TEXT("An empty slot cannot be selected"), GameInstance->SelectHotbarSlot(3));
	TestTrue(TEXT("A filled slot can be selected"), GameInstance->SelectHotbarSlot(0));
	TestTrue(TEXT("The drill is now in hand"), GameInstance->IsHolding(TEXT("tool_core_drill")));
	TestFalse(TEXT("Holding one tool is not holding another"), GameInstance->IsHolding(TEXT("weapon_pulse_cutter")));

	// Extraer regolito exige el taladro EN LA MANO, no sólo en el inventario.
	TestTrue(TEXT("The held drill extracts regolith"), GameInstance->ExtractRegolith());
	GameInstance->SelectHotbarSlot(0);
	GameInstance->AddInventoryItem(UAstraeonGameInstance::GetRationItemId(), 2);
	const TArray<FName> Hotbar = GameInstance->GetHotbarItems();
	const int32 RationSlot = Hotbar.IndexOfByKey(UAstraeonGameInstance::GetRationItemId());
	TestTrue(TEXT("The ration has a hotbar slot"), RationSlot != INDEX_NONE);
	GameInstance->SelectHotbarSlot(RationSlot);
	TestFalse(TEXT("Holding rations is not holding the drill"), GameInstance->IsHolding(TEXT("tool_core_drill")));
	TestFalse(TEXT("The drill no longer extracts once stowed"), GameInstance->ExtractRegolith());

	// Comer: el bucle caza → biomasa → raciones → saciedad.
	GameInstance->ConsumeHunger(60.0f);
	const float HungryLevel = GameInstance->GetHungerPercent();
	TestTrue(TEXT("Hunger actually drops"), HungryLevel < 100.0f);
	TestTrue(TEXT("Eating consumes the ration in hand"), GameInstance->UseHandItem());
	TestTrue(TEXT("Eating restores satiety"), GameInstance->GetHungerPercent() > HungryLevel);
	TestEqual(TEXT("Eating spends one ration"), GameInstance->GetInventoryItemCount(UAstraeonGameInstance::GetRationItemId()), 1);

	// Inanición: llegar a cero debe ser un estado, no un número decorativo.
	GameInstance->ConsumeHunger(500.0f);
	TestEqual(TEXT("Hunger never goes negative"), GameInstance->GetHungerPercent(), 0.0f);
	TestTrue(TEXT("Empty hunger reads as starving"), GameInstance->IsStarving());
	TestTrue(TEXT("Starvation actually hurts"), UAstraeonSuitComponent::GetStarvationDamagePerSecond() > 0.0f);
	TestTrue(TEXT("Hunger drains over time"), UAstraeonSuitComponent::GetHungerLossPerSecond() > 0.0f);

	// Mano y saciedad sobreviven al guardado.
	GameInstance->SelectHotbarSlot(0);
	const FName SavedHandItem = GameInstance->GetHandItemId();
	TestTrue(TEXT("The session can be saved"), GameInstance->SaveCurrentGame(SlotName, 0));

	UAstraeonGameInstance* LoadedGame = NewObject<UAstraeonGameInstance>();
	TestTrue(TEXT("The session can be loaded"), LoadedGame->LoadSavedGame(SlotName, 0));
	TestEqual(TEXT("The held item survives save/load"), LoadedGame->GetHandItemId(), SavedHandItem);
	TestEqual(TEXT("Satiety survives save/load"), LoadedGame->GetHungerPercent(), 0.0f);

	UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonBuildingLoopTest,
	"Astraeon.Building.Loop.PlaceDemolishPersist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonBuildingLoopTest::RunTest(const FString& Parameters)
{
	const FString SlotName(TEXT("AstraeonAutomationBuilding"));
	UGameplayStatics::DeleteGameInSlot(SlotName, 0);

	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	GameInstance->StartNewGame(70707);
	TestEqual(TEXT("A new expedition has nothing built"), GameInstance->GetPlacedStructures().Num(), 0);

	// Extraer regolito exige el taladro, y además llevarlo EN LA MANO: tenerlo guardado no
	// alcanza desde que existe la barra rápida.
	TestFalse(TEXT("Regolith needs the drill"), GameInstance->ExtractRegolith());
	GameInstance->AddInventoryItem(TEXT("tool_core_drill"), 1);
	TestFalse(TEXT("A stowed drill does not extract"), GameInstance->ExtractRegolith());

	const int32 DrillSlot = GameInstance->GetHotbarItems().IndexOfByKey(FName(TEXT("tool_core_drill")));
	TestTrue(TEXT("The drill has a hotbar slot"), DrillSlot != INDEX_NONE);
	GameInstance->SelectHotbarSlot(DrillSlot);
	TestTrue(TEXT("The held drill extracts regolith"), GameInstance->ExtractRegolith());
	TestTrue(TEXT("Extraction yields regolith"), GameInstance->GetInventoryItemCount(UAstraeonGameInstance::GetRegolithItemId()) > 0);

	FAstraeonPlacedStructure Wall;
	Wall.Type = EAstraeonStructureType::Wall;
	Wall.LocationCm = FVector(1200.0f, 400.0f, 130.0f);
	Wall.Rotation = FRotator(0.0f, 45.0f, 0.0f);

	// Sin martillo no se construye, aunque sobren ladrillos.
	GameInstance->AddInventoryItem(UAstraeonGameInstance::GetBrickItemId(), 20);
	TestFalse(TEXT("Building needs the hammer"), GameInstance->CanPlaceStructure(EAstraeonStructureType::Wall));
	TestFalse(TEXT("Placement without a hammer is refused"), GameInstance->PlaceStructure(Wall));

	GameInstance->AddInventoryItem(UAstraeonGameInstance::GetBuildHammerItemId(), 1);
	TestTrue(TEXT("With hammer and bricks the wall can be placed"), GameInstance->CanPlaceStructure(EAstraeonStructureType::Wall));

	const int32 BricksBefore = GameInstance->GetInventoryItemCount(UAstraeonGameInstance::GetBrickItemId());
	TestTrue(TEXT("The wall is placed"), GameInstance->PlaceStructure(Wall));
	TestEqual(TEXT("The world remembers the structure"), GameInstance->GetPlacedStructures().Num(), 1);
	TestEqual(TEXT("Building spends bricks"),
		GameInstance->GetInventoryItemCount(UAstraeonGameInstance::GetBrickItemId()),
		BricksBefore - AAstraeonBuiltStructure::GetStructureBrickCost(EAstraeonStructureType::Wall));
	TestEqual(TEXT("Rotation is preserved"), static_cast<float>(GameInstance->GetPlacedStructures()[0].Rotation.Yaw), 45.0f);

	// Demoler exige la maza, y devuelve menos de lo que costó.
	TestFalse(TEXT("Demolition needs the maul"), GameInstance->DemolishStructureAt(Wall.LocationCm));
	TestEqual(TEXT("A refused demolition keeps the structure"), GameInstance->GetPlacedStructures().Num(), 1);

	GameInstance->AddInventoryItem(UAstraeonGameInstance::GetDemolitionMaulItemId(), 1);
	TestFalse(TEXT("Demolishing empty ground finds nothing"), GameInstance->DemolishStructureAt(FVector(90000.0f, 0.0f, 0.0f)));

	const int32 BricksBeforeDemolition = GameInstance->GetInventoryItemCount(UAstraeonGameInstance::GetBrickItemId());
	TestTrue(TEXT("The wall is demolished"), GameInstance->DemolishStructureAt(Wall.LocationCm));
	TestEqual(TEXT("The structure is gone"), GameInstance->GetPlacedStructures().Num(), 0);
	const int32 Refund = GameInstance->GetInventoryItemCount(UAstraeonGameInstance::GetBrickItemId()) - BricksBeforeDemolition;
	TestTrue(TEXT("Demolition refunds something"), Refund > 0);
	TestTrue(TEXT("Demolition refunds less than it cost"),
		Refund < AAstraeonBuiltStructure::GetStructureBrickCost(EAstraeonStructureType::Wall));

	// Lo construido sobrevive al guardado: la región se rehace en cada aterrizaje, así que
	// las obras no pueden vivir sólo como actores.
	TestTrue(TEXT("A second wall is placed"), GameInstance->PlaceStructure(Wall));
	TestTrue(TEXT("The session can be saved"), GameInstance->SaveCurrentGame(SlotName, 0));

	UAstraeonGameInstance* LoadedGame = NewObject<UAstraeonGameInstance>();
	TestTrue(TEXT("The session can be loaded"), LoadedGame->LoadSavedGame(SlotName, 0));
	TestEqual(TEXT("Structures survive save/load"), LoadedGame->GetPlacedStructures().Num(), 1);
	TestEqual(TEXT("The rebuilt structure keeps its place"), LoadedGame->GetPlacedStructures()[0].LocationCm, Wall.LocationCm);

	UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonEmergencyRecallTest,
	"Astraeon.Survival.Recall.CostsOptionalCargo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonEmergencyRecallTest::RunTest(const FString& Parameters)
{
	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	TestFalse(TEXT("There is no recall without a session"), GameInstance->RecordEmergencyRecall());

	GameInstance->StartNewGame(1979);
	const FName SignatureResourceId = GameInstance->GetCurrentRegionLayout().Resources[2].ResourceId;

	// Carga del recorrido crítico y equipo fabricado: deben sobrevivir.
	GameInstance->AddInventoryItem(TEXT("silicate_fiber"), 2);
	GameInstance->AddInventoryItem(TEXT("ferrite_nodule"), 2);
	GameInstance->AddInventoryItem(SignatureResourceId, 1);
	GameInstance->AddInventoryItem(UAstraeonGameInstance::GetPulseCutterItemId(), 1);
	GameInstance->AddInventoryItem(UAstraeonGameInstance::GetProtectionItemId(EAstraeonProtectionModule::Respirator), 1);
	GameInstance->AddInventoryItem(TEXT("tool_core_drill"), 1);

	// Carga opcional: es lo único que debe perderse.
	GameInstance->AddInventoryItem(TEXT("biomass_sample"), 4);
	GameInstance->AddInventoryItem(TEXT("cryo_ferrite_vein"), 3);

	TestTrue(TEXT("Recall is recorded during a session"), GameInstance->RecordEmergencyRecall());

	TestEqual(TEXT("Critical path silicate survives"), GameInstance->GetInventoryItemCount(TEXT("silicate_fiber")), 2);
	TestEqual(TEXT("Critical path ferrite survives"), GameInstance->GetInventoryItemCount(TEXT("ferrite_nodule")), 2);
	TestEqual(TEXT("Seed signature resource survives"), GameInstance->GetInventoryItemCount(SignatureResourceId), 1);
	TestEqual(TEXT("The weapon survives"), GameInstance->GetInventoryItemCount(UAstraeonGameInstance::GetPulseCutterItemId()), 1);
	TestEqual(TEXT("Protection modules survive"), GameInstance->GetInventoryItemCount(UAstraeonGameInstance::GetProtectionItemId(EAstraeonProtectionModule::Respirator)), 1);
	TestEqual(TEXT("Tools survive"), GameInstance->GetInventoryItemCount(TEXT("tool_core_drill")), 1);

	TestEqual(TEXT("Creature samples are lost"), GameInstance->GetInventoryItemCount(TEXT("biomass_sample")), 0);
	TestEqual(TEXT("Deep vein cargo is lost"), GameInstance->GetInventoryItemCount(TEXT("cryo_ferrite_vein")), 0);

	TestTrue(TEXT("Critical path resources are recognised"), GameInstance->IsCriticalPathResource(TEXT("silicate_fiber")));
	TestFalse(TEXT("Deep veins are not critical path"), GameInstance->IsCriticalPathResource(TEXT("cryo_ferrite_vein")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonSuitHavenTest,
	"Astraeon.Survival.Suit.HavenAndSuffocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonSuitHavenTest::RunTest(const FString& Parameters)
{
	UAstraeonSuitComponent* Suit = NewObject<UAstraeonSuitComponent>();

	TestFalse(TEXT("A fresh suit is not incapacitated"), Suit->IsIncapacitated());
	TestFalse(TEXT("A fresh suit starts outside the haven"), Suit->IsInHaven());
	TestTrue(TEXT("The haven actually recovers the suit"), UAstraeonSuitComponent::GetHavenRecoveryPercentPerSecond() > 0.0f);

	// Daño total: quedar a cero debe leerse como incapacitación, no como seguir jugando.
	Suit->ApplyHazardDamage(100.0f);
	TestTrue(TEXT("A drained suit reports incapacitation"), Suit->IsIncapacitated());
	TestEqual(TEXT("Health never goes negative"), Suit->GetHealthPercent(), 0.0f);

	Suit->RestoreAfterRecall();
	TestFalse(TEXT("A rescued survivor is back on their feet"), Suit->IsIncapacitated());
	TestTrue(TEXT("The rescue does not fully heal"), Suit->GetHealthPercent() < 100.0f);
	TestTrue(TEXT("The rescue restores breathable reserves"), Suit->GetOxygenPercent() > 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonWeaponKillsCreatureTest,
	"Astraeon.Combat.Weapon.KillsCreature",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonWeaponKillsCreatureTest::RunTest(const FString& Parameters)
{
	AAstraeonCreatureActor* Creature = NewObject<AAstraeonCreatureActor>();
	FAstraeonCreatureProfile Profile;
	Profile.MaxHealth = 100.0f;
	Profile.HarvestItemId = TEXT("biomass_sample");
	Profile.HarvestQuantity = 2;
	Creature->ConfigureCreature(Profile);

	TestEqual(TEXT("A configured creature starts at full health"), Creature->GetHealth(), 100.0f);
	TestFalse(TEXT("A healthy creature is not dead"), Creature->IsDead());

	TestFalse(TEXT("A non-lethal hit does not kill"), Creature->ApplyWeaponDamage(34.0f));
	TestTrue(TEXT("A wounded creature turns hostile"), Creature->GetAwarenessState() == EAstraeonCreatureAwarenessState::Threatening);
	TestFalse(TEXT("A second non-lethal hit does not kill"), Creature->ApplyWeaponDamage(34.0f));
	TestTrue(TEXT("The third hit kills"), Creature->ApplyWeaponDamage(34.0f));
	TestTrue(TEXT("A killed creature reads as dead"), Creature->IsDead());
	TestEqual(TEXT("Health never goes negative"), Creature->GetHealth(), 0.0f);
	TestFalse(TEXT("A corpse cannot be killed again"), Creature->ApplyWeaponDamage(34.0f));

	// Matar rinde muestra y entrada de bitácora, y exige sesión activa.
	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	TestFalse(TEXT("A kill outside a session is not recorded"), GameInstance->RecordCreatureKill(Profile));

	GameInstance->StartNewGame(606);
	TestFalse(TEXT("The cutter must be built before firing"), GameInstance->HasPulseCutter());
	TestTrue(TEXT("A kill is recorded during a session"), GameInstance->RecordCreatureKill(Profile));
	TestEqual(TEXT("The kill yields its harvest"), GameInstance->GetInventoryItemCount(TEXT("biomass_sample")), 2);

	GameInstance->AddInventoryItem(UAstraeonGameInstance::GetPulseCutterItemId(), 1);
	TestTrue(TEXT("Owning the cutter enables firing"), GameInstance->HasPulseCutter());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonTerrainReliefTest,
	"Astraeon.WorldGen.Terrain.Relief",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonTerrainReliefTest::RunTest(const FString& Parameters)
{
	// Determinismo: la altura es función pura de (seed, x, y), que es lo que permitirá
	// colocar cosas sobre el suelo antes de construirlo cuando existan planetas.
	TestEqual(TEXT("Terrain height is deterministic for a seed"),
		AAstraeonTerrainField::GetHeightCm(4242, 5000.0f, -3000.0f),
		AAstraeonTerrainField::GetHeightCm(4242, 5000.0f, -3000.0f));

	bool bAnyDifference = false;
	bool bAnyRelief = false;
	for (int32 Step = 0; Step < 64; ++Step)
	{
		const float SampleX = Step * 1700.0f;
		const float SampleY = Step * -900.0f;
		const float FirstSeed = AAstraeonTerrainField::GetHeightCm(11, SampleX, SampleY);
		const float SecondSeed = AAstraeonTerrainField::GetHeightCm(22, SampleX, SampleY);

		bAnyDifference = bAnyDifference || !FMath::IsNearlyEqual(FirstSeed, SecondSeed);
		bAnyRelief = bAnyRelief || FirstSeed > 200.0f;

		// Nunca por debajo de la placa de suelo: el relieve sólo sube.
		TestTrue(TEXT("Terrain never digs below the floor plate"), FirstSeed >= 0.0f);
	}

	TestTrue(TEXT("Different seeds produce different terrain"), bAnyDifference);
	TestTrue(TEXT("The seed produces actual relief, not a flat plain"), bAnyRelief);
	TestTrue(TEXT("Flat spots are wider than a tile so clearings stay usable"),
		AAstraeonTerrainField::GetFlatSpotRadiusCm() > AAstraeonTerrainField::GetTileSizeCm());

	// Invariante que se rompió una vez y dejó el mapa intransitable: el terreno es de
	// bloques, así que el desnivel entre tiles vecinos es un escalón vertical. El SUELO
	// debe respetarlo siempre; las montañas son barreras deliberadas y quedan exentas.
	const float TileSize = AAstraeonTerrainField::GetTileSizeCm();
	const float MaxStep = AAstraeonTerrainField::GetMaxWalkableStepCm();
	float WorstGroundStepCm = 0.0f;
	float TallestMountainCm = 0.0f;
	for (int32 Seed = 1; Seed <= 6; ++Seed)
	{
		for (int32 TileX = -40; TileX <= 40; ++TileX)
		{
			for (int32 TileY = -40; TileY <= 40; ++TileY)
			{
				const float X = TileX * TileSize;
				const float Y = TileY * TileSize;
				const float Here = AAstraeonTerrainField::GetGroundHeightCm(Seed, X, Y);
				WorstGroundStepCm = FMath::Max(WorstGroundStepCm, FMath::Abs(AAstraeonTerrainField::GetGroundHeightCm(Seed, X + TileSize, Y) - Here));
				WorstGroundStepCm = FMath::Max(WorstGroundStepCm, FMath::Abs(AAstraeonTerrainField::GetGroundHeightCm(Seed, X, Y + TileSize) - Here));
				TallestMountainCm = FMath::Max(TallestMountainCm, AAstraeonTerrainField::GetMountainHeightCm(Seed, X, Y));
			}
		}
	}

	TestTrue(FString::Printf(TEXT("The walkable ground stays walkable (worst step %.0f cm, limit %.0f cm)"), WorstGroundStepCm, MaxStep),
		WorstGroundStepCm <= MaxStep);

	// El propietario pidió montañas de verdad: si la capa nunca supera una loma, el
	// paisaje volvió a ser plano y el trabajo no cumple su propósito.
	TestTrue(FString::Printf(TEXT("Mountains actually rise (tallest %.0f cm)"), TallestMountainCm),
		TallestMountainCm > 2000.0f);

	// El llano de Ítaca nivela contra la altura LOCAL, no excava hasta cero: cavar dejaba un
	// cráter enorme alrededor de la nave y exigía un radio gigante para ser caminable.
	const FVector2D ItacaSpot(12000.0f, -8000.0f);
	const TArray<FVector2D> FlatSpots = { ItacaSpot };
	const float ClearanceRadius = AAstraeonTerrainField::GetFlatSpotRadiusCm();
	float WorstClearanceStepCm = 0.0f;
	for (int32 Seed = 1; Seed <= 6; ++Seed)
	{
		for (float Offset = 0.0f; Offset <= ClearanceRadius * 2.0f; Offset += TileSize)
		{
			const FVector2D Here(ItacaSpot.X + Offset, ItacaSpot.Y);
			const FVector2D Next(ItacaSpot.X + Offset + TileSize, ItacaSpot.Y);
			WorstClearanceStepCm = FMath::Max(WorstClearanceStepCm, FMath::Abs(
				AAstraeonTerrainField::GetClearedGroundHeightCm(Seed, Next, FlatSpots)
				- AAstraeonTerrainField::GetClearedGroundHeightCm(Seed, Here, FlatSpots)));
		}
	}

	TestTrue(FString::Printf(TEXT("The Ítaca clearing stays walkable (worst step %.0f cm)"), WorstClearanceStepCm),
		WorstClearanceStepCm <= MaxStep);

	// La meseta queda a la altura del suelo local, no en cero: si se hundiera hasta Z=0
	// volvería el cráter que se veía al despegar.
	const float PlateauHeight = AAstraeonTerrainField::GetClearedGroundHeightCm(4242, ItacaSpot, FlatSpots);
	TestEqual(TEXT("The clearing levels to the local ground, not to zero"),
		PlateauHeight, AAstraeonTerrainField::GetGroundHeightCm(4242, ItacaSpot.X, ItacaSpot.Y));
	TestEqual(TEXT("Terrain far from a clearing is untouched"),
		AAstraeonTerrainField::GetClearedGroundHeightCm(4242, FVector2D(ItacaSpot.X + ClearanceRadius * 3.0f, ItacaSpot.Y), FlatSpots),
		AAstraeonTerrainField::GetGroundHeightCm(4242, ItacaSpot.X + ClearanceRadius * 3.0f, ItacaSpot.Y));

	// La exclusión de montañas se comprueba por radio explícito. Deducirla de la rampa
	// suave la dejó sin efecto y una montaña acabó tapando la escotilla de la nave.
	TestTrue(TEXT("A gameplay point is inside its own keep-out"),
		AAstraeonTerrainField::IsWithinAnySpot(ItacaSpot, FlatSpots, AAstraeonTerrainField::GetMountainClearanceCm()));
	TestFalse(TEXT("Far terrain is outside the keep-out"),
		AAstraeonTerrainField::IsWithinAnySpot(FVector2D(ItacaSpot.X + 40000.0f, ItacaSpot.Y), FlatSpots, AAstraeonTerrainField::GetMountainClearanceCm()));
	TestTrue(TEXT("Mountains are pushed further from gameplay than the ground clearing"),
		AAstraeonTerrainField::GetMountainClearanceCm() > ClearanceRadius);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonToolGatedResourcesTest,
	"Astraeon.Resources.Nodes.ToolGated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonToolGatedResourcesTest::RunTest(const FString& Parameters)
{
	const FAstraeonRegionLayout Layout = UAstraeonWorldGenerator::GenerateRegionLayout(8080);

	int32 HandCollectable = 0;
	int32 ToolGated = 0;
	for (const FAstraeonResourceNode& Node : Layout.Resources)
	{
		if (Node.RequiredToolId.IsNone())
		{
			++HandCollectable;
		}
		else
		{
			++ToolGated;
			TestEqual(TEXT("Deep veins ask for the core drill"), Node.RequiredToolId, FName(TEXT("tool_core_drill")));
		}
	}

	// Los tres recursos del recorrido crítico siguen siendo recolectables a mano: la
	// herramienta abre contenido nuevo, no bloquea el que ya existía.
	TestEqual(TEXT("The critical path resources stay hand collectable"), HandCollectable, 3);
	TestTrue(TEXT("The region offers tool gated veins"), ToolGated > 0);

	// El gating viaja hasta el marcador, que es quien lo hace cumplir en juego.
	const TArray<FAstraeonRegionActorSpec> Specs = UAstraeonRegionMaterializer::BuildActorSpecs(Layout);
	int32 GatedSpecs = 0;
	for (const FAstraeonRegionActorSpec& Spec : Specs)
	{
		if (!Spec.RequiredToolId.IsNone())
		{
			++GatedSpecs;
		}
	}
	TestEqual(TEXT("Tool requirements reach the spawned markers"), GatedSpecs, ToolGated);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonResourceNodeYieldTest,
	"Astraeon.Resources.Nodes.DeclaredYield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonResourceNodeYieldTest::RunTest(const FString& Parameters)
{
	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	GameInstance->StartNewGame(31337);

	// El generador declara cuánto rinde cada veta; recolectar debe honrarlo en vez de
	// entregar siempre una unidad.
	for (const FAstraeonResourceNode& Node : GameInstance->GetCurrentRegionLayout().Resources)
	{
		TestEqual(TEXT("Node yield matches the generated layout"), GameInstance->GetResourceNodeQuantity(Node.ResourceId), Node.Quantity);
		TestTrue(TEXT("Every node yields at least one unit"), Node.Quantity >= 1);
	}

	TestEqual(TEXT("Unknown resources fall back to a single unit"), GameInstance->GetResourceNodeQuantity(TEXT("not_a_resource")), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonProtectionMitigatesHazardsTest,
	"Astraeon.Survival.Protection.MitigatesHazards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonProtectionMitigatesHazardsTest::RunTest(const FString& Parameters)
{
	FAstraeonEnvironmentalSnapshot FrozenThinAir;
	FrozenThinAir.TemperatureKelvin = 180.0f;
	FrozenThinAir.PressureKPa = 5.0f;
	FrozenThinAir.bBreathable = false;

	const float Unprotected = UAstraeonSuitComponent::ComputeEnvironmentalDamagePercentPerSecond(FrozenThinAir, EAstraeonProtectionModule::None);
	const float WithSeal = UAstraeonSuitComponent::ComputeEnvironmentalDamagePercentPerSecond(FrozenThinAir, EAstraeonProtectionModule::PressureSeal);
	const float WithShield = UAstraeonSuitComponent::ComputeEnvironmentalDamagePercentPerSecond(FrozenThinAir, EAstraeonProtectionModule::ThermalShield);

	TestTrue(TEXT("Pressure seal reduces damage"), WithSeal < Unprotected);
	TestTrue(TEXT("Thermal shield reduces damage"), WithShield < Unprotected);
	TestTrue(TEXT("No module removes every hazard at once"), WithSeal > 0.0f && WithShield > 0.0f);
	TestTrue(TEXT("Pressure is the costlier hazard to leave uncovered"), WithSeal < WithShield);

	// El respirador actúa sobre el oxígeno, no sobre el daño ambiental.
	TestEqual(TEXT("Respirator does not change environmental damage"),
		UAstraeonSuitComponent::ComputeEnvironmentalDamagePercentPerSecond(FrozenThinAir, EAstraeonProtectionModule::Respirator), Unprotected);
	TestTrue(TEXT("Respirator slows oxygen loss in a sealed suit"),
		UAstraeonSuitComponent::ComputeOxygenConsumptionPercentPerSecond(FrozenThinAir, EAstraeonProtectionModule::Respirator)
		< UAstraeonSuitComponent::ComputeOxygenConsumptionPercentPerSecond(FrozenThinAir, EAstraeonProtectionModule::None));

	FAstraeonEnvironmentalSnapshot BreathableCalm;
	BreathableCalm.TemperatureKelvin = 290.0f;
	BreathableCalm.PressureKPa = 101.0f;
	BreathableCalm.bBreathable = true;
	TestEqual(TEXT("A safe environment needs no module"), UAstraeonSuitComponent::RecommendProtection(BreathableCalm), EAstraeonProtectionModule::None);
	TestTrue(TEXT("A safe environment lists no hazards"), UAstraeonSuitComponent::DescribeActiveHazards(BreathableCalm).IsEmpty());
	TestEqual(TEXT("Pressure hazard dominates the recommendation"), UAstraeonSuitComponent::RecommendProtection(FrozenThinAir), EAstraeonProtectionModule::PressureSeal);
	TestEqual(TEXT("Every hazard is listed for the player"), UAstraeonSuitComponent::DescribeActiveHazards(FrozenThinAir).Num(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonProtectionPersistsTest,
	"Astraeon.Survival.Protection.Persists",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonProtectionPersistsTest::RunTest(const FString& Parameters)
{
	const FString SlotName(TEXT("AstraeonAutomationProtection"));
	UGameplayStatics::DeleteGameInSlot(SlotName, 0);

	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	TestFalse(TEXT("Protection cannot be equipped before a session"), GameInstance->EquipProtection(EAstraeonProtectionModule::Respirator));

	GameInstance->StartNewGame(5150);
	TestEqual(TEXT("A new expedition starts unprotected"), GameInstance->GetEquippedProtection(), EAstraeonProtectionModule::None);
	TestFalse(TEXT("An unbuilt module cannot be equipped"), GameInstance->EquipProtection(EAstraeonProtectionModule::ThermalShield));

	GameInstance->AddInventoryItem(UAstraeonGameInstance::GetProtectionItemId(EAstraeonProtectionModule::ThermalShield), 1);
	TestTrue(TEXT("Protection can be equipped once built"), GameInstance->EquipProtection(EAstraeonProtectionModule::ThermalShield));
	TestEqual(TEXT("Equipped module is remembered"), GameInstance->GetEquippedProtection(), EAstraeonProtectionModule::ThermalShield);
	TestTrue(TEXT("Session with protection can be saved"), GameInstance->SaveCurrentGame(SlotName, 0));

	UAstraeonGameInstance* LoadedGame = NewObject<UAstraeonGameInstance>();
	TestTrue(TEXT("Saved session can be loaded"), LoadedGame->LoadSavedGame(SlotName, 0));
	TestEqual(TEXT("Equipped module survives save/load"), LoadedGame->GetEquippedProtection(), EAstraeonProtectionModule::ThermalShield);

	UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonMarkerScanUpdatesLogbookTest,
	"Astraeon.Scanning.Marker.UpdatesLogbook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonMarkerScanUpdatesLogbookTest::RunTest(const FString& Parameters)
{
	UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
	TestFalse(TEXT("Marker scan fails before a session exists"), GameInstance->RecordMarkerScan(TEXT("signal_source"), false));

	GameInstance->StartNewGame(4242);
	const int32 BaseEntryCount = GameInstance->GetRuntimeLogbookEntries().Num();

	TestFalse(TEXT("Unknown marker ids are ignored"), GameInstance->RecordMarkerScan(TEXT("itaca_surface_hatch"), false));
	TestEqual(TEXT("Ignored marker adds no entry"), GameInstance->GetRuntimeLogbookEntries().Num(), BaseEntryCount);

	TestTrue(TEXT("Resource scan is recorded"), GameInstance->RecordMarkerScan(TEXT("silicate_fiber"), true));
	TestTrue(TEXT("Signal source scan is recorded"), GameInstance->RecordMarkerScan(TEXT("signal_source"), false));
	TestTrue(TEXT("Anomaly scan is recorded"), GameInstance->RecordMarkerScan(TEXT("minor_geologic_anomaly"), false));
	TestEqual(TEXT("Each scanned category adds one entry"), GameInstance->GetRuntimeLogbookEntries().Num(), BaseEntryCount + 3);

	const FAstraeonLogbookEntry* AnomalyEntry = GameInstance->GetRuntimeLogbookEntries().FindByPredicate([](const FAstraeonLogbookEntry& Entry)
	{
		return Entry.EntryId == TEXT("anomaly.minor_geologic");
	});
	TestNotNull(TEXT("Anomaly entry exists after scanning"), AnomalyEntry);
	if (AnomalyEntry)
	{
		TestEqual(TEXT("Scanning the anomaly only observes it"), AnomalyEntry->Certainty, EAstraeonDiscoveryCertainty::Observed);
	}

	// Inspeccionar de cerca sube la certeza sin duplicar la entrada.
	TestTrue(TEXT("Anomaly inspection is recorded"), GameInstance->RecordAnomalyInspection());
	TestEqual(TEXT("Inspection reuses the anomaly entry"), GameInstance->GetRuntimeLogbookEntries().Num(), BaseEntryCount + 3);

	AnomalyEntry = GameInstance->GetRuntimeLogbookEntries().FindByPredicate([](const FAstraeonLogbookEntry& Entry)
	{
		return Entry.EntryId == TEXT("anomaly.minor_geologic");
	});
	TestNotNull(TEXT("Anomaly entry still exists after inspection"), AnomalyEntry);
	if (AnomalyEntry)
	{
		TestEqual(TEXT("Inspecting the anomaly measures it"), AnomalyEntry->Certainty, EAstraeonDiscoveryCertainty::Measured);
	}
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
	TestEqual(TEXT("Loaded content seed matches source"), LoadedGame->GetCurrentContentSeed(), SourceGame->GetCurrentContentSeed());
	TestEqual(TEXT("Loaded planet profile matches source"), LoadedGame->GetCurrentPlanetProfileId(), SourceGame->GetCurrentPlanetProfileId());
	TestEqual(TEXT("Loaded region profile matches source"), LoadedGame->GetCurrentRegionProfileId(), SourceGame->GetCurrentRegionProfileId());
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonSaveGameV1MigrationTest,
	"Astraeon.Persistence.SaveGame.V1Migration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonSaveGameV1MigrationTest::RunTest(const FString& Parameters)
{
	const FString SlotName(TEXT("AstraeonAutomationV1Migration"));
	UGameplayStatics::DeleteGameInSlot(SlotName, 0);

	UAstraeonSaveGame* LegacySave = NewObject<UAstraeonSaveGame>();
	LegacySave->SaveGameVersion = 1;
	LegacySave->WorldSeed = 777;
	LegacySave->Environment = UAstraeonWorldGenerator::GenerateEnvironment(LegacySave->WorldSeed);
	LegacySave->RegionLayout = UAstraeonWorldGenerator::GenerateRegionLayout(LegacySave->WorldSeed);
	LegacySave->Inventory.Add(TEXT("silicate_fiber"), 2);
	TestTrue(TEXT("Legacy fixture is written"), UGameplayStatics::SaveGameToSlot(LegacySave, SlotName, 0));

	UAstraeonGameInstance* LoadedGame = NewObject<UAstraeonGameInstance>();
	TestTrue(TEXT("Version 1 fixture loads"), LoadedGame->LoadSavedGame(SlotName, 0));
	TestEqual(TEXT("Legacy content seed stays intact"), LoadedGame->GetCurrentContentSeed(), 777);
	TestEqual(TEXT("Legacy layout receives explicit migration planet id"), LoadedGame->GetCurrentPlanetProfileId(), FName(TEXT("legacy_generated_planet")));
	TestEqual(TEXT("Legacy layout receives explicit migration region id"), LoadedGame->GetCurrentRegionProfileId(), FName(TEXT("legacy_generated_region")));
	TestEqual(TEXT("Legacy resource locations are preserved"), LoadedGame->GetCurrentRegionLayout().Resources[0].LocationMeters, LegacySave->RegionLayout.Resources[0].LocationMeters);
	TestEqual(TEXT("Legacy inventory is preserved"), LoadedGame->GetInventoryItemCount(TEXT("silicate_fiber")), 2);

	const UAstraeonSaveGame* MigratedSnapshot = LoadedGame->CreateSaveSnapshot();
	TestNotNull(TEXT("Migrated session creates a version 2 snapshot"), MigratedSnapshot);
	if (MigratedSnapshot)
	{
		TestEqual(TEXT("Migration writes save version 2"), MigratedSnapshot->SaveGameVersion, 2);
		TestEqual(TEXT("Migration retains legacy region id"), MigratedSnapshot->RegionProfileId, FName(TEXT("legacy_generated_region")));
	}

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
