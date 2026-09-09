#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Environment/AstraeonItacaInterior.h"
#include "Ship/AstraeonShipPawn.h"
#include "WorldGen/AstraeonRegionMarker.h"
#include "WorldGen/AstraeonRegionMaterializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonShipArtTest,
	"Astraeon.Art.Itaca.ExteriorAndStations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonShipArtTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Transient integration world"), World)) return false;

	// El exterior deja de ser un cubo escalado: casco, dos motores, cuatro patines y antena.
	AAstraeonShipPawn* Ship = World->SpawnActor<AAstraeonShipPawn>();
	if (!TestNotNull(TEXT("Ship spawns"), Ship)) return false;
	TArray<UStaticMeshComponent*> ShipMeshes;
	Ship->GetComponents(ShipMeshes);
	TMap<FString, int32> Counted;
	for (const UStaticMeshComponent* Component : ShipMeshes)
	{
		if (const UStaticMesh* Mesh = Component->GetStaticMesh())
		{
			Counted.FindOrAdd(Mesh->GetName())++;
			TestTrue(TEXT("Ship art never blocks the flight trace"),
				Component->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
			TestTrue(TEXT("Ship modules keep unit scale"),
				Component->GetRelativeScale3D().Equals(FVector::OneVector, 0.001f));
		}
	}
	TestEqual(TEXT("One hull"), Counted.FindRef(TEXT("SM_Itaca_Hull_Blockout")), 1);
	TestEqual(TEXT("Two engines"), Counted.FindRef(TEXT("SM_Itaca_Engine_Blockout")), 2);
	TestEqual(TEXT("Four landing skids"), Counted.FindRef(TEXT("SM_Itaca_LandingGear_Blockout")), 4);
	TestEqual(TEXT("One antenna"), Counted.FindRef(TEXT("SM_Itaca_Antenna_Blockout")), 1);
	TestEqual(TEXT("No engine cube survives on the ship"), Counted.FindRef(TEXT("Cube")), 0);

	// El casco tiene que seguir conteniendo la estancia de 8 x 6 x 2,8 m, o exterior e
	// interior dejan de ser el mismo objeto de ficción.
	if (const UStaticMesh* Hull = LoadObject<UStaticMesh>(nullptr,
		TEXT("/Game/Astraeon/Art/Blockouts/Itaca/SM_Itaca_Hull_Blockout.SM_Itaca_Hull_Blockout")))
	{
		const FVector Size = Hull->GetBounds().BoxExtent * 2.0f;
		TestTrue(TEXT("Hull is long enough for the room"), Size.X >= 800.0f);
		TestTrue(TEXT("Hull is wide enough for the room"), Size.Y >= 600.0f);
		TestTrue(TEXT("Hull is tall enough for the room"), Size.Z >= 280.0f);
		// Volumen de estudio del plan maestro, con el casco elevado sobre los patines.
		TestTrue(TEXT("Hull fits the 14 x 10 x 6 m study envelope"),
			Size.X <= 1400.0f && Size.Y <= 1000.0f && Size.Z <= 600.0f);
	}

	// Las dos estaciones que seguían siendo cubos ahora traen malla y colisión propias.
	for (const FAstraeonRegionActorSpec& Spec : UAstraeonRegionMaterializer::BuildItacaActorSpecs(FVector::ZeroVector))
	{
		if (Spec.ActorId != TEXT("itaca_pilot_console") && Spec.ActorId != TEXT("itaca_fabricator")) continue;
		AAstraeonRegionMarker* Marker = World->SpawnActor<AAstraeonRegionMarker>();
		Marker->ApplySpec(Spec);
		TestEqual(TEXT("Station keeps its persistent id"), Marker->GetMarkerId(), Spec.ActorId);
		const UStaticMeshComponent* Root = Cast<UStaticMeshComponent>(Marker->GetRootComponent());
		if (!TestNotNull(TEXT("Station has a mesh root"), Root)) continue;
		const UStaticMesh* Mesh = Root->GetStaticMesh();
		if (!TestNotNull(TEXT("Station art loads from Content"), Mesh)) continue;
		TestNotEqual(TEXT("Station is no longer an engine cube"), Mesh->GetName(), FString(TEXT("Cube")));
		TestTrue(TEXT("Station art uses metric unit scale"),
			Marker->GetActorScale3D().Equals(FVector::OneVector, 0.001f));
		TestEqual(TEXT("Station rests on the authored Ítaca floor height"), Marker->GetActorLocation().Z, Spec.LocationCm.Z);
		// Con la malla sin colisión, quien frena al jugador es la caja, como en ARGOS.
		TestEqual(TEXT("Station mesh cannot block"), Root->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		TArray<UBoxComponent*> Boxes;
		Marker->GetComponents(Boxes);
		const bool bBlocks = Boxes.ContainsByPredicate([](const UBoxComponent* Box)
			{ return Box->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics; });
		TestTrue(TEXT("Station keeps a blocking collision box"), bBlocks);
	}

	// La escotilla deja de ser un hueco abierto: la hoja bloquea hasta que se abre.
	AAstraeonItacaInterior* Interior = World->SpawnActor<AAstraeonItacaInterior>();
	if (!TestNotNull(TEXT("Interior spawns"), Interior)) return false;
	TArray<UStaticMeshComponent*> RoomMeshes;
	Interior->GetComponents(RoomMeshes);
	const bool bHasLeaf = RoomMeshes.ContainsByPredicate([](const UStaticMeshComponent* Component)
		{ return Component->GetStaticMesh() && Component->GetStaticMesh()->GetName() == TEXT("SM_Itaca_HatchLeaf_Blockout"); });
	TestTrue(TEXT("The hatch has a real leaf"), bHasLeaf);
	TArray<UBoxComponent*> RoomBoxes;
	Interior->GetComponents(RoomBoxes);
	UBoxComponent* LeafBlock = nullptr;
	for (UBoxComponent* Box : RoomBoxes)
	{
		if (Box->GetFName() == TEXT("HatchLeafCollision")) LeafBlock = Box;
	}
	if (TestNotNull(TEXT("The leaf has its own blocking box"), LeafBlock))
	{
		TestEqual(TEXT("A closed hatch blocks the way"), LeafBlock->GetCollisionEnabled(),
			ECollisionEnabled::QueryAndPhysics);
		// Frena a la cápsula pero deja pasar el trazo: si no, ESCOTILLA sería ininteractuable.
		TestEqual(TEXT("The leaf never blocks the interaction trace"),
			LeafBlock->GetCollisionResponseToChannel(ECC_Visibility), ECR_Ignore);
		TestFalse(TEXT("The hatch starts closed"), Interior->IsHatchOpen());
		Interior->OpenHatch();
		TestTrue(TEXT("Using the hatch opens it"), Interior->IsHatchOpen());
		TestEqual(TEXT("An open hatch stops blocking"), LeafBlock->GetCollisionEnabled(),
			ECollisionEnabled::NoCollision);
	}
	// Gira hacia dentro: hacia fuera chocaría contra el casco.
	TestTrue(TEXT("The leaf swings into the room"), AAstraeonItacaInterior::GetHatchOpenAngleDegrees() < 0.0f);

	World->DestroyWorld(false);
	return true;
}

#endif
