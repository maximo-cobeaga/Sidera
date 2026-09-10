#include "Planet/Patches/AstraeonPlanetPatchAddress.h"

namespace
{
	bool IsStableBodyId(FName BodyId)
	{
		if (BodyId.IsNone()) return false;
		for (TCHAR C : BodyId.ToString())
			if (!((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') ||
				(C >= '0' && C <= '9') || C == '_' || C == '-' || C == '.')) return false;
		return true;
	}
}

uint64 FAstraeonStableHash64::Bytes(TConstArrayView<uint8> Data, uint64 Initial)
{
	uint64 H = Initial;
	for (uint8 Byte : Data) H = (H ^ Byte) * 1099511628211ULL;
	return H;
}

bool FAstraeonPlanetPatchAddress::IsValid() const
{
	return IsStableBodyId(BodyId) && uint8(Face) < 6 && Lod <= MaxLod && X >= 0 && Y >= 0
		&& X < (1 << Lod) && Y < (1 << Lod);
}

bool FAstraeonPlanetPatchAddress::operator==(const FAstraeonPlanetPatchAddress& Other) const
{
	return BodyId == Other.BodyId && Face == Other.Face && Lod == Other.Lod && X == Other.X && Y == Other.Y;
}

bool FAstraeonPlanetPatchAddress::TryParent(FAstraeonPlanetPatchAddress& Out) const
{
	const auto Input = *this; // Supports an in-place call.
	Out = {};
	if (!Input.IsValid() || Input.Lod == 0) return false;
	Out = Input; --Out.Lod; Out.X /= 2; Out.Y /= 2;
	return true;
}

bool FAstraeonPlanetPatchAddress::TryChild(uint8 Quadrant, FAstraeonPlanetPatchAddress& Out) const
{
	const auto Input = *this;
	Out = {};
	if (!Input.IsValid() || Input.Lod == MaxLod || Quadrant >= 4) return false;
	Out = Input; ++Out.Lod; Out.X = Input.X * 2 + (Quadrant & 1); Out.Y = Input.Y * 2 + (Quadrant >> 1);
	return true;
}

bool FAstraeonPlanetPatchAddress::TryUvBounds(FVector2D& OutMin, FVector2D& OutMax) const
{
	OutMin = OutMax = FVector2D::ZeroVector;
	if (!IsValid()) return false;
	const double Step = 2.0 / (1 << Lod);
	OutMin = FVector2D(-1.0 + Step * X, -1.0 + Step * Y);
	OutMax = FVector2D(-1.0 + Step * (X + 1), -1.0 + Step * (Y + 1));
	return true;
}

bool FAstraeonPlanetPatchAddress::TryFromDirection(FName Body, const FVector& Direction, uint8 Level,
	FAstraeonPlanetPatchAddress& Out)
{
	Out = {};
	if (!IsStableBodyId(Body) || Level > MaxLod) return false;
	const auto Uv = FAstraeonPlanetCoordinates::DirectionToFaceUv(Direction);
	if (!Uv.bIsValid) return false;
	const int32 Count = 1 << Level;
	Out.BodyId = Body; Out.Face = Uv.Face; Out.Lod = Level;
	// UV == +1 belongs to the last cell; no out-of-domain input is clamped here.
	Out.X = FMath::Min(Count - 1, FMath::FloorToInt32((Uv.Uv.X + 1.0) * 0.5 * Count));
	Out.Y = FMath::Min(Count - 1, FMath::FloorToInt32((Uv.Uv.Y + 1.0) * 0.5 * Count));
	return Out.IsValid();
}

bool FAstraeonPlanetPatchAddress::TryDeriveSeed(const FAstraeonPlanetDefinition& Planet,
	EAstraeonGenerationChannel Channel, uint64& OutSeed) const
{
	OutSeed = 0;
	if (!IsValid() || !Planet.IsValid() || BodyId != Planet.BodyId ||
		(Channel != EAstraeonGenerationChannel::Terrain && Channel != EAstraeonGenerationChannel::Entities)) return false;
	// Explicit seed format version, independent of the terrain generator's version.
	TArray<uint8> Data = {'A','S','T','P','A','T','C','H',1};
	for (TCHAR C : BodyId.ToString()) Data.Add(uint8(C >= 'A' && C <= 'Z' ? C + ('a' - 'A') : C));
	Data.Add(0);
	const auto Add32 = [&Data](uint32 Value)
	{
		for (int32 Shift = 0; Shift < 32; Shift += 8) Data.Add(uint8(Value >> Shift));
	};
	Add32(uint32(Planet.WorldSeed)); Add32(uint32(Planet.BodySeed)); Add32(uint32(Channel));
	Add32(uint32(Face)); Add32(Lod); Add32(uint32(X)); Add32(uint32(Y)); Add32(uint32(Planet.GeneratorVersion));
	OutSeed = FAstraeonStableHash64::Bytes(Data);
	return true;
}
