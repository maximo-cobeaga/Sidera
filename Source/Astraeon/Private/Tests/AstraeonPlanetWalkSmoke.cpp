#include "Tests/AstraeonPlanetWalkSmoke.h"

#include "AstraeonPlayerCharacter.h"
#include "AstraeonPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
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
	Character->AddMovementInput(Character->GetActorForwardVector(), 1.0f);

	if (!bJumped && Elapsed >= JumpAtSeconds)
	{
		bJumped = true;
		Character->Jump();
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

	UE_LOG(LogTemp, Display,
		TEXT("AstraeonPlanetWalk: frames=%d recorrido=%.0f cm | desalineado=%.1f%% (peor cos=%.3f) | cayendo=%.1f%% | fuera de altitud=%.1f%% (peor=%.0f cm)"),
		FramesSampled, DistanceTravelledCm,
		MisalignedRatio * 100.0, WorstAlignment,
		FallingRatio * 100.0,
		OutOfAltitudeRatio * 100.0, WorstAltitudeCm);

	// Umbrales generosos a propósito: no se pide perfección, se pide que los tres defectos
	// reportados hayan dejado de ocurrir de forma sostenida.
	FString Failures;
	if (DistanceTravelledCm < 10000.0)
	{
		Failures += FString::Printf(TEXT("apenas se movio (%.0f cm); "), DistanceTravelledCm);
	}
	if (MisalignedRatio > 0.05)
	{
		Failures += FString::Printf(TEXT("volcado el %.1f%% del tiempo; "), MisalignedRatio * 100.0);
	}
	if (FallingRatio > 0.25)
	{
		Failures += FString::Printf(TEXT("en caida el %.1f%% del tiempo; "), FallingRatio * 100.0);
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
