#include "WorldGen/AstraeonWorldGenerator.h"

#include "Math/RandomStream.h"
#include "Misc/Crc.h"

namespace AstraeonScience
{
	constexpr float RegionRadiusMeters = 500.0f;
	constexpr int32 RequiredResourceQuantity = 3;
	constexpr int32 SeedSignatureResourceQuantity = 2;
	constexpr int32 DeepResourceQuantity = 3;
	constexpr int32 CreatureSpawnCount = 5;
	const FName CoreDrillItemId(TEXT("tool_core_drill"));

	constexpr float MinComfortTemperatureKelvin = 270.0f;
	constexpr float MaxComfortTemperatureKelvin = 315.0f;
	constexpr float MinSafePressureKPa = 50.0f;
	constexpr float MaxSafePressureKPa = 110.0f;
	constexpr float MinSafeOxygenPartialKPa = 16.0f;
	constexpr float MaxSafeOxygenPartialKPa = 24.0f;
	constexpr float MaxSafeCarbonDioxidePartialKPa = 2.0f;

	FVector2D RandomPointInRegion(FRandomStream& Stream, float RadiusMeters)
	{
		const float AngleRadians = Stream.FRandRange(0.0f, UE_TWO_PI);
		const float DistanceMeters = FMath::Sqrt(Stream.FRand()) * RadiusMeters;
		return FVector2D(FMath::Cos(AngleRadians) * DistanceMeters, FMath::Sin(AngleRadians) * DistanceMeters);
	}

	FName SelectSeedSignatureResource(FRandomStream& Stream)
	{
		static const FName SignatureResources[] =
		{
			TEXT("cryosalt_shard"),
			TEXT("vesicle_resin"),
			TEXT("basalt_glass"),
			TEXT("magnetite_thread")
		};
		return SignatureResources[Stream.RandRange(0, UE_ARRAY_COUNT(SignatureResources) - 1)];
	}
}

int32 UAstraeonWorldGenerator::DeriveSeed(int32 RootSeed, const FString& Context)
{
	const FString StableKey = FString::Printf(TEXT("Astraeon:%d:%d:%s"), CurrentGeneratorVersion, RootSeed, *Context);
	return static_cast<int32>(FCrc::StrCrc32(*StableKey));
}

FAstraeonEnvironmentalSnapshot UAstraeonWorldGenerator::GenerateEnvironment(int32 WorldSeed)
{
	FRandomStream Stream(DeriveSeed(WorldSeed, TEXT("RegionEnvironment")));

	FAstraeonEnvironmentalSnapshot Environment;
	Environment.WorldSeed = WorldSeed;
	Environment.GeneratorVersion = CurrentGeneratorVersion;
	Environment.GravityMS2 = Stream.FRandRange(3.2f, 15.2f);
	Environment.TemperatureKelvin = Stream.FRandRange(220.0f, 335.0f);
	Environment.PressureKPa = Stream.FRandRange(15.0f, 140.0f);

	// These MVP atmospheres are simplified but normalized. They are generated from
	// deterministic causes later expanded by geology and biosphere generators.
	Environment.Atmosphere.Oxygen = Stream.FRandRange(0.03f, 0.28f);
	Environment.Atmosphere.CarbonDioxide = Stream.FRandRange(0.0004f, 0.08f);
	Environment.Atmosphere.Argon = Stream.FRandRange(0.002f, 0.03f);
	Environment.Atmosphere.Nitrogen = FMath::Clamp(
		1.0f - Environment.Atmosphere.Oxygen - Environment.Atmosphere.CarbonDioxide - Environment.Atmosphere.Argon,
		0.0f,
		1.0f);

	Environment.bBreathable = IsBreathable(Environment);
	Environment.EnvironmentalRisk01 = ComputeEnvironmentalRisk01(Environment);
	return Environment;
}

FAstraeonRegionLayout UAstraeonWorldGenerator::GenerateRegionLayout(int32 WorldSeed)
{
	FRandomStream Stream(DeriveSeed(WorldSeed, TEXT("RegionLayout")));

	FAstraeonRegionLayout Layout;
	Layout.WorldSeed = WorldSeed;
	Layout.GeneratorVersion = CurrentGeneratorVersion;
	Layout.RegionRadiusMeters = AstraeonScience::RegionRadiusMeters;

	const FName RequiredResources[] =
	{
		TEXT("silicate_fiber"),
		TEXT("ferrite_nodule"),
		AstraeonScience::SelectSeedSignatureResource(Stream)
	};

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(RequiredResources); ++Index)
	{
		FAstraeonResourceNode Resource;
		Resource.ResourceId = RequiredResources[Index];
		Resource.LocationMeters = AstraeonScience::RandomPointInRegion(Stream, Layout.RegionRadiusMeters * 0.88f);
		Resource.Quantity = Index == 2 ? AstraeonScience::SeedSignatureResourceQuantity : AstraeonScience::RequiredResourceQuantity;
		Resource.bSeedSignature = Index == 2;
		Layout.Resources.Add(Resource);
	}

	// Vetas profundas: existen desde el primer momento y son visibles, pero no se pueden
	// tocar sin taladro. Dan una razón concreta para volver a un sitio ya explorado.
	const FName DeepResources[] = { TEXT("cryo_ferrite_vein"), TEXT("resonant_quartz_vein") };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(DeepResources); ++Index)
	{
		FAstraeonResourceNode DeepNode;
		DeepNode.ResourceId = DeepResources[Index];
		DeepNode.LocationMeters = AstraeonScience::RandomPointInRegion(Stream, Layout.RegionRadiusMeters * 0.82f);
		DeepNode.Quantity = AstraeonScience::DeepResourceQuantity;
		DeepNode.bSeedSignature = false;
		DeepNode.RequiredToolId = AstraeonScience::CoreDrillItemId;
		Layout.Resources.Add(DeepNode);
	}

	FAstraeonPointOfInterest Signal;
	Signal.PointId = TEXT("signal_source");
	Signal.Type = EAstraeonPointOfInterestType::SignalSource;
	Signal.LocationMeters = AstraeonScience::RandomPointInRegion(Stream, Layout.RegionRadiusMeters * 0.72f);
	Layout.PointsOfInterest.Add(Signal);

	// Con un solo punto de aparición la región se sentía deshabitada y cazar dependía de
	// encontrar al único bicho del planeta. Se reparten varios a distintas distancias.
	for (int32 CreatureIndex = 0; CreatureIndex < AstraeonScience::CreatureSpawnCount; ++CreatureIndex)
	{
		const float RadiusFraction = 0.35f + (0.5f * static_cast<float>(CreatureIndex) / FMath::Max(1, AstraeonScience::CreatureSpawnCount - 1));

		FAstraeonPointOfInterest CreatureSpawn;
		CreatureSpawn.PointId = *FString::Printf(TEXT("creature_spawn_%d"), CreatureIndex);
		CreatureSpawn.Type = EAstraeonPointOfInterestType::CreatureSpawn;
		CreatureSpawn.LocationMeters = AstraeonScience::RandomPointInRegion(Stream, Layout.RegionRadiusMeters * RadiusFraction);
		Layout.PointsOfInterest.Add(CreatureSpawn);
	}

	FAstraeonPointOfInterest MinorAnomaly;
	MinorAnomaly.PointId = TEXT("minor_geologic_anomaly");
	MinorAnomaly.Type = EAstraeonPointOfInterestType::MinorAnomaly;
	MinorAnomaly.LocationMeters = AstraeonScience::RandomPointInRegion(Stream, Layout.RegionRadiusMeters * 0.8f);
	Layout.PointsOfInterest.Add(MinorAnomaly);

	return Layout;
}

bool UAstraeonWorldGenerator::IsBreathable(const FAstraeonEnvironmentalSnapshot& Environment)
{
	const float OxygenPartialKPa = Environment.PressureKPa * Environment.Atmosphere.Oxygen;
	const float CarbonDioxidePartialKPa = Environment.PressureKPa * Environment.Atmosphere.CarbonDioxide;

	return Environment.PressureKPa >= AstraeonScience::MinSafePressureKPa
		&& Environment.PressureKPa <= AstraeonScience::MaxSafePressureKPa
		&& Environment.TemperatureKelvin >= AstraeonScience::MinComfortTemperatureKelvin
		&& Environment.TemperatureKelvin <= AstraeonScience::MaxComfortTemperatureKelvin
		&& OxygenPartialKPa >= AstraeonScience::MinSafeOxygenPartialKPa
		&& OxygenPartialKPa <= AstraeonScience::MaxSafeOxygenPartialKPa
		&& CarbonDioxidePartialKPa <= AstraeonScience::MaxSafeCarbonDioxidePartialKPa;
}

float UAstraeonWorldGenerator::ComputeEnvironmentalRisk01(const FAstraeonEnvironmentalSnapshot& Environment)
{
	const float OxygenPartialKPa = Environment.PressureKPa * Environment.Atmosphere.Oxygen;
	const float CarbonDioxidePartialKPa = Environment.PressureKPa * Environment.Atmosphere.CarbonDioxide;

	const float TemperatureRisk = FMath::Max(
		FMath::GetMappedRangeValueClamped(FVector2D(AstraeonScience::MinComfortTemperatureKelvin, 220.0f), FVector2D(0.0f, 1.0f), Environment.TemperatureKelvin),
		FMath::GetMappedRangeValueClamped(FVector2D(AstraeonScience::MaxComfortTemperatureKelvin, 335.0f), FVector2D(0.0f, 1.0f), Environment.TemperatureKelvin));

	const float PressureRisk = FMath::Max(
		FMath::GetMappedRangeValueClamped(FVector2D(AstraeonScience::MinSafePressureKPa, 15.0f), FVector2D(0.0f, 1.0f), Environment.PressureKPa),
		FMath::GetMappedRangeValueClamped(FVector2D(AstraeonScience::MaxSafePressureKPa, 140.0f), FVector2D(0.0f, 1.0f), Environment.PressureKPa));

	const float OxygenRisk = FMath::Max(
		FMath::GetMappedRangeValueClamped(FVector2D(AstraeonScience::MinSafeOxygenPartialKPa, 4.0f), FVector2D(0.0f, 1.0f), OxygenPartialKPa),
		FMath::GetMappedRangeValueClamped(FVector2D(AstraeonScience::MaxSafeOxygenPartialKPa, 39.2f), FVector2D(0.0f, 1.0f), OxygenPartialKPa));

	const float CarbonDioxideRisk = FMath::GetMappedRangeValueClamped(
		FVector2D(AstraeonScience::MaxSafeCarbonDioxidePartialKPa, 11.2f),
		FVector2D(0.0f, 1.0f),
		CarbonDioxidePartialKPa);

	return FMath::Clamp((TemperatureRisk + PressureRisk + OxygenRisk + CarbonDioxideRisk) / 4.0f, 0.0f, 1.0f);
}
