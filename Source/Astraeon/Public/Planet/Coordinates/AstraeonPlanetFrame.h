#pragma once

#include "CoreMinimal.h"

// Marco de referencia local sobre un cuerpo esférico. Es la pieza más baja del módulo Planet y
// deliberadamente no conoce actores, mundo ni motor: son funciones puras de vectores, así que se
// pueden probar sin cargar un nivel y no pueden divergir entre el runtime y las pruebas.
//
// Convención (ADR 0004): el vector arriba local es Normalize(Posición − CentroDelPlaneta).
// Ningún consumidor puede asumir que el eje Z global representa arriba.
//
// Unidades: centímetros Unreal, como el resto del proyecto. El radio del planeta entra como dato,
// nunca como escala de una malla.
struct ASTRAEON_API FAstraeonPlanetFrame
{
	// Por debajo de esta distancia al centro no existe una dirección radial definida y cualquier
	// normalización devolvería basura. Las consultas caen a un valor documentado en lugar de
	// propagar un vector cero: un fallback silencioso aquí se manifestaría como el personaje
	// mirando a un sitio arbitrario en el centro exacto del planeta.
	static constexpr double DegenerateRadiusCm = 1.0;

	// Arriba local. Es la definición normativa del ADR 0004 §Decisión.
	static FVector UpAt(const FVector& PlanetCenterCm, const FVector& LocationCm);

	// Hacia dónde cae. Es lo que espera UCharacterMovementComponent::SetGravityDirection.
	static FVector GravityDirectionAt(const FVector& PlanetCenterCm, const FVector& LocationCm);

	// Altura sobre la superficie de referencia. Negativa por debajo del radio.
	static double AltitudeCm(const FVector& PlanetCenterCm, double PlanetRadiusCm, const FVector& LocationCm);

	// Componente del vector contenida en el plano tangente. El movimiento se proyecta aquí para
	// que caminar no empuje contra la gravedad ni la acompañe.
	static FVector ProjectToTangent(const FVector& VectorCm, const FVector& UpUnit);

	// Rotación que alinea el eje Z del actor con el arriba local conservando su frente todo lo
	// posible. Es lo que impide que la cápsula quede tumbada al cruzar hacia el otro hemisferio.
	//
	// En los polos el frente puede quedar paralelo al arriba y su proyección tangente se anula;
	// en ese caso el marco se reconstruye desde el costado, que sigue siendo válido. Sin esa rama
	// la orientación daría un salto justo en los dos puntos donde más se nota.
	static FQuat AlignToUp(const FQuat& CurrentRotation, const FVector& UpUnit);

	// Interpolación de la alineación anterior. Un salto instantáneo es correcto en geometría y
	// desagradable en pantalla: la orientación se transporta, no se teletransporta.
	static FQuat AlignToUpInterpolated(const FQuat& CurrentRotation, const FVector& UpUnit,
		float DeltaSeconds, float DegreesPerSecond);

	// Comprobación de que un marco es utilizable: ortonormal y con Z sobre el arriba pedido.
	// La usan las pruebas y las validaciones; no es un `check` de runtime.
	static bool IsFrameAligned(const FQuat& Rotation, const FVector& UpUnit, double ToleranceDeg = 0.5);
};
