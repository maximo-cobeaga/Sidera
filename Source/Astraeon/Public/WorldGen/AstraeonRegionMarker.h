#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldGen/AstraeonRegionMaterializer.h"
#include "AstraeonRegionMarker.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UBoxComponent;
class UTextRenderComponent;

UCLASS()
class ASTRAEON_API AAstraeonRegionMarker : public AActor
{
	GENERATED_BODY()

public:
	// Las estaciones de Ítaca traen su propia malla y su propia caja de colisión; el resto
	// de los marcadores usan el proxy genérico y el color por tipo.
	static bool IsItacaStation(FName ActorId);

	// Todo lo que viaja con la estancia: las tres estaciones más la escotilla. Se oculta y
	// se vuelve a mostrar junto con Ítaca cuando la nave despega y aterriza.
	static bool BelongsToItacaInterior(FName ActorId);

	AAstraeonRegionMarker();

	void ApplySpec(const FAstraeonRegionActorSpec& Spec);

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	FName GetMarkerId() const { return MarkerId; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	EAstraeonRegionActorKind GetMarkerKind() const { return MarkerKind; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	FName GetRequiredToolId() const { return RequiredToolId; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static FText BuildMarkerLabel(FName ActorId, EAstraeonRegionActorKind Kind);

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	static FColor BuildMarkerColor(FName ActorId, EAstraeonRegionActorKind Kind);

private:
	// Hard references on the CDO make the original presentation meshes discoverable by cook.
	UPROPERTY()
	TMap<FName, TObjectPtr<UStaticMesh>> RegionArtMeshes;
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Presentation")
	TObjectPtr<UStaticMeshComponent> RegionArt;
	UPROPERTY()
	TObjectPtr<UStaticMesh> ItacaConsoleMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> ItacaPilotConsoleMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> ItacaFabricatorMesh;


	UPROPERTY(VisibleAnywhere, Category = "Astraeon|WorldGen")
	TObjectPtr<UBoxComponent> ConsoleCollision;
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|WorldGen")
	TObjectPtr<UStaticMeshComponent> MarkerMesh;

	UPROPERTY(VisibleAnywhere, Category = "Astraeon|WorldGen")
	TObjectPtr<UTextRenderComponent> MarkerLabel;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|WorldGen")
	FName MarkerId;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|WorldGen")
	EAstraeonRegionActorKind MarkerKind = EAstraeonRegionActorKind::Resource;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|WorldGen")
	FName RequiredToolId;
};
