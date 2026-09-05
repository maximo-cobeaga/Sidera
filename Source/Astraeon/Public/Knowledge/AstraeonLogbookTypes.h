#pragma once

#include "CoreMinimal.h"
#include "AstraeonLogbookTypes.generated.h"

UENUM(BlueprintType)
enum class EAstraeonDiscoveryCertainty : uint8
{
	Unknown UMETA(DisplayName = "Unknown"),
	Observed UMETA(DisplayName = "Observed"),
	Measured UMETA(DisplayName = "Measured"),
	Inferred UMETA(DisplayName = "Inferred"),
	Confirmed UMETA(DisplayName = "Confirmed")
};

USTRUCT(BlueprintType)
struct ASTRAEON_API FAstraeonLogbookEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Knowledge")
	FName EntryId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Knowledge")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Knowledge")
	FText Summary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Knowledge")
	EAstraeonDiscoveryCertainty Certainty = EAstraeonDiscoveryCertainty::Unknown;
};
