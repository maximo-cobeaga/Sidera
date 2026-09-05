#include "Exploration/AstraeonMapRevealLibrary.h"

FAstraeonMapCellId UAstraeonMapRevealLibrary::CellIdForLocationMeters(FVector2D LocationMeters, float CellSizeMeters)
{
	const float SafeCellSizeMeters = FMath::Max(CellSizeMeters, 1.0f);
	FAstraeonMapCellId CellId;
	CellId.X = FMath::FloorToInt(LocationMeters.X / SafeCellSizeMeters);
	CellId.Y = FMath::FloorToInt(LocationMeters.Y / SafeCellSizeMeters);
	return CellId;
}

bool UAstraeonMapRevealLibrary::RevealCell(FAstraeonRevealedMap& RevealedMap, FAstraeonMapCellId CellId)
{
	if (IsCellRevealed(RevealedMap, CellId))
	{
		return false;
	}

	RevealedMap.RevealedCells.Add(CellId);
	return true;
}

int32 UAstraeonMapRevealLibrary::RevealRadius(FAstraeonRevealedMap& RevealedMap, FVector2D CenterMeters, int32 RadiusCells)
{
	const int32 SafeRadiusCells = FMath::Max(RadiusCells, 0);
	const FAstraeonMapCellId CenterCell = CellIdForLocationMeters(CenterMeters, RevealedMap.CellSizeMeters);
	int32 NewlyRevealed = 0;

	for (int32 OffsetY = -SafeRadiusCells; OffsetY <= SafeRadiusCells; ++OffsetY)
	{
		for (int32 OffsetX = -SafeRadiusCells; OffsetX <= SafeRadiusCells; ++OffsetX)
		{
			FAstraeonMapCellId CellId;
			CellId.X = CenterCell.X + OffsetX;
			CellId.Y = CenterCell.Y + OffsetY;
			NewlyRevealed += RevealCell(RevealedMap, CellId) ? 1 : 0;
		}
	}

	return NewlyRevealed;
}

bool UAstraeonMapRevealLibrary::IsCellRevealed(const FAstraeonRevealedMap& RevealedMap, FAstraeonMapCellId CellId)
{
	return RevealedMap.RevealedCells.Contains(CellId);
}
