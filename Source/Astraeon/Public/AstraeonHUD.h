#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AstraeonHUD.generated.h"

class AAstraeonPlayerCharacter;
class AAstraeonPlayerController;
class AAstraeonShipPawn;
class UAstraeonGameInstance;

UCLASS()
class ASTRAEON_API AAstraeonHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	static TArray<FString> BuildStatusLines(const UAstraeonGameInstance* GameInstance);
	static TArray<FString> BuildMenuLines(const AAstraeonPlayerController* PlayerController);
	static TArray<FString> BuildLogbookLines(const UAstraeonGameInstance* GameInstance);
	static TArray<FString> BuildFlightLines(const AAstraeonShipPawn* ShipPawn, const UAstraeonGameInstance* GameInstance);
	static TArray<FString> BuildFabricatorLines(const UAstraeonGameInstance* GameInstance);
	static TArray<FString> BuildInventoryLines(const UAstraeonGameInstance* GameInstance);

	// Los ids de inventario son claves de guardado y recetas, así que no pueden cambiar;
	// esto los traduce sólo para mostrarlos.
	static FString DescribeItem(FName ItemId);
	static TArray<FString> BuildBuildModeLines(const AAstraeonPlayerCharacter* PlayerCharacter, const UAstraeonGameInstance* GameInstance);
};
