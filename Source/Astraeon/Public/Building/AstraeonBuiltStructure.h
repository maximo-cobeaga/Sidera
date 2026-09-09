#pragma once

#include "CoreMinimal.h"
#include "Building/AstraeonBuildingTypes.h"
#include "GameFramework/Actor.h"
#include "AstraeonBuiltStructure.generated.h"

class UStaticMeshComponent;

UCLASS()
class ASTRAEON_API AAstraeonBuiltStructure : public AActor
{
	GENERATED_BODY()

public:
	AAstraeonBuiltStructure();

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Building")
	void ApplyPlacement(const FAstraeonPlacedStructure& Placement);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Building")
	const FAstraeonPlacedStructure& GetPlacement() const { return Placement; }

	// Tamaño de cada pieza, en cm. Vive acá para que la vista previa, el actor colocado y
	// el coste consulten exactamente la misma fuente y no puedan divergir.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Building")
	static FVector GetStructureSizeCm(EAstraeonStructureType Type);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Building")
	static int32 GetStructureBrickCost(EAstraeonStructureType Type);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Building")
	static FString DescribeStructure(EAstraeonStructureType Type);

	// Lo que devuelve demoler: menos de lo que costó, para que rehacer una y otra vez no
	// sea gratis, pero sin castigar tanto como para no atreverse a corregir.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Building")
	static int32 GetStructureRefund(EAstraeonStructureType Type);

private:
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Building")
	TObjectPtr<UStaticMeshComponent> StructureMesh;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Building")
	FAstraeonPlacedStructure Placement;
};
