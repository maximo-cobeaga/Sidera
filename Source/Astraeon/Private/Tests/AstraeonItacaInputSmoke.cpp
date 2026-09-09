#include "Tests/AstraeonItacaInputSmoke.h"
#include "AstraeonGameInstance.h"
#include "AstraeonPlayerCharacter.h"
#include "AstraeonPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "WorldGen/AstraeonRegionMaterializer.h"
#include "HAL/PlatformMisc.h"
#include "Environment/AstraeonItacaInterior.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "GameFramework/HUD.h"

AAstraeonItacaInputSmoke::AAstraeonItacaInputSmoke()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f;
}

void AAstraeonItacaInputSmoke::Finish(bool bPassed, const FString& Reason)
{
	UE_LOG(LogTemp, Display, TEXT("AstraeonItacaInputSmoke: Passed=%s Stage=%d %s"), bPassed ? TEXT("true") : TEXT("false"), Stage, *Reason);
	SetActorTickEnabled(false);
	FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
}

void AAstraeonItacaInputSmoke::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	const bool bVisual = FParse::Param(FCommandLine::Get(), TEXT("AstraeonArtScreenshot"));
	if (Elapsed < (bVisual ? 8.0f : 1.0f)) return;
	auto* PC = Cast<AAstraeonPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	auto* Player = PC ? Cast<AAstraeonPlayerCharacter>(PC->GetPawn()) : nullptr;
	auto* GI = GetGameInstance<UAstraeonGameInstance>();
	if (!PC || !Player || !GI) { Finish(false, TEXT("Missing runtime player/session")); return; }
	auto Key = [PC](FKey Value, EInputEvent Event)
	{
		PC->InputKey(FInputKeyEventArgs::CreateSimulated(Value, Event, Event == IE_Pressed ? 1.0f : 0.0f));
	};
	auto HasEntry = [GI](FName Id)
	{
		return GI->GetRuntimeLogbookEntries().ContainsByPredicate([Id](const FAstraeonLogbookEntry& E) { return E.EntryId == Id; });
	};
	switch (Stage++)
	{
	case 0: Key(EKeys::Enter, IE_Pressed); break;
	case 1: Key(EKeys::Enter, IE_Released); break;
	case 2:
		if (!GI->HasStartedGame() || PC->IsMenuVisible()) { Finish(false, TEXT("Enter did not start session")); return; }
		{
			TArray<AActor*> Rooms;
			UGameplayStatics::GetAllActorsOfClass(this, AAstraeonItacaInterior::StaticClass(), Rooms);
			FCollisionQueryParams Params(SCENE_QUERY_STAT(ItacaCapsuleClearance), false, Player);
			FHitResult Hit;
			auto Blocked = [&](float Y, float Z)
			{
				return GetWorld()->SweepSingleByChannel(Hit, FVector(520,Y,Z), FVector(720,Y,Z), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(42,96), Params);
			};
			const bool bPassageClear = !Blocked(0,100);
			const bool bJambSolid = Blocked(90,100);
			const bool bHeaderSolid = Blocked(0,200);
			const UCameraComponent* Camera = Player->FindComponentByClass<UCameraComponent>();
			const bool bEyeHeightValid = Camera && FMath::IsNearlyEqual(Camera->GetRelativeLocation().Z, 64.0f);
			UE_LOG(LogTemp, Display, TEXT("ItacaClearance: Rooms=%d Passage=%d Jamb=%d Header=%d EyeHeight=%d"), Rooms.Num(), bPassageClear,bJambSolid,bHeaderSolid,bEyeHeightValid);
			if (Rooms.Num()!=1 || !bPassageClear || !bJambSolid || !bHeaderSolid || !bEyeHeightValid)
			{ Finish(false, TEXT("Room clearance/camera regression")); return; }
			if (FParse::Param(FCommandLine::Get(), TEXT("AstraeonArtScreenshot")))
				FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/ItacaInterior.png"), false, false);
		}
		Player->SetActorLocation(FVector(100,-140,100), false, nullptr, ETeleportType::TeleportPhysics);
		Player->GetCharacterMovement()->StopMovementImmediately();
		PC->SetControlRotation(FRotator(0,0,0));
		if (bVisual)
		{
			Player->SetActorLocation(FVector(0,100,100), false, nullptr, ETeleportType::TeleportPhysics);
			PC->SetControlRotation(FRotator(0,-12,0));
			if (PC->GetHUD()) PC->GetHUD()->bShowHUD = false;
		}
		break;
	case 3:
		if (bVisual)
		{
			Player->SetActorLocation(FVector(100,-140,100), false, nullptr, ETeleportType::TeleportPhysics);
			PC->SetControlRotation(FRotator::ZeroRotator);
			if (PC->GetHUD()) PC->GetHUD()->bShowHUD = true;
		}
		Key(EKeys::E, IE_Pressed); break;
	case 4: Key(EKeys::E, IE_Released); break;
	case 5:
		if (!HasEntry(TEXT("argos.first_signal_briefing"))) { Finish(false, TEXT("E did not brief ARGOS: ") + GI->GetLastFeedbackMessage()); return; }
		Player->SetActorLocation(FVector(520,0,100), false, nullptr, ETeleportType::TeleportPhysics);
		Player->GetCharacterMovement()->StopMovementImmediately();
		PC->SetControlRotation(FRotator::ZeroRotator);
		break;
	case 6: Key(EKeys::W, IE_Pressed); break;
	case 7: break; // Hold forward across several real movement ticks through the frame.
	case 8:
		Key(EKeys::W, IE_Released);
		UE_LOG(LogTemp, Display, TEXT("ItacaWalk: X=%.1f Passed=%d"), Player->GetActorLocation().X, Player->GetActorLocation().X > 662);
		if (Player->GetActorLocation().X <= 662) { Finish(false, TEXT("W could not walk through the hatch frame")); return; }
		Player->SetActorLocation(FVector(360,0,100), false, nullptr, ETeleportType::TeleportPhysics);
		Player->GetCharacterMovement()->StopMovementImmediately();
		break;
	case 9: Key(EKeys::E, IE_Pressed); break;
	case 10: Key(EKeys::E, IE_Released); break;
	case 11:
	{
		const FVector Destination = UAstraeonRegionMaterializer::GetSurfaceDeploymentLocationCm();
		const bool bAtDestination = FVector::Dist2D(Player->GetActorLocation(), Destination) < 5.0f;
		Finish(HasEntry(TEXT("itaca.surface_deployment")) && bAtDestination && Player->GetCharacterMovement()->IsMovingOnGround(),
			FString::Printf(TEXT("Argos=true Deployed=%d OnGround=%d Feedback=%s"), bAtDestination, Player->GetCharacterMovement()->IsMovingOnGround(), *GI->GetLastFeedbackMessage()));
		break;
	}
	default: Finish(false, TEXT("Unexpected stage")); break;
	}
}
