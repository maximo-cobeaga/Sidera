#include "WorldGen/AstraeonTerrainSurfacePrototype.h"

#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AAstraeonTerrainSurfacePrototype::AAstraeonTerrainSurfacePrototype()
{
	PrimaryActorTick.bCanEverTick = false;
	SurfaceMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ContinuousSurface"));
	RootComponent = SurfaceMesh;
	SurfaceMesh->SetCollisionProfileName(TEXT("BlockAll"));
	SurfaceMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SurfaceMesh->bUseAsyncCooking = true;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Geology(
		TEXT("/Game/Astraeon/Art/ItacaTerrain/M_Terrain_Geology.M_Terrain_Geology"));
	if (Geology.Succeeded())
	{
		SurfaceMesh->SetMaterial(0, Geology.Object);
	}
}

float AAstraeonTerrainSurfacePrototype::GetPrototypeSpacingCm()
{
	return AAstraeonTerrainField::GetSurfaceSpacingCm();
}

void AAstraeonTerrainSurfacePrototype::BuildMeshData(const FAstraeonTerrainSurfaceContext& Context,
	float SpacingCm, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles,
	TArray<FVector>& OutNormals, TArray<FVector2D>& OutUVs)
{
	OutVertices.Reset();
	OutTriangles.Reset();
	OutNormals.Reset();
	OutUVs.Reset();

	const float SafeSpacingCm = FMath::Max(SpacingCm, 100.0f);
	const float RadiusCm = AAstraeonTerrainField::GetFieldRadiusCm();
	const int32 QuadsPerSide = FMath::CeilToInt((RadiusCm * 2.0f) / SafeSpacingCm);
	const int32 VerticesPerSide = QuadsPerSide + 1;
	OutVertices.Reserve(VerticesPerSide * VerticesPerSide);
	OutNormals.Reserve(VerticesPerSide * VerticesPerSide);
	OutUVs.Reserve(VerticesPerSide * VerticesPerSide);
	OutTriangles.Reserve(QuadsPerSide * QuadsPerSide * 6);

	for (int32 Y = 0; Y < VerticesPerSide; ++Y)
	{
		for (int32 X = 0; X < VerticesPerSide; ++X)
		{
			const FVector2D PointCm(Context.CenterCm.X - RadiusCm + X * SafeSpacingCm,
				Context.CenterCm.Y - RadiusCm + Y * SafeSpacingCm);
			// Una sola fuente de superficie. La cuenca authored de prueba salía por aquí en
			// cuanto el contexto traía el id de Region A, así que un campo suelto podía cambiar
			// el terreno entero sin que nadie lo pidiera. Retirada del runtime según
			// Docs/PROCEDURAL_TERRAIN_CONTRACT.md.
			const FAstraeonTerrainSurfaceSample Sample = AAstraeonTerrainField::SampleSurface(Context, PointCm);
			OutVertices.Add(FVector(PointCm.X, PointCm.Y, Sample.HeightCm));
			OutNormals.Add(Sample.Normal);
			OutUVs.Add(FVector2D(static_cast<float>(X) / QuadsPerSide, static_cast<float>(Y) / QuadsPerSide));
		}
	}

	for (int32 Y = 0; Y < QuadsPerSide; ++Y)
	{
		for (int32 X = 0; X < QuadsPerSide; ++X)
		{
			const int32 BottomLeft = Y * VerticesPerSide + X;
			const int32 BottomRight = BottomLeft + 1;
			const int32 TopLeft = BottomLeft + VerticesPerSide;
			const int32 TopRight = TopLeft + 1;
			OutTriangles.Append({BottomLeft, TopLeft, BottomRight, BottomRight, TopLeft, TopRight});
		}
	}
}

void AAstraeonTerrainSurfacePrototype::BuildPrototype(const FAstraeonTerrainSurfaceContext& Context)
{
	if (!SurfaceMesh)
	{
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	BuildMeshData(Context, GetPrototypeSpacingCm(), Vertices, Triangles, Normals, UVs);
	TArray<FColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	SurfaceMesh->ClearAllMeshSections();
	SurfaceMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, true);
	VertexCount = Vertices.Num();
	TriangleCount = Triangles.Num() / 3;
}
