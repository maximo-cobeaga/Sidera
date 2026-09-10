#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Planet/Surface/AstraeonPlanetSurface.h"
#include "Planet/Surface/AstraeonPlanetTraversal.h"
#include "WorldGen/AstraeonWorldProfiles.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonKhepriRegionTest, "Astraeon.WorldGen.Khepri.RegionAOnThePlanet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonKhepriRegionTest::RunTest(const FString& Parameters)
{
	// Phase 3 starts here: Khepri is a body, and Region A is a fixed place on it.
	FAstraeonPlanetDefinition Khepri;
	if (!TestTrue(TEXT("Khepri has a body"), UAstraeonWorldProfiles::TryGetPlanetDefinition(UAstraeonWorldProfiles::GetMvpPlanetProfileId(), Khepri))) return false;
	const FAstraeonPlanetProfile Profile = UAstraeonWorldProfiles::GetMvpPlanetProfile();
	TestEqual(TEXT("Khepri is 500 km, as chosen by the owner"), Khepri.RadiusCm, 50000000.0);
	TestTrue(TEXT("Its gravity is the one the environment reports"), FMath::IsNearlyEqual(Khepri.SurfaceGravityMS2, double(Profile.Environment.GravityMS2)));
	TestTrue(TEXT("Mass and gravity agree (g = GM/R^2)"),
		FMath::IsNearlyEqual(6.67430e-11 * Khepri.MassKg / FMath::Square(Khepri.RadiusCm / 100.0), Khepri.SurfaceGravityMS2, 1.e-6));
	TestTrue(TEXT("It has mountains"), FAstraeonPlanetSurface::HasMountainLayer(Khepri));
	FAstraeonPlanetDefinition None;
	TestFalse(TEXT("A profile without a body has none"), UAstraeonWorldProfiles::TryGetPlanetDefinition(TEXT("planet_unknown"), None));

	FAstraeonPlanetRegionSurface Region, Again;
	if (!TestTrue(TEXT("Region A is placed on Khepri"), UAstraeonWorldProfiles::ResolvePlanetRegion(UAstraeonWorldProfiles::GetRegionAProfileId(), Region))) return false;
	UAstraeonWorldProfiles::ResolvePlanetRegion(UAstraeonWorldProfiles::GetRegionAProfileId(), Again);
	TestTrue(TEXT("Always the same place"), Region.Anchor == Again.Anchor);
	TestEqual(TEXT("On Khepri"), Region.Planet.BodyId, Khepri.BodyId);
	TestTrue(TEXT("Every goal of Region A is reachable there"), FAstraeonPlanetTraversal::Evaluate(Region).bPassed);

	// Plan <-> sphere: the designed metres survive the trip, and headings stay on the tangent plane.
	const FAstraeonRegionLayout Layout = UAstraeonWorldProfiles::BuildFixedRegionLayout(1001);
	double WorstRoundTripCm = 0.0;
	for (const FAstraeonResourceNode& Node : Layout.Resources)
		WorstRoundTripCm = FMath::Max(WorstRoundTripCm, FVector2D::Distance(Region.ToPlan(Region.ToDirection(Node.LocationMeters * 100.0)), Node.LocationMeters * 100.0));
	for (const FAstraeonPointOfInterest& Poi : Layout.PointsOfInterest)
		WorstRoundTripCm = FMath::Max(WorstRoundTripCm, FVector2D::Distance(Region.ToPlan(Region.ToDirection(Poi.LocationMeters * 100.0)), Poi.LocationMeters * 100.0));
	TestTrue(FString::Printf(TEXT("Plan to sphere and back is exact (worst %.4f cm)"), WorstRoundTripCm), WorstRoundTripCm < 0.1);
	const FVector Signal = Region.ToDirection(FVector2D(220.0, 145.0) * 100.0);
	const FQuat Frame = Region.PlanRotationAt(Signal);
	TestTrue(TEXT("The plan's up is the local vertical"), Frame.GetUpVector().Equals(Signal, 1.e-9));
	TestTrue(TEXT("The plan's heading stays tangent"), FMath::Abs(FVector::DotProduct(Frame.GetForwardVector(), Signal)) < 1.e-9);
	// Distances as designed: the signal is 263 m from Itaca on paper and on Khepri.
	const double ArcCm = FAstraeonPlanetSurface::ArcDistanceCm(Khepri, Region.ToDirection(FVector2D::ZeroVector), Signal);
	TestTrue(FString::Printf(TEXT("Designed distance kept (%.2f m)"), ArcCm / 100.0), FMath::IsNearlyEqual(ArcCm, FVector2D(22000.0, 14500.0).Size(), 1.0));
	AddInfo(FString::Printf(TEXT("KhepriRegion anchor=%s mesh_spacing_cm=%.1f"), *Region.Anchor.ToString(), FAstraeonPlanetTraversal::MeshSpacingCm(Khepri, Region.Anchor)));
	return true;
}

#endif
