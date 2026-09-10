#include "Planet/Patches/AstraeonPlanetPatchMesh.h"
#include "Planet/Surface/AstraeonPlanetSurface.h"

bool FAstraeonPlanetPatchBuildOptions::IsValid() const
{
	// Powers of two make shared samples exactly reproducible at adjacent LODs.
	return Quads >= 4 && Quads <= 128 && FMath::IsPowerOfTwo(Quads)
		&& FMath::IsFinite(SkirtDepthCm) && SkirtDepthCm >= 0.0;
}

bool FAstraeonPlanetPatchBuildResult::IsValid() const
{
	if (!Address.IsValid() || BuildRevision == 0 || Quads < 4 || Quads > 128 || !FMath::IsPowerOfTwo(Quads)
		|| OriginBodyCm.ContainsNaN() || !LocalBounds.IsValid
		|| SurfaceVertexCount != (Quads + 1) * (Quads + 1) || SurfaceIndexCount != Quads * Quads * 6
		|| Vertices.Num() != Normals.Num() || Vertices.Num() != UVs.Num()) return false;
	const bool HasSkirts = Vertices.Num() == SurfaceVertexCount + 4 * (Quads + 1);
	if ((!HasSkirts && Vertices.Num() != SurfaceVertexCount)
		|| Indices.Num() != SurfaceIndexCount + (HasSkirts ? 24 * Quads : 0)) return false;
	for (int32 I = 0; I < Vertices.Num(); ++I)
	{
		if (Vertices[I].ContainsNaN() || Normals[I].ContainsNaN() || UVs[I].ContainsNaN()
			|| !FMath::IsNearlyEqual(Normals[I].SizeSquared(), 1.0, 1.e-6)
			|| !LocalBounds.IsInsideOrOn(Vertices[I])) return false;
	}
	for (int32 I = 0; I < Indices.Num(); ++I)
		if (Indices[I] < 0 || Indices[I] >= (I < SurfaceIndexCount ? SurfaceVertexCount : Vertices.Num())) return false;
	return true;
}

EAstraeonPatchBuildStatus FAstraeonPlanetPatchMesh::Build(const FAstraeonPlanetDefinition& Planet,
	const FAstraeonPlanetPatchAddress& Address, uint64 Revision,
	const FAstraeonPlanetPatchBuildOptions& Options, FAstraeonPlanetPatchBuildResult& Out,
	const std::atomic<bool>* Cancelled)
{
	Out = {};
	const auto IsCancelled = [Cancelled]() { return Cancelled && Cancelled->load(std::memory_order_relaxed); };
	if (IsCancelled()) return EAstraeonPatchBuildStatus::Cancelled;
	uint64 Seed;
	if (Revision == 0 || !Options.IsValid() || !Planet.IsValid()
		|| Options.SkirtDepthCm >= Planet.RadiusCm || Planet.GeneratorVersion != FAstraeonPlanetSurface::GeneratorVersion
		|| !Address.TryDeriveSeed(Planet, EAstraeonGenerationChannel::Terrain, Seed)) return EAstraeonPatchBuildStatus::InvalidInput;

	FAstraeonPlanetPatchBuildResult Result;
	Result.Address = Address; Result.BuildRevision = Revision; Result.PatchSeed = Seed; Result.Quads = Options.Quads;
	const int32 Q = Options.Quads;
	const bool HasSkirts = Options.SkirtDepthCm > 0.0;
	Result.SurfaceVertexCount = (Q + 1) * (Q + 1); Result.SurfaceIndexCount = Q * Q * 6;
	const int32 VertexCount = Result.SurfaceVertexCount + (HasSkirts ? 4 * (Q + 1) : 0);
	Result.Vertices.Reserve(VertexCount); Result.Normals.Reserve(VertexCount); Result.UVs.Reserve(VertexCount);
	Result.Indices.Reserve(Result.SurfaceIndexCount + (HasSkirts ? 24 * Q : 0));
	FVector2D Min, Max;
	Address.TryUvBounds(Min, Max);
	Result.OriginBodyCm = FAstraeonPlanetCoordinates::FaceUvToDirection(Address.Face, (Min + Max) * 0.5) * Planet.RadiusCm;
	const double Denominator = double(int64(1) << Address.Lod) * Q;
	for (int32 Y = 0; Y <= Q; ++Y)
	{
		if (IsCancelled()) return EAstraeonPatchBuildStatus::Cancelled;
		for (int32 X = 0; X <= Q; ++X)
		{
			// Integer global grid coordinates guarantee identical shared samples across patches.
			const FVector2D Uv(-1.0 + 2.0 * (int64(Address.X) * Q + X) / Denominator,
				-1.0 + 2.0 * (int64(Address.Y) * Q + Y) / Denominator);
			const FVector Direction = FAstraeonPlanetCoordinates::FaceUvToDirection(Address.Face, Uv);
			// Never seed the height by PatchSeed: neighbours would disagree at their boundary.
			const double Height = FAstraeonPlanetSurface::SampleRadialHeightCm(Planet, Direction);
			const FVector Normal = FAstraeonPlanetSurface::SampleRadialNormal(Planet, Direction);
			if (!FMath::IsFinite(Height) || Normal.ContainsNaN()) return EAstraeonPatchBuildStatus::InvalidInput;
			const FVector Local = Direction * (Planet.RadiusCm + Height) - Result.OriginBodyCm;
			Result.Vertices.Add(Local); Result.Normals.Add(Normal); Result.UVs.Add((Uv + FVector2D(1, 1)) * 0.5);
			Result.LocalBounds += Local;
		}
	}
	for (int32 Y = 0; Y < Q; ++Y)
	for (int32 X = 0; X < Q; ++X)
	{
		const int32 A = Y * (Q + 1) + X, B = A + 1, C = A + Q + 1, D = C + 1;
		Result.Indices.Append({A, C, B, B, C, D}); // Unreal clockwise front face.
	}
	if (HasSkirts)
	{
		for (int32 Edge = 0; Edge < 4; ++Edge)
		{
			if (IsCancelled()) return EAstraeonPatchBuildStatus::Cancelled;
			// Boundary loop follows surface winding: left up, top right, right down, bottom left.
			const auto SurfaceIndex = [Q, Edge](int32 I)
			{
				switch (Edge)
				{
				case 0: return I * (Q + 1);
				case 1: return Q * (Q + 1) + I;
				case 2: return (Q - I) * (Q + 1) + Q;
				default: return Q - I;
				}
			};
			const int32 Start = Result.Vertices.Num();
			for (int32 I = 0; I <= Q; ++I)
			{
				const int32 Top = SurfaceIndex(I);
				const FVector Direction = (Result.Vertices[Top] + Result.OriginBodyCm).GetSafeNormal();
				const FVector Local = Result.Vertices[Top] - Direction * Options.SkirtDepthCm;
				const FVector Normal = Result.Normals[Top];
				const FVector2D Uv = Result.UVs[Top];
				Result.Vertices.Add(Local); Result.Normals.Add(Normal); Result.UVs.Add(Uv);
				Result.LocalBounds += Local;
			}
			for (int32 I = 0; I < Q; ++I)
			{
				const int32 A = SurfaceIndex(I), B = SurfaceIndex(I + 1), C = Start + I, D = C + 1;
				Result.Indices.Append({A, C, B, B, C, D});
			}
		}
	}
	if (IsCancelled()) return EAstraeonPatchBuildStatus::Cancelled;
	if (!Result.IsValid()) return EAstraeonPatchBuildStatus::InvalidInput;
	Out = MoveTemp(Result);
	return EAstraeonPatchBuildStatus::Success;
}
