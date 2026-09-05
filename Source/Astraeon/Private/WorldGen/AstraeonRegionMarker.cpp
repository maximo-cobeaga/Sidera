#include "WorldGen/AstraeonRegionMarker.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"

AAstraeonRegionMarker::AAstraeonRegionMarker()
{
	PrimaryActorTick.bCanEverTick = false;

	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
	RootComponent = MarkerMesh;
	MarkerMesh->SetCollisionProfileName(TEXT("BlockAll"));

	MarkerLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("MarkerLabel"));
	MarkerLabel->SetupAttachment(RootComponent);
	MarkerLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	MarkerLabel->SetHorizontalAlignment(EHTA_Center);
	MarkerLabel->SetVerticalAlignment(EVRTA_TextCenter);
	MarkerLabel->SetWorldSize(42.0f);
	MarkerLabel->SetTextRenderColor(FColor::White);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		MarkerMesh->SetStaticMesh(CubeMesh.Object);
	}
}

void AAstraeonRegionMarker::ApplySpec(const FAstraeonRegionActorSpec& Spec)
{
	MarkerId = Spec.ActorId;
	MarkerKind = Spec.Kind;
	SetActorLocation(Spec.LocationCm);
	SetActorScale3D(Spec.Scale);

	const FColor MarkerColor = BuildMarkerColor(Spec.ActorId, Spec.Kind);
	MarkerMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(MarkerColor.R / 255.0f, MarkerColor.G / 255.0f, MarkerColor.B / 255.0f));
	MarkerLabel->SetText(BuildMarkerLabel(Spec.ActorId, Spec.Kind));
	MarkerLabel->SetTextRenderColor(MarkerColor);
#if WITH_EDITOR
	SetActorLabel(FString::Printf(TEXT("Region_%s_%s"), Spec.Kind == EAstraeonRegionActorKind::Resource ? TEXT("Resource") : TEXT("POI"), *Spec.ActorId.ToString()));
#endif
}

FText AAstraeonRegionMarker::BuildMarkerLabel(FName ActorId, EAstraeonRegionActorKind Kind)
{
	if (ActorId == TEXT("itaca_argos_console"))
	{
		return FText::FromString(TEXT("ARGOS"));
	}

	if (ActorId == TEXT("signal_source"))
	{
		return FText::FromString(TEXT("SIGNAL"));
	}

	if (ActorId == TEXT("minor_anomaly"))
	{
		return FText::FromString(TEXT("ANOMALY"));
	}

	if (Kind == EAstraeonRegionActorKind::Resource)
	{
		return FText::FromString(FString::Printf(TEXT("RESOURCE\n%s"), *ActorId.ToString()));
	}

	return FText::FromName(ActorId);
}

FColor AAstraeonRegionMarker::BuildMarkerColor(FName ActorId, EAstraeonRegionActorKind Kind)
{
	if (ActorId == TEXT("itaca_argos_console"))
	{
		return FColor(80, 180, 255);
	}

	if (ActorId == TEXT("signal_source"))
	{
		return FColor(255, 80, 220);
	}

	if (ActorId == TEXT("minor_anomaly"))
	{
		return FColor(255, 180, 40);
	}

	if (Kind == EAstraeonRegionActorKind::Resource)
	{
		return FColor(80, 255, 120);
	}

	return FColor::White;
}
