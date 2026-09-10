#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Planet/Coordinates/AstraeonPlanetFrame.h"
#include "Planet/Coordinates/AstraeonPlanetCoordinates.h"
#include "Planet/AstraeonPlanetDefinition.h"
#include "Planet/Surface/AstraeonPlanetSurface.h"
#include "Planet/Surface/AstraeonCubeSphereMesh.h"
#include <limits>

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPlanetCoordinatesRoundTripTest,
	"Astraeon.Planet.Coordinates.DirectionFaceUvRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonPlanetCoordinatesRoundTripTest::RunTest(const FString& Parameters)
{
	const TArray<FVector> Directions = {
		FVector(1, 0.2, -0.4).GetSafeNormal(), FVector(-0.3, 1, 0.6).GetSafeNormal(),
		FVector(0.2, -0.4, 1).GetSafeNormal(), FVector(-0.7, 0.4, -1).GetSafeNormal(),
		FVector(-1, 0.2, 0.3).GetSafeNormal(), FVector(0.2, -1, 0.3).GetSafeNormal()
	};

	for (const FVector& Direction : Directions)
	{
		const FAstraeonPlanetFaceUv FaceUv = FAstraeonPlanetCoordinates::DirectionToFaceUv(Direction);
		TestTrue(TEXT("UV dentro de la cara"), FAstraeonPlanetCoordinates::IsValidUv(FaceUv.Uv));
		const FVector Reconstructed = FAstraeonPlanetCoordinates::FaceUvToDirection(FaceUv.Face, FaceUv.Uv);
		TestTrue(TEXT("Direccion conserva identidad"), Reconstructed.Equals(Direction, 1.0e-6));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPlanetCoordinatesEdgesMatchTest,
	"Astraeon.Planet.Topology.FaceEdgesMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonPlanetCoordinatesEdgesMatchTest::RunTest(const FString& Parameters)
{
	FAstraeonPlanetDefinition Planet;
	Planet.BodyId=TEXT("edges"); Planet.RadiusCm=1000000.0;
	Planet.MassKg=1e16; Planet.SurfaceGravityMS2=9.81; Planet.BodySeed=4242;
	struct FBoundary { int32 Face; FVector Dir; };
	TArray<FBoundary> Samples;
	for (int32 Face=0; Face<6; ++Face)
	for (int32 Y=0; Y<=8; ++Y)
	for (int32 X=0; X<=8; ++X)
	{
		if (X!=0 && X!=8 && Y!=0 && Y!=8) continue;
		Samples.Add({Face,FAstraeonPlanetCoordinates::FaceUvToDirection(
			EAstraeonPlanetFace(Face),FVector2D(-1.0+X/4.0,-1.0+Y/4.0))});
	}
	int32 MatchedPairs=0;
	for (const auto& A:Samples)
	{
		int32 OtherFaces=0;
		for (const auto& B:Samples)
		{
			if (A.Face==B.Face || !A.Dir.Equals(B.Dir,1e-12)) continue;
			++OtherFaces; ++MatchedPairs;
			TestTrue(TEXT("Two distinct faces sample identical height"),
				FMath::IsNearlyEqual(FAstraeonPlanetSurface::SampleRadialHeightCm(Planet,A.Dir),
					FAstraeonPlanetSurface::SampleRadialHeightCm(Planet,B.Dir),1e-8));
			TestTrue(TEXT("Normals shared across faces"),
				FAstraeonPlanetSurface::SampleRadialNormal(Planet,A.Dir).Equals(
					FAstraeonPlanetSurface::SampleRadialNormal(Planet,B.Dir),1e-7));
		}
		TestTrue(TEXT("Every face boundary has one or two different neighbours"),OtherFaces>=1 && OtherFaces<=2);
	}
	TestEqual(TEXT("All 12 edges and 8 corners matched"),MatchedPairs,216);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPlanetDefinitionValidationTest,
	"Astraeon.Planet.Definition.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonPlanetDefinitionValidationTest::RunTest(const FString& Parameters)
{
	FAstraeonPlanetDefinition Definition;
	Definition.BodyId = TEXT("planet_lab");
	Definition.RadiusCm = 1000000.0;
	Definition.MassKg = 1.0e16;
	Definition.SurfaceGravityMS2 = 9.81;
	Definition.GeneratorVersion = 1;
	TestTrue(TEXT("Definicion valida"), Definition.IsValid());
	Definition.RadiusCm = -1.0;
	TestFalse(TEXT("Radio negativo invalida la definicion"), Definition.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPlanetHeightDeterminismTest,
	"Astraeon.Planet.Height.Determinism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonPlanetHeightDeterminismTest::RunTest(const FString& Parameters)
{
	FAstraeonPlanetDefinition Planet;
	Planet.BodyId = TEXT("planet_test"); Planet.RadiusCm = 1000000.0;
	Planet.MassKg = 1.0e16; Planet.SurfaceGravityMS2 = 9.81;
	Planet.BodySeed = 4242; Planet.GeneratorVersion = FAstraeonPlanetSurface::GeneratorVersion;
	// Esta direccion cae sobre una montana a 10 km de radio, asi que el valor fijo cubre
	// las dos capas a la vez: si cualquiera cambia sin subir la version, esto lo delata.
	TestTrue(TEXT("Version 3 fixed fixture survives executions and builds"),
		FMath::IsNearlyEqual(FAstraeonPlanetSurface::SampleRadialHeightCm(Planet,FVector(1,0,0)),2488.334339054522388,1e-9));
	const TArray<FVector> Directions = { FVector(1, 2, 3).GetSafeNormal(), FVector(-2, 1, 0.5).GetSafeNormal() };
	for (const FVector& Direction : Directions)
	{
		const double A = FAstraeonPlanetSurface::SampleRadialHeightCm(Planet, Direction);
		const double B = FAstraeonPlanetSurface::SampleRadialHeightCm(Planet, Direction);
		TestTrue(TEXT("La misma seed y direccion son deterministas"), FMath::IsNearlyEqual(A, B, 1.0e-9));
		// El suelo nace en el nivel del mar y sube: el relieve nunca excava por debajo.
		TestTrue(TEXT("El relieve queda dentro del rango de Fase 1"),
			A >= 0.0 && A <= FAstraeonPlanetSurface::MaxReliefCm);
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


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPlanetFrameTransportTest,
	"Astraeon.Planet.Frame.TransportKeepsHeading",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonPlanetFrameTransportTest::RunTest(const FString& Parameters)
{
	// Caminar media vuelta sin tocar el raton no puede girar el rumbo. Es el defecto que se vio
	// a mano: el personaje seguia caminando de frente y terminaba boca abajo.
	const FVector Center = FVector::ZeroVector;
	const double Radius = 20000.0;

	FVector Position = Center + FVector(0, 0, Radius);          // polo norte
	FVector Forward = FVector(1, 0, 0);                          // tangente ahi

	// 360 pasos de medio grado: media vuelta completa por un meridiano.
	for (int32 Step = 0; Step < 360; ++Step)
	{
		const FQuat StepRotation(FVector(0, 1, 0), FMath::DegreesToRadians(0.5));
		Position = StepRotation.RotateVector(Position);

		const FVector Up = FAstraeonPlanetFrame::UpAt(Center, Position);
		const FVector Right = FVector::CrossProduct(Up, Forward).GetSafeNormal();
		Forward = FAstraeonPlanetFrame::TransportTangent(Forward, Up, Right);

		const FQuat Frame = FAstraeonPlanetFrame::MakeFrame(Up, Forward);
		if (!FAstraeonPlanetFrame::IsFrameAligned(Frame, Up))
		{
			AddError(FString::Printf(TEXT("Marco degenerado en el paso %d"), Step));
			return false;
		}
		// El frente tiene que seguir siendo tangente en cada paso, no solo al final.
		if (FMath::Abs(FVector::DotProduct(Forward, Up)) > 1.0e-3)
		{
			AddError(FString::Printf(TEXT("El frente dejo de ser tangente en el paso %d"), Step));
			return false;
		}
	}

	// Tras media vuelta por el meridiano, el arriba local es el opuesto al de partida.
	const FVector FinalUp = FAstraeonPlanetFrame::UpAt(Center, Position);
	TestTrue(TEXT("Media vuelta deja el arriba invertido"),
		FinalUp.Equals(-FVector::UpVector, 1.0e-2));

	// Y el rumbo transportado sigue apuntando al mismo lado del mundo: caminar de frente no
	// hizo girar al personaje sobre si mismo.
	TestTrue(TEXT("El rumbo se conserva tras media vuelta"),
		FMath::Abs(FVector::DotProduct(Forward.GetSafeNormal(), FVector(-1, 0, 0))) > 0.99);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPlanetFrameYawTest,
	"Astraeon.Planet.Frame.YawIsAroundLocalUp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonPlanetFrameYawTest::RunTest(const FString& Parameters)
{
	// En el antipoda, girar 90 grados tiene que girar 90 grados respecto al suelo del jugador.
	// Si el yaw se aplicara alrededor del Z global, aqui giraria al reves.
	const FVector Up = -FVector::UpVector;
	const FVector Forward(1, 0, 0);

	const FVector Yawed = FAstraeonPlanetFrame::YawTangent(Forward, Up, 90.0);

	TestTrue(TEXT("Sigue siendo tangente"), FMath::Abs(FVector::DotProduct(Yawed, Up)) < 1.0e-4);
	TestTrue(TEXT("Giro 90 grados"),
		FMath::Abs(FVector::DotProduct(Yawed, Forward)) < 1.0e-3);

	// Cuatro giros de 90 vuelven al punto de partida: el giro no acumula deriva.
	FVector Cycled = Forward;
	for (int32 Turn = 0; Turn < 4; ++Turn)
	{
		Cycled = FAstraeonPlanetFrame::YawTangent(Cycled, Up, 90.0);
	}
	TestTrue(TEXT("Cuatro cuartos de vuelta vuelven al origen"), Cycled.Equals(Forward, 1.0e-3));

	// El sentido del giro tiene que ser opuesto al del polo norte, porque el arriba es opuesto.
	const FVector AtNorth = FAstraeonPlanetFrame::YawTangent(Forward, FVector::UpVector, 90.0);
	TestTrue(TEXT("El sentido depende del arriba local"), AtNorth.Equals(-Yawed, 1.0e-3));

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPlanetSurfaceContinuityTest,
	"Astraeon.Planet.Surface.ContinuityAndInvalidInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonPlanetSurfaceContinuityTest::RunTest(const FString& Parameters)
{
	FAstraeonPlanetDefinition P;
	P.BodyId=TEXT("smooth"); P.RadiusCm=1000000.0; P.MassKg=1e16; P.SurfaceGravityMS2=9.81;
	double Variation=0;
	for (const double Radius : {20000.0,1000000.0,50000000.0,250000000.0})
	for (int32 Seed : {0,42,4242,-17})
	{
		P.RadiusCm=Radius; P.BodySeed=Seed;
		for (int32 I=0; I<100; ++I)
		{
			const FVector D=FVector(1.0,I*0.02-1.0,0.31).GetSafeNormal();
			const FVector T=FVector::CrossProduct(D,FVector(0,0,1)).GetSafeNormal();
			// La continuidad se le exige a la capa de SUELO, que es la que se camina. Las
			// montanas estan exentas a proposito: una montana es un muro, y pedirle
			// pendiente suave seria pedirle que deje de ser una barrera.
			const double G=FAstraeonPlanetSurface::SampleGroundHeightCm(P,D);
			const double GNear=FAstraeonPlanetSurface::SampleGroundHeightCm(P,(D+T/Radius).GetSafeNormal());
			TestTrue(TEXT("One centimetre move changes ground height less than 0.1 cm"),FMath::Abs(G-GNear)<0.1);
			const double H=FAstraeonPlanetSurface::SampleRadialHeightCm(P,D);
			TestTrue(TEXT("Height stays between sea level and the relief ceiling"),
				H>=0.0 && H<=FAstraeonPlanetSurface::MaxReliefCm);
			const FVector N=FAstraeonPlanetSurface::SampleRadialNormal(P,D);
			TestTrue(TEXT("Normal finite, unit, outward"),!N.ContainsNaN() && FMath::Abs(N.Size()-1.0)<1e-8 && FVector::DotProduct(N,D)>0.0);
			// Fuera de las montanas la superficie es casi tangente, como antes.
			if (FAstraeonPlanetSurface::SampleMountainHeightCm(P,D)==0.0)
				TestTrue(TEXT("Ground normal stays near vertical"),FVector::DotProduct(N,D)>0.99);
			auto Changed=P; ++Changed.BodySeed;
			Variation+=FMath::Abs(H-FAstraeonPlanetSurface::SampleRadialHeightCm(Changed,D));
		}
	}
	TestTrue(TEXT("Seed changes the terrain"),Variation>1.0);
	TestFalse(TEXT("Zero direction rejected"),FAstraeonPlanetCoordinates::DirectionToFaceUv(FVector::ZeroVector).bIsValid);
	TestTrue(TEXT("Out of range UV rejected"),FAstraeonPlanetCoordinates::FaceUvToDirection(EAstraeonPlanetFace::PositiveX,FVector2D(2,0)).ContainsNaN());
	TestTrue(TEXT("Zero input cannot become flat ground"),!FMath::IsFinite(FAstraeonPlanetSurface::SampleRadialHeightCm(P,FVector::ZeroVector)));
	TestTrue(TEXT("Zero input cannot become flat ground on the walkable layer"),!FMath::IsFinite(FAstraeonPlanetSurface::SampleGroundHeightCm(P,FVector::ZeroVector)));
	// Un cuerpo demasiado chico para contener varias celdas de montana no recibe la capa:
	// el banco de locomocion de 200 m de radio es todo suelo caminable.
	auto Tiny=P; Tiny.RadiusCm=20000.0; Tiny.GeneratorVersion=FAstraeonPlanetSurface::GeneratorVersion;
	TestFalse(TEXT("A 200 m body has no mountain layer"),FAstraeonPlanetSurface::HasMountainLayer(Tiny));
	auto Big=P; Big.RadiusCm=1000000.0; Big.GeneratorVersion=FAstraeonPlanetSurface::GeneratorVersion;
	TestTrue(TEXT("A 10 km body does have one"),FAstraeonPlanetSurface::HasMountainLayer(Big));
	P.GeneratorVersion=999;
	TestFalse(TEXT("Unknown version rejected"),FMath::IsFinite(FAstraeonPlanetSurface::SampleRadialHeightCm(P,FVector(1,0,0))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonCubeSphereMeshTest,
	"Astraeon.Planet.Topology.MeshAtEngineeringTiers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonCubeSphereMeshTest::RunTest(const FString& Parameters)
{
	FAstraeonPlanetDefinition P;
	P.BodyId=TEXT("mesh"); P.MassKg=1e16; P.SurfaceGravityMS2=9.81;
	for (double Radius:{20000.0,1000000.0,50000000.0,250000000.0})
	{
		P.RadiusCm=Radius;
		for (int32 Face=0; Face<6; ++Face)
		{
			FAstraeonCubeSphereMesh D;
			TestTrue(TEXT("Face builds"),FAstraeonCubeSphereMesh::BuildFace(P,EAstraeonPlanetFace(Face),32,D));
			TestEqual(TEXT("Fixed vertex budget"),D.Vertices.Num(),1089);
			TestEqual(TEXT("Fixed triangle budget"),D.Indices.Num(),6144);
			for (int32 I=0; I<D.Indices.Num(); I+=3)
			{
				const FVector A=D.Vertices[D.Indices[I]],B=D.Vertices[D.Indices[I+1]],C=D.Vertices[D.Indices[I+2]];
				const FVector Cross=FVector::CrossProduct(B-A,C-A);
				TestTrue(TEXT("Nondegenerate outward-facing Unreal winding"),
					Cross.SizeSquared()>1e-8 && FVector::DotProduct(Cross,A+D.OriginBodyCm)<0);
			}
		}
	}
	return true;
}

#endif
