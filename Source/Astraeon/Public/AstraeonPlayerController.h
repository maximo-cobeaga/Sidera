#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AstraeonPlayerController.generated.h"

UCLASS()
class ASTRAEON_API AAstraeonPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAstraeonPlayerController();

	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Astraeon|Menu")
	int32 GetSelectedMenuSeed() const { return SelectedMenuSeed; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Menu")
	bool IsMenuVisible() const { return bMenuVisible; }

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Menu")
	void StartSelectedNewGame();

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Menu")
	void ContinueSavedGame();

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Persistence")
	void SaveCurrentGame();

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Menu")
	void IncreaseSelectedSeed();

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Menu")
	void DecreaseSelectedSeed();

private:
	void ApplySessionToRuntime();

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Menu")
	bool bMenuVisible = true;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Menu")
	int32 SelectedMenuSeed = 1001;
};
