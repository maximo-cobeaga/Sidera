#include "Tests/AstraeonPlanetWalkSmoke.h"

#include "AstraeonPlayerCharacter.h"
#include "AstraeonPlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Presentation/AstraeonBodyAnimation.h"
#include "Presentation/AstraeonFirstPersonRigComponent.h"
#include "Planet/Coordinates/AstraeonPlanetFrame.h"
#include "Planet/Gravity/AstraeonPlanetGravityComponent.h"

AAstraeonPlanetWalkSmoke::AAstraeonPlanetWalkSmoke()
{
	PrimaryActorTick.bCanEverTick = true;
	FParse::Value(FCommandLine::Get(), TEXT("AstraeonWalkSeconds="), WalkSeconds);
}

void AAstraeonPlanetWalkSmoke::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bFinished)
	{
		return;
	}

	AAstraeonPlayerCharacter* Character = Cast<AAstraeonPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!Character)
	{
		return;
	}

	// Sin partida iniciada la sesión se queda en el menú y no hay nada que caminar. Se arranca
	// una, igual que hace el medidor de rendimiento.
	if (!bStartedGame)
	{
		if (AAstraeonPlayerController* Controller = Cast<AAstraeonPlayerController>(Character->GetController()))
		{
			Controller->StartSelectedNewGame();
			bStartedGame = true;
		}
		return;
	}

	UAstraeonPlanetGravityComponent* Gravity = Character->FindComponentByClass<UAstraeonPlanetGravityComponent>();
	if (!Gravity || !Gravity->IsPlanetGravityActive())
	{
		// Sin gravedad planetaria activa este smoke no mide lo que dice medir. Es un fallo, no un
		// "no aplica": se pidió sobre un mapa que debía tener harness.
		Finish(false, TEXT("La gravedad planetaria no esta activa"));
		return;
	}

	Elapsed += DeltaSeconds;

	// Los dos primeros segundos son de asentamiento: el personaje cae los 120 cm con los que
	// nace sobre la superficie y no tiene sentido juzgar su altitud mientras tanto.
	if (Elapsed < 2.0f)
	{
		LastLocationCm = Character->GetActorLocation();
		return;
	}

	// Caminar de frente sin tocar la mirada, que es exactamente lo que se hizo a mano.
	// `-AstraeonJumpOnly` lo deja quieto: sirve para separar "no aterriza" de "no aterriza
	// mientras la direccion de gravedad cambia por moverse".
	if (!FParse::Param(FCommandLine::Get(), TEXT("AstraeonJumpOnly")))
	{
		Character->AddMovementInput(Character->GetActorForwardVector(), 1.0f);
	}

	// Saltos repetidos mientras camina, que es la acción que reportó el propietario: "al saltar
	// y moverme en el aire". Un salto único no lo reproducía: hay que insistir para que la
	// racha de caída, si se atasca, se acumule y se vea.
	if (Elapsed - LastJumpSeconds >= JumpEverySeconds)
	{
		LastJumpSeconds = Elapsed;
		++JumpsRequested;
		Character->Jump();
	}
	// Soltar la tecla, como hace una persona. `Jump()` deja `bPressedJump` en true y sin esto
	// el personaje volvería a saltar en el instante en que toca el suelo, para siempre: seria
	// un defecto del smoke disfrazado de defecto del juego.
	else if (Elapsed - LastJumpSeconds >= 0.2f)
	{
		Character->StopJumping();
	}

	const FVector Location = Character->GetActorLocation();
	DistanceTravelledCm += FVector::Dist(Location, LastLocationCm);
	LastLocationCm = Location;

	const FVector LocalUp = Gravity->GetUpVector();
	const double Alignment = FVector::DotProduct(Character->GetActorQuat().GetUpVector(), LocalUp);
	const double AltitudeCm = Gravity->GetAltitudeCm();

	++FramesSampled;
	WorstAlignment = FMath::Min(WorstAlignment, Alignment);

	if (Alignment < MinUpAlignment)
	{
		++FramesMisaligned;
	}
	if (AltitudeCm < MinAltitudeCm || AltitudeCm > MaxAltitudeCm)
	{
		++FramesOutOfAltitude;
		if (FMath::Abs(AltitudeCm) > FMath::Abs(WorstAltitudeCm))
		{
			WorstAltitudeCm = AltitudeCm;
		}
	}

	// El deslizamiento reportado es el personaje en caida sostenida: sin suelo no hay control de
	// marcha, sólo control aereo. Contar frames en Falling lo convierte en un numero.
	if (const UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		if (Movement->MovementMode == MOVE_Falling)
		{
			++FramesFalling;
			CurrentFallSeconds += DeltaSeconds;
			// La racha más larga es lo que distingue "saltó" de "se quedó atascado en el aire".
			// Un porcentaje agregado no: veinte saltos cortos y un atasco largo dan el mismo.
			LongestFallSeconds = FMath::Max(LongestFallSeconds, CurrentFallSeconds);
		}
		else
		{
			CurrentFallSeconds = 0.0f;
		}
	}

	// Qué clip elegiría la presentación en este frame. Es lo que el propietario ve trabado, y
	// medirlo aquí separa "la animación no cambia" de "la física no aterriza".
	if (const UAstraeonFirstPersonRigComponent* Rig = Character->FindComponentByClass<UAstraeonFirstPersonRigComponent>())
	{
		FAstraeonBodyAnimationState AnimState;
		if (const UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			AnimState.SpeedCms = FAstraeonPlanetFrame::ProjectToTangent(
				Movement->Velocity, Character->GetActorUpVector()).Size();
			AnimState.bFalling = Movement->IsFalling();
		}
		const FName Clip = FAstraeonBodyAnimation::Choose(AnimState).ClipId;
		if (Clip == FName(TEXT("Jump_Loop")) || Clip == FName(TEXT("Jump_Start")))
		{
			++FramesJumpClip;
		}
	}

	// Progreso periódico. Sin esto, "recorrió 225 cm" no distingue entre caminar despacio y
	// pararse a mitad de camino, que es justo la diferencia que hubo que investigar.
	if (Elapsed - LastProgressLogSeconds >= 15.0f)
	{
		LastProgressLogSeconds = Elapsed;
		const FVector Direction = (Location - Gravity->GetPlanetCenterCm()).GetSafeNormal();
		const double ArcDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Direction.Z, -1.0, 1.0)));
		UE_LOG(LogTemp, Display,
			TEXT("AstraeonPlanetWalk: t=%.0fs recorrido=%.0f cm arco=%.1f grados altitud=%.1f cm modo=%d vel=%.0f"),
			Elapsed, DistanceTravelledCm, ArcDegrees, AltitudeCm,
			Character->GetCharacterMovement() ? int32(Character->GetCharacterMovement()->MovementMode) : -1,
			Character->GetVelocity().Size());
	}

	if (Elapsed < WalkSeconds)
	{
		return;
	}

	const double MisalignedRatio = FramesSampled > 0 ? double(FramesMisaligned) / FramesSampled : 1.0;
	const double FallingRatio = FramesSampled > 0 ? double(FramesFalling) / FramesSampled : 1.0;
	const double OutOfAltitudeRatio = FramesSampled > 0 ? double(FramesOutOfAltitude) / FramesSampled : 1.0;

	const double JumpClipRatio = FramesSampled > 0 ? double(FramesJumpClip) / FramesSampled : 1.0;

	UE_LOG(LogTemp, Display,
		TEXT("AstraeonPlanetWalk: frames=%d recorrido=%.0f cm saltos=%d | desalineado=%.1f%% (peor cos=%.3f) | cayendo=%.1f%% (racha mas larga %.2f s) | clip de salto=%.1f%% | fuera de altitud=%.1f%% (peor=%.0f cm)"),
		FramesSampled, DistanceTravelledCm, JumpsRequested,
		MisalignedRatio * 100.0, WorstAlignment,
		FallingRatio * 100.0, LongestFallSeconds,
		JumpClipRatio * 100.0,
		OutOfAltitudeRatio * 100.0, WorstAltitudeCm);

	// Umbrales generosos a propósito: no se pide perfección, se pide que los tres defectos
	// reportados hayan dejado de ocurrir de forma sostenida.
	FString Failures;
	if (DistanceTravelledCm < 10000.0 && !FParse::Param(FCommandLine::Get(), TEXT("AstraeonJumpOnly")))
	{
		Failures += FString::Printf(TEXT("apenas se movio (%.0f cm); "), DistanceTravelledCm);
	}
	if (MisalignedRatio > 0.05)
	{
		Failures += FString::Printf(TEXT("volcado el %.1f%% del tiempo; "), MisalignedRatio * 100.0);
	}
	if (FallingRatio > 0.60)
	{
		Failures += FString::Printf(TEXT("en caida el %.1f%% del tiempo; "), FallingRatio * 100.0);
	}
	// Lo que de verdad importa del salto: que termine. Un salto normal dura menos de 1,5 s;
	// una racha de 3 s es quedarse atascado en el aire, que es el defecto reportado.
	if (LongestFallSeconds > 3.0f)
	{
		Failures += FString::Printf(TEXT("racha en el aire de %.2f s; "), LongestFallSeconds);
	}
	if (OutOfAltitudeRatio > 0.05)
	{
		Failures += FString::Printf(TEXT("fuera de altitud el %.1f%% del tiempo; "), OutOfAltitudeRatio * 100.0);
	}

	Finish(Failures.IsEmpty(), Failures.IsEmpty() ? TEXT("OK") : Failures);
}

void AAstraeonPlanetWalkSmoke::Finish(bool bPassed, const FString& Reason)
{
	bFinished = true;
	UE_LOG(LogTemp, Display, TEXT("AstraeonPlanetWalk: RESULTADO=%s %s"),
		bPassed ? TEXT("OK") : TEXT("FALLO"), *Reason);
	FPlatformMisc::RequestExit(!bPassed);
}
