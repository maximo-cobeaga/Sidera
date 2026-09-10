#include "Planet/Patches/AstraeonPlanetProceduralPatchBackend.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

bool FAstraeonPlanetProceduralPatchBackend::CommitHidden(const FAstraeonPlanetPatchBuildResult& Mesh)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Astraeon_PlanetPatches_Commit);
	if (!Root || !Root->GetOwner() || Live.Contains(Mesh.Address)) return false;
	UProceduralMeshComponent* Component = Free.IsEmpty() ? nullptr : Free.Pop(EAllowShrinking::No);
	if (!Component)
	{
		Component = NewObject<UProceduralMeshComponent>(Root->GetOwner());
		Component->SetupAttachment(Root);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCanEverAffectNavigation(false);
		Component->RegisterComponent();
		Components.Add(Component);
	}
	// Origin in double, vertices relative to it: the float conversion happens near the patch.
	Component->SetRelativeLocation(Mesh.OriginBodyCm);
	Component->CreateMeshSection(0, Mesh.Vertices, Mesh.Indices, Mesh.Normals, Mesh.UVs,
		TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	if (Material) Component->SetMaterial(0, Material);
	Component->SetVisibility(false);
	Live.Add(Mesh.Address, Component);
	return true;
}

void FAstraeonPlanetProceduralPatchBackend::SetVisible(const FAstraeonPlanetPatchAddress& Address, bool bVisible)
{
	if (UProceduralMeshComponent* const* Component = Live.Find(Address)) (*Component)->SetVisibility(bVisible);
}

void FAstraeonPlanetProceduralPatchBackend::Remove(const FAstraeonPlanetPatchAddress& Address)
{
	UProceduralMeshComponent* Component = nullptr;
	if (!Live.RemoveAndCopyValue(Address, Component)) return;
	Component->SetVisibility(false);
	Component->ClearAllMeshSections();
	Free.Add(Component);
}

void FAstraeonPlanetProceduralPatchBackend::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Root);
	Collector.AddReferencedObject(Material);
	Collector.AddReferencedObjects(Components);
}
