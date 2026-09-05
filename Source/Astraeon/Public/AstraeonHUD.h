#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AstraeonHUD.generated.h"

class AAstraeonPlayerController;
class UAstraeonGameInstance;

UCLASS()
class ASTRAEON_API AAstraeonHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	static TArray<FString> BuildStatusLines(const UAstraeonGameInstance* GameInstance);
	static TArray<FString> BuildMenuLines(const AAstraeonPlayerController* PlayerController);
};
