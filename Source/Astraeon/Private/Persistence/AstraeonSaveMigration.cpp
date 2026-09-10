#include "Persistence/AstraeonSaveMigration.h"
#include "Persistence/AstraeonSaveGame.h"
#include "Planet/State/AstraeonPlanetEntities.h"

namespace
{
	// Anchor and its tangent basis: flat +X and +Y keep their meaning at the anchor.
	const FVector Anchor(0, 0, 1), East(1, 0, 0), North(0, 1, 0);
}

FName FAstraeonLegacyFlatProjection::BodyId()
{
	return TEXT("legacy_flat_region");
}

FAstraeonPlanetDefinition FAstraeonLegacyFlatProjection::Planet(int32 WorldSeed)
{
	FAstraeonPlanetDefinition P;
	P.BodyId = BodyId(); P.RadiusCm = RadiusCm; P.SurfaceGravityMS2 = 9.81;
	P.MassKg = P.SurfaceGravityMS2 * FMath::Square(RadiusCm / 100.0) / 6.67430e-11;
	P.WorldSeed = WorldSeed; P.BodySeed = WorldSeed;
	return P;
}

void FAstraeonLegacyFlatProjection::ToPlanet(const FVector& FlatCm, FVector& OutDirection, double& OutAltitudeCm)
{
	const FVector2D Planar(FlatCm.X, FlatCm.Y);
	const double Distance = Planar.Size();
	OutAltitudeCm = FlatCm.Z;
	if (Distance < UE_DOUBLE_SMALL_NUMBER) { OutDirection = Anchor; return; }
	const double Angle = Distance / RadiusCm;
	const FVector Bearing = (East * Planar.X + North * Planar.Y) / Distance;
	OutDirection = (Anchor * FMath::Cos(Angle) + Bearing * FMath::Sin(Angle)).GetSafeNormal();
}

FVector FAstraeonLegacyFlatProjection::ToFlat(const FVector& Direction, double AltitudeCm)
{
	const FVector Unit = Direction.GetSafeNormal();
	const double Angle = FMath::Acos(FMath::Clamp(FVector::DotProduct(Unit, Anchor), -1.0, 1.0));
	const FVector Tangent = Unit - Anchor * FVector::DotProduct(Unit, Anchor);
	if (Tangent.SizeSquared() < UE_DOUBLE_SMALL_NUMBER) return FVector(0, 0, AltitudeCm);
	const FVector Bearing = Tangent.GetSafeNormal();
	return FVector(FVector::DotProduct(Bearing, East) * Angle * RadiusCm, FVector::DotProduct(Bearing, North) * Angle * RadiusCm, AltitudeCm);
}

FVector FAstraeonLegacyFlatProjection::HeadingToPlanet(const FVector& FlatForward, const FVector& Direction)
{
	// The flat basis moved to `Direction` by the same rotation that took the anchor there.
	const FQuat Carry = FQuat::FindBetweenNormals(Anchor, Direction.GetSafeNormal());
	const FVector Heading = Carry.RotateVector(East * FlatForward.X + North * FlatForward.Y);
	const FVector Up = Direction.GetSafeNormal();
	const FVector Tangent = Heading - Up * FVector::DotProduct(Heading, Up);
	return Tangent.IsNearlyZero() ? Carry.RotateVector(East) : Tangent.GetSafeNormal();
}

void FAstraeonLegacyFlatProjection::AppendNestDeltas(const FAstraeonRegionLayout& Layout, int32 ContentSeed,
	const TMap<FName, float>& NestClocks, TArray<FAstraeonPlanetDelta>& Out)
{
	// An unknown nest id (content that changed since) stays at the anchor rather than vanishing.
	const FAstraeonPlanetDefinition Legacy = Planet(ContentSeed);
	TArray<FName> Nests;
	NestClocks.GetKeys(Nests);
	Nests.Sort(FNameLexicalLess());
	for (const FName& Nest : Nests)
	{
		const FAstraeonPointOfInterest* Poi = Layout.PointsOfInterest.FindByPredicate(
			[&Nest](const FAstraeonPointOfInterest& P) { return P.PointId == Nest; });
		FAstraeonPlanetDelta Delta;
		Delta.EntityId = Nest; Delta.BodyId = Legacy.BodyId; Delta.Kind = EAstraeonPlanetDeltaKind::CreatureDefeated;
		ToPlanet(Poi ? FVector(Poi->LocationMeters * 100.0, 0.0) : FVector::ZeroVector, Delta.Direction, Delta.AltitudeCm);
		FAstraeonPlanetPatchAddress Cell;
		FAstraeonPlanetEntities::CellAt(Legacy, Delta.Direction, Cell);
		Delta.CellFace = int32(Cell.Face); Delta.CellLod = Cell.Lod; Delta.CellX = Cell.X; Delta.CellY = Cell.Y;
		Delta.RemainingSeconds = NestClocks[Nest];
		Out.Add(Delta);
	}
}

bool FAstraeonSaveMigration::Upgrade(UAstraeonSaveGame& Save)
{
	if (Save.SaveGameVersion > CurrentVersion) return false;
	if (Save.SaveGameVersion < 2)
	{
		// A v1 save keeps its world exactly as saved, labelled as legacy (the v1 -> v2 rule).
		Save.PlanetProfileId = TEXT("legacy_generated_planet");
		Save.RegionProfileId = TEXT("legacy_generated_region");
		Save.ContentSeed = Save.WorldSeed;
		Save.RegionLayout.PlanetProfileId = Save.PlanetProfileId;
		Save.RegionLayout.RegionProfileId = Save.RegionProfileId;
		Save.RegionLayout.ContentSeed = Save.ContentSeed;
	}
	if (Save.SaveGameVersion < 3)
	{
		using FLegacy = FAstraeonLegacyFlatProjection;
		Save.PlanetBodyId = FLegacy::BodyId();
		FLegacy::ToPlanet(Save.PlayerTransform.GetLocation(), Save.PlayerDirection, Save.PlayerAltitudeCm);
		Save.PlayerForwardTangent = FLegacy::HeadingToPlanet(Save.PlayerTransform.GetRotation().GetForwardVector(), Save.PlayerDirection);
		FLegacy::ToPlanet(Save.ItacaOriginCm, Save.ItacaDirection, Save.ItacaAltitudeCm);
		FLegacy::AppendNestDeltas(Save.RegionLayout, Save.ContentSeed, Save.CreatureRespawnTimers, Save.PlanetDeltas);
	}
	Save.SaveGameVersion = CurrentVersion;
	return true;
}
