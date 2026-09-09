#include "WorldGen/AstraeonRegionMaterializer.h"

#include "WorldGen/AstraeonWorldProfiles.h"

namespace AstraeonRegionMaterializer
{
	constexpr float MetersToCentimeters = 100.0f;
	constexpr float MarkerHeightCm = 60.0f;
	// Justo afuera de la escotilla (que está en X=620), no a 12 m de distancia: desde que
	// Ítaca aterriza físicamente, salir por la escotilla es cruzar la puerta y pisar el
	// terreno junto a la nave, no viajar a una plataforma aparte.
	const FVector SurfaceDeploymentLocationCm(900.0f, 0.0f, 150.0f);
}

FVector UAstraeonRegionMaterializer::GetSurfaceDeploymentLocationCm(const FVector& ItacaOriginCm)
{
	return ItacaOriginCm + AstraeonRegionMaterializer::SurfaceDeploymentLocationCm;
}

TArray<FAstraeonRegionActorSpec> UAstraeonRegionMaterializer::BuildItacaActorSpecs(const FVector& OriginCm)
{
	TArray<FAstraeonRegionActorSpec> Specs;
	Specs.Reserve(4);

	FAstraeonRegionActorSpec ArgosConsole;
	ArgosConsole.ActorId = TEXT("itaca_argos_console");
	ArgosConsole.Kind = EAstraeonRegionActorKind::PointOfInterest;
	ArgosConsole.LocationCm = OriginCm + FVector(250.0f, -140.0f, 6.0f);
	ArgosConsole.Scale = FVector(0.6f, 0.6f, 1.2f);
	Specs.Add(ArgosConsole);

	FAstraeonRegionActorSpec PilotConsole;
	PilotConsole.ActorId = TEXT("itaca_pilot_console");
	PilotConsole.Kind = EAstraeonRegionActorKind::PointOfInterest;
	PilotConsole.LocationCm = OriginCm + FVector(250.0f, 140.0f, 6.0f);
	PilotConsole.Scale = FVector(0.6f, 0.6f, 1.2f);
	Specs.Add(PilotConsole);

	FAstraeonRegionActorSpec Fabricator;
	Fabricator.ActorId = TEXT("itaca_fabricator");
	Fabricator.Kind = EAstraeonRegionActorKind::PointOfInterest;
	Fabricator.LocationCm = OriginCm + FVector(-60.0f, 0.0f, 6.0f);
	Fabricator.Scale = FVector(0.9f, 0.6f, 1.0f);
	Specs.Add(Fabricator);

	FAstraeonRegionActorSpec SurfaceHatch;
	SurfaceHatch.ActorId = TEXT("itaca_surface_hatch");
	SurfaceHatch.Kind = EAstraeonRegionActorKind::PointOfInterest;
	SurfaceHatch.LocationCm = OriginCm + FVector(620.0f, 0.0f, 6.0f);
	SurfaceHatch.Scale = FVector(1.2f, 0.5f, 1.2f);
	Specs.Add(SurfaceHatch);

	return Specs;
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
		Spec.RequiredToolId = Resource.RequiredToolId;
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

FAstraeonTerrainSurfaceContext UAstraeonRegionMaterializer::BuildSurfaceContext(int32 TerrainSeed,
	const FVector& ItacaOriginCm, const FAstraeonRegionLayout& Layout)
{
	FAstraeonTerrainSurfaceContext Context;
	Context.RegionProfileId = Layout.RegionProfileId;
	Context.WorldSeed = TerrainSeed;
	// El campo cubre la región, que está fija en el mundo; la nave sólo aporta su huella.
	Context.CenterCm = UAstraeonWorldProfiles::GetRegionAProfile().LandingZoneMeters * AstraeonRegionMaterializer::MetersToCentimeters;
	Context.ItacaOriginCm = FVector2D(ItacaOriginCm.X, ItacaOriginCm.Y);

	// Sólo Ítaca y su puerta exigen suelo llano: la estancia es rígida y no puede seguir el
	// relieve. Todo lo demás se apoya sobre el terreno consultando su altura.
	const FVector DeploymentCm = GetSurfaceDeploymentLocationCm(ItacaOriginCm);
	Context.GroundFlatSpotsCm = {
		FVector2D(ItacaOriginCm.X, ItacaOriginCm.Y),
		FVector2D(DeploymentCm.X, DeploymentCm.Y)
	};

	// Ninguna montaña puede nacer sobre un punto jugable ni sobre un tramo de ruta authored.
	Context.MountainKeepOutCm = Context.GroundFlatSpotsCm;
	for (const FAstraeonRegionActorSpec& Spec : BuildActorSpecs(Layout))
	{
		Context.MountainKeepOutCm.Add(FVector2D(Spec.LocationCm.X, Spec.LocationCm.Y));
	}
	const FAstraeonRegionProfile RegionProfile = UAstraeonWorldProfiles::GetRegionAProfile();
	for (const FVector2D& WaypointMeters : RegionProfile.DirectRouteWaypointsMeters)
	{
		Context.MountainKeepOutCm.Add(WaypointMeters * AstraeonRegionMaterializer::MetersToCentimeters);
	}
	for (const FVector2D& WaypointMeters : RegionProfile.SafeRouteWaypointsMeters)
	{
		Context.MountainKeepOutCm.Add(WaypointMeters * AstraeonRegionMaterializer::MetersToCentimeters);
	}

	Context.ItacaPadHeightCm = AAstraeonTerrainField::GetItacaPadHeightCm(TerrainSeed, ItacaOriginCm.X, ItacaOriginCm.Y);
	return Context;
}

TArray<FAstraeonTraversalGoal> UAstraeonRegionMaterializer::BuildTraversalGoals(const FAstraeonRegionLayout& Layout)
{
	TArray<FAstraeonTraversalGoal> Goals;
	Goals.Reserve(Layout.Resources.Num() + Layout.PointsOfInterest.Num());
	for (const FAstraeonRegionActorSpec& Spec : BuildActorSpecs(Layout))
	{
		FAstraeonTraversalGoal Goal;
		Goal.GoalId = Spec.ActorId;
		Goal.LocationCm = FVector2D(Spec.LocationCm.X, Spec.LocationCm.Y);
		Goals.Add(Goal);
	}
	return Goals;
}
