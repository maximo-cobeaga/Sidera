#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WorldGen/AstraeonRegionTypes.h"
#include "AstraeonRegionMaterializer.generated.h"

UENUM(BlueprintType)
enum class EAstraeonRegionActorKind : uint8
{
	Resource UMETA(DisplayName = "Resource"),
	PointOfInterest UMETA(DisplayName = "Point Of Interest")
};

USTRUCT(BlueprintType)
struct ASTRAEON_API FAstraeonRegionActorSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	FName ActorId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	EAstraeonRegionActorKind Kind = EAstraeonRegionActorKind::Resource;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	FVector LocationCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|WorldGen")
	FVector Scale = FVector(1.0f, 1.0f, 1.0f);
};

UCLASS()
class ASTRAEON_API UAstraeonRegionMaterializer : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static TArray<FAstraeonRegionActorSpec> BuildActorSpecs(const FAstraeonRegionLayout& Layout);
};
