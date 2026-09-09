#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "WorldGen/AstraeonRegionMarker.h"
#include "WorldGen/AstraeonWorldGenerator.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonRegionArtTest,
	"Astraeon.Art.Region.PresentationPreservesGameplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonRegionArtTest::RunTest(const FString& Parameters)
{
	TSet<FString> LoadedSilhouettes;
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Transient integration world"), World)) return false;
	// CreateWorld already initializes its persistent level and physics scene in UE 5.7.
	for (const int32 Seed : {13579, 42, 7, 2026, 12345, 99999})
	{
		const auto Layout = UAstraeonWorldGenerator::GenerateRegionLayout(Seed);
		for (const auto& Spec : UAstraeonRegionMaterializer::BuildActorSpecs(Layout))
		{
			// Creature spawns are layout instructions, not one of the seven resource/POI
			// visuals. Desde que hay varios, sus ids son creature_spawn_N y ya no el
			// antiguo first_mob_patrol_origin.
			if (Spec.ActorId.ToString().StartsWith(TEXT("creature_spawn"))) continue;
			AAstraeonRegionMarker* Marker = World->SpawnActor<AAstraeonRegionMarker>();
			Marker->ApplySpec(Spec);
			TestEqual(TEXT("Art preserves persistent id"), Marker->GetMarkerId(), Spec.ActorId);
			TestEqual(TEXT("Art preserves tool requirement"), Marker->GetRequiredToolId(), Spec.RequiredToolId);
			TestEqual(TEXT("Art preserves saved actor position"), Marker->GetActorLocation(), Spec.LocationCm);
			TestEqual(TEXT("Art preserves interaction envelope"), Marker->GetActorScale3D(), Spec.Scale);
			const auto* Proxy = Cast<UStaticMeshComponent>(Marker->GetRootComponent());
			TestTrue(TEXT("Interaction proxy retains collision"), Proxy && Proxy->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics);
			TestTrue(TEXT("Interaction trace still hits the proxy"), Proxy && Proxy->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block);
			TestTrue(TEXT("Old cube is hidden"), Proxy && !Proxy->IsVisible());
			TArray<UStaticMeshComponent*> Components;
			Marker->GetComponents(Components);
			UStaticMeshComponent* Art = nullptr;
			for (auto* Component : Components)
			{
				if (Component->GetFName() == TEXT("RegionArt")) Art = Component;
			}
			if (!TestNotNull(TEXT("Presentation component exists"), Art) ||
				!TestNotNull(TEXT("Original art loads from Content"), Art->GetStaticMesh().Get())) continue;
			LoadedSilhouettes.Add(Art->GetStaticMesh()->GetName());
			TestTrue(TEXT("Art is visible"), Art->IsVisible());
			TestEqual(TEXT("Art cannot block collection or scanner"), Art->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			TestTrue(TEXT("Art uses metric unit scale"), Art->GetComponentScale().Equals(FVector::OneVector));
			const FBoxSphereBounds Bounds = Art->GetStaticMesh()->GetBounds();
			TestTrue(TEXT("Source pivot rests on ground"), FMath::Abs(Bounds.Origin.Z - Bounds.BoxExtent.Z) < 0.1f);
			TestTrue(TEXT("Presentation is grounded"), FMath::Abs(Art->GetComponentLocation().Z) < 0.1f);
			FAstraeonRegionActorSpec Moved = Spec;
			Moved.LocationCm += FVector(8000, -9000, 320);
			Marker->ApplySpec(Moved);
			TestTrue(TEXT("Presentation follows relocated marker"),
				Art->GetComponentLocation().Equals(FVector(Moved.LocationCm.X, Moved.LocationCm.Y, 320), 0.1f));
			Moved.ActorId = TEXT("unknown_future_resource");
			Moved.Kind = EAstraeonRegionActorKind::Resource;
			Marker->ApplySpec(Moved);
			TestTrue(TEXT("Unknown content retains visible proxy fallback"), Proxy->IsVisible() && !Art->IsVisible());
		}
	}
	TestEqual(TEXT("Seven distinct silhouettes are integrated"), LoadedSilhouettes.Num(), 7);
	World->DestroyWorld(false);
	return true;
}

#endif
