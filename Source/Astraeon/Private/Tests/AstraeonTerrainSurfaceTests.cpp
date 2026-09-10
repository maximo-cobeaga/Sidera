#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Planet/LOD/AstraeonPlanetLODManager.h"
#include "Planet/Patches/AstraeonPlanetPatchAddress.h"
#include "Planet/Patches/AstraeonPlanetPatchMesh.h"
#include "Planet/Surface/AstraeonPlanetSurface.h"
#include "Planet/Surface/AstraeonPlanetTraversal.h"
#include "WorldGen/AstraeonTerrainField.h"
#include "WorldGen/AstraeonRegionASurface.h"

// Migrated to the sphere on 2026-09-10 (Phase 3, P3.2) to leave quarantine (ADR 0005). Every
// assertion of the flat contract keeps its counterpart: the regional context is a region placed on
// a 500 km body, the sample is that body's only height query, and the mesh is the patch the
// runtime builds. Nothing was relaxed.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonTerrainSurfaceContractTest,
	"Astraeon.WorldGen.Terrain.SurfaceContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace AstraeonSurfaceContractTest
{
	FAstraeonPlanetDefinition Body(int32 Seed)
	{
		FAstraeonPlanetDefinition Planet;
		Planet.BodyId = TEXT("planet_surface_contract");
		Planet.RadiusCm = 50000000.0; Planet.SurfaceGravityMS2 = 8.05;
		Planet.MassKg = 8.05 * FMath::Square(Planet.RadiusCm / 100.0) / 6.67430e-11;
		Planet.BodySeed = Seed; Planet.WorldSeed = Seed;
		return Planet;
	}

	// The flat contract's context: a clearing under Itaca and one at its door, both kept free of
	// mountains, in a field of the regional radius.
	FAstraeonPlanetRegionPlan Plan()
	{
		FAstraeonPlanetRegionPlan Plan;
		Plan.RadiusCm = AAstraeonTerrainField::GetFieldRadiusCm();
		Plan.FlatSpotsCm = { FVector2D::ZeroVector, FVector2D(900.0, 0.0) };
		Plan.MountainKeepOutCm = Plan.FlatSpotsCm;
		return Plan;
	}

	FAstraeonPlanetRegionSurface Place(int32 Seed)
	{
		const FAstraeonPlanetDefinition Planet = Body(Seed);
		return FAstraeonPlanetRegionSurface::Place(Planet, FAstraeonPlanetTraversal::CandidateAnchor(Planet, Seed, 0), Plan());
	}

	bool BuildPatchAt(const FAstraeonPlanetDefinition& Planet, const FVector& Direction, FAstraeonPlanetPatchBuildResult& Out)
	{
		const FAstraeonPlanetLODSettings Settings;
		FAstraeonPlanetPatchAddress Address;
		if (!FAstraeonPlanetPatchAddress::TryFromDirection(Planet.BodyId, Direction, FAstraeonPlanetLODManager::FinestAllowedLod(Planet, Settings), Address))
			return false;
		FAstraeonPlanetPatchBuildOptions Options;
		Options.Quads = Settings.Quads;
		Options.SkirtDepthCm = FAstraeonPlanetLODManager::SkirtDepthCm(Planet, Address, Settings.Quads);
		return FAstraeonPlanetPatchMesh::Build(Planet, Address, 1, Options, Out) == EAstraeonPatchBuildStatus::Success;
	}
}

bool FAstraeonTerrainSurfaceContractTest::RunTest(const FString& Parameters)
{
	using namespace AstraeonSurfaceContractTest;
	for (const int32 Seed : {11, 22, 42, 123, 999, 4242, 13579, 24680, 65535, 104729})
	{
		const FAstraeonPlanetRegionSurface Surface = Place(Seed);
		const FVector2D Probe(12345.0, -6789.0);
		const FAstraeonPlanetRegionSample First = Surface.SampleSurface(Probe);
		const FAstraeonPlanetRegionSample Second = Surface.SampleSurface(Probe);
		TestTrue(FString::Printf(TEXT("Seed %d has a valid in-bounds sample"), Seed), First.bIsValid);
		TestEqual(FString::Printf(TEXT("Seed %d surface height is deterministic"), Seed), First.HeightCm, Second.HeightCm);
		TestTrue(FString::Printf(TEXT("Seed %d surface normal is normalized"), Seed),
			FMath::IsNearlyEqual(First.Normal.Size(), 1.0, 0.001));
		// There is one height: the region's sample is the body's query at that point.
		TestEqual(FString::Printf(TEXT("Seed %d region sample is the body's height"), Seed), First.HeightCm,
			FAstraeonPlanetSurface::SampleRadialHeightCm(Surface.Planet, Surface.ToDirection(Probe)));

		const double PadCm = FAstraeonPlanetSurface::SampleGroundHeightCm(Surface.Planet, Surface.Relief->ItacaDirection);
		const FAstraeonPlanetRegionSample Pad = Surface.SampleSurface(FVector2D::ZeroVector);
		TestEqual(FString::Printf(TEXT("Seed %d Itaca pad is represented by the surface contract"), Seed), Pad.HeightCm, PadCm);
		// The whole rigid footprint is the pad, not only its origin.
		for (const FVector2D& Corner : {FVector2D(-170.0, -290.0), FVector2D(610.0, 290.0)})
			TestEqual(FString::Printf(TEXT("Seed %d Itaca footprint is flat"), Seed), Surface.SampleSurface(Corner).HeightCm, PadCm);
	}

	const FAstraeonPlanetRegionSurface MeshSurface = Place(13579);
	FAstraeonPlanetPatchBuildResult Mesh;
	TestTrue(TEXT("The patch under Itaca builds"), BuildPatchAt(MeshSurface.Planet, MeshSurface.Relief->ItacaDirection, Mesh));
	TestTrue(TEXT("Patch stays within the first-pass vertex budget"), Mesh.Vertices.Num() > 0 && Mesh.Vertices.Num() <= 30000);
	TestEqual(TEXT("Patch has a normal for every vertex"), Mesh.Vertices.Num(), Mesh.Normals.Num());
	TestEqual(TEXT("Patch has a UV for every vertex"), Mesh.Vertices.Num(), Mesh.UVs.Num());
	TestTrue(TEXT("Patch triangles are complete"), Mesh.Indices.Num() > 0 && Mesh.Indices.Num() % 3 == 0);
	for (const int32 Index : Mesh.Indices)
	{
		if (!Mesh.Vertices.IsValidIndex(Index)) { AddError(TEXT("Patch triangle index is invalid")); break; }
	}
	// What the runtime draws and collides with is the contract's height, vertex by vertex.
	double WorstCm = 0.0;
	for (int32 I = 0; I < Mesh.SurfaceVertexCount; ++I)
	{
		const FVector BodyCm = Mesh.Vertices[I] + Mesh.OriginBodyCm;
		const FVector Direction = BodyCm.GetSafeNormal();
		WorstCm = FMath::Max(WorstCm, FMath::Abs(BodyCm.Size() -(MeshSurface.Planet.RadiusCm + FAstraeonPlanetSurface::SampleRadialHeightCm(MeshSurface.Planet, Direction))));
	}
	TestTrue(FString::Printf(TEXT("The patch is the contract's surface (worst %.4f cm)"), WorstCm), WorstCm < 0.01);

	const FAstraeonPlanetRegionSample OutOfBounds = MeshSurface.SampleSurface(FVector2D(AAstraeonTerrainField::GetFieldRadiusCm() + 1.0, 0.0));
	TestFalse(TEXT("Surface contract rejects coordinates outside the regional field"), OutOfBounds.bIsValid);

	const FAstraeonTerrainSurfaceSample Landing = FAstraeonRegionASurface::Sample(FVector2D::ZeroVector);
	const FAstraeonTerrainSurfaceSample Signal = FAstraeonRegionASurface::Sample(FVector2D(22000.0f, 14500.0f));
	TestTrue(TEXT("Designed Region A surface contains landing zone"), Landing.bIsValid);
	TestTrue(TEXT("Designed Region A surface contains signal"), Signal.bIsValid);
	TestTrue(TEXT("Designed signal ridge rises above landing basin"), Signal.HeightCm > Landing.HeightCm);
	TestTrue(TEXT("Designed corridor remains walkable"), Landing.Normal.Z > 0.95f);

	// A region never swaps the body's surface for another one: beyond its own cone the patch is
	// the bare body's, bit for bit. (Flat: the region id no longer changed the materialized mesh.)
	FAstraeonPlanetDefinition Bare = MeshSurface.Planet;
	Bare.RegionRelief.Reset();
	FAstraeonPlanetPatchBuildResult WithRegion, WithoutRegion;
	const FVector Antipode = -MeshSurface.Anchor;
	TestTrue(TEXT("Far patches build"), BuildPatchAt(MeshSurface.Planet, Antipode, WithRegion) && BuildPatchAt(Bare, Antipode, WithoutRegion));
	bool bSame = WithRegion.Vertices.Num() == WithoutRegion.Vertices.Num() && WithRegion.Vertices.Num() > 0;
	for (int32 I = 0; bSame && I < WithRegion.Vertices.Num(); ++I)
		bSame = WithRegion.Vertices[I] == WithoutRegion.Vertices[I] && WithRegion.Normals[I] == WithoutRegion.Normals[I];
	TestTrue(TEXT("Outside the region the body keeps its own surface"), bSame);
	return true;
}

#endif
