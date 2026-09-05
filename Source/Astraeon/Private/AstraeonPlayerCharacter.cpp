#include "AstraeonPlayerCharacter.h"

#include "AstraeonGameInstance.h"
#include "Camera/CameraComponent.h"
#include "Creatures/AstraeonCreatureActor.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Survival/AstraeonSuitComponent.h"
#include "WorldGen/AstraeonRegionMarker.h"

AAstraeonPlayerCharacter::AAstraeonPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

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

	FText Feedback = FText::FromString(TEXT("ARGOS: no active session to scan."));
	bool bScanSucceeded = false;

	if (bHit)
	{
		if (AAstraeonCreatureActor* Creature = Cast<AAstraeonCreatureActor>(HitResult.GetActor()))
		{
			bScanSucceeded = AstraeonGameInstance->RecordCreatureScan(Creature->GetCreatureProfile());
			Feedback = FText::FromString(TEXT("ARGOS: organism observed and added to logbook."));
		}
	}

	if (!bScanSucceeded)
	{
		bScanSucceeded = AstraeonGameInstance->ScanCurrentEnvironment();
		Feedback = FText::FromString(TEXT("ARGOS: environmental scan confirmed in logbook."));
	}

	if (bScanSucceeded)
	{
		AstraeonGameInstance->RevealMapAroundLocationMeters(FVector2D(GetActorLocation().X, GetActorLocation().Y) / 100.0f, 2);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.0f, bScanSucceeded ? FColor::Cyan : FColor::Red, Feedback.ToString());
	}
}

void AAstraeonPlayerCharacter::Interact()
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	if (!AstraeonGameInstance)
	{
		return;
	}

	FHitResult HitResult;
	const FVector TraceStart = FirstPersonCamera ? FirstPersonCamera->GetComponentLocation() : GetActorLocation();
	const FVector TraceEnd = TraceStart + GetControlRotation().Vector() * 3500.0f;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AstraeonInteract), false, this);
	const bool bHit = GetWorld() && GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

	bool bCollected = false;
	bool bResolvedSignal = false;
	bool bSignalSourceAttempted = false;
	bool bArgosBriefingRead = false;
	FName CollectedId;
	if (bHit)
	{
		if (AAstraeonRegionMarker* Marker = Cast<AAstraeonRegionMarker>(HitResult.GetActor()))
		{
			if (Marker->GetMarkerKind() == EAstraeonRegionActorKind::Resource)
			{
				CollectedId = Marker->GetMarkerId();
				bCollected = AstraeonGameInstance->AddInventoryItem(CollectedId, 1);
				if (bCollected)
				{
					Marker->Destroy();
				}
			}
			else if (Marker->GetMarkerId() == TEXT("signal_source"))
			{
				bSignalSourceAttempted = true;
				bResolvedSignal = AstraeonGameInstance->TryResolveSignalSource();
			}
			else if (Marker->GetMarkerId() == TEXT("itaca_argos_console"))
			{
				AstraeonGameInstance->RecordArgosBriefing();
				bArgosBriefingRead = true;
			}
		}
	}

	if (GEngine)
	{
		const FString Message = bResolvedSignal
			? TEXT("Signal source resolved. Vertical slice complete.")
			: (bArgosBriefingRead
				? TEXT("ARGOS briefing recorded in logbook.")
				: (bSignalSourceAttempted
					? TEXT("Signal source requires signal_resonator. Gather resources and craft it with C.")
					: (bCollected ? FString::Printf(TEXT("Collected: %s"), *CollectedId.ToString()) : TEXT("No ARGOS console, collectible resource, or signal source in reach."))));
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.0f, (bCollected || bResolvedSignal || bArgosBriefingRead) ? FColor::Green : FColor::Yellow, Message);
	}
}

void AAstraeonPlayerCharacter::CraftSignalResonator()
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	const bool bCrafted = AstraeonGameInstance && AstraeonGameInstance->CraftSignalResonator();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			INDEX_NONE,
			2.0f,
			bCrafted ? FColor::Green : FColor::Yellow,
			bCrafted ? TEXT("Crafted: signal_resonator") : TEXT("Missing resources for signal_resonator."));
	}
}
