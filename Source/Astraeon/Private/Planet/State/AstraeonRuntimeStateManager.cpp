#include "Planet/State/AstraeonRuntimeStateManager.h"
#include "Planet/State/AstraeonPlanetEntities.h"

namespace
{
	FString CellKey(FName BodyId, int32 Face, int32 Lod, int32 X, int32 Y)
	{
		return FString::Printf(TEXT("%s/%d/%d/%d/%d"), *BodyId.ToString().ToLower(), Face, Lod, X, Y);
	}
	FString CellKey(const FAstraeonPlanetDelta& D) { return CellKey(D.BodyId, D.CellFace, D.CellLod, D.CellX, D.CellY); }
}

bool UAstraeonRuntimeStateManager::RecordCreatureDefeat(const FAstraeonPlanetDefinition& Planet, FName EntityId,
	const FVector& Direction, double AltitudeCm, float RespawnSeconds)
{
	FAstraeonPlanetPatchAddress Cell;
	FVector Unit;
	if (EntityId.IsNone() || !FMath::IsFinite(AltitudeCm) || !FAstraeonPlanetCoordinates::TryNormalizeDirection(Direction, Unit)
		|| !FAstraeonPlanetEntities::CellAt(Planet, Unit, Cell)) return false;
	FAstraeonPlanetDelta Delta;
	Delta.EntityId = EntityId; Delta.BodyId = Planet.BodyId; Delta.Kind = EAstraeonPlanetDeltaKind::CreatureDefeated;
	Delta.Direction = Unit; Delta.AltitudeCm = AltitudeCm;
	Delta.CellFace = int32(Cell.Face); Delta.CellLod = Cell.Lod; Delta.CellX = Cell.X; Delta.CellY = Cell.Y;
	Delta.RemainingSeconds = RespawnSeconds;
	// A second defeat of the same entity restarts its clock instead of duplicating it.
	if (const int32* Existing = ById.Find(EntityId)) Deltas[*Existing] = Delta;
	else Deltas.Add(Delta);
	RebuildIndex();
	return true;
}

bool UAstraeonRuntimeStateManager::IsDefeated(FName EntityId) const
{
	const FAstraeonPlanetDelta* Delta = Find(EntityId);
	return Delta && Delta->Kind == EAstraeonPlanetDeltaKind::CreatureDefeated;
}

const FAstraeonPlanetDelta* UAstraeonRuntimeStateManager::Find(FName EntityId) const
{
	const int32* Index = ById.Find(EntityId);
	return Index ? &Deltas[*Index] : nullptr;
}

TArray<FName> UAstraeonRuntimeStateManager::Advance(float DeltaSeconds)
{
	TArray<FName> Lapsed;
	if (DeltaSeconds <= 0.f) return Lapsed;
	for (FAstraeonPlanetDelta& Delta : Deltas)
	{
		if (Delta.RemainingSeconds < 0.f) continue;
		Delta.RemainingSeconds -= DeltaSeconds;
		if (Delta.RemainingSeconds <= 0.f) Lapsed.Add(Delta.EntityId);
	}
	if (!Lapsed.IsEmpty())
	{
		Deltas.RemoveAll([&Lapsed](const FAstraeonPlanetDelta& D) { return Lapsed.Contains(D.EntityId); });
		RebuildIndex();
	}
	return Lapsed;
}

TArray<FAstraeonPlanetDelta> UAstraeonRuntimeStateManager::GetDeltasInCells(FName BodyId, const TArray<FAstraeonPlanetPatchAddress>& Cells) const
{
	TArray<FAstraeonPlanetDelta> Result;
	for (const auto& Cell : Cells)
		if (const TArray<int32>* Indices = ByCell.Find(CellKey(BodyId, int32(Cell.Face), Cell.Lod, Cell.X, Cell.Y)))
			for (const int32 Index : *Indices) Result.Add(Deltas[Index]);
	Result.Sort([](const FAstraeonPlanetDelta& A, const FAstraeonPlanetDelta& B) { return A.EntityId.LexicalLess(B.EntityId); });
	return Result;
}

void UAstraeonRuntimeStateManager::Restore(const TArray<FAstraeonPlanetDelta>& Saved)
{
	Deltas = Saved;
	RebuildIndex();
}

void UAstraeonRuntimeStateManager::Reset()
{
	Deltas.Reset();
	RebuildIndex();
}

void UAstraeonRuntimeStateManager::RebuildIndex()
{
	ById.Reset(); ByCell.Reset();
	for (int32 I = 0; I < Deltas.Num(); ++I)
	{
		ById.Add(Deltas[I].EntityId, I);
		ByCell.FindOrAdd(CellKey(Deltas[I])).Add(I);
	}
}
