#include "Ship/AstraeonShipPawn.h"

#include "AstraeonPlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace AstraeonShip
{
	constexpr float MaxForwardSpeedCms = 3600.0f;
	constexpr float BoostMultiplier = 2.4f;
	constexpr float StrafeSpeedCms = 1600.0f;
	constexpr float LiftSpeedCms = 1400.0f;
	constexpr float AccelerationPerSecond = 2.6f;
	constexpr float DampingPerSecond = 1.8f;

	// Sin otros destinos registrados, subir más alto no lleva a ninguna parte: el techo
	// es narrativo antes que técnico y se comunica al jugador en el HUD.
	constexpr float AltitudeCeilingCm = 24000.0f;
	constexpr float CeilingWarningMarginCm = 2000.0f;

	// Aterrizar pide estar bajo y sin picada: evita "clavar" la nave contra el suelo.
	constexpr float LandingAltitudeCm = 900.0f;
	constexpr float LandingMaxSpeedCms = 900.0f;

	constexpr float MaxPitchDegrees = 55.0f;
	constexpr float GroundTraceDistanceCm = 200000.0f;

	// Montaje del exterior, en centímetros. Los valores vienen del bloque `assembly` de
	// Tools/Blender/configs/ship_blockout.json, que es donde se valida que el conjunto
	// entre en el volumen de estudio de 14 x 10 x 6 m y que los patines sostengan el casco.
	constexpr float HullMountZCm = 45.0f;
	constexpr float EngineMountXCm = -340.0f;
	constexpr float EngineMountYCm = 425.0f;
	constexpr float EngineMountZCm = 115.0f;
	constexpr float GearMountFrontXCm = 430.0f;
	constexpr float GearMountRearXCm = -360.0f;
	constexpr float GearMountYCm = 290.0f;
	constexpr float AntennaMountXCm = -160.0f;
	constexpr float AntennaMountZCm = 432.0f;

	UStaticMesh* LoadModule(const TCHAR* Name)
	{
		const FString Reference = FString::Printf(
			TEXT("/Game/Astraeon/Art/Blockouts/Itaca/%s.%s"), Name, Name);
		ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(*Reference);
		return Finder.Succeeded() ? Finder.Object : nullptr;
	}
}

AAstraeonShipPawn::AAstraeonShipPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// El origen del actor es el punto de apoyo de los patines: la cota que traza el vuelo
	// contra el terreno es directamente la altura a la que aterriza la nave.
	HullMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HullMesh"));
	RootComponent = HullMesh;
	HullMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HullMesh->SetRelativeLocation(FVector(0.0f, 0.0f, AstraeonShip::HullMountZCm));
	HullMesh->SetStaticMesh(AstraeonShip::LoadModule(TEXT("SM_Itaca_Hull_Blockout")));

	// Módulos montados como piezas separadas, no como un casco monolítico: las toberas y
	// los patines son los anclajes naturales de los VFX de empuje y polvo que faltan.
	UStaticMesh* const EngineMesh = AstraeonShip::LoadModule(TEXT("SM_Itaca_Engine_Blockout"));
	UStaticMesh* const GearMesh = AstraeonShip::LoadModule(TEXT("SM_Itaca_LandingGear_Blockout"));
	int32 GearIndex = 0;
	for (const int32 Side : {-1, 1})
	{
		UStaticMeshComponent* Engine = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("EngineMesh_%d"), Side < 0 ? 0 : 1));
		Engine->SetupAttachment(HullMesh);
		Engine->SetStaticMesh(EngineMesh);
		Engine->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Engine->SetRelativeLocation(FVector(AstraeonShip::EngineMountXCm,
			Side * AstraeonShip::EngineMountYCm, AstraeonShip::EngineMountZCm));
		HullModules.Add(Engine);

		for (const float MountX : {AstraeonShip::GearMountFrontXCm, AstraeonShip::GearMountRearXCm})
		{
			UStaticMeshComponent* Gear = CreateDefaultSubobject<UStaticMeshComponent>(
				*FString::Printf(TEXT("LandingGearMesh_%d"), GearIndex++));
			Gear->SetupAttachment(HullMesh);
			Gear->SetStaticMesh(GearMesh);
			Gear->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			// Los patines llegan al suelo, así que cuelgan del origen del actor, no del casco.
			Gear->SetRelativeLocation(FVector(MountX, Side * AstraeonShip::GearMountYCm,
				-AstraeonShip::HullMountZCm));
			HullModules.Add(Gear);
		}
	}

	UStaticMeshComponent* Antenna = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AntennaMesh"));
	Antenna->SetupAttachment(HullMesh);
	Antenna->SetStaticMesh(AstraeonShip::LoadModule(TEXT("SM_Itaca_Antenna_Blockout")));
	Antenna->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Antenna->SetRelativeLocation(FVector(AstraeonShip::AntennaMountXCm, 0.0f, AstraeonShip::AntennaMountZCm));
	HullModules.Add(Antenna);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	// El casco pasó de 9 m de cubo a 13 m con motores y antena: con el brazo anterior la
	// cámara quedaba dentro de la nave.
	CameraBoom->TargetArmLength = 2600.0f;
	CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 620.0f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bInheritPitch = true;
	CameraBoom->bInheritYaw = true;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 6.0f;

	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(CameraBoom);
}

void AAstraeonShipPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AAstraeonShipPawn::ApplyThrottle);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AAstraeonShipPawn::ApplyStrafe);
	PlayerInputComponent->BindAxis(TEXT("ShipLift"), this, &AAstraeonShipPawn::ApplyLift);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AAstraeonShipPawn::StartBoost);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AAstraeonShipPawn::StopBoost);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AAstraeonShipPawn::RequestLanding);
}

void AAstraeonShipPawn::ApplyThrottle(float Value)
{
	ThrottleInput = FMath::Clamp(Value, -1.0f, 1.0f);
}

void AAstraeonShipPawn::ApplyStrafe(float Value)
{
	StrafeInput = FMath::Clamp(Value, -1.0f, 1.0f);
}

void AAstraeonShipPawn::ApplyLift(float Value)
{
	LiftInput = FMath::Clamp(Value, -1.0f, 1.0f);
}

void AAstraeonShipPawn::StartBoost()
{
	bBoosting = true;
}

void AAstraeonShipPawn::StopBoost()
{
	bBoosting = false;
}

void AAstraeonShipPawn::RequestLanding()
{
	if (AAstraeonPlayerController* AstraeonPlayerController = Cast<AAstraeonPlayerController>(GetController()))
	{
		AstraeonPlayerController->RequestShipLanding();
	}
}

void AAstraeonShipPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (DeltaSeconds <= 0.0f)
	{
		return;
	}

	// La nave apunta hacia donde mira el jugador, con el cabeceo limitado para que no
	// termine invertida: es una nave de carga, no un caza.
	if (const AController* OwningController = GetController())
	{
		const FRotator ControlRotation = OwningController->GetControlRotation();
		SetActorRotation(FRotator(
			FMath::ClampAngle(ControlRotation.Pitch, -AstraeonShip::MaxPitchDegrees, AstraeonShip::MaxPitchDegrees),
			ControlRotation.Yaw,
			0.0f));
	}

	const float MaxForwardSpeed = AstraeonShip::MaxForwardSpeedCms * (bBoosting ? AstraeonShip::BoostMultiplier : 1.0f);
	const FVector TargetVelocity =
		GetActorForwardVector() * (ThrottleInput * MaxForwardSpeed)
		+ GetActorRightVector() * (StrafeInput * AstraeonShip::StrafeSpeedCms)
		+ FVector::UpVector * (LiftInput * AstraeonShip::LiftSpeedCms);

	const float BlendRate = TargetVelocity.IsNearlyZero() ? AstraeonShip::DampingPerSecond : AstraeonShip::AccelerationPerSecond;
	VelocityCms = FMath::VInterpTo(VelocityCms, TargetVelocity, DeltaSeconds, BlendRate);

	FVector NextLocation = GetActorLocation() + VelocityCms * DeltaSeconds;

	float GroundZ = 0.0f;
	if (TraceGroundZ(GroundZ))
	{
		const float CeilingZ = GroundZ + AstraeonShip::AltitudeCeilingCm;
		if (NextLocation.Z > CeilingZ)
		{
			NextLocation.Z = CeilingZ;
			VelocityCms.Z = FMath::Min(VelocityCms.Z, 0.0f);
		}

		// Suelo blando: la nave se frena antes de atravesar el terreno en vez de chocar.
		const float MinimumZ = GroundZ + 260.0f;
		if (NextLocation.Z < MinimumZ)
		{
			NextLocation.Z = MinimumZ;
			VelocityCms.Z = FMath::Max(VelocityCms.Z, 0.0f);
		}
	}

	SetActorLocation(NextLocation);
}

bool AAstraeonShipPawn::TraceGroundZ(float& OutGroundZ) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AstraeonShipGround), false, this);
	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, AstraeonShip::GroundTraceDistanceCm);
	if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		OutGroundZ = GroundHit.ImpactPoint.Z;
		return true;
	}

	return false;
}

float AAstraeonShipPawn::GetAltitudeCm() const
{
	float GroundZ = 0.0f;
	return TraceGroundZ(GroundZ) ? GetActorLocation().Z - GroundZ : -1.0f;
}

float AAstraeonShipPawn::GetSpeedKmH() const
{
	return VelocityCms.Size() * 0.036f;
}

bool AAstraeonShipPawn::IsAtAltitudeCeiling() const
{
	const float Altitude = GetAltitudeCm();
	return Altitude >= 0.0f && Altitude >= AstraeonShip::AltitudeCeilingCm - AstraeonShip::CeilingWarningMarginCm;
}

bool AAstraeonShipPawn::CanLandHere() const
{
	const float Altitude = GetAltitudeCm();
	return Altitude >= 0.0f
		&& Altitude <= AstraeonShip::LandingAltitudeCm
		&& VelocityCms.Size() <= AstraeonShip::LandingMaxSpeedCms;
}

float AAstraeonShipPawn::GetAltitudeCeilingCm()
{
	return AstraeonShip::AltitudeCeilingCm;
}
