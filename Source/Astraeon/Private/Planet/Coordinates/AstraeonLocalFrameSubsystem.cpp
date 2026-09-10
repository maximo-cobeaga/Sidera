#include "Planet/Coordinates/AstraeonLocalFrameSubsystem.h"
#include "Planet/AstraeonPlanetRuntime.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/LightComponentBase.h"
#include "Misc/CoreDelegates.h"
#include "UObject/UObjectIterator.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

bool UAstraeonLocalFrameSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UAstraeonLocalFrameSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Only planets need it; the flat regions and menus keep the authored origin untouched.
	// `-AstraeonNoFrameShift` exists for the A/B measurement of TL_14, never for play.
	bEnabled = AAstraeonPlanetRuntime::FindActive(&InWorld) != nullptr
		&& !FParse::Param(FCommandLine::Get(), TEXT("AstraeonNoFrameShift"));
	FParse::Value(FCommandLine::Get(), TEXT("AstraeonFrameShiftCm="), ShiftThresholdCm);
	ShiftThresholdCm = FMath::Max(ShiftThresholdCm, 1000.0);
	if (bEnabled) ShiftHandle = FCoreDelegates::PostWorldOriginOffset.AddUObject(this, &UAstraeonLocalFrameSubsystem::RefreshLightsAfterShift);
	UE_LOG(LogTemp, Display, TEXT("LocalFrame: %s threshold_cm=%.0f"), bEnabled ? TEXT("enabled") : TEXT("disabled"), ShiftThresholdCm);
}

void UAstraeonLocalFrameSubsystem::Deinitialize()
{
	FCoreDelegates::PostWorldOriginOffset.Remove(ShiftHandle);
	Super::Deinitialize();
}

void UAstraeonLocalFrameSubsystem::RefreshLightsAfterShift(UWorld* ShiftedWorld, FIntVector From, FIntVector To)
{
	if (ShiftedWorld != GetWorld()) return;
	for (TObjectIterator<ULightComponentBase> It; It; ++It)
		if (It->GetWorld() == ShiftedWorld && It->IsRegistered()) It->MarkRenderStateDirty();
}

TStatId UAstraeonLocalFrameSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAstraeonLocalFrameSubsystem, STATGROUP_Tickables);
}

FIntVector UAstraeonLocalFrameSubsystem::GetOriginCm() const
{
	const UWorld* World = GetWorld();
	return World ? World->OriginLocation : FIntVector::ZeroValue;
}

void UAstraeonLocalFrameSubsystem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC) return;
	// The view is what must stay precise: the camera, or the pawn before a camera exists.
	const FVector View = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraLocation()
		: PC->GetPawn() ? PC->GetPawn()->GetActorLocation() : FVector::ZeroVector;
	MaxViewDistanceCm = FMath::Max(MaxViewDistanceCm, View.Size());
	if (!bEnabled || View.Size() <= ShiftThresholdCm) return;
	// Applied by the engine at the start of the next world tick, between two frames.
	const FIntVector NewOrigin = World->OriginLocation + FIntVector(FMath::RoundToInt32(View.X), FMath::RoundToInt32(View.Y), FMath::RoundToInt32(View.Z));
	if (World->RequestedOriginLocation == NewOrigin) return;
	World->RequestNewWorldOrigin(NewOrigin);
	++ShiftCount;
	UE_LOG(LogTemp, Verbose, TEXT("LocalFrame: shift %d to %s"), ShiftCount, *NewOrigin.ToString());
}
