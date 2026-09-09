#include "Planet/Gravity/AstraeonPlanetGravityComponent.h"

#include "AstraeonDiagnostics.h"
#include "GameFramework/Character.h"
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
	// Después del movimiento: alinear la cápsula antes de que el movimiento se haya resuelto
	// orientaría contra una posición que ya no es la del frame.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UAstraeonPlanetGravityComponent::BeginPlay()
{
	Super::BeginPlay();
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
	const FQuat Current = Owner->GetActorQuat();
	const FQuat Aligned = FAstraeonPlanetFrame::AlignToUpInterpolated(Current, Up, DeltaSeconds, AlignDegreesPerSecond);

	if (!Current.Equals(Aligned, KINDA_SMALL_NUMBER))
	{
		// Sin barrido: la alineación es un giro sobre el sitio y barrer contra el suelo al que la
		// cápsula ya está pegada la haría rebotar.
		Owner->SetActorRotation(Aligned, ETeleportType::None);
	}
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
