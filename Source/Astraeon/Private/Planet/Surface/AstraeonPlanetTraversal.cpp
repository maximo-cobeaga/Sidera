#include "Planet/Surface/AstraeonPlanetTraversal.h"
#include "Environment/AstraeonItacaInterior.h"
#include "Planet/LOD/AstraeonPlanetLODManager.h"
#include "Planet/Patches/AstraeonPlanetPatchAddress.h"
#include "Planet/Surface/AstraeonPlanetSurface.h"
#include "WorldGen/AstraeonRegionMaterializer.h"
#include "WorldGen/AstraeonTerrainField.h"
#include "WorldGen/AstraeonWorldProfiles.h"

FAstraeonPlanetRegionPlan FAstraeonPlanetRegionPlan::FromFlatRegion(int32 ContentSeed, const FVector& ItacaOriginCm)
{
	const FAstraeonRegionLayout Layout = UAstraeonWorldProfiles::BuildFixedRegionLayout(ContentSeed);
	const FAstraeonTerrainSurfaceContext Context = UAstraeonRegionMaterializer::BuildSurfaceContext(ContentSeed, ItacaOriginCm, Layout);
	const FVector Deployment = UAstraeonRegionMaterializer::GetSurfaceDeploymentLocationCm(ItacaOriginCm);
	FAstraeonPlanetRegionPlan Plan;
	Plan.CenterCm = Context.CenterCm;
	Plan.RadiusCm = AAstraeonTerrainField::GetFieldRadiusCm();
	Plan.ItacaOriginCm = Context.ItacaOriginCm;
	Plan.StartCm = FVector2D(Deployment.X, Deployment.Y);
	Plan.Goals = UAstraeonRegionMaterializer::BuildTraversalGoals(Layout);
	Plan.FlatSpotsCm = Context.GroundFlatSpotsCm;
	Plan.MountainKeepOutCm = Context.MountainKeepOutCm;
	return Plan;
}

FAstraeonPlanetRegionSurface FAstraeonPlanetRegionSurface::Place(const FAstraeonPlanetDefinition& Planet, const FVector& InAnchor,
	const FAstraeonPlanetRegionPlan& Plan)
{
	FAstraeonPlanetRegionSurface Surface;
	Surface.Planet = Planet; Surface.Plan = Plan;
	Surface.Anchor = InAnchor.GetSafeNormal();
	const FVector Reference = FMath::Abs(Surface.Anchor.Z) < 0.9 ? FVector(0, 0, 1) : FVector(1, 0, 0);
	Surface.East = FVector::CrossProduct(Reference, Surface.Anchor).GetSafeNormal();
	Surface.North = FVector::CrossProduct(Surface.Anchor, Surface.East);
	for (const FVector2D& Spot : Plan.FlatSpotsCm) Surface.FlatSpots.Add(Surface.ToDirection(Spot));
	for (const FVector2D& Spot : Plan.MountainKeepOutCm) Surface.MountainKeepOut.Add(Surface.ToDirection(Spot));
	Surface.ItacaDirection = Surface.ToDirection(Plan.ItacaOriginCm);
	return Surface;
}

FVector FAstraeonPlanetRegionSurface::ToDirection(const FVector2D& PlanCm) const
{
	const FVector2D Offset = PlanCm - Plan.CenterCm;
	const double Distance = Offset.Size();
	if (Distance < UE_DOUBLE_SMALL_NUMBER) return Anchor;
	const double Angle = Distance / Planet.RadiusCm;
	return (Anchor * FMath::Cos(Angle) + (East * Offset.X + North * Offset.Y) / Distance * FMath::Sin(Angle)).GetSafeNormal();
}

FVector2D FAstraeonPlanetRegionSurface::ToPlan(const FVector& Direction) const
{
	const FVector Unit = Direction.GetSafeNormal();
	const double Angle = FMath::Acos(FMath::Clamp(FVector::DotProduct(Unit, Anchor), -1.0, 1.0));
	const FVector Tangent = Unit - Anchor * FVector::DotProduct(Unit, Anchor);
	if (Tangent.SizeSquared() < UE_DOUBLE_SMALL_NUMBER) return Plan.CenterCm;
	const FVector Bearing = Tangent.GetSafeNormal();
	return Plan.CenterCm + FVector2D(FVector::DotProduct(Bearing, East), FVector::DotProduct(Bearing, North)) * (Angle * Planet.RadiusCm);
}

FQuat FAstraeonPlanetRegionSurface::PlanRotationAt(const FVector& Direction) const
{
	// East x North = Anchor, so this basis is a proper rotation.
	const FQuat AtAnchor = FMatrix(FPlane(East, 0.0), FPlane(North, 0.0), FPlane(Anchor, 0.0), FPlane(0, 0, 0, 1)).ToQuat();
	return FQuat::FindBetweenNormals(Anchor, Direction.GetSafeNormal()) * AtAnchor;
}

double FAstraeonPlanetRegionSurface::HeightCm(const FVector2D& PlanCm) const
{
	if (AAstraeonItacaInterior::IsInsideFootprint(PlanCm - Plan.ItacaOriginCm))
		return FAstraeonPlanetSurface::SampleGroundHeightCm(Planet, ItacaDirection);
	const FVector Direction = ToDirection(PlanCm);
	const double Ground = FAstraeonPlanetSurface::SampleClearedGroundHeightCm(Planet, Direction, FlatSpots);
	const bool bMountainAllowed = !FAstraeonPlanetSurface::IsWithinAnySpot(Planet, Direction, MountainKeepOut, FAstraeonPlanetSurface::MountainClearanceCm);
	return Ground + (bMountainAllowed ? FAstraeonPlanetSurface::SampleMountainHeightCm(Planet, Direction) : 0.0);
}

double FAstraeonPlanetTraversal::MeshSpacingCm(const FAstraeonPlanetDefinition& Planet, const FVector& Direction)
{
	const FAstraeonPlanetLODSettings Settings;
	const auto Uv = FAstraeonPlanetCoordinates::DirectionToFaceUv(Direction);
	if (!Uv.bIsValid) return 0.0;
	// One step of the finest global grid, taken inward so it stays on the same face.
	const double Step = 2.0 / (double(int64(1) << FAstraeonPlanetLODManager::FinestAllowedLod(Planet, Settings)) * Settings.Quads);
	const FVector2D Next(Uv.Uv.X + (Uv.Uv.X + Step <= 1.0 ? Step : -Step), Uv.Uv.Y);
	return FAstraeonPlanetSurface::ArcDistanceCm(Planet, FAstraeonPlanetCoordinates::FaceUvToDirection(Uv.Face, Uv.Uv),
		FAstraeonPlanetCoordinates::FaceUvToDirection(Uv.Face, Next));
}

FVector FAstraeonPlanetTraversal::CandidateAnchor(const FAstraeonPlanetDefinition& Planet, int32 ContentSeed, int32 Attempt)
{
	// Stable hash of explicit bytes: the same session always lands its region in the same place.
	const int32 Words[] = {0x52474e41 /* "ANGR" */, Planet.BodySeed, Planet.WorldSeed, ContentSeed, Attempt};
	const uint64 Hash = FAstraeonStableHash64::Bytes(TConstArrayView<uint8>(reinterpret_cast<const uint8*>(Words), sizeof(Words)));
	const double U = double(Hash >> 32) / 4294967296.0, V = double(Hash & 0xffffffffULL) / 4294967296.0;
	const double Z = 2.0 * U - 1.0, Phi = 2.0 * PI * V, Ring = FMath::Sqrt(FMath::Max(0.0, 1.0 - Z * Z));
	return FVector(Ring * FMath::Cos(Phi), Ring * FMath::Sin(Phi), Z);
}

bool FAstraeonPlanetTraversal::FindSafeAnchor(const FAstraeonPlanetDefinition& Planet, int32 ContentSeed, double RadiusCm, FVector& OutAnchor)
{
	constexpr int32 MaxCandidates = 256;
	const double Step = FAstraeonPlanetSurface::TileSizeCm;
	for (int32 Candidate = 0; Candidate < MaxCandidates; ++Candidate)
	{
		// A separate candidate stream from the regular attempts.
		const FVector Anchor = CandidateAnchor(Planet, ContentSeed, 1000 + Candidate);
		const FVector Reference = FMath::Abs(Anchor.Z) < 0.9 ? FVector(0, 0, 1) : FVector(1, 0, 0);
		const FVector East = FVector::CrossProduct(Reference, Anchor).GetSafeNormal(), North = FVector::CrossProduct(Anchor, East);
		bool bMountainFree = true;
		for (double X = -RadiusCm; X <= RadiusCm && bMountainFree; X += Step)
		for (double Y = -RadiusCm; Y <= RadiusCm && bMountainFree; Y += Step)
		{
			const FVector Direction = (Anchor + (East * X + North * Y) / Planet.RadiusCm).GetSafeNormal();
			bMountainFree = FAstraeonPlanetSurface::SampleMountainHeightCm(Planet, Direction) <= 0.0;
		}
		if (bMountainFree) { OutAnchor = Anchor; return true; }
	}
	return false;
}

FAstraeonTraversalReport FAstraeonPlanetTraversal::Evaluate(const FAstraeonPlanetRegionSurface& Surface)
{
	FAstraeonTraversalReport Report;
	const FAstraeonPlanetRegionPlan& Plan = Surface.Plan;
	// The mesh is a plane between two of its vertices: validate at its spacing, not another.
	const double SpacingCm = MeshSpacingCm(Surface.Planet, Surface.Anchor);
	if (SpacingCm <= 0.0 || Plan.RadiusCm <= 0.0) return Report;
	const double MaxRiseCm = FAstraeonTerrainTraversal::GetMaxWalkableRiseCm(float(SpacingCm));
	const int32 CellsPerSide = FMath::FloorToInt32(Plan.RadiusCm * 2.0 / SpacingCm) + 1;
	const FVector2D Origin = Plan.CenterCm - FVector2D(Plan.RadiusCm, Plan.RadiusCm);
	const auto Index = [CellsPerSide](int32 X, int32 Y) { return Y * CellsPerSide + X; };
	const auto ToCell = [&](const FVector2D& PointCm)
	{
		return FIntPoint(FMath::Clamp(FMath::RoundToInt32((PointCm.X - Origin.X) / SpacingCm), 0, CellsPerSide - 1),
			FMath::Clamp(FMath::RoundToInt32((PointCm.Y - Origin.Y) / SpacingCm), 0, CellsPerSide - 1));
	};
	TArray<double> Heights;
	Heights.SetNumUninitialized(CellsPerSide * CellsPerSide);
	for (int32 Y = 0; Y < CellsPerSide; ++Y)
		for (int32 X = 0; X < CellsPerSide; ++X)
			Heights[Index(X, Y)] = Surface.HeightCm(Origin + FVector2D(X, Y) * SpacingCm);

	// Same flood as the flat validator: four neighbours, a rise above the limit is a wall.
	TArray<bool> Visited;
	Visited.Init(false, Heights.Num());
	TArray<FIntPoint> Frontier;
	const FIntPoint Start = ToCell(Plan.StartCm);
	Visited[Index(Start.X, Start.Y)] = true;
	Frontier.Add(Start);
	const FIntPoint Neighbours[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	while (!Frontier.IsEmpty())
	{
		const FIntPoint Cell = Frontier.Pop(EAllowShrinking::No);
		const double Here = Heights[Index(Cell.X, Cell.Y)];
		++Report.ReachableCells;
		for (const FIntPoint& Offset : Neighbours)
		{
			const FIntPoint Next = Cell + Offset;
			if (Next.X < 0 || Next.Y < 0 || Next.X >= CellsPerSide || Next.Y >= CellsPerSide || Visited[Index(Next.X, Next.Y)]) continue;
			const double Rise = FMath::Abs(Heights[Index(Next.X, Next.Y)] - Here);
			if (Rise > MaxRiseCm) continue;
			Report.WorstReachableRiseCm = FMath::Max(Report.WorstReachableRiseCm, float(Rise));
			Visited[Index(Next.X, Next.Y)] = true;
			Frontier.Add(Next);
		}
	}
	for (const FAstraeonTraversalGoal& Goal : Plan.Goals)
	{
		// Outside the region there is no surface to cross: a clamped far goal would otherwise
		// read as reached just for being far.
		const FIntPoint Cell = ToCell(Goal.LocationCm);
		if (FVector2D::DistSquared(Goal.LocationCm, Plan.CenterCm) > FMath::Square(Plan.RadiusCm) || !Visited[Index(Cell.X, Cell.Y)])
			Report.UnreachableGoals.Add(Goal.GoalId);
	}
	Report.bPassed = Report.UnreachableGoals.IsEmpty();
	return Report;
}

FAstraeonPlanetRegionResolution FAstraeonPlanetTraversal::ResolveRegion(const FAstraeonPlanetDefinition& Planet, int32 ContentSeed,
	const FAstraeonPlanetRegionPlan& Plan)
{
	FAstraeonPlanetRegionResolution Resolution;
	Resolution.Anchor = CandidateAnchor(Planet, ContentSeed, 0);
	// Nothing to guarantee without goals: a half-built session keeps its requested place.
	if (Plan.Goals.IsEmpty()) { Resolution.Report.bPassed = true; return Resolution; }
	for (int32 Attempt = 0; Attempt < MaxAnchorAttempts; ++Attempt)
	{
		const FVector Anchor = CandidateAnchor(Planet, ContentSeed, Attempt);
		Resolution.Report = Evaluate(FAstraeonPlanetRegionSurface::Place(Planet, Anchor, Plan));
		Resolution.AttemptsUsed = Attempt + 1;
		if (Attempt == 0) Resolution.RejectedGoals = Resolution.Report.UnreachableGoals;
		if (Resolution.Report.bPassed) { Resolution.Anchor = Anchor; return Resolution; }
	}
	// No attempt kept every goal reachable: publishing one would leave a goal behind a wall.
	Resolution.bUsedFallback = true;
	if (!FindSafeAnchor(Planet, ContentSeed, Plan.RadiusCm, Resolution.Anchor))
	{
		Resolution.Report = FAstraeonTraversalReport();
		UE_LOG(LogTemp, Error, TEXT("PlanetTraversal: no safe region found on %s for content seed %d"), *Planet.BodyId.ToString(), ContentSeed);
		return Resolution;
	}
	Resolution.Report = Evaluate(FAstraeonPlanetRegionSurface::Place(Planet, Resolution.Anchor, Plan));
	UE_LOG(LogTemp, Error, TEXT("PlanetTraversal: no anchor of %d kept the region traversable; safe variant used (traversable=%d)"),
		ContentSeed, Resolution.Report.bPassed);
	return Resolution;
}
