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
	static constexpr float JumpAtSeconds = 30.0f;

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
	bool bJumped = false;
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
