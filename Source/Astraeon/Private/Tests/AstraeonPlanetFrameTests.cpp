#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Planet/Coordinates/AstraeonPlanetFrame.h"

// Pruebas del marco de referencia planetario. Son funciones puras: no cargan mundo, no dependen
// de tick y no pueden pasar por accidente porque el nivel de prueba estuviera bien colocado.
//
// Cubren el spike de la Fase 0 (ADR 0004). Lo que NO cubren, a propósito: cámara, salto, caída y
// locomoción, que son de la Fase 1 y necesitan una partida.

namespace AstraeonPlanetFrameTests
{
	// Radio del tier Target del plan: 500 km. Se usa en las pruebas de escala para que un error
	// de precisión aparezca aquí y no la primera vez que alguien abra el mapa grande.
	constexpr double TargetRadiusCm = 50000000.0;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPlanetFrameUpIsRadialTest,
	"Astraeon.Planet.Frame.UpIsRadial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonPlanetFrameUpIsRadialTest::RunTest(const FString& Parameters)
{
	const FVector Center(1000.0, -2000.0, 500.0);

	// Arriba es radial en cualquier dirección, no el Z global. Es la regla del ADR 0004.
	const TArray<FVector> Directions = {
		FVector(1, 0, 0), FVector(-1, 0, 0),
		FVector(0, 1, 0), FVector(0, -1, 0),
		FVector(0, 0, 1), FVector(0, 0, -1),
		FVector(1, 1, 1).GetSafeNormal()
	};

	for (const FVector& Direction : Directions)
	{
		const FVector Location = Center + Direction * 100000.0;
		const FVector Up = FAstraeonPlanetFrame::UpAt(Center, Location);

		TestTrue(FString::Printf(TEXT("Up es la direccion radial en %s"), *Direction.ToString()),
			Up.Equals(Direction, 1.0e-4));
		TestTrue(TEXT("Up es unitario"), FMath::IsNearlyEqual(Up.Size(), 1.0, 1.0e-6));
		TestTrue(TEXT("La gravedad apunta al centro"),
			FAstraeonPlanetFrame::GravityDirectionAt(Center, Location).Equals(-Direction, 1.0e-4));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPlanetFrameAntipodeTest,
	"Astraeon.Planet.Frame.AntipodeIsInverted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonPlanetFrameAntipodeTest::RunTest(const FString& Parameters)
{
	// El polo opuesto es el caso que la puerta de la Fase 0 exige probar a mano. Aquí se fija su
	// invariante: dos puntos opuestos tienen arribas opuestos, y ambos marcos son válidos.
	const FVector Center = FVector::ZeroVector;
	const double Radius = AstraeonPlanetFrameTests::TargetRadiusCm;

	const FVector North = Center + FVector(0, 0, Radius);
	const FVector South = Center + FVector(0, 0, -Radius);

	const FVector UpNorth = FAstraeonPlanetFrame::UpAt(Center, North);
	const FVector UpSouth = FAstraeonPlanetFrame::UpAt(Center, South);

	TestTrue(TEXT("Los arribas de dos antipodas son opuestos"), UpNorth.Equals(-UpSouth, 1.0e-6));

	// En el polo sur, un personaje que venia orientado al Z global queda cabeza abajo si nadie lo
	// realinea. Esto comprueba que la alineacion lo endereza y que el marco sigue siendo sano.
	const FQuat WorldUpright = FQuat::Identity;
	const FQuat AlignedSouth = FAstraeonPlanetFrame::AlignToUp(WorldUpright, UpSouth);

	TestTrue(TEXT("La capsula queda alineada al arriba local en el antipoda"),
		FAstraeonPlanetFrame::IsFrameAligned(AlignedSouth, UpSouth));
	TestTrue(TEXT("Su eje Z apunta al arriba local, no al Z global"),
		AlignedSouth.GetUpVector().Equals(UpSouth, 1.0e-4));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPlanetFrameAlignmentTest,
	"Astraeon.Planet.Frame.AlignmentIsOrthonormal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonPlanetFrameAlignmentTest::RunTest(const FString& Parameters)
{
	const FVector Center = FVector::ZeroVector;

	// Barrido sobre la esfera: si la alineacion se degrada en alguna latitud, aparece aca y no
	// caminando. Los pasos son primos entre si para no caer siempre en los mismos meridianos.
	int32 Checked = 0;
	for (int32 PitchStep = 0; PitchStep <= 18; ++PitchStep)
	{
		for (int32 YawStep = 0; YawStep < 23; ++YawStep)
		{
			const double Polar = FMath::DegreesToRadians(PitchStep * 10.0);
			const double Azimuth = FMath::DegreesToRadians(YawStep * (360.0 / 23.0));

			const FVector Direction(
				FMath::Sin(Polar) * FMath::Cos(Azimuth),
				FMath::Sin(Polar) * FMath::Sin(Azimuth),
				FMath::Cos(Polar));

			const FVector Up = FAstraeonPlanetFrame::UpAt(Center, Center + Direction * 100000.0);
			const FQuat Aligned = FAstraeonPlanetFrame::AlignToUp(FQuat::Identity, Up);

			if (!FAstraeonPlanetFrame::IsFrameAligned(Aligned, Up))
			{
				AddError(FString::Printf(TEXT("Marco degenerado en polar=%.0f azimut=%.0f"),
					PitchStep * 10.0, YawStep * (360.0 / 23.0)));
				return false;
			}
			++Checked;
		}
	}

	TestTrue(TEXT("El barrido cubrio toda la esfera"), Checked == 19 * 23);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPlanetFramePoleDegeneracyTest,
	"Astraeon.Planet.Frame.PoleDegeneracyIsHandled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonPlanetFramePoleDegeneracyTest::RunTest(const FString& Parameters)
{
	// El caso que rompe una alineacion ingenua: el frente del actor paralelo al arriba local. La
	// proyeccion tangente del frente se anula y MakeFromZX devolveria un marco invalido.
	const FVector Up = FVector::UpVector;
	const FQuat LookingStraightUp = FRotationMatrix::MakeFromX(FVector::UpVector).ToQuat();
	const FQuat LookingStraightDown = FRotationMatrix::MakeFromX(-FVector::UpVector).ToQuat();

	for (const FQuat& Degenerate : { LookingStraightUp, LookingStraightDown })
	{
		const FQuat Aligned = FAstraeonPlanetFrame::AlignToUp(Degenerate, Up);

		TestTrue(TEXT("El marco sigue siendo ortonormal con el frente paralelo al arriba"),
			FAstraeonPlanetFrame::IsFrameAligned(Aligned, Up));
		TestFalse(TEXT("La rotacion resultante no contiene NaN"), Aligned.ContainsNaN());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPlanetFrameTangentTest,
	"Astraeon.Planet.Frame.TangentProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonPlanetFrameTangentTest::RunTest(const FString& Parameters)
{
	const FVector Up = FVector(1, 1, 1).GetSafeNormal();

	// Lo proyectado no tiene componente vertical: es la condicion que hace que caminar no empuje
	// contra la gravedad ni la acompanie (documento de transicion 4.8).
	const FVector Desired(1.0, 0.0, 0.0);
	const FVector Tangent = FAstraeonPlanetFrame::ProjectToTangent(Desired, Up);

	TestTrue(TEXT("La proyeccion es perpendicular al arriba"),
		FMath::IsNearlyZero(FVector::DotProduct(Tangent, Up), 1.0e-6));

	// Un vector ya tangente no cambia, y uno puramente vertical desaparece.
	TestTrue(TEXT("Un vector tangente se conserva"),
		FAstraeonPlanetFrame::ProjectToTangent(Tangent, Up).Equals(Tangent, 1.0e-6));
	TestTrue(TEXT("Un vector vertical se anula"),
		FAstraeonPlanetFrame::ProjectToTangent(Up * 500.0, Up).IsNearlyZero(1.0e-4));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPlanetFrameAltitudeTest,
	"Astraeon.Planet.Frame.AltitudeAtTargetScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonPlanetFrameAltitudeTest::RunTest(const FString& Parameters)
{
	// A 500 km de radio, un personaje de 180 cm tiene que seguir siendo medible. Es la prueba de
	// que la altitud no se pierde en el ruido de la coma flotante a escala planetaria: con float
	// de 32 bits, 50.000.000 cm no distingue 180 cm.
	const FVector Center = FVector::ZeroVector;
	const double Radius = AstraeonPlanetFrameTests::TargetRadiusCm;

	const FVector Standing = Center + FVector(0, 0, Radius + 180.0);
	const double Altitude = FAstraeonPlanetFrame::AltitudeCm(Center, Radius, Standing);

	TestTrue(TEXT("Una altura de 180 cm sobre un radio de 500 km es representable"),
		FMath::IsNearlyEqual(Altitude, 180.0, 0.5));

	const FVector Buried = Center + FVector(0, 0, Radius - 50.0);
	TestTrue(TEXT("Por debajo de la superficie la altitud es negativa"),
		FAstraeonPlanetFrame::AltitudeCm(Center, Radius, Buried) < 0.0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPlanetFrameInterpolationTest,
	"Astraeon.Planet.Frame.AlignmentIsProgressive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonPlanetFrameInterpolationTest::RunTest(const FString& Parameters)
{
	// La orientacion se transporta, no se teletransporta: un giro de 180 grados no puede
	// resolverse en un frame o la camara da un tiron. Este es el invariante que la Fase 1 tendra
	// que calibrar, y aca queda fijado su contrato.
	const FVector TargetUp = -FVector::UpVector;
	const FQuat Start = FQuat::Identity;

	const float DeltaSeconds = 1.0f / 60.0f;
	const float Rate = 90.0f;

	const FQuat OneStep = FAstraeonPlanetFrame::AlignToUpInterpolated(Start, TargetUp, DeltaSeconds, Rate);
	const float StepRadians = Start.AngularDistance(OneStep);
	const float MaxRadians = FMath::DegreesToRadians(Rate) * DeltaSeconds;

	TestTrue(TEXT("Un solo frame no completa el giro"), StepRadians <= MaxRadians + 1.0e-4f);
	TestTrue(TEXT("Un solo frame avanza algo"), StepRadians > 0.0f);

	// Iterando converge: si no lo hiciera, el personaje quedaria inclinado para siempre.
	FQuat Current = Start;
	for (int32 Frame = 0; Frame < 240; ++Frame)
	{
		Current = FAstraeonPlanetFrame::AlignToUpInterpolated(Current, TargetUp, DeltaSeconds, Rate);
	}

	TestTrue(TEXT("Tras dos segundos de giro la alineacion converge"),
		FAstraeonPlanetFrame::IsFrameAligned(Current, TargetUp));

	// Con delta cero la funcion tiene que ser total y no dividir por cero.
	const FQuat Instant = FAstraeonPlanetFrame::AlignToUpInterpolated(Start, TargetUp, 0.0f, Rate);
	TestFalse(TEXT("Delta cero no produce NaN"), Instant.ContainsNaN());

	return true;
}

#endif
