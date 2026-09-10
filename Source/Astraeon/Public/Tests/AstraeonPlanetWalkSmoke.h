#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstraeonPlanetWalkSmoke.generated.h"

/**
 * Camina sobre el harness planetario y comprueba lo que la primera prueba manual encontró roto.
 *
 * Existe porque esos tres defectos —el personaje volcándose boca abajo, el deslizamiento sin
 * control tras saltar y la caída a través de la esfera— son de comportamiento sostenido, y una
 * prueba unitaria de matemática no los ve: la matemática del marco estaba bien y aun así el
 * personaje terminaba cabeza abajo, porque quien lo rotaba era otro sistema.
 *
 * Lo que NO cubre, a propósito: la sensación de la cámara. Que la vista acompañe al jugador sin
 * marearlo sigue necesitando una partida humana, y es el criterio abierto de la Fase 0.
 *
 * Uso:
 *   Astraeon.exe /Game/Maps/TL_10_RadialGravity -game -AstraeonSmokePlanetWalk
 */
UCLASS()
class ASTRAEON_API AAstraeonPlanetWalkSmoke : public AActor
{
	GENERATED_BODY()

public:
	AAstraeonPlanetWalkSmoke();

	virtual void Tick(float DeltaSeconds) override;

private:
	// Cuánto camina antes de juzgar. Media vuelta a 200 m de radio son ~630 m; con la velocidad
	// del personaje esto alcanza para cruzar el ecuador, que es donde apareció el vuelco.
	static constexpr float DefaultWalkSeconds = 75.0f;

	// Ajustable por linea de comandos: -AstraeonWalkSeconds=240 cubre la media vuelta completa
	// hasta el antipoda, que es donde aparecio el vuelco en la prueba manual.
	float WalkSeconds = DefaultWalkSeconds;

	// El salto va en mitad del recorrido: el deslizamiento reportado aparecía al saltar y
	// desplazarse en el aire.
	static constexpr float JumpEverySeconds = 4.0f;

	// Margen de altitud tolerado sobre la superficie. Por debajo, el personaje atravesó la
	// esfera; muy por encima, salió despedido.
	static constexpr double MinAltitudeCm = -50.0;
	static constexpr double MaxAltitudeCm = 400.0;

	// Coseno mínimo entre el arriba de la cápsula y el arriba local. 0,95 son unos 18 grados:
	// tolera la inclinación transitoria de un salto y no tolera un vuelco.
	static constexpr double MinUpAlignment = 0.95;

	void Finish(bool bPassed, const FString& Reason);

	float Elapsed = 0.0f;
	bool bStartedGame = false;
	float LastProgressLogSeconds = 0.0f;
	float LastJumpSeconds = 0.0f;
	int32 JumpsRequested = 0;
	float CurrentFallSeconds = 0.0f;
	float LongestFallSeconds = 0.0f;
	int32 FramesJumpClip = 0;

	// Experimento de control del 2026-09-10: la locomocion se ve cortada aunque nadie salte.
	// `Jump_Land` dura 0,6 s y tapa el ciclo, asi que contarlo aparte separa "el clip esta mal
	// animado" de "algo lo interrumpe". `LocomotionRestarts` es la medida directa del corte.
	int32 FramesLandClip = 0;
	int32 LocomotionRestarts = 0;
	FName LastClipId;
	// Reparto de clips y transiciones: sin esto "cambia 2,7 veces por segundo" no dice
	// ENTRE QUE dos clips oscila, que es la unica pregunta que queda.
	TMap<FName,int32> ClipFrames;
	TMap<FString,int32> ClipTransitions;
	double SpeedSumCms = 0.0;
	double MinSpeedCms = 1e9;
	double MaxSpeedCms = 0.0;
	// El defecto del 2026-09-10: caminando en linea recta, el personaje se quedaba sin suelo
	// dos frames cada vez que se rehacia la colision cercana, la velocidad caia a 0 y el
	// selector elegia `Idle`, reiniciando el ciclo de paso ~1,3 veces por segundo. Contar los
	// frames en `Idle` mientras se camina es la medida directa de que eso no vuelve.
	int32 FramesIdleWhileMoving = 0;
	bool bFinished = false;

	// Se acumulan en vez de abortar al primer frame malo: un frame aislado no es un defecto, y
	// distinguir "una vez" de "todo el rato" es justamente lo que hace falta aquí.
	int32 FramesSampled = 0;
	int32 FramesMisaligned = 0;
	int32 FramesFalling = 0;
	int32 FramesOutOfAltitude = 0;
	double WorstAlignment = 1.0;
	double WorstAltitudeCm = 0.0;
	double DistanceTravelledCm = 0.0;
	FVector LastLocationCm = FVector::ZeroVector;
};
