#pragma once

#include "CoreMinimal.h"
#include "Planet/AstraeonPlanetDefinition.h"
#include "Planet/Coordinates/AstraeonPlanetCoordinates.h"

// Values are part of the seed format. Never reorder/reuse them.
enum class EAstraeonGenerationChannel : uint32 { Terrain = 1, Entities = 2 };

// FNV-1a over explicit bytes, not native struct memory or FName pool indices.
struct ASTRAEON_API FAstraeonStableHash64
{
	static constexpr uint64 OffsetBasis = 14695981039346656037ULL;
	static uint64 Bytes(TConstArrayView<uint8> Data, uint64 Initial = OffsetBasis);
};

// Lod 0 = whole face; each level doubles X/Y resolution. Address is representation,
// not persistent entity identity (which must survive a change of LOD).
struct ASTRAEON_API FAstraeonPlanetPatchAddress
{
	static constexpr uint8 MaxLod = 24;
	FName BodyId = NAME_None;
	EAstraeonPlanetFace Face = EAstraeonPlanetFace::PositiveX;
	uint8 Lod = 0;
	int32 X = 0;
	int32 Y = 0;

	bool IsValid() const;
	bool operator==(const FAstraeonPlanetPatchAddress& Other) const;
	bool TryParent(FAstraeonPlanetPatchAddress& Out) const;
	bool TryChild(uint8 Quadrant, FAstraeonPlanetPatchAddress& Out) const;
	bool TryUvBounds(FVector2D& OutMin, FVector2D& OutMax) const;
	static bool TryFromDirection(FName Body, const FVector& Direction, uint8 Level,
		FAstraeonPlanetPatchAddress& Out);
	// Format 1: domain + lowercase ASCII BodyId + zero + LE32 fields. ASCII identity
	// avoids Unicode case-table/platform changes; display names belong in presentation.
	bool TryDeriveSeed(const FAstraeonPlanetDefinition& Planet, EAstraeonGenerationChannel Channel,
		uint64& OutSeed) const;
};

// In-memory lookup only. This engine hash must never generate persistent content.
FORCEINLINE uint32 GetTypeHash(const FAstraeonPlanetPatchAddress& Address)
{
	uint32 H = HashCombineFast(GetTypeHash(Address.BodyId), uint32(Address.Face));
	H = HashCombineFast(H, Address.Lod);
	return HashCombineFast(HashCombineFast(H, GetTypeHash(Address.X)), GetTypeHash(Address.Y));
}
