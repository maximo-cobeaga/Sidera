#include "AstraeonPlayerCharacter.h"

#include "AstraeonGameInstance.h"
#include "AstraeonGameModeBase.h"
#include "Camera/CameraComponent.h"
#include "Creatures/AstraeonCreatureActor.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Survival/AstraeonSuitComponent.h"
#include "WorldGen/AstraeonRegionMarker.h"
#include "WorldGen/AstraeonRegionMaterializer.h"

namespace AstraeonPlayerCharacterInteraction
{
	constexpr float InteractionTraceRangeCm = 3500.0f;
	constexpr float ProximityInteractionRadiusCm = 180.0f;
}

namespace AstraeonPlayerCharacterRescue
{
	// How far below the last confirmed-safe standing position the character has to fall
	// before an emergency recall kicks in. Large enough that a normal step off a ledge or a
	// jump never triggers it, small enough that an actual fall through the world (missing
	// or not-yet-registered collision, for example right after a SURFACE HATCH teleport) is
	// caught within a fraction of a second instead of free-falling out of the level forever.
	constexpr float RescueFallDistanceCm = 2000.0f;
}

AAstraeonPlayerCharacter::AAstraeonPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetCharacterMovement()->MaxWalkSpeed = 520.0f;
	GetCharacterMovement()->JumpZVelocity = 420.0f;
	GetCharacterMovement()->AirControl = 0.35f;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(-10.0f, 0.0f, 64.0f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	SuitComponent = CreateDefaultSubobject<UAstraeonSuitComponent>(TEXT("SuitComponent"));
}

void AAstraeonPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AAstraeonPlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AAstraeonPlayerCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AAstraeonPlayerCharacter::StartJump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &AAstraeonPlayerCharacter::StopJump);
	PlayerInputComponent->BindAction(TEXT("Scan"), IE_Pressed, this, &AAstraeonPlayerCharacter::ScanEnvironment);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AAstraeonPlayerCharacter::Interact);
	PlayerInputComponent->BindAction(TEXT("Craft"), IE_Pressed, this, &AAstraeonPlayerCharacter::CraftSignalResonator);
}

void AAstraeonPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Whatever the level places the character on at spawn (PlayerStart) counts as the
	// first known-safe ground location for the fall-rescue safety net.
	MarkLocationAsSafeGround(GetActorLocation());
}

void AAstraeonPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RescueFromVoidIfNeeded();
}

void AAstraeonPlayerCharacter::MoveForward(float Value)
{
	if (!FMath::IsNearlyZero(Value) && Controller)
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AAstraeonPlayerCharacter::MoveRight(float Value)
{
	if (!FMath::IsNearlyZero(Value) && Controller)
	{
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AAstraeonPlayerCharacter::StartJump()
{
	Jump();
}

void AAstraeonPlayerCharacter::StopJump()
{
	StopJumping();
}

void AAstraeonPlayerCharacter::ScanEnvironment()
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	if (!AstraeonGameInstance)
	{
		return;
	}

	FHitResult HitResult;
	const FVector TraceStart = FirstPersonCamera ? FirstPersonCamera->GetComponentLocation() : GetActorLocation();
	const FVector TraceEnd = TraceStart + GetControlRotation().Vector() * 6000.0f;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AstraeonScan), false, this);
	const bool bHit = GetWorld() && GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

	bool bScanSucceeded = false;

	if (bHit)
	{
		if (AAstraeonCreatureActor* Creature = Cast<AAstraeonCreatureActor>(HitResult.GetActor()))
		{
			bScanSucceeded = AstraeonGameInstance->RecordCreatureScan(Creature->GetCreatureProfile());
		}
	}

	if (!bScanSucceeded)
	{
		bScanSucceeded = AstraeonGameInstance->ScanCurrentEnvironment();
	}

	if (bScanSucceeded)
	{
		AstraeonGameInstance->RevealMapAroundLocationMeters(FVector2D(GetActorLocation().X, GetActorLocation().Y) / 100.0f, 2);
	}

	if (!bScanSucceeded)
	{
		AstraeonGameInstance->SetLastFeedbackMessage(TEXT("Sin sesión activa para escanear."));
	}
}

void AAstraeonPlayerCharacter::Interact()
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	if (!AstraeonGameInstance)
	{
		return;
	}

	bool bSignalSourceAttempted = false;
	bool bInteractionSucceeded = false;
	if (AAstraeonRegionMarker* Marker = FindFocusedRegionMarker())
	{
		bInteractionSucceeded = InteractWithRegionMarker(*Marker, *AstraeonGameInstance, bSignalSourceAttempted);
	}
	else if (AAstraeonRegionMarker* NearbyMarker = FindNearestRegionMarkerInReach(AstraeonPlayerCharacterInteraction::ProximityInteractionRadiusCm))
	{
		// Manual play should not require pixel-perfect aim at temporary cube markers.
		// The direct trace remains the preferred interaction, while this proximity
		// fallback makes mandatory MVP interactions reliable when the player is close.
		bInteractionSucceeded = InteractWithRegionMarker(*NearbyMarker, *AstraeonGameInstance, bSignalSourceAttempted);
	}

	if (!bInteractionSucceeded)
	{
		AstraeonGameInstance->SetLastFeedbackMessage(bSignalSourceAttempted
			? TEXT("La fuente de señal requiere signal_resonator. Recoge recursos y fábrícalo con C.")
			: TEXT("Sin consola ARGOS, escotilla, recurso recolectable ni fuente de señal al alcance."));
	}
}

AAstraeonRegionMarker* AAstraeonPlayerCharacter::FindFocusedRegionMarker() const
{
	FHitResult HitResult;
	const FVector TraceStart = FirstPersonCamera ? FirstPersonCamera->GetComponentLocation() : GetActorLocation();
	const FVector TraceEnd = TraceStart + GetControlRotation().Vector() * AstraeonPlayerCharacterInteraction::InteractionTraceRangeCm;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AstraeonInteract), false, this);
	const bool bHit = GetWorld() && GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	return bHit ? Cast<AAstraeonRegionMarker>(HitResult.GetActor()) : nullptr;
}

AAstraeonRegionMarker* AAstraeonPlayerCharacter::FindNearestRegionMarkerInReach(float RadiusCm) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> MarkerActors;
	UGameplayStatics::GetAllActorsOfClass(World, AAstraeonRegionMarker::StaticClass(), MarkerActors);

	AAstraeonRegionMarker* BestMarker = nullptr;
	float BestDistanceSquared = FMath::Square(RadiusCm);
	for (AActor* MarkerActor : MarkerActors)
	{
		AAstraeonRegionMarker* Marker = Cast<AAstraeonRegionMarker>(MarkerActor);
		if (!Marker || Marker->IsActorBeingDestroyed())
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Marker->GetActorLocation());
		if (DistanceSquared <= BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestMarker = Marker;
		}
	}

	return BestMarker;
}

bool AAstraeonPlayerCharacter::InteractWithRegionMarker(AAstraeonRegionMarker& Marker, UAstraeonGameInstance& AstraeonGameInstance, bool& bOutSignalSourceAttempted)
{
	if (Marker.GetMarkerKind() == EAstraeonRegionActorKind::Resource)
	{
		const FName CollectedId = Marker.GetMarkerId();
		const bool bCollected = AstraeonGameInstance.AddInventoryItem(CollectedId, 1);
		if (bCollected)
		{
			Marker.Destroy();
		}
		return bCollected;
	}

	if (Marker.GetMarkerId() == TEXT("signal_source"))
	{
		bOutSignalSourceAttempted = true;
		return AstraeonGameInstance.TryResolveSignalSource();
	}

	if (Marker.GetMarkerId() == TEXT("itaca_argos_console"))
	{
		AstraeonGameInstance.RecordArgosBriefing();
		return true;
	}

	if (Marker.GetMarkerId() == TEXT("itaca_surface_hatch"))
	{
		return DeployToSurface(AstraeonGameInstance);
	}

	return false;
}

bool AAstraeonPlayerCharacter::DeployToSurface(UAstraeonGameInstance& AstraeonGameInstance)
{
	if (!AstraeonGameInstance.RecordSurfaceDeployment())
	{
		return false;
	}

	const FVector DeploymentLocationCm = UAstraeonRegionMaterializer::GetSurfaceDeploymentLocationCm();

	FHitResult FloorHit;
	if (!TraceForDeploymentFloor(DeploymentLocationCm, FloorHit))
	{
		// The runtime region surface (AAstraeonGameModeBase::MaterializeCurrentRegion)
		// should already have a dedicated deployment pad under this point. If it is
		// missing - for example the region was never materialized for this session -
		// ask the GameMode to (re)materialize it instead of guessing at a homemade
		// collision volume, then try the trace once more before giving up.
		if (AAstraeonGameModeBase* AstraeonGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AAstraeonGameModeBase>() : nullptr)
		{
			AstraeonGameMode->MaterializeCurrentRegion();
		}

		if (!TraceForDeploymentFloor(DeploymentLocationCm, FloorHit))
		{
			AstraeonGameInstance.SetLastFeedbackMessage(TEXT("ESCOTILLA BLOQUEADA: no se detectó suelo transitable."));
			return false;
		}
	}

	const float SafeCapsuleHalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 96.0f;
	const FVector SafeActorLocation(DeploymentLocationCm.X, DeploymentLocationCm.Y, FloorHit.ImpactPoint.Z + SafeCapsuleHalfHeight + 4.0f);
	SetActorLocation(SafeActorLocation, false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->SetMovementMode(MOVE_Walking);
	}

	// Record this as safe ground immediately: if the floor under the hatch destination
	// turns out to be unreliable in some edge case the tick-based rescue net will bring
	// the character straight back here rather than letting them fall indefinitely.
	MarkLocationAsSafeGround(SafeActorLocation);

	AstraeonGameInstance.RevealMapAroundLocationMeters(FVector2D(GetActorLocation().X, GetActorLocation().Y) / 100.0f, 2);
	return true;
}

bool AAstraeonPlayerCharacter::TraceForDeploymentFloor(const FVector& DeploymentLocationCm, FHitResult& OutHit) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams FloorQueryParams(SCENE_QUERY_STAT(AstraeonSurfaceHatchFloor), false, this);
	const FVector FloorTraceStart = DeploymentLocationCm + FVector(0.0f, 0.0f, 300.0f);
	const FVector FloorTraceEnd = DeploymentLocationCm - FVector(0.0f, 0.0f, 600.0f);
	return World->LineTraceSingleByChannel(OutHit, FloorTraceStart, FloorTraceEnd, ECC_Visibility, FloorQueryParams);
}

void AAstraeonPlayerCharacter::MarkLocationAsSafeGround(const FVector& LocationCm)
{
	LastSafeGroundLocationCm = LocationCm;
	bHasSafeGroundLocation = true;
}

void AAstraeonPlayerCharacter::RescueFromVoidIfNeeded()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	if (MovementComponent->IsMovingOnGround())
	{
		MarkLocationAsSafeGround(GetActorLocation());
		return;
	}

	if (!bHasSafeGroundLocation)
	{
		return;
	}

	if (GetActorLocation().Z < LastSafeGroundLocationCm.Z - AstraeonPlayerCharacterRescue::RescueFallDistanceCm)
	{
		SetActorLocation(LastSafeGroundLocationCm, false, nullptr, ETeleportType::TeleportPhysics);
		MovementComponent->StopMovementImmediately();
		MovementComponent->SetMovementMode(MOVE_Walking);

		if (UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>())
		{
			AstraeonGameInstance->SetLastFeedbackMessage(TEXT("Rescate de emergencia: recuperado de una zona sin soporte."));
		}
	}
}

void AAstraeonPlayerCharacter::CraftSignalResonator()
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	const bool bCrafted = AstraeonGameInstance && AstraeonGameInstance->CraftSignalResonator();

	if (AstraeonGameInstance && !bCrafted)
	{
		AstraeonGameInstance->SetLastFeedbackMessage(TEXT("Faltan recursos para fabricar signal_resonator."));
	}
}
