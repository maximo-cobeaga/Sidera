#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "Planet/Patches/AstraeonPlanetPatchManager.h"

class USceneComponent;
class UMaterialInterface;
class UProceduralMeshComponent;

// Phase 2 backend: one ProceduralMeshComponent per patch, render only, never collision.
// Released components are cleared and reused instead of destroyed, so a relay costs a mesh
// upload rather than a component registration. Measured before any production decision.
class ASTRAEON_API FAstraeonPlanetProceduralPatchBackend final : public IAstraeonPlanetPatchMeshBackend, public FGCObject
{
public:
	FAstraeonPlanetProceduralPatchBackend(USceneComponent& InRoot, UMaterialInterface* InMaterial)
		: Root(&InRoot), Material(InMaterial) {}

	virtual bool CommitHidden(const FAstraeonPlanetPatchBuildResult& Mesh) override;
	virtual void SetVisible(const FAstraeonPlanetPatchAddress& Address, bool bVisible) override;
	virtual void Remove(const FAstraeonPlanetPatchAddress& Address) override;

	int32 GetComponentCount() const { return Components.Num(); }
	int32 GetLiveCount() const { return Live.Num(); }

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FAstraeonPlanetProceduralPatchBackend"); }

private:
	TObjectPtr<USceneComponent> Root;
	TObjectPtr<UMaterialInterface> Material;
	TArray<TObjectPtr<UProceduralMeshComponent>> Components; // Every component ever created.
	TArray<UProceduralMeshComponent*> Free;
	TMap<FAstraeonPlanetPatchAddress, UProceduralMeshComponent*> Live;
};
