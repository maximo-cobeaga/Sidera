#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AstraeonPlayerController.generated.h"

class AAstraeonShipPawn;

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

	UFUNCTION(BlueprintPure, Category = "Astraeon|Knowledge")
	bool IsLogbookVisible() const { return bLogbookVisible; }

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Knowledge")
	void ToggleLogbook();

	// Ítaca despega con el jugador a bordo: el personaje se guarda, la estancia se oculta
	// y el control pasa a la nave hasta que se aterriza en otro punto de la región.
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Ship")
	bool BeginShipFlight();

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Ship")
	bool RequestShipLanding();

	UFUNCTION(BlueprintPure, Category = "Astraeon|Inventory")
	bool IsInventoryVisible() const { return bInventoryVisible; }

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Inventory")
	void ToggleInventory();

	UFUNCTION(BlueprintPure, Category = "Astraeon|Crafting")
	bool IsFabricatorOpen() const { return bFabricatorOpen; }

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Crafting")
	void SetFabricatorOpen(bool bOpen);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Ship")
	bool IsFlyingShip() const { return ShipPawn != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Ship")
	AAstraeonShipPawn* GetShipPawn() const { return ShipPawn; }

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

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Knowledge")
	bool bLogbookVisible = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Inventory")
	bool bInventoryVisible = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Crafting")
	bool bFabricatorOpen = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Ship")
	TObjectPtr<AAstraeonShipPawn> ShipPawn;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Ship")
	TObjectPtr<APawn> GroundedPawn;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Menu")
	int32 SelectedMenuSeed = 1001;
};
