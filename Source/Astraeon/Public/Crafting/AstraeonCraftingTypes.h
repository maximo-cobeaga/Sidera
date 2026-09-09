#pragma once

#include "CoreMinimal.h"
#include "AstraeonCraftingTypes.generated.h"

USTRUCT(BlueprintType)
struct ASTRAEON_API FAstraeonRecipeInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Crafting")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Crafting")
	int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct ASTRAEON_API FAstraeonCraftingRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Crafting")
	FName RecipeId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Crafting")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Crafting")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Crafting")
	TArray<FAstraeonRecipeInput> Inputs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Crafting")
	FName OutputItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Crafting")
	int32 OutputQuantity = 1;
};
