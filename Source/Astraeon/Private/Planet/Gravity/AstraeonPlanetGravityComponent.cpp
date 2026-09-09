#include "Planet/Gravity/AstraeonPlanetGravityComponent.h"

#include "AstraeonDiagnostics.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Planet/Coordinates/AstraeonPlanetFrame.h"
#include "Planet/Gravity/AstraeonPlanetGravityHarness.h"

namespace AstraeonPlanetGravity
{
	constexpr float EarthGravityMS2 = 9.81f;

	// Cuánto tiene que cambiar la dirección de gravedad para reescribirla. Caminando sobre un
	// planeta de 500 km, el arriba local cambia muy despacio: sin este umbral se reescribiría
	// cada frame por ruido de coma flotante.
	constexpr double GravityDirectionEpsilon = 1.0e-4;
}

UAstraeonPlanetGravityComponent::UAstraeonPlanetGravityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// ANTES del movimiento, y con prioridad explícita sobre CharacterMovement (ver BeginPlay).
	//
	// La primera versión tickeaba en TG_PostPhysics, con el razonamiento de que alinear después
	// del movimiento usa la posición final del frame. Estaba mal: dejaba que CharacterMovement
	// resolviera el suelo con la orientación y la gravedad del frame ANTERIOR. En una superficie
	// curva eso se siente como resbalar sin control tras un salto, que es lo que se reportó en
	// la primera prueba manual. El desfase de un frame en la posición es invisible; el desfase en
	// la orientación con la que se busca el suelo, no.
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UAstraeonPlanetGravityComponent::BeginPlay()
{
	Super::BeginPlay();

	// El grupo de tick no basta para garantizar el orden dentro del mismo grupo. Esta
	// dependencia sí: CharacterMovement no corre hasta que la gravedad y la orientación de este
	// frame están puestas.
	if (ACharacter* Character = GetOwnerCharacter())
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->PrimaryComponentTick.AddPrerequisite(this, PrimaryComponentTick);
		}
	}

	TryBindHarness();
}

bool UAstraeonPlanetGravityComponent::TryBindHarness()
{
	AAstraeonPlanetGravityHarness* Harness = AAstraeonPlanetGravityHarness::FindActiveHarness(GetWorld());
	if (!Harness)
	{
		// Sin harness el mundo es el plano actual. No es un error: es el estado normal fuera de
		// los test levels planetarios, y el componente se queda callado.
		bActive = false;
		return false;
	}

	SetPlanetBody(Harness->GetPlanetCenterCm(), Harness->GetPlanetRadiusCm(), Harness->SurfaceGravityMS2);
	return true;
}

void UAstraeonPlanetGravityComponent::SetPlanetBody(const FVector& CenterCm, double RadiusCm, float SurfaceGravityMS2)
{
	PlanetCenterCm = CenterCm;
	PlanetRadiusCm = RadiusCm;
	PlanetSurfaceGravityMS2 = SurfaceGravityMS2;
	bActive = true;
	LastAppliedGravityDirection = FVector::ZeroVector;

	TakeOverPawnRotation();

	if (ACharacter* Character = GetOwnerCharacter())
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			// La magnitud sale del cuerpo; la dirección, del radio. Son datos distintos y se
			// fijan por separado a propósito.
			Movement->GravityScale = SurfaceGravityMS2 / AstraeonPlanetGravity::EarthGravityMS2;
		}
	}

	UE_LOG(LogAstraeonDiag, Log, TEXT("[Planet] Gravedad radial activa sobre centro=%s radio=%.0f cm"),
		*PlanetCenterCm.ToString(), PlanetRadiusCm);
}

void UAstraeonPlanetGravityComponent::ClearPlanetBody()
{
	RestorePawnRotation();
	bActive = false;
	LastAppliedGravityDirection = FVector::ZeroVector;

	if (ACharacter* Character = GetOwnerCharacter())
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->SetGravityDirection(UCharacterMovementComponent::DefaultGravityDirection);
		}
	}
}

void UAstraeonPlanetGravityComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	if (!bActive)
	{
		return;
	}

	ApplyGravityDirection();
	AlignOwnerToUp(DeltaSeconds);
}

void UAstraeonPlanetGravityComponent::ApplyGravityDirection()
{
	ACharacter* Character = GetOwnerCharacter();
	if (!Character)
	{
		return;
	}

	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	const FVector Desired = GetGravityDirection();
	if (FVector::DistSquared(Desired, LastAppliedGravityDirection)
		<= AstraeonPlanetGravity::GravityDirectionEpsilon * AstraeonPlanetGravity::GravityDirectionEpsilon)
	{
		return;
	}

	Movement->SetGravityDirection(Desired);
	LastAppliedGravityDirection = Desired;
}

void UAstraeonPlanetGravityComponent::AlignOwnerToUp(float DeltaSeconds)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const FVector Up = GetUpVector();

	// El frente se ARRASTRA al nuevo plano tangente en vez de deducirse de la rotación actual.
	// Deducirlo funcionaba mientras la rotación del actor fuera sólo nuestra; con el motor
	// escribiendo encima desde una FRotator de mundo, el frente heredaba ese vuelco.
	TransportedForward = FAstraeonPlanetFrame::TransportTangent(
		TransportedForward, Up, Owner->GetActorQuat().GetRightVector());

	const FQuat Target = FAstraeonPlanetFrame::MakeFrame(Up, TransportedForward);
	const FQuat Current = Owner->GetActorQuat();

	// La orientación se transporta, no se teletransporta: un salto instantáneo es correcto en
	// geometría y desagradable en pantalla.
	FQuat Next = Target;
	if (DeltaSeconds > 0.0f && AlignDegreesPerSecond > 0.0f)
	{
		const float MaxRadians = FMath::DegreesToRadians(AlignDegreesPerSecond) * DeltaSeconds;
		const float AngleToTarget = Current.AngularDistance(Target);
		if (AngleToTarget > MaxRadians && AngleToTarget > KINDA_SMALL_NUMBER)
		{
			Next = FQuat::Slerp(Current, Target, MaxRadians / AngleToTarget).GetNormalized();
		}
	}

	if (!Current.Equals(Next, KINDA_SMALL_NUMBER))
	{
		// Sin barrido: la alineación es un giro sobre el sitio y barrer contra el suelo al que la
		// cápsula ya está pegada la haría rebotar.
		Owner->SetActorRotation(Next, ETeleportType::None);
	}
}

void UAstraeonPlanetGravityComponent::AddYawInput(float DeltaDegrees)
{
	if (!bActive || FMath::IsNearlyZero(DeltaDegrees))
	{
		return;
	}

	TransportedForward = FAstraeonPlanetFrame::YawTangent(TransportedForward, GetUpVector(), DeltaDegrees);
}

void UAstraeonPlanetGravityComponent::AddPitchInput(float DeltaDegrees)
{
	if (!bActive)
	{
		return;
	}

	ViewPitchDegrees = FMath::Clamp(ViewPitchDegrees + DeltaDegrees, -MaxViewPitchDegrees, MaxViewPitchDegrees);
}

FVector UAstraeonPlanetGravityComponent::GetViewDirection() const
{
	if (!bActive)
	{
		const AActor* Owner = GetOwner();
		return Owner ? Owner->GetActorForwardVector() : FVector::ForwardVector;
	}

	const FVector Up = GetUpVector();
	const FVector Forward = FAstraeonPlanetFrame::ProjectToTangent(TransportedForward, Up).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(Up, Forward).GetSafeNormal();

	// Pitch positivo mira hacia arriba, igual que la convención de la cámara del proyecto.
	const FQuat Pitch(Right, FMath::DegreesToRadians(-ViewPitchDegrees));
	return Pitch.RotateVector(Forward).GetSafeNormal();
}

void UAstraeonPlanetGravityComponent::TakeOverPawnRotation()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || bHasSavedRotationFlags)
	{
		return;
	}

	bSavedUseControllerRotationYaw = Pawn->bUseControllerRotationYaw;
	bSavedUseControllerRotationPitch = Pawn->bUseControllerRotationPitch;
	bSavedUseControllerRotationRoll = Pawn->bUseControllerRotationRoll;
	bHasSavedRotationFlags = true;

	Pawn->bUseControllerRotationYaw = false;
	Pawn->bUseControllerRotationPitch = false;
	Pawn->bUseControllerRotationRoll = false;

	// El frente arranca desde donde el actor ya miraba, para que activar la gravedad planetaria
	// no gire al jugador de golpe.
	const FVector Up = GetUpVector();
	TransportedForward = FAstraeonPlanetFrame::TransportTangent(
		Pawn->GetActorForwardVector(), Up, Pawn->GetActorRightVector());
}

void UAstraeonPlanetGravityComponent::RestorePawnRotation()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !bHasSavedRotationFlags)
	{
		return;
	}

	Pawn->bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
	Pawn->bUseControllerRotationPitch = bSavedUseControllerRotationPitch;
	Pawn->bUseControllerRotationRoll = bSavedUseControllerRotationRoll;
	bHasSavedRotationFlags = false;
}

FVector UAstraeonPlanetGravityComponent::GetUpVector() const
{
	const AActor* Owner = GetOwner();
	if (!bActive || !Owner)
	{
		return FVector::UpVector;
	}

	return FAstraeonPlanetFrame::UpAt(PlanetCenterCm, Owner->GetActorLocation());
}

FVector UAstraeonPlanetGravityComponent::GetGravityDirection() const
{
	const AActor* Owner = GetOwner();
	if (!bActive || !Owner)
	{
		return UCharacterMovementComponent::DefaultGravityDirection;
	}

	return FAstraeonPlanetFrame::GravityDirectionAt(PlanetCenterCm, Owner->GetActorLocation());
}

double UAstraeonPlanetGravityComponent::GetAltitudeCm() const
{
	const AActor* Owner = GetOwner();
	if (!bActive || !Owner)
	{
		return 0.0;
	}

	return FAstraeonPlanetFrame::AltitudeCm(PlanetCenterCm, PlanetRadiusCm, Owner->GetActorLocation());
}

ACharacter* UAstraeonPlanetGravityComponent::GetOwnerCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}
