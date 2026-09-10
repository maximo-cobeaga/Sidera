#include "WorldGen/AstraeonWorldProfiles.h"

#include "WorldGen/AstraeonWorldGenerator.h"
#include "Planet/Patches/AstraeonPlanetPatchAddress.h"
#include "Planet/Surface/AstraeonPlanetTraversal.h"

namespace AstraeonMvpWorld
{
	const FName PlanetKhepri(TEXT("planet_khepri"));
	const FName RegionFirstSignalBasin(TEXT("region_first_signal_basin"));

	// The body alone, before any region is placed on it.
	bool TryGetBareBody(FName PlanetProfileId, FAstraeonPlanetDefinition& OutDefinition)
	{
		if (PlanetProfileId != PlanetKhepri) return false;
		const FAstraeonPlanetProfile Profile = UAstraeonWorldProfiles::GetMvpPlanetProfile();
		OutDefinition = FAstraeonPlanetDefinition();
		OutDefinition.BodyId = Profile.PlanetProfileId;
		OutDefinition.RadiusCm = Profile.RadiusCm;
		OutDefinition.SurfaceGravityMS2 = Profile.Environment.GravityMS2;
		// Mass follows gravity and radius (g = GM/R^2), so the two can never contradict each other.
		OutDefinition.MassKg = Profile.Environment.GravityMS2 * FMath::Square(Profile.RadiusCm / 100.0) / 6.67430e-11;
		OutDefinition.BodySeed = Profile.BodySeed;
		OutDefinition.WorldSeed = Profile.BodySeed;
		return OutDefinition.IsValid();
	}

	// A region is a place: resolved once per process, and every definition of its body shares
	// the result. A region that cannot be placed stays unplaced; it is not retried per query.
	TSharedPtr<const FAstraeonPlanetRegionSurface> PlacedRegion(FName RegionProfileId)
	{
		static FCriticalSection Lock;
		static TMap<FName, TSharedPtr<const FAstraeonPlanetRegionSurface>> Placed;
		FScopeLock Scope(&Lock);
		if (const TSharedPtr<const FAstraeonPlanetRegionSurface>* Found = Placed.Find(RegionProfileId)) return *Found;
		TSharedPtr<const FAstraeonPlanetRegionSurface>& Slot = Placed.Add(RegionProfileId);
		FAstraeonRegionProfile Region;
		FAstraeonPlanetDefinition Body;
		if (!UAstraeonWorldProfiles::TryGetRegionProfile(RegionProfileId, Region) || !TryGetBareBody(Region.PlanetProfileId, Body)) return Slot;
		// The region's own seed: sessions never move it.
		const int32 RegionSeed = UAstraeonWorldProfiles::GetPlanetRegionSeed(RegionProfileId);
		// Its content is fixed too: any content seed yields the same plan, with Itaca at the landing zone.
		const FAstraeonPlanetRegionPlan Plan = FAstraeonPlanetRegionPlan::FromFlatRegion(RegionSeed, FVector(Region.LandingZoneMeters * 100.0, 0.0));
		const FAstraeonPlanetRegionResolution Resolution = FAstraeonPlanetTraversal::ResolveRegion(Body, RegionSeed, Plan);
		if (Resolution.Report.bPassed) Slot = MakeShared<FAstraeonPlanetRegionSurface>(FAstraeonPlanetRegionSurface::Place(Body, Resolution.Anchor, Plan));
		return Slot;
	}
}

FName UAstraeonWorldProfiles::GetMvpPlanetProfileId()
{
	return AstraeonMvpWorld::PlanetKhepri;
}

FName UAstraeonWorldProfiles::GetRegionAProfileId()
{
	return AstraeonMvpWorld::RegionFirstSignalBasin;
}

FAstraeonPlanetProfile UAstraeonWorldProfiles::GetMvpPlanetProfile()
{
	FAstraeonPlanetProfile Profile;
	Profile.PlanetProfileId = GetMvpPlanetProfileId();
	Profile.DisplayName = FText::FromString(TEXT("Khepri"));
	Profile.Biomes = { EAstraeonBiomeId::RockyDesert };

	// Valores authored: hostiles pero estables. La ciencia sigue pasando por los mismos
	// validadores de respirabilidad/riesgo que una fuente futura procedural.
	Profile.Environment.GeneratorVersion = UAstraeonWorldGenerator::CurrentGeneratorVersion;
	Profile.Environment.GravityMS2 = 8.05f;
	Profile.Environment.TemperatureKelvin = 306.15f;
	Profile.Environment.PressureKPa = 64.0f;
	Profile.Environment.Atmosphere.Oxygen = 0.16f;
	Profile.Environment.Atmosphere.Nitrogen = 0.73f;
	Profile.Environment.Atmosphere.CarbonDioxide = 0.07f;
	Profile.Environment.Atmosphere.Argon = 0.04f;
	Profile.Environment.bBreathable = UAstraeonWorldGenerator::IsBreathable(Profile.Environment);
	Profile.Environment.EnvironmentalRisk01 = UAstraeonWorldGenerator::ComputeEnvironmentalRisk01(Profile.Environment);
	// 500 km: the Target tier proven at the Phase 2 gate, chosen by the owner on 2026-09-10.
	Profile.RadiusCm = 50000000.0;
	Profile.BodySeed = 4242;
	return Profile;
}

bool UAstraeonWorldProfiles::TryGetPlanetDefinition(FName PlanetProfileId, FAstraeonPlanetDefinition& OutDefinition)
{
	if (!AstraeonMvpWorld::TryGetBareBody(PlanetProfileId, OutDefinition)) return false;
	// Region A's plateau and keep-outs are part of Khepri's relief for everyone who asks.
	const TSharedPtr<const FAstraeonPlanetRegionSurface> Region = AstraeonMvpWorld::PlacedRegion(GetRegionAProfileId());
	if (Region.IsValid() && Region->Planet.BodyId == OutDefinition.BodyId) OutDefinition.RegionRelief = Region->Relief;
	return true;
}

bool UAstraeonWorldProfiles::ResolvePlanetRegion(FName RegionProfileId, FAstraeonPlanetRegionSurface& OutRegion)
{
	const TSharedPtr<const FAstraeonPlanetRegionSurface> Region = AstraeonMvpWorld::PlacedRegion(RegionProfileId);
	if (!Region.IsValid()) return false;
	OutRegion = *Region;
	return true;
}

int32 UAstraeonWorldProfiles::GetPlanetRegionSeed(FName RegionProfileId)
{
	const FString Id = RegionProfileId.ToString().ToLower();
	const FTCHARToUTF8 Utf8(*Id);
	return int32(FAstraeonStableHash64::Bytes(TConstArrayView<uint8>(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length())) & 0x7fffffff);
}

FAstraeonRegionProfile UAstraeonWorldProfiles::GetRegionAProfile()
{
	FAstraeonRegionProfile Profile;
	Profile.RegionProfileId = GetRegionAProfileId();
	Profile.PlanetProfileId = GetMvpPlanetProfileId();
	Profile.HalfExtentMeters = 250.0f;
	Profile.Biome = EAstraeonBiomeId::RockyDesert;
	Profile.LandingZoneMeters = FVector2D::ZeroVector;
	Profile.SecondaryContentSeeds = { 100, 200, 300, 400, 500 };
	Profile.DirectRouteWaypointsMeters = {
		FVector2D(0.0f, 0.0f), FVector2D(70.0f, -35.0f), FVector2D(145.0f, 70.0f),
		FVector2D(185.0f, 105.0f), FVector2D(220.0f, 145.0f) };
	Profile.SafeRouteWaypointsMeters = {
		FVector2D(0.0f, 0.0f), FVector2D(70.0f, -35.0f), FVector2D(190.0f, -85.0f),
		FVector2D(205.0f, 20.0f), FVector2D(220.0f, 145.0f) };

	// La cadena de misión sigue usando tres recursos, pero sus posiciones se diseñan y no
	// pueden cambiar por seed. El cristal es la firma de Khepri.
	Profile.FixedResources = {
		{ TEXT("silicate_fiber"), FVector2D(70.0f, -35.0f), 1, false, NAME_None },
		{ TEXT("ferrite_nodule"), FVector2D(145.0f, 70.0f), 1, false, NAME_None },
		{ TEXT("khepri_resonant_crystal"), FVector2D(190.0f, -85.0f), 1, true, NAME_None },
		{ TEXT("cryo_ferrite_vein"), FVector2D(-120.0f, -130.0f), 3, false, TEXT("tool_core_drill") },
		{ TEXT("resonant_quartz_vein"), FVector2D(105.0f, 175.0f), 3, false, TEXT("tool_core_drill") }
	};
	Profile.FixedPointsOfInterest = {
		{ TEXT("signal_source"), EAstraeonPointOfInterestType::SignalSource, FVector2D(220.0f, 145.0f) },
		{ TEXT("minor_geologic_anomaly"), EAstraeonPointOfInterestType::MinorAnomaly, FVector2D(-165.0f, 95.0f) },
		{ TEXT("creature_nest_west"), EAstraeonPointOfInterestType::CreatureSpawn, FVector2D(-75.0f, -100.0f) },
		{ TEXT("creature_nest_east"), EAstraeonPointOfInterestType::CreatureSpawn, FVector2D(130.0f, 120.0f) }
	};
	return Profile;
}

FAstraeonRegionLayout UAstraeonWorldProfiles::BuildFixedRegionLayout(int32 ContentSeed)
{
	const FAstraeonRegionProfile Profile = GetRegionAProfile();
	FAstraeonRegionLayout Layout;
	Layout.PlanetProfileId = Profile.PlanetProfileId;
	Layout.RegionProfileId = Profile.RegionProfileId;
	Layout.ContentSeed = ContentSeed;
	// Campo legado: mantiene compatibilidad con snapshots y diagnósticos mientras los
	// consumidores se trasladan a ContentSeed. No controla ubicación de contenido fijo.
	Layout.WorldSeed = ContentSeed;
	Layout.GeneratorVersion = 2;
	Layout.RegionRadiusMeters = Profile.HalfExtentMeters;
	Layout.Resources = Profile.FixedResources;
	Layout.PointsOfInterest = Profile.FixedPointsOfInterest;
	return Layout;
}

bool UAstraeonWorldProfiles::TryGetRegionProfile(FName RegionProfileId, FAstraeonRegionProfile& OutProfile)
{
	if (RegionProfileId != GetRegionAProfileId())
	{
		return false;
	}
	OutProfile = GetRegionAProfile();
	return true;
}

bool UAstraeonWorldProfiles::IsPointInsideRegion(const FAstraeonRegionProfile& Profile, const FVector2D& PointMeters)
{
	return FMath::Abs(PointMeters.X - Profile.LandingZoneMeters.X) <= Profile.HalfExtentMeters
		&& FMath::Abs(PointMeters.Y - Profile.LandingZoneMeters.Y) <= Profile.HalfExtentMeters;
}
