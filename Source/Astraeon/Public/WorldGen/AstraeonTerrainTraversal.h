#pragma once

#include "CoreMinimal.h"
#include "WorldGen/AstraeonTerrainField.h"

// Objetivo que la región tiene que dejar alcanzable a pie desde la salida de Ítaca. El id
// existe para poder decir cuál falló, no para buscarlo: la validación no conoce gameplay.
struct FAstraeonTraversalGoal
{
	FName GoalId;
	FVector2D LocationCm = FVector2D::ZeroVector;
};

struct FAstraeonTraversalReport
{
	bool bPassed = false;
	TArray<FName> UnreachableGoals;
	int32 ReachableCells = 0;
	// Peor desnivel entre celdas vecinas dentro de lo alcanzado. Sirve para ver el margen
	// que queda contra el límite, no como criterio: el criterio es alcanzar los objetivos.
	float WorstReachableRiseCm = 0.0f;
};

struct FAstraeonTerrainSeedResolution
{
	int32 TerrainSeed = 0;
	int32 AttemptsUsed = 0;
	bool bUsedFallback = false;
	// Qué objetivos quedaban tapados con la seed pedida. Sin esto el log diría que se
	// cambió de relieve pero no por culpa de qué punto del mapa.
	TArray<FName> RejectedGoals;
	FAstraeonTraversalReport Report;
};

// Comprueba que la superficie que se va a materializar se pueda recorrer a pie, y elige una
// seed de relieve que lo cumpla. El contrato de `Docs/PROCEDURAL_TERRAIN_CONTRACT.md` dice
// que una seed inválida no se publica: aquí se decide cuál se publica.
class ASTRAEON_API FAstraeonTerrainTraversal
{
public:
	// Recorre la superficie a la misma resolución con la que se construye la malla. Entre
	// dos vértices la malla es un plano, así que validar a otra resolución mediría una
	// superficie que el jugador nunca pisa.
	static float GetSampleSpacingCm();

	// Lo que el personaje puede superar entre dos muestras vecinas: el escalón de la cápsula
	// o la pendiente caminable, lo que sea mayor. Por debajo de eso el motor lo deja subir;
	// por encima, resbala, y el objetivo queda del otro lado de un muro.
	static float GetMaxWalkableRiseCm(float SpacingCm);

	static int32 GetMaxSeedAttempts();

	// Variante segura documentada, usada sólo si ninguna sub-seed pasa. Está cubierta por
	// `Astraeon.WorldGen.Terrain.Connectivity`, así que es una garantía y no una esperanza.
	static int32 GetFallbackTerrainSeed();

	static FAstraeonTraversalReport Evaluate(const FAstraeonTerrainSurfaceContext& Context,
		const FVector2D& StartCm, const TArray<FAstraeonTraversalGoal>& Goals);

	// Devuelve la primera sub-seed determinista cuya región es transitable. El contexto
	// entra con la seed pedida; la que sale es la que deben usar todos los consumidores.
	static FAstraeonTerrainSeedResolution ResolveTerrainSeed(const FAstraeonTerrainSurfaceContext& BaseContext,
		const FVector2D& StartCm, const TArray<FAstraeonTraversalGoal>& Goals);

	// Aplica una seed ya resuelta al contexto, recalculando lo que depende de ella.
	static FAstraeonTerrainSurfaceContext WithSeed(const FAstraeonTerrainSurfaceContext& Context, int32 TerrainSeed);
};
