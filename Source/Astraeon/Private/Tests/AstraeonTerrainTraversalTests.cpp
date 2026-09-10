#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Planet/LOD/AstraeonPlanetLODManager.h"
#include "Planet/Patches/AstraeonPlanetPatchMesh.h"
#include "Planet/Surface/AstraeonPlanetSurface.h"
#include "Planet/Surface/AstraeonPlanetTraversal.h"
#include "WorldGen/AstraeonTerrainTraversal.h"

// Migrated to the sphere on 2026-09-10 to leave quarantine (Phase 2, P2.7). Every assertion of the
// flat versions is kept; what changes is the surface they walk: Region A's plan placed on a
// planet by exponential map, heights from the radial two-layer relief, spacing of the finest
// patch grid. The flat validator stays for the flat build and is no longer what these test.
namespace AstraeonTraversalTest
{
	// The five content seeds `Docs/MVP_WORLD_ARCHITECTURE.md` asks to check, plus the smoke's
	// seed and the safe variant's, which has to stand on its own.
	const TArray<int32> Seeds = { 100, 200, 300, 400, 500, 13579, 1001 };

	// A walkable 50 km planet with mountains: the size of TL_13, where regions will live.
	FAstraeonPlanetDefinition Planet()
	{
		FAstraeonPlanetDefinition P;
		P.BodyId = TEXT("traversal_lab"); P.RadiusCm = 5000000.0; P.MassKg = 1.e16; P.SurfaceGravityMS2 = 9.81;
		P.WorldSeed = 4242; P.BodySeed = 4242;
		return P;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonTerrainConnectivityTest,
	"Astraeon.WorldGen.Terrain.Connectivity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonTerrainConnectivityTest::RunTest(const FString& Parameters)
{
	using namespace AstraeonTraversalTest;
	const FAstraeonPlanetDefinition P = Planet();
	TestTrue(TEXT("The planet has a mountain layer, so walls can exist"), FAstraeonPlanetSurface::HasMountainLayer(P));

	// The limit has to be the engine's, not an invented one: a laxer one would pass slopes the
	// player slides down. Measured against the actual finest patch mesh at the region.
	const FVector Probe = FAstraeonPlanetTraversal::CandidateAnchor(P, 1001, 0);
	const double SpacingCm = FAstraeonPlanetTraversal::MeshSpacingCm(P, Probe);
	{
		const FAstraeonPlanetLODSettings Settings;
		FAstraeonPlanetPatchAddress Patch;
		FAstraeonPlanetPatchAddress::TryFromDirection(P.BodyId, Probe, FAstraeonPlanetLODManager::FinestAllowedLod(P, Settings), Patch);
		FAstraeonPlanetPatchBuildOptions Options; Options.Quads = Settings.Quads; Options.SkirtDepthCm = 0.0;
		FAstraeonPlanetPatchBuildResult Mesh;
		FAstraeonPlanetPatchMesh::Build(P, Patch, 1, Options, Mesh);
		// The mesh edge closest to the probe, along the grid's X.
		int32 Nearest = 0; double Best = TNumericLimits<double>::Max();
		for (int32 I = 0; I < Mesh.SurfaceVertexCount; ++I)
		{
			if (I % (Options.Quads + 1) == Options.Quads) continue;
			const double D = FVector::DistSquared((Mesh.Vertices[I] + Mesh.OriginBodyCm).GetSafeNormal(), Probe);
			if (D < Best) { Best = D; Nearest = I; }
		}
		const double MeshEdgeCm = FAstraeonPlanetSurface::ArcDistanceCm(P, Mesh.Vertices[Nearest] + Mesh.OriginBodyCm, Mesh.Vertices[Nearest + 1] + Mesh.OriginBodyCm);
		TestTrue(FString::Printf(TEXT("La validación mide la superficie a la resolución de la malla (%.1f cm contra %.1f cm)"), SpacingCm, MeshEdgeCm),
			FMath::IsNearlyEqual(SpacingCm, MeshEdgeCm, MeshEdgeCm * 0.01));
	}
	TestTrue(TEXT("El desnivel admitido nunca baja del escalón de la cápsula"),
		FAstraeonTerrainTraversal::GetMaxWalkableRiseCm(float(SpacingCm)) >= FAstraeonPlanetSurface::MaxWalkableStepCm);

	const FVector ItacaOriginCm = FVector::ZeroVector;
	for (const int32 Seed : Seeds)
	{
		const FAstraeonPlanetRegionPlan Plan = FAstraeonPlanetRegionPlan::FromFlatRegion(Seed, ItacaOriginCm);
		TestTrue(FString::Printf(TEXT("Seed %d declara objetivos que alcanzar"), Seed), Plan.Goals.Num() > 0);

		const FAstraeonPlanetRegionResolution Resolution = FAstraeonPlanetTraversal::ResolveRegion(P, Seed, Plan);
		// What is demanded is not that the requested place works, but that the published one
		// does: no session can end with a critical resource or the signal behind a wall.
		TestTrue(FString::Printf(TEXT("Seed %d publica una región transitable"), Seed), Resolution.Report.bPassed);
		TestEqual(FString::Printf(TEXT("Seed %d no deja objetivos inalcanzables"), Seed), Resolution.Report.UnreachableGoals.Num(), 0);
		TestFalse(FString::Printf(TEXT("Seed %d no necesita la variante segura"), Seed), Resolution.bUsedFallback);

		// Determinism: the same seed always resolves to the same place, or loading a saved
		// session would put its region somewhere else.
		const FAstraeonPlanetRegionResolution Repeat = FAstraeonPlanetTraversal::ResolveRegion(P, Seed, Plan);
		TestTrue(FString::Printf(TEXT("Seed %d resuelve siempre al mismo relieve"), Seed), Repeat.Anchor == Resolution.Anchor);
		AddInfo(FString::Printf(TEXT("PlanetTraversal seed=%d attempts=%d reachable_cells=%d worst_rise_cm=%.1f"),
			Seed, Resolution.AttemptsUsed, Resolution.Report.ReachableCells, Resolution.Report.WorstReachableRiseCm));
	}

	// The safe variant cannot be a hope: it is checked with the same goals.
	{
		const FAstraeonPlanetRegionPlan Plan = FAstraeonPlanetRegionPlan::FromFlatRegion(1001, ItacaOriginCm);
		FVector SafeAnchor;
		TestTrue(TEXT("Existe una variante segura"), FAstraeonPlanetTraversal::FindSafeAnchor(P, 1001, Plan.RadiusCm, SafeAnchor));
		const FAstraeonTraversalReport Report = FAstraeonPlanetTraversal::Evaluate(FAstraeonPlanetRegionSurface::Place(P, SafeAnchor, Plan));
		TestTrue(TEXT("La variante segura es transitable por sí misma"), Report.bPassed);
	}

	// Itaca can land anywhere in the region. The region still covers its whole field and the
	// cleared footprint travels with the ship, so the guarantee holds.
	{
		const FVector LandedOriginCm(12000.0f, -8000.0f, 0.0f);
		const FAstraeonPlanetRegionPlan Plan = FAstraeonPlanetRegionPlan::FromFlatRegion(1001, LandedOriginCm);
		const FAstraeonPlanetRegionResolution Resolution = FAstraeonPlanetTraversal::ResolveRegion(P, 1001, Plan);
		TestTrue(TEXT("Con Ítaca aterrizada lejos la región sigue siendo transitable"), Resolution.Report.bPassed);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonTerrainTraversalDetectsWallsTest,
	"Astraeon.WorldGen.Terrain.TraversalDetectsWalls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonTerrainTraversalDetectsWallsTest::RunTest(const FString& Parameters)
{
	// A connectivity test that always passes proves nothing. This one checks that the validator
	// detects the case it exists for: a goal outside the region, unreachable however gentle the
	// relief. On the sphere as on the plane.
	using namespace AstraeonTraversalTest;
	const FAstraeonPlanetDefinition P = Planet();
	FAstraeonPlanetRegionPlan Plan = FAstraeonPlanetRegionPlan::FromFlatRegion(1001, FVector::ZeroVector);
	const int32 ReachableGoals = Plan.Goals.Num();
	const FVector Anchor = FAstraeonPlanetTraversal::ResolveRegion(P, 1001, Plan).Anchor;

	FAstraeonTraversalGoal Unreachable;
	Unreachable.GoalId = TEXT("prueba_fuera_de_la_region");
	// Outside the region's field. Making a relief wall would depend on what the seed produces
	// that day; being outside the field is unreachable by construction.
	Unreachable.LocationCm = Plan.CenterCm + FVector2D(Plan.RadiusCm * 4.0, 0.0);
	Plan.Goals.Add(Unreachable);
	Plan.StartCm = FVector2D(900.0, 0.0); // As in the flat version: just outside Itaca.

	const FAstraeonTraversalReport Report = FAstraeonPlanetTraversal::Evaluate(FAstraeonPlanetRegionSurface::Place(P, Anchor, Plan));
	TestFalse(TEXT("Un objetivo inalcanzable hace fallar la validación"), Report.bPassed);
	// The goal is named, not just the failure: without that the report would not say what to fix.
	TestTrue(TEXT("El objetivo de fuera de la región aparece señalado"), Report.UnreachableGoals.Contains(Unreachable.GoalId));
	TestTrue(TEXT("La región declara objetivos que alcanzar"), ReachableGoals > 0);
	return true;
}

#endif
