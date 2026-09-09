#include "Building/AstraeonBuiltStructure.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace AstraeonBuilding
{
	// Medidas pensadas contra la cápsula del jugador (1,92 m de alto): un muro tapa, una
	// plataforma se pisa y un pilar se rodea.
	const FVector WallSizeCm(320.0f, 40.0f, 260.0f);
	const FVector FloorSizeCm(320.0f, 320.0f, 30.0f);
	const FVector PillarSizeCm(60.0f, 60.0f, 300.0f);
}

AAstraeonBuiltStructure::AAstraeonBuiltStructure()
{
	PrimaryActorTick.bCanEverTick = false;

	StructureMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StructureMesh"));
	RootComponent = StructureMesh;
	StructureMesh->SetCollisionProfileName(TEXT("BlockAll"));
	StructureMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	StructureMesh->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		StructureMesh->SetStaticMesh(CubeMesh.Object);
	}
}

void AAstraeonBuiltStructure::ApplyPlacement(const FAstraeonPlacedStructure& NewPlacement)
{
	Placement = NewPlacement;

	const FVector SizeCm = GetStructureSizeCm(Placement.Type);
	SetActorLocation(Placement.LocationCm);
	SetActorRotation(Placement.Rotation);
	// El cubo base mide 100 cm, así que la escala es el tamaño en metros.
	SetActorScale3D(SizeCm / 100.0f);

	if (StructureMesh)
	{
		// Ladrillo de regolito compactado: cálido y mate, distinto del terreno y del metal
		// de Ítaca para que se lea como obra humana.
		StructureMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.42f, 0.30f, 0.22f));
	}

#if WITH_EDITOR
	SetActorLabel(FString::Printf(TEXT("Built_%s"), *DescribeStructure(Placement.Type)));
#endif
}

FVector AAstraeonBuiltStructure::GetStructureSizeCm(EAstraeonStructureType Type)
{
	switch (Type)
	{
	case EAstraeonStructureType::Floor:
		return AstraeonBuilding::FloorSizeCm;
	case EAstraeonStructureType::Pillar:
		return AstraeonBuilding::PillarSizeCm;
	default:
		return AstraeonBuilding::WallSizeCm;
	}
}

int32 AAstraeonBuiltStructure::GetStructureBrickCost(EAstraeonStructureType Type)
{
	switch (Type)
	{
	case EAstraeonStructureType::Floor:
		return 3;
	case EAstraeonStructureType::Pillar:
		return 2;
	default:
		return 4;
	}
}

int32 AAstraeonBuiltStructure::GetStructureRefund(EAstraeonStructureType Type)
{
	return FMath::Max(1, GetStructureBrickCost(Type) / 2);
}

FString AAstraeonBuiltStructure::DescribeStructure(EAstraeonStructureType Type)
{
	switch (Type)
	{
	case EAstraeonStructureType::Floor:
		return TEXT("Plataforma");
	case EAstraeonStructureType::Pillar:
		return TEXT("Pilar");
	default:
		return TEXT("Muro");
	}
}
