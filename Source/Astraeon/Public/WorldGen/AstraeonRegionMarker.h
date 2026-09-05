#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldGen/AstraeonRegionMaterializer.h"
#include "AstraeonRegionMarker.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class ASTRAEON_API AAstraeonRegionMarker : public AActor
{
	GENERATED_BODY()

public:
	AAstraeonRegionMarker();

	void ApplySpec(const FAstraeonRegionActorSpec& Spec);

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	FName GetMarkerId() const { return MarkerId; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	EAstraeonRegionActorKind GetMarkerKind() const { return MarkerKind; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static FText BuildMarkerLabel(FName ActorId, EAstraeonRegionActorKind Kind);

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static FColor BuildMarkerColor(FName ActorId, EAstraeonRegionActorKind Kind);

private:
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|WorldGen")
	TObjectPtr<UStaticMeshComponent> MarkerMesh;

	UPROPERTY(VisibleAnywhere, Category = "Astraeon|WorldGen")
	TObjectPtr<UTextRenderComponent> MarkerLabel;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|WorldGen")
	FName MarkerId;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|WorldGen")
	EAstraeonRegionActorKind MarkerKind = EAstraeonRegionActorKind::Resource;
};
