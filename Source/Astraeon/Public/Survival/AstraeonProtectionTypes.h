#pragma once

#include "CoreMinimal.h"
#include "AstraeonProtectionTypes.generated.h"

// Sólo se puede llevar un módulo activo a la vez: medir el ambiente y elegir cuál
// compensa la amenaza dominante es la decisión que pide el recorrido crítico del MVP
// (paso 7, "Elegir protección básica adecuada"). Un planeta frío y despresurizado
// obliga a priorizar en vez de acumular.
UENUM(BlueprintType)
enum class EAstraeonProtectionModule : uint8
{
	None UMETA(DisplayName = "Sin protección"),
	Respirator UMETA(DisplayName = "Respirador"),
	ThermalShield UMETA(DisplayName = "Aislante térmico"),
	PressureSeal UMETA(DisplayName = "Sellado de presión")
};
