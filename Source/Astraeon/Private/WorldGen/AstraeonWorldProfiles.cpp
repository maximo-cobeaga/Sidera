#include "WorldGen/AstraeonWorldProfiles.h"

#include "WorldGen/AstraeonWorldGenerator.h"
#include "Planet/Patches/AstraeonPlanetPatchAddress.h"
#include "Planet/Surface/AstraeonPlanetTraversal.h"

namespace AstraeonMvpWorld
{
	const FName PlanetKhepri(TEXT("planet_khepri"));
	const FName RegionFirstSignalBasin(TEXT("region_first_signal_basin"));
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
	if (PlanetProfileId != GetMvpPlanetProfileId()) return false;
	const FAstraeonPlanetProfile Profile = GetMvpPlanetProfile();
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

bool UAstraeonWorldProfiles::ResolvePlanetRegion(FName RegionProfileId, FAstraeonPlanetRegionSurface& OutRegion)
{
	FAstraeonRegionProfile Region;
	FAstraeonPlanetDefinition Planet;
	if (!TryGetRegionProfile(RegionProfileId, Region) || !TryGetPlanetDefinition(Region.PlanetProfileId, Planet)) return false;
	// The region's own seed, from its id through the stable hash: sessions never move it.
	const FString Id = RegionProfileId.ToString().ToLower();
	const FTCHARToUTF8 Utf8(*Id);
	const int32 RegionSeed = int32(FAstraeonStableHash64::Bytes(TConstArrayView<uint8>(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length())) & 0x7fffffff);
	// Its content is fixed too: any content seed yields the same plan, with Itaca at the landing zone.
	const FAstraeonPlanetRegionPlan Plan = FAstraeonPlanetRegionPlan::FromFlatRegion(RegionSeed, FVector(Region.LandingZoneMeters * 100.0, 0.0));
	const FAstraeonPlanetRegionResolution Resolution = FAstraeonPlanetTraversal::ResolveRegion(Planet, RegionSeed, Plan);
	if (!Resolution.Report.bPassed) return false;
	OutRegion = FAstraeonPlanetRegionSurface::Place(Planet, Resolution.Anchor, Plan);
	return true;
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
