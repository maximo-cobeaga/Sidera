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

	UFUNCTION(BlueprintCallable, Category = "Astraeon|WorldGen")
	void MaterializeCurrentRegion();

protected:
	virtual void BeginPlay() override;

private:
	void RunCriticalPathSmokeIfRequested();
	bool RunSurfaceHatchInteractionSmoke();
	void EnsureRuntimeLighting();
};
