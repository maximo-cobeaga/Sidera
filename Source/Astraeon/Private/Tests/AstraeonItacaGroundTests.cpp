#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AstraeonGameInstance.h"
#include "Environment/AstraeonItacaInterior.h"
#include "WorldGen/AstraeonTerrainField.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonItacaGroundTest,
	"Astraeon.WorldGen.Itaca.DeckRestsOnTerrain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonItacaGroundTest::RunTest(const FString& Parameters)
{
	const float Deck = AAstraeonItacaInterior::GetDeckThicknessCm();
	TestTrue(TEXT("The deck has real thickness"), Deck > 0.0f);

	for (const int32 Seed : {0, 7, 42, 606, 13579, 99999})
	{
		UAstraeonGameInstance* GameInstance = NewObject<UAstraeonGameInstance>();
		GameInstance->StartNewGame(Seed);
		const FVector Origin = GameInstance->GetItacaOriginCm();
		// La instancia normaliza la seed pedida, así que la cota se consulta con la seed
		// efectiva: comparar contra la pedida probaría otra cosa.
		const int32 TerrainSeed = GameInstance->GetCurrentTerrainSeed();
		const float Pad = AAstraeonTerrainField::GetItacaPadHeightCm(TerrainSeed, Origin.X, Origin.Y);

		// La invariante que se rompió: el piso de la estancia ocupa de -12 a 0 en el espacio
		// del actor, y el bloque de terreno bajo la nave llega hasta la cota de la
		// plataforma. Si la estancia se posa EN esa cota en vez de SOBRE ella, las dos caras
		// superiores coinciden: se ve roca en lugar de la cubierta y el jugador queda
		// atrapado entre ambas superficies. Pasaba ya al iniciar partida, sin volar.
		TestTrue(FString::Printf(TEXT("Seed %d: the deck rests on the terrain pad"), Seed),
			FMath::IsNearlyEqual(Origin.Z, Pad + Deck, 0.01f));
		TestTrue(FString::Printf(TEXT("Seed %d: the room floor clears the terrain"), Seed),
			Origin.Z - Deck >= Pad - 0.01f);

		// La plataforma nunca puede quedar por debajo del umbral de instanciado: si su
		// baldosa se descarta, la estancia se apoya sobre un agujero.
		TestTrue(FString::Printf(TEXT("Seed %d: the pad is always instanced"), Seed), Pad >= 20.0f);

		// El umbral de la escotilla tiene que seguir siendo caminable hacia la superficie.
		TestTrue(FString::Printf(TEXT("Seed %d: the hatch threshold stays walkable"), Seed),
			Deck <= AAstraeonTerrainField::GetMaxWalkableStepCm());
	}
	return true;
}

#endif
