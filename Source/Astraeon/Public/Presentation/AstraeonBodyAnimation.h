#pragma once

#include "CoreMinimal.h"

// Acción puntual del cuerpo. Nace de un gesto de gameplay ya existente —escanear,
// interactuar, usar la herramienta— y tapa la locomoción mientras dura.
enum class EAstraeonBodyAction : uint8
{
	None,
	Scan,
	Interact,
	Pickup,
	ToolUse,
	Consume,
	Present
};

// Todo lo que necesita saber la selección de clip. Es un struct plano y sin dependencias de
// motor a propósito: así la elección se puede probar sin mundo, sin actor y sin assets.
struct FAstraeonBodyAnimationState
{
	float SpeedCms = 0.0f;
	// Velocidad en el espacio del actor: X hacia donde mira, Y a su derecha. Normalizada.
	FVector2D LocalDirection = FVector2D::ZeroVector;
	bool bFalling = false;
	float TakeoffSecondsRemaining = 0.0f;
	float LandingSecondsRemaining = 0.0f;
	EAstraeonBodyAction Action = EAstraeonBodyAction::None;
	float ActionSecondsRemaining = 0.0f;
};

struct FAstraeonBodyClip
{
	FName ClipId;
	bool bLoop = true;
};

/**
 * Elige qué clip del protagonista corresponde a un estado.
 *
 * El set importado trae 45 clips y el runtime reproducía cinco: caminar de lado usaba el
 * clip de caminar de frente, escanear no tenía gesto de cuerpo y el salto era un único
 * bucle sin despegue ni aterrizaje. Esto no añade arte: engancha el que ya estaba.
 *
 * Sin BlendSpace: la reproducción sigue siendo de un solo nodo, pero el selector aplica
 * histeresis y el componente conserva la fase normalizada al cambiar entre ciclos.
 */
class ASTRAEON_API FAstraeonBodyAnimation
{
public:
	// Por debajo de esto el personaje se considera quieto: el ruido de velocidad de la
	// cápsula al apoyarse bastaba para encender el clip de caminar.
	static float GetStandingSpeedCms();

	// A partir de aquí corre. Coincide con el umbral que ya usaban las manos.
	static float GetRunSpeedThresholdCms();

	static FAstraeonBodyClip Choose(const FAstraeonBodyAnimationState& State);

	// Todos los ids que Choose puede devolver. La prueba lo recorre para exigir que cada uno
	// exista como asset: un id mal escrito dejaría al cuerpo congelado sin avisar.
	static TArray<FName> GetAllClipIds();

	static FName GetClipIdForAction(EAstraeonBodyAction Action);
};
