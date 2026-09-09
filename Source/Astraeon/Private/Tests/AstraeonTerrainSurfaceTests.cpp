#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "WorldGen/AstraeonTerrainField.h"
#include "WorldGen/AstraeonTerrainSurfacePrototype.h"
#include "WorldGen/AstraeonRegionASurface.h"
#include "WorldGen/AstraeonWorldProfiles.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonTerrainSurfaceContractTest,
	"Astraeon.WorldGen.Terrain.SurfaceContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonTerrainSurfaceContractTest::RunTest(const FString& Parameters)
{
	for (const int32 Seed : {11, 22, 42, 123, 999, 4242, 13579, 24680, 65535, 104729})
	{
		FAstraeonTerrainSurfaceContext Context;
		Context.WorldSeed = Seed;
		Context.CenterCm = FVector2D::ZeroVector;
		Context.GroundFlatSpotsCm = { FVector2D::ZeroVector, FVector2D(900.0f, 0.0f) };
		Context.MountainKeepOutCm = Context.GroundFlatSpotsCm;
		Context.ItacaPadHeightCm = AAstraeonTerrainField::GetItacaPadHeightCm(Seed, 0.0f, 0.0f);

		const FAstraeonTerrainSurfaceSample First = AAstraeonTerrainField::SampleSurface(Context, FVector2D(12345.0f, -6789.0f));
		const FAstraeonTerrainSurfaceSample Second = AAstraeonTerrainField::SampleSurface(Context, FVector2D(12345.0f, -6789.0f));
		TestTrue(FString::Printf(TEXT("Seed %d has a valid in-bounds sample"), Seed), First.bIsValid);
		TestEqual(FString::Printf(TEXT("Seed %d surface height is deterministic"), Seed), First.HeightCm, Second.HeightCm);
		TestTrue(FString::Printf(TEXT("Seed %d surface normal is normalized"), Seed),
			FMath::IsNearlyEqual(First.Normal.Size(), 1.0f, 0.001f));

		const FAstraeonTerrainSurfaceSample Pad = AAstraeonTerrainField::SampleSurface(Context, FVector2D::ZeroVector);
		TestEqual(FString::Printf(TEXT("Seed %d Ítaca pad is represented by the surface contract"), Seed),
			Pad.HeightCm, Context.ItacaPadHeightCm);
	}

	FAstraeonTerrainSurfaceContext MeshContext;
	MeshContext.WorldSeed = 13579;
	MeshContext.ItacaPadHeightCm = AAstraeonTerrainField::GetItacaPadHeightCm(13579, 0.0f, 0.0f);
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	AAstraeonTerrainSurfacePrototype::BuildMeshData(MeshContext,
		AAstraeonTerrainSurfacePrototype::GetPrototypeSpacingCm(), Vertices, Triangles, Normals, UVs);
	TestTrue(TEXT("Prototype stays within the first-pass vertex budget"), Vertices.Num() > 0 && Vertices.Num() <= 30000);
	TestEqual(TEXT("Prototype has a normal and UV for every vertex"), Vertices.Num(), Normals.Num());
	TestEqual(TEXT("Prototype has a UV for every vertex"), Vertices.Num(), UVs.Num());
	TestTrue(TEXT("Prototype triangles are complete"), Triangles.Num() > 0 && Triangles.Num() % 3 == 0);
	for (const int32 Index : Triangles)
	{
		TestTrue(TEXT("Prototype triangle index is valid"), Vertices.IsValidIndex(Index));
	}

	const FAstraeonTerrainSurfaceSample OutOfBounds = AAstraeonTerrainField::SampleSurface(MeshContext,
		FVector2D(AAstraeonTerrainField::GetFieldRadiusCm() + 1.0f, 0.0f));
	TestFalse(TEXT("Surface contract rejects coordinates outside the regional field"), OutOfBounds.bIsValid);

	const FAstraeonTerrainSurfaceSample Landing = FAstraeonRegionASurface::Sample(FVector2D::ZeroVector);
	const FAstraeonTerrainSurfaceSample Signal = FAstraeonRegionASurface::Sample(FVector2D(22000.0f, 14500.0f));
	TestTrue(TEXT("Designed Region A surface contains landing zone"), Landing.bIsValid);
	TestTrue(TEXT("Designed Region A surface contains signal"), Signal.bIsValid);
	TestTrue(TEXT("Designed signal ridge rises above landing basin"), Signal.HeightCm > Landing.HeightCm);
	TestTrue(TEXT("Designed corridor remains walkable"), Landing.Normal.Z > 0.95f);
	MeshContext.RegionProfileId = UAstraeonWorldProfiles::GetRegionAProfileId();
	AAstraeonTerrainSurfacePrototype::BuildMeshData(MeshContext, 400.0f, Vertices, Triangles, Normals, UVs);
	TestTrue(TEXT("Prototype consumes designed Region A surface"), Vertices.ContainsByPredicate([](const FVector& Vertex) { return Vertex.Z > 1000.0f; }));
	return true;
}

#endif
