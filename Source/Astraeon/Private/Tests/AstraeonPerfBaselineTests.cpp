#if WITH_DEV_AUTOMATION_TESTS

#include "Debug/AstraeonPerfBaseline.h"
#include "Misc/AutomationTest.h"

// El resumen es una función pura sobre una lista de tiempos de frame, así que se puede probar
// con muestras construidas a mano. Importa: un baseline mal calculado no falla, miente, y se
// descubre tres fases después comparando contra un número que nunca fue cierto.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPerfSummaryTest,
	"Astraeon.Debug.PerfBaseline.Summary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonPerfSummaryTest::RunTest(const FString& Parameters)
{
	// 100 frames exactos a 16,67 ms: 60 FPS limpios, sin hitches.
	TArray<double> Steady;
	for (int32 Index = 0; Index < 100; ++Index)
	{
		Steady.Add(1000.0 / 60.0);
	}

	const FAstraeonPerfSample SteadySample = UAstraeonPerfBaselineSubsystem::Summarize(Steady, 100.0, 110.0);
	TestEqual(TEXT("Cuenta todos los frames"), SteadySample.FrameCount, 100);
	TestTrue(TEXT("60 FPS constantes dan 60 de media"), FMath::IsNearlyEqual(SteadySample.MeanFps, 60.0, 0.01));
	TestTrue(TEXT("La mediana es el frame nominal"), FMath::IsNearlyEqual(SteadySample.MedianFrameMs, 1000.0 / 60.0, 0.01));
	TestEqual(TEXT("Sin hitches"), SteadySample.HitchCount50Ms, 0);
	TestTrue(TEXT("Conserva la memoria medida"), FMath::IsNearlyEqual(SteadySample.PhysicalMemoryEndMb, 110.0));

	// El caso que justifica medir percentiles: 99 frames buenos y uno de 200 ms. La media apenas
	// se mueve; el pico tiene que seguir siendo visible.
	TArray<double> WithHitch;
	for (int32 Index = 0; Index < 99; ++Index)
	{
		WithHitch.Add(1000.0 / 60.0);
	}
	WithHitch.Add(200.0);

	const FAstraeonPerfSample HitchSample = UAstraeonPerfBaselineSubsystem::Summarize(WithHitch, 0.0, 0.0);
	TestEqual(TEXT("El hitch se cuenta"), HitchSample.HitchCount50Ms, 1);
	TestTrue(TEXT("El peor frame es el pico"), FMath::IsNearlyEqual(HitchSample.WorstFrameMs, 200.0));
	TestTrue(TEXT("La mediana no se entera del pico"),
		FMath::IsNearlyEqual(HitchSample.MedianFrameMs, 1000.0 / 60.0, 0.01));
	// Esta es la comprobación que da sentido al informe: la media sigue pareciendo sana.
	TestTrue(TEXT("La media disfraza el pico y por eso no basta"), HitchSample.MeanFps > 50.0);

	// Muestra vacía: puede ocurrir si la sesión muere durante el calentamiento. No debe dividir
	// por cero ni inventar un 0 que se lea como "rendimiento medido y pésimo".
	const FAstraeonPerfSample Empty = UAstraeonPerfBaselineSubsystem::Summarize({}, 0.0, 0.0);
	TestEqual(TEXT("Una muestra vacia no tiene frames"), Empty.FrameCount, 0);
	TestTrue(TEXT("Una muestra vacia no produce NaN ni infinito"), FMath::IsFinite(Empty.MeanFps));

	// Un solo frame tampoco puede romper la interpolación de percentiles.
	const FAstraeonPerfSample Single = UAstraeonPerfBaselineSubsystem::Summarize({ 33.0 }, 0.0, 0.0);
	TestTrue(TEXT("Con un frame, mediana y p99 son ese frame"),
		FMath::IsNearlyEqual(Single.MedianFrameMs, 33.0) && FMath::IsNearlyEqual(Single.P99FrameMs, 33.0));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPerfPercentileTest,
	"Astraeon.Debug.PerfBaseline.PercentileOrdering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonPerfPercentileTest::RunTest(const FString& Parameters)
{
	// Muestra desordenada a propósito: si el resumen no ordenara antes de sacar percentiles,
	// devolvería un valor arbitrario y el informe parecería correcto igual.
	TArray<double> Shuffled;
	for (int32 Index = 100; Index >= 1; --Index)
	{
		Shuffled.Add(static_cast<double>(Index));
	}

	const FAstraeonPerfSample Sample = UAstraeonPerfBaselineSubsystem::Summarize(Shuffled, 0.0, 0.0);

	TestTrue(TEXT("El peor frame es el mayor, no el ultimo de la lista"),
		FMath::IsNearlyEqual(Sample.WorstFrameMs, 100.0));
	TestTrue(TEXT("La mediana cae en el centro del rango"),
		Sample.MedianFrameMs > 49.0 && Sample.MedianFrameMs < 52.0);
	TestTrue(TEXT("El p99 esta cerca del extremo alto"), Sample.P99FrameMs > 98.0);
	TestTrue(TEXT("Los percentiles respetan el orden"),
		Sample.MedianFrameMs <= Sample.P99FrameMs && Sample.P99FrameMs <= Sample.WorstFrameMs);

	return true;
}

#endif
