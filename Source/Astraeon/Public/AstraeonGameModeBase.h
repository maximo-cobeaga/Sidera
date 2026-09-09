#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AstraeonGameModeBase.generated.h"

UCLASS()
class ASTRAEON_API AAstraeonGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAstraeonGameModeBase();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Astraeon|WorldGen")
	void MaterializeCurrentRegion();

	// Repone la fauna de un nido concreto, al materializar la región y al vencer su reloj.
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Creatures")
	class AAstraeonCreatureActor* SpawnCreatureAtPoint(FName SpawnPointId, const FVector2D& LocationMeters);

	// Durante el vuelo la estancia se oculta: el jugador está pilotando Ítaca desde fuera.
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Ship")
	void SetItacaInteriorHidden(bool bInteriorHidden);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<class UMaterialInterface> TerrainGeologyMaterial;

	void RunCriticalPathSmokeIfRequested();
	bool RunSurfaceHatchInteractionSmoke();
	void EnsureRuntimeLighting();
};
