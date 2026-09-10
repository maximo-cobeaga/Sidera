#include "Planet/LOD/AstraeonPlanetLODManager.h"
#include "Planet/Surface/AstraeonPlanetSurface.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

namespace
{
	using FAddress = FAstraeonPlanetPatchAddress;
	using FSet = TSet<FAddress>;
	void Split(FSet& Leaves, const FAddress& A)
	{
		Leaves.Remove(A);
		for (uint8 Q = 0; Q < 4; ++Q) { FAddress Child; A.TryChild(Q, Child); Leaves.Add(Child); }
	}
	bool CoveringLeaf(const FSet& Leaves, FAddress Query, FAddress& Out)
	{
		do { if (Leaves.Contains(Query)) { Out = Query; return true; } } while (Query.TryParent(Query));
		return false; // This neighbour is subdivided; its fine leaves check us in turn.
	}
	bool Balance(FSet& Leaves, int32 Budget)
	{
		for (;;)
		{
			TArray<FAddress> Ordered = Leaves.Array(); Ordered.Sort(FAstraeonPlanetLODManager::Less);
			bool Changed = false;
			for (const auto& A : Ordered)
			{
				for (uint8 E = 0; E < 4; ++E)
				{
					FAddress Neighbor, Cover;
					if (!FAstraeonPlanetLODManager::SameLevelNeighbor(A, EAstraeonPatchEdge(E), Neighbor)) return false;
					if (CoveringLeaf(Leaves, Neighbor, Cover) && A.Lod > Cover.Lod + 1)
					{
						if (Leaves.Num() + 3 > Budget) return false;
						Split(Leaves, Cover); Changed = true; break;
					}
				}
				if (Changed) break;
			}
			if (!Changed) return true;
		}
	}
	double Priority(const FAstraeonPlanetDefinition& P, const FAstraeonPlanetLODView& V,
		const FAstraeonPlanetLODSettings& S, const FAddress& A)
	{
		FVector2D Min, Max; A.TryUvBounds(Min, Max);
		const FVector Center = FAstraeonPlanetCoordinates::FaceUvToDirection(A.Face, (Min + Max) * 0.5) * P.RadiusCm;
		double Bound = 0;
		for (double U : {Min.X, Max.X}) for (double W : {Min.Y, Max.Y})
			Bound = FMath::Max(Bound, FVector::Distance(Center,
				FAstraeonPlanetCoordinates::FaceUvToDirection(A.Face, FVector2D(U,W)) * P.RadiusCm));
		const FVector Predicted = V.ObserverBodyCm + V.VelocityBodyCmS * S.PredictionSeconds;
		const double Distance = FMath::Max(1.0, FMath::Min(FVector::Distance(Center, V.ObserverBodyCm),
			FVector::Distance(Center, Predicted)) - Bound - FAstraeonPlanetSurface::MaxReliefCm);
		const double Cell = 2.0 * P.RadiusCm / (double(int64(1) << A.Lod) * S.Quads);
		const double Error = Cell * Cell / (4.0 * P.RadiusCm)
			+ FMath::Min(FAstraeonPlanetSurface::MaxReliefCm, 0.1 * Cell);
		return Error * V.ViewHeightPixels / (2.0 * FMath::Tan(FMath::DegreesToRadians(V.VerticalFovDegrees * 0.5)) * Distance);
	}
}

bool FAstraeonPlanetLODManager::Less(const FAddress& A, const FAddress& B)
{
	if (A.BodyId != B.BodyId) return A.BodyId.LexicalLess(B.BodyId);
	if (A.Face != B.Face) return uint8(A.Face) < uint8(B.Face);
	if (A.Lod != B.Lod) return A.Lod < B.Lod;
	return A.Y != B.Y ? A.Y < B.Y : A.X < B.X;
}

bool FAstraeonPlanetLODManager::SameLevelNeighbor(const FAddress& Address, EAstraeonPatchEdge Edge, FAddress& Out)
{
	const auto A = Address; Out = {};
	if (!A.IsValid() || uint8(Edge) >= 4) return false;
	FAddress Neighbor = A;
	if (Edge == EAstraeonPatchEdge::Left) --Neighbor.X;
	if (Edge == EAstraeonPatchEdge::Right) ++Neighbor.X;
	if (Edge == EAstraeonPatchEdge::Bottom) --Neighbor.Y;
	if (Edge == EAstraeonPatchEdge::Top) ++Neighbor.Y;
	if (Neighbor.IsValid()) { Out = Neighbor; return true; }
	FVector2D Min, Max; A.TryUvBounds(Min, Max);
	FVector2D Uv = (Min + Max) * 0.5;
	const double Beyond = (Max.X - Min.X) * 0.25;
	if (Edge == EAstraeonPatchEdge::Left) Uv.X = -1.0 - Beyond;
	if (Edge == EAstraeonPatchEdge::Right) Uv.X = 1.0 + Beyond;
	if (Edge == EAstraeonPatchEdge::Bottom) Uv.Y = -1.0 - Beyond;
	if (Edge == EAstraeonPatchEdge::Top) Uv.Y = 1.0 + Beyond;
	// Only basis candidates use body axes; the probe is a radial cube projection. Do
	// not clamp UV to an edge: the dominant-face tie would select the original face.
	const FVector N = FAstraeonPlanetCoordinates::FaceUvToCube(A.Face, FVector2D(0,0));
	const FVector U = FAstraeonPlanetCoordinates::FaceUvToCube(A.Face, FVector2D(1,0)) - N;
	const FVector V = FAstraeonPlanetCoordinates::FaceUvToCube(A.Face, FVector2D(0,1)) - N;
	return FAddress::TryFromDirection(A.BodyId, N + U * Uv.X + V * Uv.Y, A.Lod, Out);
}

namespace
{
	bool Partition(const TArray<FAddress>& Leaves, int32 MaxLeaves, FSet& Set, FString* Reason)
	{
		const auto Fail = [Reason](const TCHAR* Text) { if (Reason) *Reason = Text; return false; };
		if (Leaves.Num() < 6 || Leaves.Num() > MaxLeaves) return Fail(TEXT("Invalid leaf budget"));
		uint64 Area[6] = {};
		for (const auto& A : Leaves)
		{
			if (!A.IsValid() || A.BodyId != Leaves[0].BodyId || Set.Contains(A)) return Fail(TEXT("Invalid or duplicate address"));
			Set.Add(A);
			Area[uint8(A.Face)] += uint64(1) << (2 * (FAddress::MaxLod - A.Lod));
		}
		for (const auto& A : Leaves)
		{
			auto Parent = A;
			while (Parent.TryParent(Parent)) if (Set.Contains(Parent)) return Fail(TEXT("Ancestor overlaps a leaf"));
		}
		for (uint64 FaceArea : Area) if (FaceArea != (uint64(1) << (2 * FAddress::MaxLod))) return Fail(TEXT("Face is not completely covered"));
		if (Reason) Reason->Reset();
		return true;
	}
}

bool FAstraeonPlanetLODManager::ValidatePartition(const TArray<FAddress>& Leaves, FString* Reason)
{
	// A relay can briefly show the outgoing and incoming selections side by side.
	FSet Set;
	return Partition(Leaves, 2 * FAstraeonPlanetLODSettings::MaxAllowedPatches, Set, Reason);
}

bool FAstraeonPlanetLODManager::ValidateCover(const TArray<FAddress>& Leaves, FString* Reason)
{
	const auto Fail = [Reason](const TCHAR* Text) { if (Reason) *Reason = Text; return false; };
	FSet Set;
	if (!Partition(Leaves, FAstraeonPlanetLODSettings::MaxAllowedPatches, Set, Reason)) return false;
	for (const auto& A : Leaves)
	{
		for (uint8 E = 0; E < 4; ++E)
		{
			FAddress Neighbor, Cover;
			if (!SameLevelNeighbor(A, EAstraeonPatchEdge(E), Neighbor)) return Fail(TEXT("Missing neighbour"));
			if (CoveringLeaf(Set, Neighbor, Cover) && A.Lod > Cover.Lod + 1) return Fail(TEXT("Adjacent LOD delta exceeds one"));
		}
	}
	return true;
}

bool FAstraeonPlanetLODManager::Select(const FAstraeonPlanetDefinition& P, const FAstraeonPlanetLODView& V,
	const FAstraeonPlanetLODSettings& S, FAstraeonPlanetLODSelection& Out)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Astraeon_PlanetLOD_Select);
	Out = {};
	if (!P.IsValid() || V.ObserverBodyCm.ContainsNaN() || V.ObserverBodyCm.Size() < P.RadiusCm * 0.5
		|| V.VelocityBodyCmS.ContainsNaN() || !FMath::IsFinite(V.VerticalFovDegrees)
		|| V.VerticalFovDegrees < 10 || V.VerticalFovDegrees > 150 || V.ViewHeightPixels < 1 || V.ViewHeightPixels > 16384
		|| S.MaxPatches < 6 || S.MaxPatches > FAstraeonPlanetLODSettings::MaxAllowedPatches
		|| S.Quads < 4 || S.Quads > 128 || !FMath::IsPowerOfTwo(S.Quads)
		|| !FMath::IsFinite(S.MinCellSpanCm) || S.MinCellSpanCm < 1
		|| !FMath::IsFinite(S.MaxErrorPixels) || S.MaxErrorPixels <= 0
		|| !FMath::IsFinite(S.PredictionSeconds) || S.PredictionSeconds < 0 || S.PredictionSeconds > 10) return false;
	Out.FinestAllowedLod = FinestAllowedLod(P, S);
	FSet Leaves;
	for (uint8 Face = 0; Face < 6; ++Face) { FAddress Root; Root.BodyId = P.BodyId; Root.Face = EAstraeonPlanetFace(Face); Leaves.Add(Root); }
	for (;;)
	{
		TArray<FAddress> Ordered = Leaves.Array(); Ordered.Sort(Less);
		double Best = S.MaxErrorPixels;
		FAddress Candidate;
		for (const auto& A : Ordered)
		{
			if (A.Lod >= Out.FinestAllowedLod) continue;
			const double Error = Priority(P, V, S, A);
			if (Error > Best) { Best = Error; Candidate = A; }
		}
		if (!Candidate.IsValid()) break;
		if (Leaves.Num() + 3 > S.MaxPatches) { Out.bBudgetLimited = true; break; }
		FSet Trial = Leaves;
		Split(Trial, Candidate);
		if (!Balance(Trial, S.MaxPatches)) { Out.bBudgetLimited = true; break; }
		Leaves = MoveTemp(Trial);
	}
	Out.Leaves = Leaves.Array(); Out.Leaves.Sort(Less);
	return ValidateCover(Out.Leaves);
}

uint8 FAstraeonPlanetLODManager::FinestAllowedLod(const FAstraeonPlanetDefinition& P, const FAstraeonPlanetLODSettings& S)
{
	uint8 Lod = 0;
	while (Lod < FAddress::MaxLod && 2.0 * P.RadiusCm / (double(int64(1) << Lod) * S.Quads) > S.MinCellSpanCm) ++Lod;
	return Lod;
}

double FAstraeonPlanetLODManager::SkirtDepthCm(const FAstraeonPlanetDefinition& P, const FAddress& A, int32 Quads)
{
	// The neighbour can be one level coarser: its edge chord can dip below our surface.
	// Bound both relief extremes and that coarse-cell spherical sagitta. No collision skirts.
	const double CoarseCell = 4.0 * P.RadiusCm / (double(int64(1) << A.Lod) * Quads);
	return FMath::Min(P.RadiusCm * 0.1, FAstraeonPlanetSurface::MaxReliefCm + CoarseCell * CoarseCell / P.RadiusCm + 100.0);
}
