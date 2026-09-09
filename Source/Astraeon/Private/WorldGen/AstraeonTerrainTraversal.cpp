#include "WorldGen/AstraeonTerrainTraversal.h"

#include "WorldGen/AstraeonWorldGenerator.h"

namespace AstraeonTraversal
{
	// Ángulo de suelo caminable por defecto del CharacterMovement. El personaje no lo
	// modifica, así que es el que decide de verdad si una ladera se sube o se resbala.
	constexpr float WalkableFloorAngleDegrees = 44.765f;
	constexpr int32 MaxSeedAttempts = 8;

	// Seed de respaldo. La cubre la prueba de conectividad, de modo que recurrir a ella
	// nunca puede publicar una región rota.
	constexpr int32 FallbackTerrainSeed = 13579;

	int32 CellIndex(int32 X, int32 Y, int32 CellsPerSide)
	{
		return Y * CellsPerSide + X;
	}
}

float FAstraeonTerrainTraversal::GetSampleSpacingCm()
{
	return AAstraeonTerrainField::GetSurfaceSpacingCm();
}

float FAstraeonTerrainTraversal::GetMaxWalkableRiseCm(float SpacingCm)
{
	const float SlopeRiseCm = SpacingCm * FMath::Tan(FMath::DegreesToRadians(AstraeonTraversal::WalkableFloorAngleDegrees));
	return FMath::Max(AAstraeonTerrainField::GetMaxWalkableStepCm(), SlopeRiseCm);
}

int32 FAstraeonTerrainTraversal::GetMaxSeedAttempts()
{
	return AstraeonTraversal::MaxSeedAttempts;
}

int32 FAstraeonTerrainTraversal::GetFallbackTerrainSeed()
{
	return AstraeonTraversal::FallbackTerrainSeed;
}

FAstraeonTerrainSurfaceContext FAstraeonTerrainTraversal::WithSeed(const FAstraeonTerrainSurfaceContext& Context, int32 TerrainSeed)
{
	FAstraeonTerrainSurfaceContext Result = Context;
	Result.WorldSeed = TerrainSeed;
	// La cota de la plataforma depende de la seed: copiarla sin recalcular dejaría a Ítaca
	// flotando o enterrada en cuanto la validación cambiara de sub-seed.
	Result.ItacaPadHeightCm = AAstraeonTerrainField::GetItacaPadHeightCm(TerrainSeed, Context.ItacaOriginCm.X, Context.ItacaOriginCm.Y);
	return Result;
}

FAstraeonTraversalReport FAstraeonTerrainTraversal::Evaluate(const FAstraeonTerrainSurfaceContext& Context,
	const FVector2D& StartCm, const TArray<FAstraeonTraversalGoal>& Goals)
{
	FAstraeonTraversalReport Report;

	const float SpacingCm = GetSampleSpacingCm();
	const float RadiusCm = AAstraeonTerrainField::GetFieldRadiusCm();
	const float MaxRiseCm = GetMaxWalkableRiseCm(SpacingCm);
	const int32 CellsPerSide = FMath::FloorToInt((RadiusCm * 2.0f) / SpacingCm) + 1;
	const FVector2D OriginCm = Context.CenterCm - FVector2D(RadiusCm, RadiusCm);

	auto ToCell = [&OriginCm, SpacingCm, CellsPerSide](const FVector2D& PointCm)
	{
		return FIntPoint(
			FMath::Clamp(FMath::RoundToInt((PointCm.X - OriginCm.X) / SpacingCm), 0, CellsPerSide - 1),
			FMath::Clamp(FMath::RoundToInt((PointCm.Y - OriginCm.Y) / SpacingCm), 0, CellsPerSide - 1));
	};

	// Alturas precalculadas: la inundación consulta cada celda hasta cuatro veces y el
	// muestreo es la parte cara.
	TArray<float> Heights;
	Heights.SetNumUninitialized(CellsPerSide * CellsPerSide);
	for (int32 Y = 0; Y < CellsPerSide; ++Y)
	{
		for (int32 X = 0; X < CellsPerSide; ++X)
		{
			const FVector2D PointCm(OriginCm.X + X * SpacingCm, OriginCm.Y + Y * SpacingCm);
			Heights[AstraeonTraversal::CellIndex(X, Y, CellsPerSide)] = AAstraeonTerrainField::SampleHeightCm(Context, PointCm);
		}
	}

	TArray<bool> Visited;
	Visited.Init(false, Heights.Num());

	TArray<FIntPoint> Frontier;
	const FIntPoint StartCell = ToCell(StartCm);
	Visited[AstraeonTraversal::CellIndex(StartCell.X, StartCell.Y, CellsPerSide)] = true;
	Frontier.Add(StartCell);

	const FIntPoint Neighbours[] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
	while (Frontier.Num() > 0)
	{
		const FIntPoint Cell = Frontier.Pop(EAllowShrinking::No);
		const float CellHeightCm = Heights[AstraeonTraversal::CellIndex(Cell.X, Cell.Y, CellsPerSide)];
		++Report.ReachableCells;

		for (const FIntPoint& Offset : Neighbours)
		{
			const FIntPoint Next(Cell.X + Offset.X, Cell.Y + Offset.Y);
			if (Next.X < 0 || Next.Y < 0 || Next.X >= CellsPerSide || Next.Y >= CellsPerSide)
			{
				continue;
			}
			const int32 NextIndex = AstraeonTraversal::CellIndex(Next.X, Next.Y, CellsPerSide);
			if (Visited[NextIndex])
			{
				continue;
			}
			const float RiseCm = FMath::Abs(Heights[NextIndex] - CellHeightCm);
			if (RiseCm > MaxRiseCm)
			{
				continue;
			}
			Report.WorstReachableRiseCm = FMath::Max(Report.WorstReachableRiseCm, RiseCm);
			Visited[NextIndex] = true;
			Frontier.Add(Next);
		}
	}

	for (const FAstraeonTraversalGoal& Goal : Goals)
	{
		// Fuera del campo no hay superficie que recorrer. Sin esta comprobación el objetivo
		// se recortaría contra el borde de la rejilla y un punto inalcanzable se leería como
		// alcanzado sólo por estar lejos.
		if (FVector2D::DistSquared(Goal.LocationCm, Context.CenterCm) > FMath::Square(RadiusCm))
		{
			Report.UnreachableGoals.Add(Goal.GoalId);
			continue;
		}
		const FIntPoint GoalCell = ToCell(Goal.LocationCm);
		if (!Visited[AstraeonTraversal::CellIndex(GoalCell.X, GoalCell.Y, CellsPerSide)])
		{
			Report.UnreachableGoals.Add(Goal.GoalId);
		}
	}

	Report.bPassed = Report.UnreachableGoals.Num() == 0;
	return Report;
}

FAstraeonTerrainSeedResolution FAstraeonTerrainTraversal::ResolveTerrainSeed(const FAstraeonTerrainSurfaceContext& BaseContext,
	const FVector2D& StartCm, const TArray<FAstraeonTraversalGoal>& Goals)
{
	FAstraeonTerrainSeedResolution Resolution;
	Resolution.TerrainSeed = BaseContext.WorldSeed;

	// Sin objetivos no hay nada que garantizar: es el caso de una sesión a medio construir,
	// y forzar una sub-seed ahí sólo cambiaría el relieve sin motivo.
	if (Goals.Num() == 0)
	{
		Resolution.Report.bPassed = true;
		return Resolution;
	}

	for (int32 Attempt = 0; Attempt < AstraeonTraversal::MaxSeedAttempts; ++Attempt)
	{
		// El primer intento usa la seed pedida; los siguientes derivan sub-seeds estables de
		// ella, para que la misma partida siempre acabe en el mismo relieve.
		const int32 CandidateSeed = Attempt == 0
			? BaseContext.WorldSeed
			: UAstraeonWorldGenerator::DeriveSeed(BaseContext.WorldSeed, FString::Printf(TEXT("TerrainRetry:%d"), Attempt));

		const FAstraeonTerrainSurfaceContext Candidate = WithSeed(BaseContext, CandidateSeed);
		const FAstraeonTraversalReport Report = Evaluate(Candidate, StartCm, Goals);
		Resolution.AttemptsUsed = Attempt + 1;
		Resolution.Report = Report;
		if (Attempt == 0)
		{
			Resolution.RejectedGoals = Report.UnreachableGoals;
		}
		if (Report.bPassed)
		{
			Resolution.TerrainSeed = CandidateSeed;
			return Resolution;
		}
	}

	// Ninguna sub-seed sirvió. Publicar la pedida dejaría un objetivo crítico detrás de un
	// muro, así que se recurre a la variante segura y se deja constancia en el log.
	Resolution.bUsedFallback = true;
	Resolution.TerrainSeed = AstraeonTraversal::FallbackTerrainSeed;
	Resolution.Report = Evaluate(WithSeed(BaseContext, Resolution.TerrainSeed), StartCm, Goals);
	UE_LOG(LogTemp, Error,
		TEXT("AstraeonTerrainSeed: ninguna sub-seed de %d dejó la región transitable; se usa la variante segura %d (transitable=%s)"),
		BaseContext.WorldSeed, Resolution.TerrainSeed, Resolution.Report.bPassed ? TEXT("sí") : TEXT("no"));
	return Resolution;
}
