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
#include "Planet/Coordinates/AstraeonLocalFrameSubsystem.h"
#include "Planet/LOD/AstraeonPlanetLODManager.h"
#include "Camera/PlayerCameraManager.h"
#include "Planet/Gravity/AstraeonPlanetGravityComponent.h"
#include "Planet/AstraeonPlanetRuntime.h"
#include "Planet/Surface/AstraeonPlanetSurface.h"
#include "UnrealClient.h"

AAstraeonPlanetWalkSmoke::AAstraeonPlanetWalkSmoke()
{
	PrimaryActorTick.bCanEverTick = true;
	// Compare movement against this frame's dt, after CharacterMovement consumed it.
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	FParse::Value(FCommandLine::Get(), TEXT("AstraeonWalkSeconds="), WalkSeconds);
	FParse::Value(FCommandLine::Get(), TEXT("AstraeonWalkStillSeconds="), StillSeconds);
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
	const AAstraeonPlanetRuntime* PlanetRuntime = AAstraeonPlanetRuntime::FindActive(GetWorld());
	const FVector Origin = PlanetRuntime ? PlanetRuntime->GetActorLocation() : FVector::ZeroVector;
	const bool bStill = Elapsed >= WalkSeconds;

	// Los dos primeros segundos son de asentamiento: el personaje cae los 120 cm con los que
	// nace sobre la superficie y no tiene sentido juzgar su altitud mientras tanto.
	if (Elapsed < 2.0f)
	{
		LastLocationCm = Character->GetActorLocation() - Origin;
		return;
	}

	// Caminar de frente sin tocar la mirada, que es exactamente lo que se hizo a mano.
	// `-AstraeonJumpOnly` lo deja quieto: sirve para separar "no aterriza" de "no aterriza
	// mientras la direccion de gravedad cambia por moverse".
	if (!bStill && !FParse::Param(FCommandLine::Get(), TEXT("AstraeonJumpOnly")))
	{
		Character->AddMovementInput(Character->GetActorForwardVector(), 1.0f);
	}

	// Saltos repetidos mientras camina, que es la acción que reportó el propietario: "al saltar
	// y moverme en el aire". Un salto único no lo reproducía: hay que insistir para que la
	// racha de caída, si se atasca, se acumule y se vea.
	// `-AstraeonNoJump` es el control: si la locomocion se corta igual sin un solo salto, la
	// causa no esta en el clip de salto ni en quien lo pide. `-AstraeonSprint` reproduce la
	// carrera, que es donde el propietario reporto que ademas se frena.
	const bool bNoJump = FParse::Param(FCommandLine::Get(), TEXT("AstraeonNoJump"));
	if (FParse::Param(FCommandLine::Get(), TEXT("AstraeonSprint")))
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = 900.0f;
	}
	if (!bNoJump && !bStill && Elapsed - LastJumpSeconds >= JumpEverySeconds)
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

	const FVector Location = Character->GetActorLocation() - Origin;
	const double FrameDistance=FVector::Dist(Location,LastLocationCm);
	const double MaxPlausible=FMath::Max(100.0,Character->GetVelocity().Size()*DeltaSeconds*4.0);
	if (FrameDistance>MaxPlausible)
	{
		Finish(false,FString::Printf(TEXT("Impossible frame displacement %.1f cm (limit %.1f)"),FrameDistance,MaxPlausible));
		return;
	}
	DistanceTravelledCm += FVector::Dist(Location, LastLocationCm);
	LastLocationCm = Location;

	// Jitter. The camera is what the player sees; relative to the planet it survives shifts.
	if (const APlayerController* PC = Cast<APlayerController>(Character->GetController()))
	{
		const FVector Eye = PC->PlayerCameraManager->GetCameraLocation() - Origin;
		const auto* Frame = GetWorld()->GetSubsystem<UAstraeonLocalFrameSubsystem>();
		ShiftsAtSample[2] = ShiftsAtSample[1]; ShiftsAtSample[1] = ShiftsAtSample[0];
		ShiftsAtSample[0] = Frame ? Frame->GetShiftCount() : 0;
		// A frame much longer or shorter than the previous one is a hitch (the screenshot at
		// 30 s): movement is clamped to a maximum step and no longer tracks the frame time. Hitches
		// are measured by the profiler; here only steady frames count.
		// Movement catches up over the frames after a hitch, so a few frames after one are skipped too.
		if (EyeDeltaSeconds > 0.f && (DeltaSeconds > EyeDeltaSeconds * 1.5f || DeltaSeconds < EyeDeltaSeconds / 1.5f)) EyeCooldownFrames = 4;
		const bool bSteady = EyeCooldownFrames == 0;
		EyeCooldownFrames = FMath::Max(0, EyeCooldownFrames - 1);
		if (!bStill && Elapsed > 5.0f && EyeSamples >= 2 && bSteady)
		{
			// Distance from where uniform motion would have put the camera, with the real frame
			// times: a long frame (a screenshot) is not jitter, a sideways twitch is.
			const FVector Predicted = EyeHistory[0] + (EyeHistory[0] - EyeHistory[1]) * (DeltaSeconds / EyeDeltaSeconds);
			const double D2 = (Eye - Predicted).Size();
			EyeSecondDifferencesCm.Add(D2);
			const int32 Shifts = ShiftsAtSample[0] - ShiftsAtSample[2];
			EyeSamplesNearShift += Shifts > 0;
			if (D2 > EyeWorstCm) { EyeWorstCm = D2; EyeWorstSeconds = Elapsed; EyeWorstShiftDelta = Shifts; }
		}
		EyeHistory[1] = EyeHistory[0]; EyeHistory[0] = Eye; ++EyeSamples; EyeDeltaSeconds = DeltaSeconds;
		// Standing: 1.5 s to stop, then the camera must not move at all.
		if (bStill && Elapsed >= WalkSeconds + 1.5f)
		{
			if (!bStillAnchored) { StillAnchorCm = Eye; bStillAnchored = true; }
			StillMaxDriftCm = FMath::Max(StillMaxDriftCm, FVector::Dist(Eye, StillAnchorCm));
			StillMaxStepCm = FMath::Max(StillMaxStepCm, FVector::Dist(EyeHistory[0], EyeHistory[1]));
		}
	}

	const FVector LocalUp = Gravity->GetUpVector();
	const double Alignment = FVector::DotProduct(Character->GetActorQuat().GetUpVector(), LocalUp);
	double AltitudeCm = Gravity->GetAltitudeCm();
	if (const auto* Planet=AAstraeonPlanetRuntime::FindActive(GetWorld()))
		AltitudeCm-=FAstraeonPlanetSurface::SampleRadialHeightCm(Planet->GetDefinition(),LocalUp);
	if (Elapsed>30.0f && Elapsed-DeltaSeconds<=30.0f)
		FScreenshotRequest::RequestScreenshot(TEXT("PlanetWalkLab"),true,false);

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
		if (Clip == FName(TEXT("Jump_Land")))
		{
			++FramesLandClip;
		}
		// Cada cambio de clip reinicia la reproduccion: es exactamente lo que se ve como
		// "el paso se corta antes de terminar".
		if (!LastClipId.IsNone() && Clip != LastClipId)
		{
			++LocomotionRestarts;
			++ClipTransitions.FindOrAdd(FString::Printf(TEXT("%s->%s"), *LastClipId.ToString(), *Clip.ToString()));
		}
		LastClipId = Clip;
		++ClipFrames.FindOrAdd(Clip);
		// Pasado el arranque, el smoke mantiene la entrada de avance pulsada sin soltarla: un
		// solo frame en `Idle` significa que algo paro al personaje, no que dejo de caminar.
		if (Elapsed > 5.0f && !bStill && Clip == FName(TEXT("Idle"))
			&& !FParse::Param(FCommandLine::Get(), TEXT("AstraeonJumpOnly")))
		{
			++FramesIdleWhileMoving;
		}
		SpeedSumCms += AnimState.SpeedCms;
		MinSpeedCms = FMath::Min(MinSpeedCms, double(AnimState.SpeedCms));
		MaxSpeedCms = FMath::Max(MaxSpeedCms, double(AnimState.SpeedCms));
	}

	// Progreso periódico. Sin esto, "recorrió 225 cm" no distingue entre caminar despacio y
	// pararse a mitad de camino, que es justo la diferencia que hubo que investigar.
	if (Elapsed - LastProgressLogSeconds >= 15.0f)
	{
		LastProgressLogSeconds = Elapsed;
		const FVector Direction = (Character->GetActorLocation() - Gravity->GetPlanetCenterCm()).GetSafeNormal();
		const double ArcDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Direction.Z, -1.0, 1.0)));
		UE_LOG(LogTemp, Display,
			TEXT("AstraeonPlanetWalk: t=%.0fs recorrido=%.0f cm arco=%.1f grados altitud=%.1f cm modo=%d vel=%.0f"),
			Elapsed, DistanceTravelledCm, ArcDegrees, AltitudeCm,
			Character->GetCharacterMovement() ? int32(Character->GetCharacterMovement()->MovementMode) : -1,
			Character->GetVelocity().Size());
	}

	if (Elapsed < WalkSeconds + StillSeconds)
	{
		return;
	}

	const double MisalignedRatio = FramesSampled > 0 ? double(FramesMisaligned) / FramesSampled : 1.0;
	const double FallingRatio = FramesSampled > 0 ? double(FramesFalling) / FramesSampled : 1.0;
	const double OutOfAltitudeRatio = FramesSampled > 0 ? double(FramesOutOfAltitude) / FramesSampled : 1.0;

	const double JumpClipRatio = FramesSampled > 0 ? double(FramesJumpClip) / FramesSampled : 1.0;
	const double LandClipRatio = FramesSampled > 0 ? double(FramesLandClip) / FramesSampled : 0.0;

	UE_LOG(LogTemp, Display,
		TEXT("AstraeonPlanetWalk: clip de aterrizaje=%.1f%% | cambios de clip=%d (%.2f por segundo)"),
		LandClipRatio * 100.0, LocomotionRestarts,
		Elapsed > 0.0f ? LocomotionRestarts / Elapsed : 0.0);

	UE_LOG(LogTemp, Display, TEXT("AstraeonPlanetWalk: frames en Idle mientras camina=%d (%.2f%%)"),
		FramesIdleWhileMoving,
		FramesSampled > 0 ? 100.0 * FramesIdleWhileMoving / FramesSampled : 0.0);

	UE_LOG(LogTemp, Display, TEXT("AstraeonPlanetWalk: velocidad de animacion min=%.0f media=%.0f max=%.0f (umbral de carrera %.0f)"),
		MinSpeedCms, FramesSampled > 0 ? SpeedSumCms / FramesSampled : 0.0, MaxSpeedCms,
		FAstraeonBodyAnimation::GetRunSpeedThresholdCms());
	for (const auto& Pair : ClipFrames)
	{
		UE_LOG(LogTemp, Display, TEXT("AstraeonPlanetWalk: clip %s = %d frames (%.1f%%)"),
			*Pair.Key.ToString(), Pair.Value, FramesSampled > 0 ? 100.0 * Pair.Value / FramesSampled : 0.0);
	}
	for (const auto& Pair : ClipTransitions)
	{
		UE_LOG(LogTemp, Display, TEXT("AstraeonPlanetWalk: transicion %s = %d veces"), *Pair.Key, Pair.Value);
	}
	if (const auto* PlanetActor = AAstraeonPlanetRuntime::FindActive(GetWorld()))
	{
		UE_LOG(LogTemp, Display, TEXT("AstraeonPlanetWalk: reconstrucciones de colision=%d (%.2f por segundo) | de emergencia=%d | frames sin colision bajo el jugador=%d | patches de colision vivos=%d"),
			PlanetActor->GetCollisionRebuildCount(),
			Elapsed > 0.0f ? PlanetActor->GetCollisionRebuildCount() / Elapsed : 0.0,
			PlanetActor->GetCollisionEmergencyBuilds(), PlanetActor->GetCollisionMissingFrames(),
			PlanetActor->GetCollisionPatchCount());
	}

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
	// Guardian del corte de locomocion. El defecto medido daba 53 cortes en 40 s; el arreglo
	// da 0. Un 0,5% deja pasar el frame suelto de un arranque y no deja pasar una recaida.
	const double IdleWhileMovingRatio = FramesSampled > 0 ? double(FramesIdleWhileMoving) / FramesSampled : 0.0;
	if (IdleWhileMovingRatio > 0.005)
	{
		Failures += FString::Printf(
			TEXT("el ciclo de paso se corta: %.1f%% de los frames en Idle mientras camina (%d frames); "),
			IdleWhileMovingRatio * 100.0, FramesIdleWhileMoving);
	}

	if (const auto* Planet=AAstraeonPlanetRuntime::FindActive(GetWorld()))
	{
		// Marco local y jitter (P2.5).
		if (const auto* Frame = GetWorld()->GetSubsystem<UAstraeonLocalFrameSubsystem>())
		{
			TArray<double> Sorted = EyeSecondDifferencesCm; Sorted.Sort();
			const double P99 = Sorted.IsEmpty() ? 0.0 : Sorted[FMath::Min(Sorted.Num() - 1, int32(Sorted.Num() * 0.99))];
			const double P50 = Sorted.IsEmpty() ? 0.0 : Sorted[Sorted.Num() / 2];
			UE_LOG(LogTemp, Display, TEXT("AstraeonPlanetWalk: marco local %s cambios=%d umbral=%.0f cm vista max desde el origen=%.0f cm | camara 2a diferencia p50=%.4f p99=%.4f max=%.4f cm (%d frames) | quieto: deriva=%.4f cm paso max=%.4f cm"),
				Frame->IsEnabled() ? TEXT("activo") : TEXT("apagado"), Frame->GetShiftCount(), Frame->GetShiftThresholdCm(),
				Frame->GetMaxViewDistanceCm(), P50, P99, Sorted.IsEmpty() ? 0.0 : Sorted.Last(), Sorted.Num(), StillMaxDriftCm, StillMaxStepCm);
			UE_LOG(LogTemp, Display, TEXT("AstraeonPlanetWalk: peor 2a diferencia %.3f cm en t=%.2f s, cambios de marco en esa ventana=%d | muestras junto a un cambio=%d"),
				EyeWorstCm, EyeWorstSeconds, EyeWorstShiftDelta, EyeSamplesNearShift);
			// Quieto no es "casi quieto": un milimetro de temblor ya es visible a un metro de la
			// camara. Sin fase quieta no hay nada que juzgar.
			if (StillSeconds > 0.f && (!bStillAnchored || StillMaxStepCm > 0.1 || StillMaxDriftCm > 0.1))
				Failures += FString::Printf(TEXT("la camara tiembla quieta (deriva %.3f cm, paso %.3f cm); "), StillMaxDriftCm, StillMaxStepCm);
		}
		// La vuelta completa se exige donde cabe en la caminata (banco de 200 m); en un planeta
		// grande basta la distancia, que ya se exige arriba.
		constexpr double LapPlanetMaxCircumferenceCm = 150000.0;
		if (2.0*PI*Planet->RadiusCm <= LapPlanetMaxCircumferenceCm && DistanceTravelledCm < 2.0*PI*Planet->RadiusCm)
			Failures+=TEXT("No complete logical lap; ");
		// Guardian del anillo de colision: nunca un frame sin suelo bajo el jugador.
		if (Planet->GetCollisionMissingFrames()>0)
			Failures+=FString::Printf(TEXT("%d frames sin colision bajo el jugador; "),Planet->GetCollisionMissingFrames());
		// El suelo que se pisa es el del patch mas fino, asi que ese patch tiene que ser el que
		// se ve. Mismo margen que el corte de locomocion.
		if (const auto* Patches=Planet->GetPatchManager())
		{
			const auto& S=Patches->GetStats();
			const int32 Checked=Planet->GetGroundCheckedFrames();
			const double MismatchRatio=Checked>0 ? double(Planet->GetGroundMismatchFrames())/Checked : 1.0;
			// Puerta de Fase 2: el detalle alto no crece con el radio. Se cuenta lo que esta en
			// pantalla al nivel mas fino, que es lo que cuesta.
			const uint8 Finest=FAstraeonPlanetLODManager::FinestAllowedLod(Planet->GetDefinition(),{});
			int32 FinestVisible=0;
			for (const auto& A:Patches->GetVisible()) FinestVisible+=A.Lod==Finest;
			UE_LOG(LogTemp,Display,TEXT("AstraeonPlanetWalk: radio=%.0f km patches visibles=%d al nivel mas fino (%d)=%d pedidos=%d confirmados=%d relevos=%d obsoletos=%d fallos=%d | suelo visible distinto del pisado=%d de %d frames"),
				Planet->RadiusCm/100000.0,Patches->GetVisibleCount(),Finest,FinestVisible,S.Requests,S.Commits,S.Relays,S.StaleDrops,S.Failures,
				Planet->GetGroundMismatchFrames(),Checked);
			if (Patches->GetVisibleCount()==0) Failures+=TEXT("ningun patch visible; ");
			if (S.Failures>0) Failures+=FString::Printf(TEXT("%d patches fallidos; "),S.Failures);
			if (MismatchRatio>0.005)
				Failures+=FString::Printf(TEXT("suelo visible distinto del pisado el %.1f%% de los frames; "),MismatchRatio*100.0);
		}
	}
	Finish(Failures.IsEmpty(), Failures.IsEmpty() ? TEXT("OK") : Failures);
}

void AAstraeonPlanetWalkSmoke::Finish(bool bPassed, const FString& Reason)
{
	bFinished = true;
	UE_LOG(LogTemp, Display, TEXT("AstraeonPlanetWalk: RESULTADO=%s %s"),
		bPassed ? TEXT("OK") : TEXT("FALLO"), *Reason);
	FPlatformMisc::RequestExitWithStatus(false,bPassed?0:1);
}
