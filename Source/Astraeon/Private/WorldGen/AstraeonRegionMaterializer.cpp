#include "WorldGen/AstraeonRegionMaterializer.h"

namespace AstraeonRegionMaterializer
{
	constexpr float MetersToCentimeters = 100.0f;
	constexpr float MarkerHeightCm = 60.0f;
}

TArray<FAstraeonRegionActorSpec> UAstraeonRegionMaterializer::BuildActorSpecs(const FAstraeonRegionLayout& Layout)
{
	TArray<FAstraeonRegionActorSpec> Specs;
	Specs.Reserve(Layout.Resources.Num() + Layout.PointsOfInterest.Num());

	for (const FAstraeonResourceNode& Resource : Layout.Resources)
	{
		FAstraeonRegionActorSpec Spec;
		Spec.ActorId = Resource.ResourceId;
		Spec.Kind = EAstraeonRegionActorKind::Resource;
		Spec.LocationCm = FVector(
			Resource.LocationMeters.X * AstraeonRegionMaterializer::MetersToCentimeters,
			Resource.LocationMeters.Y * AstraeonRegionMaterializer::MetersToCentimeters,
			AstraeonRegionMaterializer::MarkerHeightCm);
		Spec.Scale = Resource.bSeedSignature ? FVector(1.1f, 1.1f, 1.1f) : FVector(0.8f, 0.8f, 0.8f);
		Specs.Add(Spec);
	}

	for (const FAstraeonPointOfInterest& PointOfInterest : Layout.PointsOfInterest)
	{
		FAstraeonRegionActorSpec Spec;
		Spec.ActorId = PointOfInterest.PointId;
		Spec.Kind = EAstraeonRegionActorKind::PointOfInterest;
		Spec.LocationCm = FVector(
			PointOfInterest.LocationMeters.X * AstraeonRegionMaterializer::MetersToCentimeters,
			PointOfInterest.LocationMeters.Y * AstraeonRegionMaterializer::MetersToCentimeters,
			AstraeonRegionMaterializer::MarkerHeightCm);
		Spec.Scale = PointOfInterest.Type == EAstraeonPointOfInterestType::SignalSource
			? FVector(1.6f, 1.6f, 2.2f)
			: FVector(1.0f, 1.0f, 1.4f);
		Specs.Add(Spec);
	}

	return Specs;
}
