#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "WorldGen/AstraeonWorldProfiles.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonRegionAProfileTest,
	"Astraeon.WorldGen.Profiles.RegionA",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonRegionAProfileTest::RunTest(const FString& Parameters)
{
	const FAstraeonPlanetProfile Planet = UAstraeonWorldProfiles::GetMvpPlanetProfile();
	const FAstraeonRegionProfile Region = UAstraeonWorldProfiles::GetRegionAProfile();
	TestEqual(TEXT("Region A belongs to the authored MVP planet"), Region.PlanetProfileId, Planet.PlanetProfileId);
	TestEqual(TEXT("Region A has the authored 500m square footprint"), Region.HalfExtentMeters, 250.0f);
	TestTrue(TEXT("Region A landing zone stays inside its own bounds"), UAstraeonWorldProfiles::IsPointInsideRegion(Region, Region.LandingZoneMeters));
	TestFalse(TEXT("Region A rejects points outside its fixed boundary"), UAstraeonWorldProfiles::IsPointInsideRegion(Region, FVector2D(251.0f, 0.0f)));
	TestEqual(TEXT("Region A has the three mandatory authored resources"), Region.FixedResources.Num(), 5);
	TestEqual(TEXT("Direct route has authored waypoints"), Region.DirectRouteWaypointsMeters.Num(), 5);
	TestEqual(TEXT("Safe route has authored waypoints"), Region.SafeRouteWaypointsMeters.Num(), 5);
	TestEqual(TEXT("Critical corridor preserves the documented usable width"), Region.CriticalCorridorWidthMeters, 3.0f);
	TestEqual(TEXT("Direct route starts at the landing zone"), Region.DirectRouteWaypointsMeters[0], Region.LandingZoneMeters);
	TestEqual(TEXT("Safe route starts at the landing zone"), Region.SafeRouteWaypointsMeters[0], Region.LandingZoneMeters);
	TestEqual(TEXT("Region A exposes one narrative signal"), Region.FixedPointsOfInterest.FilterByPredicate([](const FAstraeonPointOfInterest& Point)
	{
		return Point.Type == EAstraeonPointOfInterestType::SignalSource;
	}).Num(), 1);

	TSet<FName> Identifiers;
	for (const FAstraeonResourceNode& Resource : Region.FixedResources)
	{
		TestTrue(TEXT("Fixed resource is inside Region A"), UAstraeonWorldProfiles::IsPointInsideRegion(Region, Resource.LocationMeters));
		TestTrue(TEXT("Fixed resource id is unique"), !Identifiers.Contains(Resource.ResourceId));
		Identifiers.Add(Resource.ResourceId);
	}
	for (const FAstraeonPointOfInterest& Point : Region.FixedPointsOfInterest)
	{
		TestTrue(TEXT("Fixed POI is inside Region A"), UAstraeonWorldProfiles::IsPointInsideRegion(Region, Point.LocationMeters));
		TestTrue(TEXT("Fixed POI id is unique across Region A"), !Identifiers.Contains(Point.PointId));
		Identifiers.Add(Point.PointId);
	}

	FAstraeonRegionProfile Loaded;
	TestTrue(TEXT("The default profile can be resolved by stable id"), UAstraeonWorldProfiles::TryGetRegionProfile(Region.RegionProfileId, Loaded));
	TestEqual(TEXT("Resolved profile keeps its fixed signal location"), Loaded.FixedPointsOfInterest[0].LocationMeters, Region.FixedPointsOfInterest[0].LocationMeters);
	TestFalse(TEXT("Unknown region id cannot invent a generated region"), UAstraeonWorldProfiles::TryGetRegionProfile(TEXT("unknown_region"), Loaded));
	return true;
}

#endif
