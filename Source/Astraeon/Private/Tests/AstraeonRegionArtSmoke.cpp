#include "Tests/AstraeonRegionArtSmoke.h"
#include "AstraeonGameInstance.h"
#include "AstraeonPlayerCharacter.h"
#include "AstraeonPlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"
#include "HAL/PlatformMisc.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "WorldGen/AstraeonRegionMarker.h"

AAstraeonRegionArtSmoke::AAstraeonRegionArtSmoke()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;
}

void AAstraeonRegionArtSmoke::Finish(bool bPassed, const FString& Reason)
{
	UE_LOG(LogTemp, Display, TEXT("AstraeonRegionArtSmoke: Passed=%s %s"), bPassed ? TEXT("true") : TEXT("false"), *Reason);
	SetActorTickEnabled(false);
	FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
}

void AAstraeonRegionArtSmoke::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	if (Elapsed < 8) return;
	auto* PC = Cast<AAstraeonPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	auto* Player = PC ? Cast<AAstraeonPlayerCharacter>(PC->GetPawn()) : nullptr;
	auto* GI = GetGameInstance<UAstraeonGameInstance>();
	if (!PC || !Player || !GI) { Finish(false, TEXT("Missing session/player")); return; }
	auto Key = [PC](FKey Value, EInputEvent Event)
	{
		PC->InputKey(FInputKeyEventArgs::CreateSimulated(Value, Event, Event == IE_Pressed ? 1.0f : 0.0f));
	};
	if (Stage == 0) { Key(EKeys::Enter, IE_Pressed); ++Stage; return; }
	if (Stage == 1)
	{
		Key(EKeys::Enter, IE_Released);
		if (!GI->HasStartedGame()) { Finish(false, TEXT("New game did not start")); return; }
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(this, AAstraeonRegionMarker::StaticClass(), Found);
		for (auto* Actor : Found)
		{
			auto* Marker = CastChecked<AAstraeonRegionMarker>(Actor);
			if (Marker->GetMarkerKind() == EAstraeonRegionActorKind::Resource ||
				Marker->GetMarkerId() == TEXT("signal_source") || Marker->GetMarkerId() == TEXT("minor_geologic_anomaly"))
				Markers.Add(Marker);
		}
		if (Markers.Num() != 7) { Finish(false, TEXT("Expected five resources and two POIs")); return; }
		Player->GetCharacterMovement()->DisableMovement();
		PC->ConsoleCommand(TEXT("r.SetRes 1920x1080w"), false);
		if (PC->GetHUD()) PC->GetHUD()->bShowHUD = false;
		++Stage;
		return;
	}
	const int32 Index = (Stage - 2) / 8;
	const int32 Phase = (Stage - 2) % 8;
	if (!Markers.IsValidIndex(Index)) { Finish(true, TEXT("Seven meshes, live traces, scan and harvest/tool gates checked")); return; }
	auto* Marker = Markers[Index].Get();
	if (Phase != 7 && !IsValid(Marker)) { Finish(false, TEXT("Marker disappeared before inspection")); return; }
	const auto* Camera = Player->FindComponentByClass<UCameraComponent>();
	if (!Camera) { Finish(false, TEXT("Missing camera")); return; }
	if (Phase == 0)
	{
		CurrentId = Marker->GetMarkerId();
		bCurrentResource = Marker->GetMarkerKind() == EAstraeonRegionActorKind::Resource;
		bCurrentToolGated = !Marker->GetRequiredToolId().IsNone();
		Player->SetActorLocation(Marker->GetActorLocation() + FVector(260, 0, 36), false, nullptr, ETeleportType::TeleportPhysics);
		const FVector ViewTarget = Marker->GetActorLocation() + (CurrentId == TEXT("signal_source") ? FVector(0, 0, 50) : FVector::ZeroVector);
		PC->SetControlRotation((ViewTarget - Camera->GetComponentLocation()).Rotation());
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(RegionArtSmoke), false, Player);
		const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Camera->GetComponentLocation(), Marker->GetActorLocation(), ECC_Visibility, Params);
		if (!bHit || Hit.GetActor() != Marker) { Finish(false, TEXT("Art obstructs interaction ray: ") + Marker->GetMarkerId().ToString()); return; }
		TArray<UStaticMeshComponent*> Components;
		Marker->GetComponents(Components);
		const bool bHasArt = Components.ContainsByPredicate([](const UStaticMeshComponent* C)
		{
			return C->GetFName() == TEXT("RegionArt") && C->IsVisible() && C->GetStaticMesh() && C->GetCollisionEnabled() == ECollisionEnabled::NoCollision;
		});
		if (!bHasArt) { Finish(false, TEXT("Missing visible original mesh")); return; }
	}
	if (Phase == 3 && FParse::Param(FCommandLine::Get(), TEXT("AstraeonRegionArtScreenshots")))
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/RegionArt") /
			(Marker->GetMarkerId().ToString() + TEXT(".png")), false, false);
	}
	if (Phase == 4)
	{
		PC->SetControlRotation((Marker->GetActorLocation() - Camera->GetComponentLocation()).Rotation());
		Key(EKeys::LeftMouseButton, IE_Pressed);
	}
	if (Phase == 5) Key(EKeys::LeftMouseButton, IE_Released);
	if (Phase == 6 && Marker->GetMarkerKind() == EAstraeonRegionActorKind::Resource) Key(EKeys::E, IE_Pressed);
	if (Phase == 7)
	{
		Key(EKeys::E, IE_Released);
		const FName Id = CurrentId;
		const FName ExpectedEntry = bCurrentResource ? FName(*(TEXT("resource.") + Id.ToString())) :
			(Id == TEXT("signal_source") ? FName(TEXT("signal.source_survey")) : FName(TEXT("anomaly.minor_geologic")));
		const bool bLogged = GI->GetRuntimeLogbookEntries().ContainsByPredicate([ExpectedEntry](const FAstraeonLogbookEntry& Entry)
		{
			return Entry.EntryId == ExpectedEntry;
		});
		if (!bLogged) { Finish(false, TEXT("Scanner did not record ") + Id.ToString()); return; }
		if (bCurrentResource)
		{
			const int32 ExpectedCount = bCurrentToolGated ? 0 : GI->GetResourceNodeQuantity(Id);
			if (GI->GetInventoryItemCount(Id) != ExpectedCount || (bCurrentToolGated && !IsValid(Marker)))
			{ Finish(false, TEXT("Harvest quantity/tool gate changed: ") + Id.ToString()); return; }
			if (bCurrentToolGated && !GI->GetLastFeedbackMessage().Contains(TEXT("tool_core_drill")))
			{ Finish(false, TEXT("Missing actionable drill requirement")); return; }
		}
		UE_LOG(LogTemp, Display, TEXT("RegionArtInspected: Id=%s Feedback=%s"), *Id.ToString(), *GI->GetLastFeedbackMessage());
	}
	++Stage;
}
