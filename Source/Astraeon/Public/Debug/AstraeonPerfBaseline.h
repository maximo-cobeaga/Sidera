#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstraeonPerfBaseline.generated.h"

// Muestra de rendimiento de una sesión: media, percentiles, hitches y memoria.
struct FAstraeonPerfSample
{
	int32 FrameCount = 0;
	double MeanFps = 0.0;
	// Percentiles de TIEMPO de frame, no de FPS: promediar FPS pondera mal los frames lentos,
	// que son justamente los que se quieren ver. P99 del tiempo es el 1% peor.
	double MedianFrameMs = 0.0;
	double P99FrameMs = 0.0;
	double WorstFrameMs = 0.0;
	// El presupuesto del proyecto es "ningún pico recurrente superior a 50 ms" (AGENTS.md §8).
	int32 HitchCount50Ms = 0;
	double PhysicalMemoryStartMb = 0.0;
	double PhysicalMemoryEndMb = 0.0;
};

// Mide una línea base de rendimiento y escribe un JSON reproducible.
//
// Existe porque la puerta de la Fase 0 pide "registrar métricas actuales" y el presupuesto de
// AGENTS.md §8 se define por percentiles y picos, no por promedio: un hitch de 200 ms desaparece
// dentro de una media sana. Un número suelto copiado de `stat unit` no sirve para comparar dentro
// de tres fases; un JSON con el mismo formato, sí.
//
// Es un subsistema de mundo y no un componente del personaje a propósito: la comparación que
// importa es esta misma medida sobre `TL_10_RadialGravity` y sobre los mapas planetarios que
// vengan, donde puede no haber protagonista.
//
// Uso:
//   Astraeon.exe /Game/Maps/L_AstraeonBootstrap -AstraeonPerfBaseline
//                [-AstraeonPerfSeconds=30] [-AstraeonPerfWarmup=10]
//
// Inactivo salvo que esté el flag. Sin él, ni siquiera tickea.
UCLASS()
class ASTRAEON_API UAstraeonPerfBaselineSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return bEnabled && !bFinished; }
	// Sin esto el subsistema se crearía también para el mundo del editor y para mundos de
	// previsualización, que no miden nada.
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	// Duración de muestreo por defecto, en segundos. Suficiente para que un percentil 99 tenga
	// del orden de veinte frames detrás y no sea un solo pico.
	static constexpr float DefaultSampleSeconds = 30.0f;

	// Descarte inicial. Los primeros segundos de una sesión miden compilación de shaders y
	// carga de streaming, no el juego, y arruinan cualquier percentil.
	static constexpr float DefaultWarmupSeconds = 10.0f;

	// Expuesto para pruebas: el cálculo de la muestra no depende del mundo ni del reloj.
	static FAstraeonPerfSample Summarize(const TArray<double>& FrameMillis,
		double MemoryStartMb, double MemoryEndMb);

private:
	void WriteReport(const FAstraeonPerfSample& Sample) const;

	bool bEnabled = false;
	bool bFinished = false;
	bool bSampling = false;

	float SampleSeconds = DefaultSampleSeconds;
	float WarmupSeconds = DefaultWarmupSeconds;
	float ElapsedSeconds = 0.0f;

	double MemoryStartMb = 0.0;
	TArray<double> FrameMillis;
};
