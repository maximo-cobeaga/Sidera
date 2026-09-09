#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

class AActor;

ASTRAEON_API DECLARE_LOG_CATEGORY_EXTERN(LogAstraeonDiag, Log, All);

/**
 * Instrumentación de sesión, apagada salvo que se arranque con `-AstraeonDiag`.
 *
 * Existe porque los síntomas que se ven jugando —la vista que gira, el suelo que parpadea
 * "entre líneas", el rescate que devuelve a la nave— no dicen dónde está la causa. Esto
 * escribe al log del juego la posición, el suelo que hay bajo los pies y, sobre todo, qué
 * geometría se solapa con la cápsula: dos superficies coincidentes bajo el jugador son la
 * explicación habitual de un empuje que parece aleatorio.
 *
 * Los mensajes en pantalla no sirven acá: el GameMode ejecuta `DisableAllScreenMessages` en
 * BeginPlay, así que `AddOnScreenDebugMessage` es un no-op silencioso (ver KNOWN_ISSUES).
 */
namespace AstraeonDiagnostics
{
	ASTRAEON_API bool IsEnabled();

	/** Qué hay bajo los pies: actor, componente, cota de impacto y distancia. */
	ASTRAEON_API FString DescribeGroundUnder(const AActor& Actor, float TraceDistanceCm);

	/** Geometría bloqueante que solapa la cápsula del jugador aquí y ahora. */
	ASTRAEON_API FString DescribeBlockingOverlaps(const AActor& Actor, float RadiusCm, float HalfHeightCm);
}
