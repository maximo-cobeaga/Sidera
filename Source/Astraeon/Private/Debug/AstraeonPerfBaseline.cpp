#include "Debug/AstraeonPerfBaseline.h"

#include "AstraeonPlayerController.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "GenericPlatform/GenericPlatformMemory.h"
#include "HAL/PlatformMemory.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

namespace AstraeonPerf
{
	// Umbral de hitch del presupuesto del proyecto (AGENTS.md §8).
	constexpr double HitchThresholdMs = 50.0;

	constexpr double BytesPerMb = 1024.0 * 1024.0;

	double CurrentPhysicalMemoryMb()
	{
		const FPlatformMemoryStats Stats = FPlatformMemory::GetStats();
		return static_cast<double>(Stats.UsedPhysical) / BytesPerMb;
	}

	// Percentil por interpolación lineal sobre la muestra ya ordenada. Con muestras de miles de
	// frames la diferencia con el método "índice más cercano" es irrelevante, pero interpolar
	// evita que el p99 salte de golpe al cruzar un múltiplo del tamaño de la muestra.
	double Percentile(const TArray<double>& SortedValues, double Fraction)
	{
		if (SortedValues.Num() == 0)
		{
			return 0.0;
		}
		if (SortedValues.Num() == 1)
		{
			return SortedValues[0];
		}

		const double Position = Fraction * (SortedValues.Num() - 1);
		const int32 Lower = FMath::FloorToInt32(Position);
		const int32 Upper = FMath::Min(Lower + 1, SortedValues.Num() - 1);
		const double Alpha = Position - Lower;

		return FMath::Lerp(SortedValues[Lower], SortedValues[Upper], Alpha);
	}
}

bool UAstraeonPerfBaselineSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UAstraeonPerfBaselineSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAstraeonPerfBaselineSubsystem, STATGROUP_Tickables);
}

void UAstraeonPerfBaselineSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	bEnabled = FParse::Param(FCommandLine::Get(), TEXT("AstraeonPerfBaseline"));
	if (!bEnabled)
	{
		return;
	}

	FParse::Value(FCommandLine::Get(), TEXT("AstraeonPerfSeconds="), SampleSeconds);
	FParse::Value(FCommandLine::Get(), TEXT("AstraeonPerfWarmup="), WarmupSeconds);

	// Sin partida iniciada la sesión se queda en el menú, y medir un menú no dice nada del
	// juego. Misma razón por la que `-AstraeonCameraShot` arranca una partida antes de capturar.
	if (AAstraeonPlayerController* Controller = Cast<AAstraeonPlayerController>(InWorld.GetFirstPlayerController()))
	{
		Controller->StartSelectedNewGame();
		UE_LOG(LogTemp, Display, TEXT("AstraeonPerf: partida iniciada para la medicion"));
	}
	else
	{
		// No se aborta: en un test level planetario puede no haber este controlador y la medida
		// del mapa vacío sigue siendo comparable consigo misma. Pero tiene que constar.
		UE_LOG(LogTemp, Warning,
			TEXT("AstraeonPerf: sin AAstraeonPlayerController; se mide el mundo tal como arranca"));
	}

	UE_LOG(LogTemp, Display, TEXT("AstraeonPerf: calentamiento %.0f s, muestreo %.0f s"),
		WarmupSeconds, SampleSeconds);
}

void UAstraeonPerfBaselineSubsystem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bEnabled || bFinished)
	{
		return;
	}

	ElapsedSeconds += DeltaSeconds;

	if (!bSampling)
	{
		if (ElapsedSeconds < WarmupSeconds)
		{
			return;
		}

		bSampling = true;
		ElapsedSeconds = 0.0f;
		MemoryStartMb = AstraeonPerf::CurrentPhysicalMemoryMb();
		FrameMillis.Reserve(FMath::CeilToInt(SampleSeconds * 120.0f));
		UE_LOG(LogTemp, Display, TEXT("AstraeonPerf: calentamiento terminado, midiendo"));
		return;
	}

	FrameMillis.Add(static_cast<double>(DeltaSeconds) * 1000.0);

	if (ElapsedSeconds < SampleSeconds)
	{
		return;
	}

	bFinished = true;
	const FAstraeonPerfSample Sample = Summarize(FrameMillis, MemoryStartMb, AstraeonPerf::CurrentPhysicalMemoryMb());
	WriteReport(Sample);

	UE_LOG(LogTemp, Display,
		TEXT("AstraeonPerf: %d frames | media %.1f FPS | mediana %.2f ms | p99 %.2f ms | peor %.2f ms | hitches>%.0fms: %d | memoria %.0f->%.0f MB"),
		Sample.FrameCount, Sample.MeanFps, Sample.MedianFrameMs, Sample.P99FrameMs, Sample.WorstFrameMs,
		AstraeonPerf::HitchThresholdMs, Sample.HitchCount50Ms, Sample.PhysicalMemoryStartMb, Sample.PhysicalMemoryEndMb);

	// A concurrent movement smoke owns its completion; recording must not terminate it early.
	if (!FParse::Param(FCommandLine::Get(),TEXT("AstraeonPerfKeepRunning")))
		FPlatformMisc::RequestExit(false);
}

FAstraeonPerfSample UAstraeonPerfBaselineSubsystem::Summarize(const TArray<double>& InFrameMillis,
	double InMemoryStartMb, double InMemoryEndMb)
{
	FAstraeonPerfSample Sample;
	Sample.PhysicalMemoryStartMb = InMemoryStartMb;
	Sample.PhysicalMemoryEndMb = InMemoryEndMb;
	Sample.FrameCount = InFrameMillis.Num();

	if (InFrameMillis.Num() == 0)
	{
		return Sample;
	}

	double TotalMs = 0.0;
	for (const double Ms : InFrameMillis)
	{
		TotalMs += Ms;
		if (Ms > AstraeonPerf::HitchThresholdMs)
		{
			++Sample.HitchCount50Ms;
		}
	}

	// FPS medio a partir del tiempo total, no del promedio de los FPS por frame: promediar FPS
	// da un número optimista porque los frames rápidos pesan lo mismo que los lentos.
	Sample.MeanFps = TotalMs > 0.0 ? (InFrameMillis.Num() * 1000.0) / TotalMs : 0.0;

	TArray<double> Sorted = InFrameMillis;
	Sorted.Sort();

	Sample.MedianFrameMs = AstraeonPerf::Percentile(Sorted, 0.50);
	Sample.P99FrameMs = AstraeonPerf::Percentile(Sorted, 0.99);
	Sample.WorstFrameMs = Sorted.Last();

	return Sample;
}

void UAstraeonPerfBaselineSubsystem::WriteReport(const FAstraeonPerfSample& Sample) const
{
	const UWorld* CurrentWorld = GetWorld();
	const FString MapName = CurrentWorld ? CurrentWorld->GetMapName() : TEXT("desconocido");

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("timestamp"), FDateTime::Now().ToIso8601());
	Root->SetStringField(TEXT("map"), MapName);
	Root->SetNumberField(TEXT("warmup_seconds"), WarmupSeconds);
	Root->SetNumberField(TEXT("sample_seconds"), SampleSeconds);
	Root->SetNumberField(TEXT("frame_count"), Sample.FrameCount);
	Root->SetNumberField(TEXT("mean_fps"), Sample.MeanFps);
	Root->SetNumberField(TEXT("median_frame_ms"), Sample.MedianFrameMs);
	Root->SetNumberField(TEXT("p99_frame_ms"), Sample.P99FrameMs);
	Root->SetNumberField(TEXT("worst_frame_ms"), Sample.WorstFrameMs);
	Root->SetNumberField(TEXT("hitches_over_50ms"), Sample.HitchCount50Ms);
	Root->SetNumberField(TEXT("memory_start_mb"), Sample.PhysicalMemoryStartMb);
	Root->SetNumberField(TEXT("memory_end_mb"), Sample.PhysicalMemoryEndMb);

	// Qué se midió y en qué condiciones. Sin esto, dentro de tres fases el número no se puede
	// comparar con nada: una medida sin sus condiciones es una anécdota.
	Root->SetStringField(TEXT("command_line"), FCommandLine::Get());
	Root->SetStringField(TEXT("build_configuration"), LexToString(FApp::GetBuildConfiguration()));
	if (CurrentWorld && CurrentWorld->GetGameViewport() && CurrentWorld->GetGameViewport()->Viewport)
	{
		const FIntPoint Size=CurrentWorld->GetGameViewport()->Viewport->GetSizeXY();
		Root->SetNumberField(TEXT("viewport_width"),Size.X);
		Root->SetNumberField(TEXT("viewport_height"),Size.Y);
	}

	FString Json;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
	FJsonSerializer::Serialize(Root, Writer);

	const FString FileName = FString::Printf(TEXT("perf_baseline_%s.json"),
		*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	const FString FullPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Docs/evidencia"), FileName);

	if (FFileHelper::SaveStringToFile(Json, *FullPath))
	{
		UE_LOG(LogTemp, Display, TEXT("AstraeonPerf: informe escrito en %s"), *FullPath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AstraeonPerf: no se pudo escribir %s"), *FullPath);
	}
}
