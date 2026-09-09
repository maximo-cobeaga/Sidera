#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "WorldGen/AstraeonRegionMaterializer.h"
#include "WorldGen/AstraeonTerrainTraversal.h"
#include "WorldGen/AstraeonWorldProfiles.h"

namespace AstraeonTraversalTest
{
	// Las cinco seeds de contenido que `Docs/MVP_WORLD_ARCHITECTURE.md` exige comprobar,
	// más la seed del smoke y la variante segura, que tiene que valerse por sí misma.
	const TArray<int32> Seeds = { 100, 200, 300, 400, 500, 13579, 1001 };

	FVector2D DeploymentXY(const FVector& ItacaOriginCm)
	{
		const FVector Deployment = UAstraeonRegionMaterializer::GetSurfaceDeploymentLocationCm(ItacaOriginCm);
		return FVector2D(Deployment.X, Deployment.Y);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonTerrainConnectivityTest,
	"Astraeon.WorldGen.Terrain.Connectivity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonTerrainConnectivityTest::RunTest(const FString& Parameters)
{
	// El límite tiene que ser el del motor, no uno inventado: si fuera más permisivo la
	// prueba aprobaría laderas por las que el jugador resbala.
	const float SpacingCm = FAstraeonTerrainTraversal::GetSampleSpacingCm();
	TestTrue(TEXT("La validación mide la superficie a la resolución de la malla"),
		FMath::IsNearlyEqual(SpacingCm, AAstraeonTerrainField::GetSurfaceSpacingCm()));
	TestTrue(TEXT("El desnivel admitido nunca baja del escalón de la cápsula"),
		FAstraeonTerrainTraversal::GetMaxWalkableRiseCm(SpacingCm) >= AAstraeonTerrainField::GetMaxWalkableStepCm());

	const FVector ItacaOriginCm = FVector::ZeroVector;
	for (const int32 Seed : AstraeonTraversalTest::Seeds)
	{
		const FAstraeonRegionLayout Layout = UAstraeonWorldProfiles::BuildFixedRegionLayout(Seed);
		const TArray<FAstraeonTraversalGoal> Goals = UAstraeonRegionMaterializer::BuildTraversalGoals(Layout);
		TestTrue(FString::Printf(TEXT("Seed %d declara objetivos que alcanzar"), Seed), Goals.Num() > 0);

		const FAstraeonTerrainSurfaceContext Context =
			UAstraeonRegionMaterializer::BuildSurfaceContext(Seed, ItacaOriginCm, Layout);
		const FAstraeonTerrainSeedResolution Resolution = FAstraeonTerrainTraversal::ResolveTerrainSeed(
			Context, AstraeonTraversalTest::DeploymentXY(ItacaOriginCm), Goals);

		// Lo que se exige no es que la seed pedida sirva, sino que la publicada sí: ninguna
		// partida puede acabar con un recurso crítico o la señal detrás de un muro.
		TestTrue(FString::Printf(TEXT("Seed %d publica una región transitable"), Seed), Resolution.Report.bPassed);
		TestEqual(FString::Printf(TEXT("Seed %d no deja objetivos inalcanzables"), Seed),
			Resolution.Report.UnreachableGoals.Num(), 0);
		TestFalse(FString::Printf(TEXT("Seed %d no necesita la variante segura"), Seed), Resolution.bUsedFallback);

		// Determinismo: la misma seed tiene que resolver siempre al mismo relieve, o cargar
		// una partida guardada devolvería otra región.
		const FAstraeonTerrainSeedResolution Repeat = FAstraeonTerrainTraversal::ResolveTerrainSeed(
			Context, AstraeonTraversalTest::DeploymentXY(ItacaOriginCm), Goals);
		TestEqual(FString::Printf(TEXT("Seed %d resuelve siempre al mismo relieve"), Seed),
			Repeat.TerrainSeed, Resolution.TerrainSeed);
	}

	// La variante segura no puede ser una esperanza: se comprueba con los mismos objetivos.
	{
		const int32 FallbackSeed = FAstraeonTerrainTraversal::GetFallbackTerrainSeed();
		const FAstraeonRegionLayout Layout = UAstraeonWorldProfiles::BuildFixedRegionLayout(FallbackSeed);
		const FAstraeonTerrainSurfaceContext Context =
			UAstraeonRegionMaterializer::BuildSurfaceContext(FallbackSeed, ItacaOriginCm, Layout);
		const FAstraeonTraversalReport Report = FAstraeonTerrainTraversal::Evaluate(Context,
			AstraeonTraversalTest::DeploymentXY(ItacaOriginCm),
			UAstraeonRegionMaterializer::BuildTraversalGoals(Layout));
		TestTrue(TEXT("La variante segura es transitable por sí misma"), Report.bPassed);
	}

	// Ítaca puede aterrizar en cualquier parte de la región. El campo sigue cubriendo la
	// región entera y la huella plana viaja con la nave, así que la garantía se mantiene.
	{
		const FVector LandedOriginCm(12000.0f, -8000.0f, 0.0f);
		const FAstraeonRegionLayout Layout = UAstraeonWorldProfiles::BuildFixedRegionLayout(1001);
		const FAstraeonTerrainSurfaceContext Context =
			UAstraeonRegionMaterializer::BuildSurfaceContext(1001, LandedOriginCm, Layout);
		const FAstraeonTerrainSeedResolution Resolution = FAstraeonTerrainTraversal::ResolveTerrainSeed(
			Context, AstraeonTraversalTest::DeploymentXY(LandedOriginCm),
			UAstraeonRegionMaterializer::BuildTraversalGoals(Layout));
		TestTrue(TEXT("Con Ítaca aterrizada lejos la región sigue siendo transitable"), Resolution.Report.bPassed);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonTerrainTraversalDetectsWallsTest,
	"Astraeon.WorldGen.Terrain.TraversalDetectsWalls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonTerrainTraversalDetectsWallsTest::RunTest(const FString& Parameters)
{
	// Una prueba de conectividad que aprueba siempre no prueba nada. Esta comprueba que
	// detecta el caso que existe para detectar: un objetivo fuera del campo de la región,
	// al que no se llega por mucho que el relieve sea suave.
	const FAstraeonRegionLayout Layout = UAstraeonWorldProfiles::BuildFixedRegionLayout(1001);
	const FAstraeonTerrainSurfaceContext Context =
		UAstraeonRegionMaterializer::BuildSurfaceContext(1001, FVector::ZeroVector, Layout);

	TArray<FAstraeonTraversalGoal> Goals = UAstraeonRegionMaterializer::BuildTraversalGoals(Layout);
	const int32 ReachableGoals = Goals.Num();

	FAstraeonTraversalGoal Unreachable;
	Unreachable.GoalId = TEXT("prueba_fuera_de_la_region");
	// Fuera del campo de la región. Fabricar un muro de relieve dependería de qué produzca
	// la seed del día; quedarse fuera del campo es inalcanzable por construcción.
	Unreachable.LocationCm = FVector2D(AAstraeonTerrainField::GetFieldRadiusCm() * 4.0f, 0.0f);
	Goals.Add(Unreachable);

	const FAstraeonTraversalReport Report = FAstraeonTerrainTraversal::Evaluate(Context,
		FVector2D(900.0f, 0.0f), Goals);
	TestFalse(TEXT("Un objetivo inalcanzable hace fallar la validación"), Report.bPassed);
	// Se nombra el objetivo, no sólo el fallo: sin eso el reporte no diría qué arreglar.
	// No se exige que sea el único, porque la seed pedida puede tener sus propios
	// objetivos tapados; para eso existe la resolución de sub-seeds.
	TestTrue(TEXT("El objetivo de fuera de la región aparece señalado"),
		Report.UnreachableGoals.Contains(Unreachable.GoalId));
	TestTrue(TEXT("La región declara objetivos que alcanzar"), ReachableGoals > 0);
	return true;
}

#endif
