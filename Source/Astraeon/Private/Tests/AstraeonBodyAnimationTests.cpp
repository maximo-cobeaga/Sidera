#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Presentation/AstraeonBodyAnimation.h"

namespace AstraeonBodyAnimationTest
{
	const TCHAR* PlayerPath = TEXT("/Game/Astraeon/Characters/Player/Optimized/");

	FAstraeonBodyAnimationState Walking(const FVector2D& Direction, float SpeedCms)
	{
		FAstraeonBodyAnimationState State;
		State.SpeedCms = SpeedCms;
		State.LocalDirection = Direction.GetSafeNormal();
		return State;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonBodyAnimationSelectionTest,
	"Astraeon.Art.Character.BodyAnimationSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonBodyAnimationSelectionTest::RunTest(const FString& Parameters)
{
	using namespace AstraeonBodyAnimationTest;

	const float WalkSpeed = FAstraeonBodyAnimation::GetRunSpeedThresholdCms() - 100.0f;
	const float RunSpeed = FAstraeonBodyAnimation::GetRunSpeedThresholdCms() + 100.0f;

	// Quieto es quieto: el ruido de velocidad de la cápsula apoyada no puede encender el
	// clip de caminar.
	TestEqual(TEXT("Parado reproduce Idle"),
		FAstraeonBodyAnimation::Choose(Walking(FVector2D(1.0f, 0.0f), 1.0f)).ClipId, FName(TEXT("Idle")));

	// El motivo de todo esto: caminar de lado usaba el clip de caminar de frente.
	TestEqual(TEXT("Caminar de frente"),
		FAstraeonBodyAnimation::Choose(Walking(FVector2D(1.0f, 0.0f), WalkSpeed)).ClipId, FName(TEXT("Walk_F")));
	TestEqual(TEXT("Caminar hacia atrás"),
		FAstraeonBodyAnimation::Choose(Walking(FVector2D(-1.0f, 0.0f), WalkSpeed)).ClipId, FName(TEXT("Walk_B")));
	TestEqual(TEXT("Caminar a la derecha"),
		FAstraeonBodyAnimation::Choose(Walking(FVector2D(0.0f, 1.0f), WalkSpeed)).ClipId, FName(TEXT("Walk_R")));
	TestEqual(TEXT("Caminar a la izquierda"),
		FAstraeonBodyAnimation::Choose(Walking(FVector2D(0.0f, -1.0f), WalkSpeed)).ClipId, FName(TEXT("Walk_L")));
	TestEqual(TEXT("Correr de frente"),
		FAstraeonBodyAnimation::Choose(Walking(FVector2D(1.0f, 0.0f), RunSpeed)).ClipId, FName(TEXT("Run_F")));
	TestEqual(TEXT("Correr de lado"),
		FAstraeonBodyAnimation::Choose(Walking(FVector2D(0.0f, -1.0f), RunSpeed)).ClipId, FName(TEXT("Run_L")));

	// En diagonal manda el avance: es lo que el jugador siente que está haciendo.
	TestEqual(TEXT("En diagonal gana el avance"),
		FAstraeonBodyAnimation::Choose(Walking(FVector2D(1.0f, 1.0f), WalkSpeed)).ClipId, FName(TEXT("Walk_F")));

	TestTrue(TEXT("La locomoción se repite"),
		FAstraeonBodyAnimation::Choose(Walking(FVector2D(1.0f, 0.0f), WalkSpeed)).bLoop);

	// Salto en tres fases: antes era un único bucle de caída, sin despegue ni aterrizaje.
	{
		FAstraeonBodyAnimationState State = Walking(FVector2D(1.0f, 0.0f), WalkSpeed);
		State.bFalling = true;
		State.TakeoffSecondsRemaining = 0.2f;
		TestEqual(TEXT("Al despegar"), FAstraeonBodyAnimation::Choose(State).ClipId, FName(TEXT("Jump_Start")));
		TestFalse(TEXT("El despegue no se repite"), FAstraeonBodyAnimation::Choose(State).bLoop);

		State.TakeoffSecondsRemaining = 0.0f;
		TestEqual(TEXT("En el aire"), FAstraeonBodyAnimation::Choose(State).ClipId, FName(TEXT("Jump_Loop")));
		TestTrue(TEXT("La caída se repite"), FAstraeonBodyAnimation::Choose(State).bLoop);

		State.bFalling = false;
		State.LandingSecondsRemaining = 0.3f;
		TestEqual(TEXT("Al aterrizar"), FAstraeonBodyAnimation::Choose(State).ClipId, FName(TEXT("Jump_Land")));
	}

	// Las acciones tapan la locomoción en el suelo...
	{
		FAstraeonBodyAnimationState State = Walking(FVector2D(1.0f, 0.0f), WalkSpeed);
		State.Action = EAstraeonBodyAction::Scan;
		State.ActionSecondsRemaining = 0.5f;
		TestEqual(TEXT("Escanear tiene gesto de cuerpo"),
			FAstraeonBodyAnimation::Choose(State).ClipId, FName(TEXT("Scan")));
		TestFalse(TEXT("El gesto no se repite"), FAstraeonBodyAnimation::Choose(State).bLoop);

		State.Action = EAstraeonBodyAction::Interact;
		TestEqual(TEXT("Interactuar tiene gesto de cuerpo"),
			FAstraeonBodyAnimation::Choose(State).ClipId, FName(TEXT("Interact")));

		// ...pero no en el aire: caer escaneando tiene que verse como caer.
		State.bFalling = true;
		TestEqual(TEXT("Caer manda sobre el gesto"),
			FAstraeonBodyAnimation::Choose(State).ClipId, FName(TEXT("Jump_Loop")));

		State.bFalling = false;
		State.ActionSecondsRemaining = 0.0f;
		TestEqual(TEXT("Terminado el gesto vuelve la locomoción"),
			FAstraeonBodyAnimation::Choose(State).ClipId, FName(TEXT("Walk_F")));
	}

	// Un id mal escrito dejaría al cuerpo congelado en silencio: cada id que el selector
	// puede devolver tiene que existir como asset y pertenecer al esqueleto del jugador.
	const USkeleton* PlayerSkeleton = nullptr;
	const TArray<FName> ClipIds = FAstraeonBodyAnimation::GetAllClipIds();
	TestEqual(TEXT("El selector cubre el set completo de locomoción, salto y gestos"), ClipIds.Num(), 18);
	for (const FName& ClipId : ClipIds)
	{
		const FString AssetName = FString::Printf(TEXT("AN_Astraeon_Player_All_Armature_AN_Player_%s"), *ClipId.ToString());
		const FString ObjectPath = FString::Printf(TEXT("%s%s.%s"), PlayerPath, *AssetName, *AssetName);
		UAnimSequence* Sequence = LoadObject<UAnimSequence>(nullptr, *ObjectPath);
		if (!TestNotNull(*FString::Printf(TEXT("El clip %s existe"), *ClipId.ToString()), Sequence))
		{
			continue;
		}
		TestTrue(*FString::Printf(TEXT("El clip %s dura algo"), *ClipId.ToString()), Sequence->GetPlayLength() > 0.0f);
		if (!PlayerSkeleton)
		{
			PlayerSkeleton = Sequence->GetSkeleton();
		}
		TestTrue(*FString::Printf(TEXT("El clip %s vive en el esqueleto del protagonista"), *ClipId.ToString()),
			Sequence->GetSkeleton() == PlayerSkeleton);
	}

	return true;
}

#endif
