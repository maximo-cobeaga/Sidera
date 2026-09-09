#include "AstraeonPlayerCharacter.h"

#include "AstraeonDiagnostics.h"
#include "AstraeonGameInstance.h"
#include "AstraeonGameModeBase.h"
#include "AstraeonPlayerController.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Camera/CameraComponent.h"
#include "Creatures/AstraeonCreatureActor.h"
#include "Building/AstraeonBuiltStructure.h"
#include "Environment/AstraeonItacaInterior.h"
#include "Presentation/AstraeonFirstPersonRigComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "TimerManager.h"
#include "InputKeyEventArgs.h"
#include "GameFramework/Actor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Survival/AstraeonSuitComponent.h"
#include "WorldGen/AstraeonRegionMarker.h"
#include "WorldGen/AstraeonRegionMaterializer.h"
#include "WorldGen/AstraeonTerrainField.h"

namespace AstraeonPlayerCharacterInteraction
{
	constexpr float InteractionTraceRangeCm = 3500.0f;
	constexpr float ProximityInteractionRadiusCm = 450.0f;
	constexpr float CloseInteractionRadiusCm = 220.0f;
	constexpr float AimForgivenessRadiusCm = 160.0f;
	constexpr float FabricatorReachCm = 500.0f;
}

namespace AstraeonPlayerCharacterBuilding
{
	constexpr float PlacementRangeCm = 900.0f;
}

namespace AstraeonPlayerCharacterCombat
{
	// Tres impactos abaten a un Umbra Grazer (100 de vida): matar cuesta algo, pero no
	// tanto como para que esquivar deje de ser una opción legítima.
	constexpr float WeaponDamage = 34.0f;
	constexpr float WeaponRangeCm = 6000.0f;
}

namespace AstraeonPlayerCharacterMovement
{
	constexpr float WalkSpeedCms = 520.0f;
	constexpr float SprintSpeedCms = 900.0f;
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
	// Por frame, para que el rig de primera persona pueda cambiar de clip sin 200 ms de
	// retraso. La lógica de juego que antes corría a 5 Hz se sigue evaluando a 5 Hz en Tick.
	PrimaryActorTick.TickInterval = 0.0f;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetCharacterMovement()->MaxWalkSpeed = AstraeonPlayerCharacterMovement::WalkSpeedCms;
	GetCharacterMovement()->JumpZVelocity = 420.0f;
	GetCharacterMovement()->AirControl = 0.35f;
	// Por defecto el Character HEREDA la rotación de aquello sobre lo que se apoya. Si pisa
	// algo que rota cada frame —la criatura gira hacia donde se mueve— la vista del jugador
	// se va girando sola, y deja de hacerlo al saltar porque en el aire no hay base. Una
	// vista en primera persona nunca debe rotar por culpa del suelo.
	GetCharacterMovement()->bIgnoreBaseRotation = true;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(-10.0f, 0.0f, 64.0f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	// Vista de tercera persona. Arranca desactivada: la cámara activa es la de primera.
	ThirdPersonBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("ThirdPersonBoom"));
	ThirdPersonBoom->SetupAttachment(GetCapsuleComponent());
	ThirdPersonBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
	ThirdPersonBoom->TargetArmLength = 320.0f;
	// Desplazado a un lado para que el cuerpo no tape el centro de la pantalla.
	ThirdPersonBoom->SocketOffset = FVector(0.0f, 55.0f, 20.0f);
	ThirdPersonBoom->bUsePawnControlRotation = true;
	// Sin esto la cámara atraviesa paredes y terreno al pegarse a una superficie.
	ThirdPersonBoom->bDoCollisionTest = true;
	ThirdPersonBoom->ProbeSize = 12.0f;

	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(ThirdPersonBoom, USpringArmComponent::SocketName);
	ThirdPersonCamera->bUsePawnControlRotation = false;
	// `SetActive(false)` no alcanza: los componentes se auto-activan al registrarse y la
	// vista arrancaría en tercera persona.
	ThirdPersonCamera->bAutoActivate = false;
	ThirdPersonCamera->SetActive(false);

	SuitComponent = CreateDefaultSubobject<UAstraeonSuitComponent>(TEXT("SuitComponent"));

	FirstPersonRig = CreateDefaultSubobject<UAstraeonFirstPersonRigComponent>(TEXT("FirstPersonRig"));
	FirstPersonRig->SetupAttachment(FirstPersonCamera);

	// Cuerpo visible en tercera persona y sombra propia en primera persona.
	// La raíz del rig está en los pies, de ahí el -96 (media altura de la cápsula); el
	// frente del rig queda en +Y tras la importación, de ahí el yaw de -90.
	if (USkeletalMeshComponent* BodyMesh = GetMesh())
	{
		// Protagonista definitivo: 65 284 triángulos, 4 LOD, 75 huesos, 45 clips y tres
		// morph targets, validado por Scripts/Editor/ValidateMainCharacterImport.py.
		// Lleva su propio esqueleto, distinto del SKEL_Humanoid_A de 57 huesos que anima
		// las manos de primera persona. Cada vista usa clips compatibles con su esqueleto.
		// El blockout queda como respaldo si el paquete no está en el checkout.
		static ConstructorHelpers::FObjectFinder<USkeletalMesh> PlayerBody(
			TEXT("/Game/Astraeon/Characters/Player/Optimized/SK_Astraeon_Player.SK_Astraeon_Player"));
		static ConstructorHelpers::FObjectFinder<USkeletalMesh> BodyBlockout(
			TEXT("/Game/Astraeon/Art/Blockouts/Human/SK_Human_Body_Blockout.SK_Human_Body_Blockout"));
		if (PlayerBody.Succeeded())
		{
			BodyMesh->SetSkeletalMeshAsset(PlayerBody.Object);
		}
		else if (BodyBlockout.Succeeded())
		{
			BodyMesh->SetSkeletalMeshAsset(BodyBlockout.Object);
		}
		BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
		BodyMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		BodyMesh->SetOwnerNoSee(true);
		BodyMesh->SetCastHiddenShadow(true);
		BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BodyMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}
	// Las piezas fueron exportadas en el mismo espacio y esqueleto que el cuerpo.
	// Comparten su pose, sin duplicar animación ni multiplicar la escala de un socket.
	for (const TCHAR* Name : { TEXT("Helmet"), TEXT("Backpack"), TEXT("WristComputer") })
	{
		USkeletalMeshComponent* Part = CreateDefaultSubobject<USkeletalMeshComponent>(FName(Name));
		Part->SetupAttachment(GetMesh());
		const FString Asset = FString::Printf(TEXT("/Game/Astraeon/Characters/Player/Optimized/SK_Astraeon_%s.SK_Astraeon_%s"), Name, Name);
		ConstructorHelpers::FObjectFinder<USkeletalMesh> Finder(*Asset);
		Part->SetSkeletalMeshAsset(Finder.Object);
		Part->SetLeaderPoseComponent(GetMesh());
		Part->SetOwnerNoSee(true);
		Part->SetCastHiddenShadow(true);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BodyEquipment.Add(Part);
	}

	BuildPreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildPreviewMesh"));
	BuildPreviewMesh->SetupAttachment(GetCapsuleComponent());
	// El fantasma se posiciona en mundo, no relativo al jugador: sigue la superficie
	// apuntada, no la cápsula.
	BuildPreviewMesh->SetAbsolute(true, true, true);
	BuildPreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BuildPreviewMesh->SetCastShadow(false);
	BuildPreviewMesh->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PreviewCube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (PreviewCube.Succeeded())
	{
		BuildPreviewMesh->SetStaticMesh(PreviewCube.Object);
	}
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
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AAstraeonPlayerCharacter::StartSprint);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AAstraeonPlayerCharacter::StopSprint);
	PlayerInputComponent->BindAction(TEXT("Scan"), IE_Pressed, this, &AAstraeonPlayerCharacter::ScanEnvironment);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &AAstraeonPlayerCharacter::FireWeapon);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AAstraeonPlayerCharacter::Interact);
	PlayerInputComponent->BindAction(TEXT("Craft"), IE_Pressed, this, &AAstraeonPlayerCharacter::CraftSignalResonator);
	PlayerInputComponent->BindAction(TEXT("Slot1"), IE_Pressed, this, &AAstraeonPlayerCharacter::SelectSlotOne);
	PlayerInputComponent->BindAction(TEXT("Slot2"), IE_Pressed, this, &AAstraeonPlayerCharacter::SelectSlotTwo);
	PlayerInputComponent->BindAction(TEXT("Slot3"), IE_Pressed, this, &AAstraeonPlayerCharacter::SelectSlotThree);
	PlayerInputComponent->BindAction(TEXT("Slot4"), IE_Pressed, this, &AAstraeonPlayerCharacter::SelectSlotFour);
	PlayerInputComponent->BindAction(TEXT("Slot5"), IE_Pressed, this, &AAstraeonPlayerCharacter::SelectSlotFive);
	PlayerInputComponent->BindAction(TEXT("Slot6"), IE_Pressed, this, &AAstraeonPlayerCharacter::SelectSlotSix);
	PlayerInputComponent->BindAction(TEXT("UseHandItem"), IE_Pressed, this, &AAstraeonPlayerCharacter::UseHandItem);
	PlayerInputComponent->BindAction(TEXT("CycleProtection"), IE_Pressed, this, &AAstraeonPlayerCharacter::CycleProtectionModule);
	PlayerInputComponent->BindAction(TEXT("ToggleCameraView"), IE_Pressed, this, &AAstraeonPlayerCharacter::ToggleCameraView);
	PlayerInputComponent->BindAction(TEXT("ToggleBuildMode"), IE_Pressed, this, &AAstraeonPlayerCharacter::ToggleBuildMode);
	PlayerInputComponent->BindAction(TEXT("CycleStructure"), IE_Pressed, this, &AAstraeonPlayerCharacter::CycleStructureType);
	PlayerInputComponent->BindAction(TEXT("RotateStructure"), IE_Pressed, this, &AAstraeonPlayerCharacter::RotateStructure);
}

void AAstraeonPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Whatever the level places the character on at spawn (PlayerStart) counts as the
	// first known-safe ground location for the fall-rescue safety net.
	MarkLocationAsSafeGround(GetActorLocation());

	// Permite arrancar en tercera persona desde línea de comandos para diagnosticar sin
	// depender de que alguien pulse la tecla.
	if (FParse::Param(FCommandLine::Get(), TEXT("AstraeonStartThirdPerson")))
	{
		bThirdPersonView = true;
	}
	ApplyCameraView();
	LogCameraState();

	// `-AstraeonCameraShot` pide una captura unos segundos después de arrancar. Es la única
	// forma de comprobar qué se dibuja de verdad: los reportes de estado dicen que la malla
	// está visible y bien colocada, y aun así no se veía.
	if (FParse::Param(FCommandLine::Get(), TEXT("AstraeonCameraShot")))
	{
		if (UWorld* CurrentWorld = GetWorld())
		{
			// Sin partida iniciada la vista es la del menú, no la del personaje: la captura
			// no diría nada. Se arranca una para que haya realmente algo que mirar.
			FTimerHandle StartTimer;
			CurrentWorld->GetTimerManager().SetTimer(StartTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (AAstraeonPlayerController* Controller = Cast<AAstraeonPlayerController>(GetController()))
				{
					Controller->StartSelectedNewGame();
					UE_LOG(LogTemp, Display, TEXT("AstraeonCamera: partida iniciada para la captura"));
				}
			}), 1.5f, false);

			FTimerHandle ShotTimer;
			CurrentWorld->GetTimerManager().SetTimer(ShotTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				// A/B para separar "no se dibuja" de "se dibuja con un material que no se ve":
				// con este parámetro el cuerpo usa un material del motor, conocido y visible.
				if (FParse::Param(FCommandLine::Get(), TEXT("AstraeonBasicBodyMaterial")))
				{
					if (USkeletalMeshComponent* BodyMesh = GetMesh())
					{
						if (UMaterialInterface* Fallback = LoadObject<UMaterialInterface>(
							nullptr, TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial")))
						{
							for (int32 Index = 0; Index < BodyMesh->GetNumMaterials(); ++Index)
							{
								BodyMesh->SetMaterial(Index, Fallback);
							}
							UE_LOG(LogTemp, Display, TEXT("AstraeonCamera: cuerpo forzado a WorldGridMaterial"));
						}
					}
				}
				LogCameraState();
				FScreenshotRequest::RequestScreenshot(TEXT("AstraeonCameraShot"), false, false);
			}), 12.0f, false);
			// Verifica la misma entrada V que usa el jugador, en ambos sentidos.
			for (float ToggleTime : { 13.0f, 17.0f })
			{
				FTimerHandle ToggleTimer;
				CurrentWorld->GetTimerManager().SetTimer(ToggleTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
				{
					const bool bPreviousView = bThirdPersonView;
					if (APlayerController* PC = Cast<APlayerController>(GetController()))
					{
						PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::V, IE_Pressed, 1.0f));
						PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::V, IE_Released, 0.0f));
					}
					// InputKey encola la tecla; evaluar después de ProcessPlayerInput.
					FTimerHandle VerifyTimer;
					GetWorld()->GetTimerManager().SetTimer(VerifyTimer, FTimerDelegate::CreateWeakLambda(this, [this, bPreviousView]()
					{
						const float HeadHeight = GetMesh()->GetSocketLocation(TEXT("head")).Z - GetMesh()->GetSocketLocation(TEXT("root")).Z;
						const bool bPassed = bPreviousView != bThirdPersonView && HeadHeight > 140.0f && HeadHeight < 195.0f
							&& FirstPersonRig->IsVisible() == !bThirdPersonView && GetMesh()->bOwnerNoSee == !bThirdPersonView;
						UE_LOG(LogTemp, Display, TEXT("AstraeonCharacterViewSmoke: Passed=%s ThirdPerson=%d HeadHeightCm=%.2f"),
							bPassed ? TEXT("true") : TEXT("false"), bThirdPersonView, HeadHeight);
						if (!bPassed) FPlatformMisc::RequestExitWithStatus(false, 1);
					}), 0.25f, false);
				}), ToggleTime, false);
			}
			FTimerHandle AlternateShotTimer;
			CurrentWorld->GetTimerManager().SetTimer(AlternateShotTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				FScreenshotRequest::RequestScreenshot(TEXT("AstraeonCameraShot_Alternate"), false, false);
			}), 15.0f, false);
			FTimerHandle ExitTimer;
			CurrentWorld->GetTimerManager().SetTimer(ExitTimer, FTimerDelegate::CreateWeakLambda(this, []()
			{
				FPlatformMisc::RequestExit(false);
			}), 19.0f, false);
		}
	}

	if (FirstPersonRig)
	{
		FirstPersonRig->SetShadowBodyMesh(GetMesh());
	}
}

void AAstraeonPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// El rig se anima por frame (lo hace su propio componente); esta lógica no lo necesita
	// y mantiene la cadencia de 0,2 s que tenía cuando el Tick del actor iba a esa tasa.
	GameplayTickAccumulatorSeconds += DeltaSeconds;
	if (GameplayTickAccumulatorSeconds < 0.2f)
	{
		return;
	}
	GameplayTickAccumulatorSeconds = 0.0f;

	LogDiagnosticState();
	RescueFromVoidIfNeeded();
	CloseFabricatorIfOutOfReach();
	UpdateHavenAndRecall();
	UpdateBuildPreview();
}

void AAstraeonPlayerCharacter::UpdateHavenAndRecall()
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	if (!AstraeonGameInstance || !SuitComponent || !AstraeonGameInstance->HasStartedGame())
	{
		return;
	}

	// El footprint se evalúa en espacio local de Ítaca porque la nave se mueve.
	const FVector ItacaOrigin = AstraeonGameInstance->GetItacaOriginCm();
	const FVector ActorLocation = GetActorLocation();
	const FVector2D LocalXY(ActorLocation.X - ItacaOrigin.X, ActorLocation.Y - ItacaOrigin.Y);
	SuitComponent->SetInHaven(AAstraeonItacaInterior::IsInsideFootprint(LocalXY));

	if (!SuitComponent->IsIncapacitated())
	{
		return;
	}

	AstraeonGameInstance->RecordEmergencyRecall();
	SuitComponent->RestoreAfterRecall();

	const FVector RecallLocation = ItacaOrigin + FVector(220.0f, 0.0f, 110.0f);
	SetActorLocation(RecallLocation, false, nullptr, ETeleportType::TeleportPhysics);
	MarkLocationAsSafeGround(RecallLocation);
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->SetMovementMode(MOVE_Walking);
	}
}

void AAstraeonPlayerCharacter::ToggleCameraView()
{
	bThirdPersonView = !bThirdPersonView;
	ApplyCameraView();
	LogCameraState();
}

void AAstraeonPlayerCharacter::LogCameraState() const
{
	for (const USkeletalMeshComponent* Part : { GetMesh(), static_cast<USkeletalMeshComponent*>(FirstPersonRig) })
	{
		if (!Part) continue;
		UE_LOG(LogTemp, Display, TEXT("AstraeonPose: %s actorHidden=%d mainPass=%d rendered=%d bones=%d"),
			*Part->GetName(), IsHidden(), Part->bRenderInMainPass, Part->WasRecentlyRendered(), Part->GetNumBones());
		for (const TCHAR* Bone : { TEXT("root"), TEXT("head"), TEXT("hand_r"), TEXT("pelvis") })
		{
			const FTransform Transform = Part->GetSocketTransform(FName(Bone));
			UE_LOG(LogTemp, Display, TEXT("AstraeonPose: %s %s %s"), *Part->GetName(), Bone, *Transform.ToHumanReadableString());
		}
	}
	// Diagnóstico de una línea por cada cosa que puede dejar al personaje invisible: el
	// brazo colapsado por colisión mete la cámara dentro de la propia malla, y las banderas
	// de visibilidad la sacan del render aunque la cámara esté bien.
	const USkeletalMeshComponent* BodyMesh = GetMesh();
	const float RequestedArm = ThirdPersonBoom ? ThirdPersonBoom->TargetArmLength : 0.0f;
	float ActualArm = 0.0f;
	if (ThirdPersonBoom && ThirdPersonCamera)
	{
		ActualArm = FVector::Dist(ThirdPersonBoom->GetComponentLocation(),
			ThirdPersonCamera->GetComponentLocation());
	}
	UE_LOG(LogTemp, Display, TEXT("AstraeonCamera: vista=%s brazo_pedido=%.0f brazo_real=%.0f"),
		bThirdPersonView ? TEXT("tercera") : TEXT("primera"), RequestedArm, ActualArm);
	if (BodyMesh)
	{
		const FBoxSphereBounds Bounds = BodyMesh->Bounds;
		UE_LOG(LogTemp, Display,
			TEXT("AstraeonCamera: cuerpo malla=%s oculto=%d ownerNoSee=%d visible=%d origen=(%.0f,%.0f,%.0f) extension=(%.0f,%.0f,%.0f)"),
			BodyMesh->GetSkeletalMeshAsset() ? *BodyMesh->GetSkeletalMeshAsset()->GetName() : TEXT("NINGUNA"),
			BodyMesh->bHiddenInGame ? 1 : 0, BodyMesh->bOwnerNoSee ? 1 : 0,
			BodyMesh->IsVisible() ? 1 : 0,
			Bounds.Origin.X, Bounds.Origin.Y, Bounds.Origin.Z,
			Bounds.BoxExtent.X, Bounds.BoxExtent.Y, Bounds.BoxExtent.Z);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AstraeonCamera: el personaje no tiene componente de malla"));
	}
	if (BodyMesh)
	{
		const FVector BodyLocation = BodyMesh->GetComponentLocation();
		const FVector CameraLocation = ThirdPersonCamera ? ThirdPersonCamera->GetComponentLocation() : FVector::ZeroVector;
		UE_LOG(LogTemp, Display,
			TEXT("AstraeonCamera: cuerpo en (%.0f,%.0f,%.0f) camara en (%.0f,%.0f,%.0f) distancia=%.0f registrado=%d"),
			BodyLocation.X, BodyLocation.Y, BodyLocation.Z,
			CameraLocation.X, CameraLocation.Y, CameraLocation.Z,
			FVector::Dist(BodyLocation, CameraLocation), BodyMesh->IsRegistered() ? 1 : 0);
		const int32 MaterialCount = BodyMesh->GetNumMaterials();
		UE_LOG(LogTemp, Display, TEXT("AstraeonCamera: materiales=%d"), MaterialCount);
		for (int32 Index = 0; Index < MaterialCount; ++Index)
		{
			const UMaterialInterface* Material = BodyMesh->GetMaterial(Index);
			UE_LOG(LogTemp, Display, TEXT("AstraeonCamera:   slot %d = %s"), Index,
				Material ? *Material->GetPathName() : TEXT("NULO"));
		}
		if (const UAnimSingleNodeInstance* Single = BodyMesh->GetSingleNodeInstance())
		{
			const UAnimationAsset* Asset = Single->GetAnimationAsset();
			UE_LOG(LogTemp, Display, TEXT("AstraeonCamera: animacion=%s"),
				Asset ? *Asset->GetName() : TEXT("NINGUNA"));
		}
		else
		{
			UE_LOG(LogTemp, Display, TEXT("AstraeonCamera: sin instancia de animacion (pose de referencia)"));
		}
	}
}

#if !UE_BUILD_SHIPPING
// Permite forzar la vista desde `-ExecCmds` para diagnosticar sin depender de que alguien
// pulse la tecla.
static FAutoConsoleCommandWithWorld GAstraeonToggleCameraCommand(
	TEXT("Astraeon.ToggleCamera"),
	TEXT("Alterna primera/tercera persona en el personaje local y registra el estado."),
	FConsoleCommandWithWorldDelegate::CreateStatic([](UWorld* World)
	{
		if (!World)
		{
			return;
		}
		if (APlayerController* Controller = World->GetFirstPlayerController())
		{
			if (AAstraeonPlayerCharacter* Character = Cast<AAstraeonPlayerCharacter>(Controller->GetPawn()))
			{
				Character->ToggleCameraView();
				return;
			}
			UE_LOG(LogTemp, Warning, TEXT("AstraeonCamera: el pawn poseído no es AstraeonPlayerCharacter"));
		}
	}));
#endif

void AAstraeonPlayerCharacter::ApplyCameraView()
{
	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetActive(!bThirdPersonView);
	}
	if (ThirdPersonCamera)
	{
		ThirdPersonCamera->SetActive(bThirdPersonView);
	}
	// En primera persona el cuerpo se oculta al propio jugador para no quedar dentro de la
	// malla; en tercera es justamente lo que se quiere ver.
	if (USkeletalMeshComponent* BodyMesh = GetMesh())
	{
		BodyMesh->SetOwnerNoSee(bThirdPersonView ? false : true);
	}
	for (USkeletalMeshComponent* Part : BodyEquipment)
	{
		Part->SetOwnerNoSee(!bThirdPersonView);
	}
	// Las manos están pegadas a la cámara de primera persona: en tercera flotarían delante
	// del encuadre.
	if (FirstPersonRig)
	{
		FirstPersonRig->SetFirstPersonVisible(!bThirdPersonView);
	}
}

void AAstraeonPlayerCharacter::ToggleBuildMode()
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	if (!AstraeonGameInstance || !AstraeonGameInstance->HasStartedGame())
	{
		return;
	}

	if (!bBuildModeActive && !AstraeonGameInstance->IsHolding(UAstraeonGameInstance::GetBuildHammerItemId()))
	{
		AstraeonGameInstance->SetLastFeedbackMessage(AstraeonGameInstance->HasBuildHammer()
			? TEXT("Lleva el martillo de obra en la mano para construir (teclas 1-6).")
			: TEXT("Necesitas el martillo de obra. Fabrícalo en la mesa de Ítaca."));
		return;
	}

	bBuildModeActive = !bBuildModeActive;

	// El tick normal corre a 0,2 s, suficiente para supervivencia pero no para un fantasma
	// que sigue la mira: en modo construcción se pasa a cada frame.
	PrimaryActorTick.TickInterval = bBuildModeActive ? 0.0f : 0.2f;

	if (!bBuildModeActive && BuildPreviewMesh)
	{
		BuildPreviewMesh->SetVisibility(false);
	}

	AstraeonGameInstance->SetLastFeedbackMessage(bBuildModeActive
		? TEXT("Modo construcción. Q cambia pieza, R rota, Click izq. coloca, Click der. demuele, B sale.")
		: TEXT("Modo construcción cerrado."));
}

void AAstraeonPlayerCharacter::CycleStructureType()
{
	if (!bBuildModeActive)
	{
		return;
	}

	switch (SelectedStructureType)
	{
	case EAstraeonStructureType::Wall:
		SelectedStructureType = EAstraeonStructureType::Floor;
		break;
	case EAstraeonStructureType::Floor:
		SelectedStructureType = EAstraeonStructureType::Pillar;
		break;
	default:
		SelectedStructureType = EAstraeonStructureType::Wall;
		break;
	}
}

void AAstraeonPlayerCharacter::RotateStructure()
{
	if (!bBuildModeActive)
	{
		return;
	}

	// Pasos de 45°: suficientes para alinear piezas entre sí sin exigir puntería fina.
	BuildYawDegrees = FMath::Fmod(BuildYawDegrees + 45.0f, 360.0f);
}

bool AAstraeonPlayerCharacter::TraceBuildSurface(FVector& OutLocationCm) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FHitResult SurfaceHit;
	const FVector TraceStart = FirstPersonCamera ? FirstPersonCamera->GetComponentLocation() : GetActorLocation();
	const FVector TraceEnd = TraceStart + GetControlRotation().Vector() * AstraeonPlayerCharacterBuilding::PlacementRangeCm;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AstraeonBuildSurface), false, this);
	if (!World->LineTraceSingleByChannel(SurfaceHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return false;
	}

	// La pieza se apoya sobre la superficie golpeada: se sube media altura para que quede
	// encima y no medio hundida.
	const FVector SizeCm = AAstraeonBuiltStructure::GetStructureSizeCm(SelectedStructureType);
	OutLocationCm = SurfaceHit.ImpactPoint + FVector(0.0f, 0.0f, SizeCm.Z * 0.5f);
	return true;
}

void AAstraeonPlayerCharacter::UpdateBuildPreview()
{
	if (!BuildPreviewMesh)
	{
		return;
	}

	const UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	if (!bBuildModeActive || !AstraeonGameInstance)
	{
		BuildPreviewMesh->SetVisibility(false);
		bPlacementValid = false;
		return;
	}

	FVector SurfaceLocation;
	const bool bHasSurface = TraceBuildSurface(SurfaceLocation);
	bPlacementValid = bHasSurface && AstraeonGameInstance->CanPlaceStructure(SelectedStructureType);

	if (!bHasSurface)
	{
		BuildPreviewMesh->SetVisibility(false);
		return;
	}

	PreviewLocationCm = SurfaceLocation;
	const FVector SizeCm = AAstraeonBuiltStructure::GetStructureSizeCm(SelectedStructureType);
	BuildPreviewMesh->SetVisibility(true);
	BuildPreviewMesh->SetWorldLocation(PreviewLocationCm);
	BuildPreviewMesh->SetWorldRotation(FRotator(0.0f, BuildYawDegrees, 0.0f));
	BuildPreviewMesh->SetWorldScale3D(SizeCm / 100.0f);

	// Verde si se puede colocar, rojo si falta material: la respuesta tiene que llegar
	// antes de gastar, no después.
	BuildPreviewMesh->SetVectorParameterValueOnMaterials(TEXT("Color"),
		bPlacementValid ? FVector(0.25f, 1.0f, 0.35f) : FVector(1.0f, 0.25f, 0.20f));
}

void AAstraeonPlayerCharacter::ConfirmPlacement()
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	UWorld* World = GetWorld();
	if (!bBuildModeActive || !AstraeonGameInstance || !World)
	{
		return;
	}

	FVector SurfaceLocation;
	if (!TraceBuildSurface(SurfaceLocation))
	{
		AstraeonGameInstance->SetLastFeedbackMessage(TEXT("Apunta a una superficie para construir."));
		return;
	}

	FAstraeonPlacedStructure Placement;
	Placement.Type = SelectedStructureType;
	Placement.LocationCm = SurfaceLocation;
	Placement.Rotation = FRotator(0.0f, BuildYawDegrees, 0.0f);

	if (!AstraeonGameInstance->PlaceStructure(Placement))
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AAstraeonBuiltStructure* Structure = World->SpawnActor<AAstraeonBuiltStructure>(
		AAstraeonBuiltStructure::StaticClass(), Placement.LocationCm, Placement.Rotation, SpawnParameters))
	{
		Structure->ApplyPlacement(Placement);
	}
}

void AAstraeonPlayerCharacter::DemolishAimedStructure()
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	UWorld* World = GetWorld();
	if (!AstraeonGameInstance || !World)
	{
		return;
	}

	FHitResult SurfaceHit;
	const FVector TraceStart = FirstPersonCamera ? FirstPersonCamera->GetComponentLocation() : GetActorLocation();
	const FVector TraceEnd = TraceStart + GetControlRotation().Vector() * AstraeonPlayerCharacterBuilding::PlacementRangeCm;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AstraeonDemolish), false, this);
	if (!World->LineTraceSingleByChannel(SurfaceHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return;
	}

	AAstraeonBuiltStructure* Structure = Cast<AAstraeonBuiltStructure>(SurfaceHit.GetActor());
	if (!Structure)
	{
		AstraeonGameInstance->SetLastFeedbackMessage(TEXT("Eso no es una construcción."));
		return;
	}

	if (AstraeonGameInstance->DemolishStructureAt(Structure->GetActorLocation()))
	{
		Structure->Destroy();
	}
}

void AAstraeonPlayerCharacter::CloseFabricatorIfOutOfReach()
{
	AAstraeonPlayerController* AstraeonPlayerController = Cast<AAstraeonPlayerController>(GetController());
	UWorld* World = GetWorld();
	if (!AstraeonPlayerController || !World || !AstraeonPlayerController->IsFabricatorOpen())
	{
		return;
	}

	TArray<AActor*> Markers;
	UGameplayStatics::GetAllActorsOfClass(World, AAstraeonRegionMarker::StaticClass(), Markers);
	for (const AActor* MarkerActor : Markers)
	{
		const AAstraeonRegionMarker* Marker = Cast<AAstraeonRegionMarker>(MarkerActor);
		if (Marker && Marker->GetMarkerId() == TEXT("itaca_fabricator"))
		{
			if (FVector::Dist(GetActorLocation(), Marker->GetActorLocation()) <= AstraeonPlayerCharacterInteraction::FabricatorReachCm)
			{
				return;
			}
			break;
		}
	}

	AstraeonPlayerController->SetFabricatorOpen(false);
	if (UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>())
	{
		AstraeonGameInstance->SetLastFeedbackMessage(TEXT("Te alejaste de la mesa de fabricación."));
	}
}

void AAstraeonPlayerCharacter::PrepareForShipFlight()
{
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}
}

void AAstraeonPlayerCharacter::RecoverFromShipFlight(const FVector& LandingLocationCm)
{
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);
	SetActorLocation(LandingLocationCm, false, nullptr, ETeleportType::TeleportPhysics);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->SetMovementMode(MOVE_Walking);
	}

	// Sin esto, el ancla de rescate seguiría apuntando al punto anterior al despegue.
	MarkLocationAsSafeGround(LandingLocationCm);
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

void AAstraeonPlayerCharacter::StartSprint()
{
	GetCharacterMovement()->MaxWalkSpeed = AstraeonPlayerCharacterMovement::SprintSpeedCms;
}

void AAstraeonPlayerCharacter::StopSprint()
{
	GetCharacterMovement()->MaxWalkSpeed = AstraeonPlayerCharacterMovement::WalkSpeedCms;
}

void AAstraeonPlayerCharacter::EquipProtectionModule(EAstraeonProtectionModule Protection)
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	if (!AstraeonGameInstance)
	{
		return;
	}

	// Las mismas teclas sirven para fabricar mientras la mesa está abierta y para equipar
	// cuando está cerrada: fabricar y ponerse el módulo comparten el 1/2/3 deliberadamente.
	const AAstraeonPlayerController* AstraeonPlayerController = Cast<AAstraeonPlayerController>(GetController());
	if (AstraeonPlayerController && AstraeonPlayerController->IsFabricatorOpen())
	{
		AstraeonGameInstance->CraftRecipe(UAstraeonGameInstance::GetProtectionItemId(Protection));
		return;
	}

	AstraeonGameInstance->EquipProtection(Protection);
}

void AAstraeonPlayerCharacter::EquipNoProtection()
{
	// Con la mesa abierta, 0 fabrica el resonador; cerrada, retira el módulo puesto.
	const AAstraeonPlayerController* AstraeonPlayerController = Cast<AAstraeonPlayerController>(GetController());
	if (AstraeonPlayerController && AstraeonPlayerController->IsFabricatorOpen())
	{
		if (UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>())
		{
			AstraeonGameInstance->CraftRecipe(TEXT("signal_resonator"));
		}
		return;
	}

	EquipProtectionModule(EAstraeonProtectionModule::None);
}

void AAstraeonPlayerCharacter::EquipRespirator()
{
	EquipProtectionModule(EAstraeonProtectionModule::Respirator);
}

void AAstraeonPlayerCharacter::EquipThermalShield()
{
	EquipProtectionModule(EAstraeonProtectionModule::ThermalShield);
}

void AAstraeonPlayerCharacter::EquipPressureSeal()
{
	EquipProtectionModule(EAstraeonProtectionModule::PressureSeal);
}

void AAstraeonPlayerCharacter::HandleNumberKey(int32 Index, FName FabricatorRecipeId)
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	const AAstraeonPlayerController* AstraeonPlayerController = Cast<AAstraeonPlayerController>(GetController());
	if (!AstraeonGameInstance)
	{
		return;
	}

	// Con la mesa abierta las teclas fabrican; cerrada, eligen la ranura de la mano. Así la
	// barra rápida no necesita robarle teclas al taller.
	if (AstraeonPlayerController && AstraeonPlayerController->IsFabricatorOpen())
	{
		if (!FabricatorRecipeId.IsNone())
		{
			AstraeonGameInstance->CraftRecipe(FabricatorRecipeId);
		}
		return;
	}

	if (!AstraeonGameInstance->SelectHotbarSlot(Index))
	{
		AstraeonGameInstance->SetLastFeedbackMessage(TEXT("No tienes nada en esa ranura."));
	}
}

void AAstraeonPlayerCharacter::SelectSlotOne() { HandleNumberKey(0, TEXT("module_respirator")); }
void AAstraeonPlayerCharacter::SelectSlotTwo() { HandleNumberKey(1, TEXT("module_thermal_shield")); }
void AAstraeonPlayerCharacter::SelectSlotThree() { HandleNumberKey(2, TEXT("module_pressure_seal")); }
void AAstraeonPlayerCharacter::SelectSlotFour() { HandleNumberKey(3, TEXT("weapon_pulse_cutter")); }
void AAstraeonPlayerCharacter::SelectSlotFive() { HandleNumberKey(4, TEXT("tool_core_drill")); }
void AAstraeonPlayerCharacter::SelectSlotSix() { HandleNumberKey(5, TEXT("ration_pack")); }

void AAstraeonPlayerCharacter::UseHandItem()
{
	if (UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>())
	{
		const FName HeldItemId = AstraeonGameInstance->GetHandItemId();
		if (HeldItemId == UAstraeonGameInstance::GetRationItemId())
		{
			PlayHandGesture(EAstraeonHandGesture::Consume);
		}
		else if (HeldItemId == TEXT("signal_resonator"))
		{
			PlayHandGesture(EAstraeonHandGesture::Present);
		}
		else if (!HeldItemId.IsNone())
		{
			PlayHandGesture(EAstraeonHandGesture::Grip);
		}

		AstraeonGameInstance->UseHandItem();
	}
}

void AAstraeonPlayerCharacter::PlayHandGesture(EAstraeonHandGesture Gesture)
{
	if (FirstPersonRig)
	{
		FirstPersonRig->PlayGesture(Gesture);
	}
}

void AAstraeonPlayerCharacter::CycleProtectionModule()
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	if (!AstraeonGameInstance)
	{
		return;
	}

	// La protección dejó las teclas numéricas a la barra rápida: ahora se cicla entre los
	// módulos que se hayan fabricado.
	const EAstraeonProtectionModule Order[] = {
		EAstraeonProtectionModule::Respirator,
		EAstraeonProtectionModule::ThermalShield,
		EAstraeonProtectionModule::PressureSeal,
		EAstraeonProtectionModule::None
	};

	int32 CurrentIndex = 3;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Order); ++Index)
	{
		if (Order[Index] == AstraeonGameInstance->GetEquippedProtection())
		{
			CurrentIndex = Index;
			break;
		}
	}

	for (int32 Step = 1; Step <= UE_ARRAY_COUNT(Order); ++Step)
	{
		const EAstraeonProtectionModule Candidate = Order[(CurrentIndex + Step) % UE_ARRAY_COUNT(Order)];
		if (Candidate == EAstraeonProtectionModule::None
			|| AstraeonGameInstance->GetInventoryItemCount(UAstraeonGameInstance::GetProtectionItemId(Candidate)) > 0)
		{
			AstraeonGameInstance->EquipProtection(Candidate);
			return;
		}
	}
}

void AAstraeonPlayerCharacter::CraftAtFabricator(FName RecipeId)
{
	// Arma y taladro no se equipan: basta con llevarlos. Por eso estas teclas sólo hacen
	// algo con la mesa abierta.
	const AAstraeonPlayerController* AstraeonPlayerController = Cast<AAstraeonPlayerController>(GetController());
	if (!AstraeonPlayerController || !AstraeonPlayerController->IsFabricatorOpen())
	{
		return;
	}

	if (UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>())
	{
		AstraeonGameInstance->CraftRecipe(RecipeId);
	}
}

void AAstraeonPlayerCharacter::CraftPulseCutter()
{
	CraftAtFabricator(TEXT("weapon_pulse_cutter"));
}

void AAstraeonPlayerCharacter::CraftCoreDrill()
{
	CraftAtFabricator(TEXT("tool_core_drill"));
}

void AAstraeonPlayerCharacter::ScanEnvironment()
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	if (!AstraeonGameInstance)
	{
		return;
	}

	// En modo construcción el click izquierdo coloca en vez de escanear: es el gesto que
	// el jugador espera con una pieza fantasma delante.
	if (bBuildModeActive)
	{
		PlayHandGesture(EAstraeonHandGesture::Hammer);
		ConfirmPlacement();
		return;
	}

	PlayHandGesture(EAstraeonHandGesture::Scan);

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
		else if (const AAstraeonRegionMarker* Marker = Cast<AAstraeonRegionMarker>(HitResult.GetActor()))
		{
			bScanSucceeded = AstraeonGameInstance->RecordMarkerScan(
				Marker->GetMarkerId(),
				Marker->GetMarkerKind() == EAstraeonRegionActorKind::Resource);
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

void AAstraeonPlayerCharacter::FireWeapon()
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	UWorld* World = GetWorld();
	if (!AstraeonGameInstance || !World || !AstraeonGameInstance->HasStartedGame())
	{
		return;
	}

	// En modo construcción el click derecho demuele: martillo para levantar, maza para
	// derribar, sin cambiar de arma.
	if (bBuildModeActive)
	{
		PlayHandGesture(EAstraeonHandGesture::Maul);
		DemolishAimedStructure();
		return;
	}

	// Ya no alcanza con tenerla guardada: hay que llevarla en la mano. Eso obliga a elegir
	// qué se lleva y convierte el inventario en una decisión.
	if (!AstraeonGameInstance->IsHolding(UAstraeonGameInstance::GetPulseCutterItemId()))
	{
		AstraeonGameInstance->SetLastFeedbackMessage(AstraeonGameInstance->HasPulseCutter()
			? TEXT("Lleva la cortadora en la mano para disparar (teclas 1-6).")
			: TEXT("Sin arma. Fabrica la cortadora de pulso en la mesa de Ítaca."));
		return;
	}

	PlayHandGesture(EAstraeonHandGesture::Pulse);

	FHitResult HitResult;
	const FVector TraceStart = FirstPersonCamera ? FirstPersonCamera->GetComponentLocation() : GetActorLocation();
	const FVector TraceEnd = TraceStart + GetControlRotation().Vector() * AstraeonPlayerCharacterCombat::WeaponRangeCm;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AstraeonFire), false, this);
	if (!World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		AstraeonGameInstance->SetLastFeedbackMessage(TEXT("Disparo al vacío."));
		return;
	}

	AAstraeonCreatureActor* Creature = Cast<AAstraeonCreatureActor>(HitResult.GetActor());
	if (!Creature)
	{
		AstraeonGameInstance->SetLastFeedbackMessage(TEXT("El pulso no afecta a esa superficie."));
		return;
	}

	const FAstraeonCreatureProfile Profile = Creature->GetCreatureProfile();
	if (Creature->ApplyWeaponDamage(AstraeonPlayerCharacterCombat::WeaponDamage))
	{
		AstraeonGameInstance->RecordCreatureKill(Profile);
		// El cadáver se queda a la vista mientras cae; destruirlo en este mismo frame hacía
		// que matar fuese sólo una línea de texto.
		Creature->BeginDeathSequence();
		return;
	}

	AstraeonGameInstance->SetLastFeedbackMessage(FString::Printf(TEXT("Impacto en %s: %.0f%% de integridad restante."),
		*Profile.DisplayName.ToString(), Creature->GetHealthPercent()));
}

void AAstraeonPlayerCharacter::Interact()
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	if (!AstraeonGameInstance)
	{
		return;
	}

	// El taladro tiene su propio gesto, más largo: extraer es la única acción del juego con
	// duración implícita, y hasta ahora era instantánea y muda.
	PlayHandGesture(AstraeonGameInstance->IsHolding(TEXT("tool_core_drill"))
		? EAstraeonHandGesture::Drill
		: EAstraeonHandGesture::Interact);

	bool bSignalSourceAttempted = false;
	bool bInteractionSucceeded = false;
	bool bToolRequirementExplained = false;
	if (AAstraeonRegionMarker* Marker = FindFocusedRegionMarker())
	{
		bToolRequirementExplained = !Marker->GetRequiredToolId().IsNone()
			&& AstraeonGameInstance->GetInventoryItemCount(Marker->GetRequiredToolId()) <= 0;
		bInteractionSucceeded = InteractWithRegionMarker(*Marker, *AstraeonGameInstance, bSignalSourceAttempted);
	}
	else if (AAstraeonRegionMarker* NearbyMarker = FindBestRegionMarkerInReach(AstraeonPlayerCharacterInteraction::ProximityInteractionRadiusCm))
	{
		// Manual play should not require pixel-perfect aim at temporary cube markers.
		// The direct trace remains the preferred interaction, while this forgiving
		// fallback selects either a very close marker or the marker nearest to the
		// center-view ray, so the ESCOTILLA still works when the player is beside it
		// but looking a little above/around the small debug cube.
		bToolRequirementExplained = !NearbyMarker->GetRequiredToolId().IsNone()
			&& AstraeonGameInstance->GetInventoryItemCount(NearbyMarker->GetRequiredToolId()) <= 0;
		bInteractionSucceeded = InteractWithRegionMarker(*NearbyMarker, *AstraeonGameInstance, bSignalSourceAttempted);
	}

	// Sin nada interactuable al alcance pero con el taladro en mano y el suelo delante, E
	// extrae regolito: la materia prima de la construcción sale del propio terreno, sin
	// necesitar otra herramienta ni otro gesto.
	if (!bInteractionSucceeded && !bToolRequirementExplained
		&& AstraeonGameInstance->IsHolding(TEXT("tool_core_drill")))
	{
		FVector GroundLocation;
		if (TraceBuildSurface(GroundLocation) && AstraeonGameInstance->ExtractRegolith())
		{
			return;
		}
	}

	if (!bInteractionSucceeded && !bToolRequirementExplained)
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

AAstraeonRegionMarker* AAstraeonPlayerCharacter::FindBestRegionMarkerInReach(float RadiusCm) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> MarkerActors;
	UGameplayStatics::GetAllActorsOfClass(World, AAstraeonRegionMarker::StaticClass(), MarkerActors);

	const FVector ActorLocationCm = GetActorLocation();
	const FVector ViewLocationCm = FirstPersonCamera ? FirstPersonCamera->GetComponentLocation() : ActorLocationCm;
	const FVector ViewForward = GetControlRotation().Vector().GetSafeNormal();

	AAstraeonRegionMarker* BestAimedMarker = nullptr;
	float BestAimedScore = TNumericLimits<float>::Max();
	AAstraeonRegionMarker* BestCloseMarker = nullptr;
	float BestCloseDistanceSquared = FMath::Square(AstraeonPlayerCharacterInteraction::CloseInteractionRadiusCm);

	for (AActor* MarkerActor : MarkerActors)
	{
		AAstraeonRegionMarker* Marker = Cast<AAstraeonRegionMarker>(MarkerActor);
		if (!Marker || Marker->IsActorBeingDestroyed())
		{
			continue;
		}

		const FVector MarkerLocationCm = Marker->GetActorLocation();
		const float ActorDistanceSquared = FVector::DistSquared(ActorLocationCm, MarkerLocationCm);
		if (ActorDistanceSquared > FMath::Square(RadiusCm))
		{
			continue;
		}

		if (ActorDistanceSquared <= BestCloseDistanceSquared)
		{
			BestCloseDistanceSquared = ActorDistanceSquared;
			BestCloseMarker = Marker;
		}

		const FVector ViewToMarker = MarkerLocationCm - ViewLocationCm;
		const float DistanceAlongView = FVector::DotProduct(ViewToMarker, ViewForward);
		if (DistanceAlongView <= 0.0f || DistanceAlongView > AstraeonPlayerCharacterInteraction::InteractionTraceRangeCm)
		{
			continue;
		}

		const FVector ClosestPointOnViewRay = ViewLocationCm + ViewForward * DistanceAlongView;
		const float AimDistanceCm = FVector::Dist(MarkerLocationCm, ClosestPointOnViewRay);
		if (AimDistanceCm > AstraeonPlayerCharacterInteraction::AimForgivenessRadiusCm)
		{
			continue;
		}

		// Prefer the marker closest to where the player is looking, with a small
		// distance term so two similarly aimed markers choose the nearer interaction.
		const float ActorDistanceCm = FMath::Sqrt(ActorDistanceSquared);
		const float AimedScore = AimDistanceCm + ActorDistanceCm * 0.15f;
		if (AimedScore < BestAimedScore)
		{
			BestAimedScore = AimedScore;
			BestAimedMarker = Marker;
		}
	}

	return BestAimedMarker ? BestAimedMarker : BestCloseMarker;
}

bool AAstraeonPlayerCharacter::InteractWithRegionMarker(AAstraeonRegionMarker& Marker, UAstraeonGameInstance& AstraeonGameInstance, bool& bOutSignalSourceAttempted)
{
	if (Marker.GetMarkerKind() == EAstraeonRegionActorKind::Resource)
	{
		// Las vetas profundas se ven desde el principio, pero exigen herramienta: dan una
		// razón para volver a un sitio ya explorado en vez de agotarlo de una pasada.
		const FName RequiredToolId = Marker.GetRequiredToolId();
		if (!RequiredToolId.IsNone() && AstraeonGameInstance.GetInventoryItemCount(RequiredToolId) <= 0)
		{
			AstraeonGameInstance.SetLastFeedbackMessage(FString::Printf(
				TEXT("Veta profunda: necesitas %s. Fabrícalo en la mesa de Ítaca."), *RequiredToolId.ToString()));
			return false;
		}

		// El nodo declara cuánto rinde (3 para los básicos, 2 para el característico de la
		// seed); antes se ignoraba y toda veta entregaba una sola unidad.
		const FName CollectedId = Marker.GetMarkerId();
		const bool bCollected = AstraeonGameInstance.AddInventoryItem(CollectedId, AstraeonGameInstance.GetResourceNodeQuantity(CollectedId));
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
		// Desde afuera, desplegar a la superficie es redundante: el jugador ya está afuera.
		// La escotilla es una puerta, así que en ese sentido devuelve a la estancia.
		const FVector ItacaOrigin = AstraeonGameInstance.GetItacaOriginCm();
		const FVector2D LocalXY(GetActorLocation().X - ItacaOrigin.X, GetActorLocation().Y - ItacaOrigin.Y);
		if (!AAstraeonItacaInterior::IsInsideFootprint(LocalXY))
		{
			return EnterItacaThroughHatch(AstraeonGameInstance);
		}
		return DeployToSurface(AstraeonGameInstance);
	}

	if (Marker.GetMarkerId() == TEXT("itaca_fabricator"))
	{
		if (AAstraeonPlayerController* AstraeonPlayerController = Cast<AAstraeonPlayerController>(GetController()))
		{
			AstraeonPlayerController->SetFabricatorOpen(!AstraeonPlayerController->IsFabricatorOpen());
			AstraeonGameInstance.SetLastFeedbackMessage(AstraeonPlayerController->IsFabricatorOpen()
				? TEXT("Mesa de fabricación abierta. 1/2/3 fabrica, E cierra.")
				: TEXT("Mesa de fabricación cerrada."));
			return true;
		}
		return false;
	}

	if (Marker.GetMarkerId() == TEXT("itaca_pilot_console"))
	{
		AAstraeonPlayerController* AstraeonPlayerController = Cast<AAstraeonPlayerController>(GetController());
		return AstraeonPlayerController && AstraeonPlayerController->BeginShipFlight();
	}

	if (Marker.GetMarkerId() == TEXT("minor_geologic_anomaly"))
	{
		return AstraeonGameInstance.RecordAnomalyInspection();
	}

	return false;
}

bool AAstraeonPlayerCharacter::DeployToSurface(UAstraeonGameInstance& AstraeonGameInstance)
{
	if (!AstraeonGameInstance.HasStartedGame())
	{
		return false;
	}

	// Abrir la hoja antes de salir. Es el gesto que faltaba: hasta ahora la escotilla se
	// comportaba como una puerta permanentemente abierta.
	if (AActor* Interior = UGameplayStatics::GetActorOfClass(this, AAstraeonItacaInterior::StaticClass()))
	{
		Cast<AAstraeonItacaInterior>(Interior)->OpenHatch();
	}

	const FVector DeploymentLocationCm = UAstraeonRegionMaterializer::GetSurfaceDeploymentLocationCm(AstraeonGameInstance.GetItacaOriginCm());

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
	// Failed floor validation must never advance the logbook/progress.
	AstraeonGameInstance.RecordSurfaceDeployment();

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

bool AAstraeonPlayerCharacter::EnterItacaThroughHatch(UAstraeonGameInstance& AstraeonGameInstance)
{
	if (!AstraeonGameInstance.HasStartedGame())
	{
		return false;
	}

	// Justo del lado de adentro del umbral, no en el hueco: quedarse en el marco dejaba al
	// jugador dentro de la hoja cuando la escotilla se cierra.
	const FVector ItacaOrigin = AstraeonGameInstance.GetItacaOriginCm();
	const float CapsuleHalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 96.0f;
	SetActorLocation(ItacaOrigin + FVector(430.0f, 0.0f, CapsuleHalfHeight + 6.0f),
		false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->SetMovementMode(MOVE_Walking);
	}
	MarkLocationAsSafeGround(GetActorLocation());

	if (AActor* Interior = UGameplayStatics::GetActorOfClass(this, AAstraeonItacaInterior::StaticClass()))
	{
		Cast<AAstraeonItacaInterior>(Interior)->OpenHatch();
	}
	AstraeonGameInstance.SetLastFeedbackMessage(TEXT("De vuelta dentro de Ítaca."));
	return true;
}

void AAstraeonPlayerCharacter::LogDiagnosticState() const
{
	if (!AstraeonDiagnostics::IsEnabled())
	{
		return;
	}

	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	const FVector Location = GetActorLocation();
	const FVector ItacaOrigin = AstraeonGameInstance ? AstraeonGameInstance->GetItacaOriginCm() : FVector::ZeroVector;
	const FVector2D Local(Location.X - ItacaOrigin.X, Location.Y - ItacaOrigin.Y);
	const int32 WorldSeed = AstraeonGameInstance ? AstraeonGameInstance->GetCurrentTerrainSeed() : 0;

	UE_LOG(LogAstraeonDiag, Log,
		TEXT("PLAYER pos=(%.0f,%.0f,%.1f) vel=(%.0f,%.0f,%.0f) mode=%d onGround=%d ")
		TEXT("ctrlRot=(P%.1f Y%.1f R%.1f) actorYaw=%.1f itacaOrigin=(%.0f,%.0f,%.1f) ")
		TEXT("inRoom=%d terrainZ=%.1f safeZ=%.1f %s %s"),
		Location.X, Location.Y, Location.Z,
		MovementComponent ? MovementComponent->Velocity.X : 0.0f,
		MovementComponent ? MovementComponent->Velocity.Y : 0.0f,
		MovementComponent ? MovementComponent->Velocity.Z : 0.0f,
		MovementComponent ? static_cast<int32>(MovementComponent->MovementMode) : -1,
		MovementComponent && MovementComponent->IsMovingOnGround() ? 1 : 0,
		GetControlRotation().Pitch, GetControlRotation().Yaw, GetControlRotation().Roll,
		GetActorRotation().Yaw,
		ItacaOrigin.X, ItacaOrigin.Y, ItacaOrigin.Z,
		AAstraeonItacaInterior::IsInsideFootprint(Local) ? 1 : 0,
		AAstraeonTerrainField::GetGroundHeightCm(WorldSeed, Location.X, Location.Y),
		bHasSafeGroundLocation ? LastSafeGroundLocationCm.Z : -99999.0f,
		*AstraeonDiagnostics::DescribeGroundUnder(*this, 30000.0f),
		*AstraeonDiagnostics::DescribeBlockingOverlaps(*this,
			Capsule ? Capsule->GetScaledCapsuleRadius() : 42.0f,
			Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 96.0f));
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
		// Siempre, no sólo con -AstraeonDiag: un rescate es un síntoma, y sin registro no
		// se puede saber desde dónde se cayó el jugador.
		UE_LOG(LogAstraeonDiag, Warning, TEXT("RESCUE from=(%.0f,%.0f,%.1f) to=(%.0f,%.0f,%.1f) fell=%.0fcm %s"),
			GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z,
			LastSafeGroundLocationCm.X, LastSafeGroundLocationCm.Y, LastSafeGroundLocationCm.Z,
			LastSafeGroundLocationCm.Z - GetActorLocation().Z,
			*AstraeonDiagnostics::DescribeGroundUnder(*this, 60000.0f));

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
