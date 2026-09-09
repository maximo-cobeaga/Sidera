#include "Planet/Coordinates/AstraeonPlanetFrame.h"

namespace AstraeonPlanetFrameLocal
{
	// Por debajo de esto la proyección tangente del frente no define una dirección: el frente
	// mira casi exactamente hacia arriba o hacia abajo. Ocurre en los polos y al mirar a plomo.
	constexpr double MinTangentLength = 1.0e-3;

	// Arriba de referencia cuando no hay dirección radial. Sólo se alcanza en el centro exacto
	// del cuerpo, que no es una posición jugable; existe para que la función sea total.
	const FVector FallbackUp = FVector::UpVector;
}

FVector FAstraeonPlanetFrame::UpAt(const FVector& PlanetCenterCm, const FVector& LocationCm)
{
	const FVector Radial = LocationCm - PlanetCenterCm;
	if (Radial.Size() < DegenerateRadiusCm)
	{
		return AstraeonPlanetFrameLocal::FallbackUp;
	}

	return Radial.GetSafeNormal();
}

FVector FAstraeonPlanetFrame::GravityDirectionAt(const FVector& PlanetCenterCm, const FVector& LocationCm)
{
	return -UpAt(PlanetCenterCm, LocationCm);
}

double FAstraeonPlanetFrame::AltitudeCm(const FVector& PlanetCenterCm, double PlanetRadiusCm, const FVector& LocationCm)
{
	return (LocationCm - PlanetCenterCm).Size() - PlanetRadiusCm;
}

FVector FAstraeonPlanetFrame::ProjectToTangent(const FVector& VectorCm, const FVector& UpUnit)
{
	// Idéntico a FVector::VectorPlaneProject, escrito explícito porque es la fórmula que el
	// documento de transición §4.8 fija como contrato del movimiento.
	return VectorCm - (FVector::DotProduct(VectorCm, UpUnit) * UpUnit);
}

FQuat FAstraeonPlanetFrame::AlignToUp(const FQuat& CurrentRotation, const FVector& UpUnit)
{
	const FVector Up = UpUnit.GetSafeNormal();
	if (Up.IsNearlyZero())
	{
		return CurrentRotation;
	}

	const FVector CurrentForward = CurrentRotation.GetForwardVector();
	const FVector TangentForward = ProjectToTangent(CurrentForward, Up);

	if (TangentForward.Size() >= AstraeonPlanetFrameLocal::MinTangentLength)
	{
		return FRotationMatrix::MakeFromZX(Up, TangentForward).ToQuat();
	}

	// El frente es paralelo al arriba: no informa de la orientación. El costado sí, porque un
	// marco ortonormal no puede tener sus tres ejes degenerados a la vez.
	const FVector TangentRight = ProjectToTangent(CurrentRotation.GetRightVector(), Up);
	if (TangentRight.Size() >= AstraeonPlanetFrameLocal::MinTangentLength)
	{
		return FRotationMatrix::MakeFromZY(Up, TangentRight).ToQuat();
	}

	// Inalcanzable con un cuaternión unitario: los tres ejes no pueden ser paralelos al arriba.
	// Se conserva como red por si entra una rotación degenerada, en vez de devolver basura.
	return FRotationMatrix::MakeFromZX(Up, FVector::ForwardVector).ToQuat();
}

FQuat FAstraeonPlanetFrame::AlignToUpInterpolated(const FQuat& CurrentRotation, const FVector& UpUnit,
	float DeltaSeconds, float DegreesPerSecond)
{
	const FQuat Target = AlignToUp(CurrentRotation, UpUnit);

	if (DeltaSeconds <= 0.0f || DegreesPerSecond <= 0.0f)
	{
		return Target;
	}

	const float MaxRadians = FMath::DegreesToRadians(DegreesPerSecond) * DeltaSeconds;
	const float AngleToTarget = CurrentRotation.AngularDistance(Target);
	if (AngleToTarget <= MaxRadians || AngleToTarget <= KINDA_SMALL_NUMBER)
	{
		return Target;
	}

	const float Alpha = MaxRadians / AngleToTarget;
	return FQuat::Slerp(CurrentRotation, Target, Alpha).GetNormalized();
}

bool FAstraeonPlanetFrame::IsFrameAligned(const FQuat& Rotation, const FVector& UpUnit, double ToleranceDeg)
{
	const FVector Up = UpUnit.GetSafeNormal();
	if (Up.IsNearlyZero())
	{
		return false;
	}

	const FVector FrameUp = Rotation.GetUpVector();
	const double CosTolerance = FMath::Cos(FMath::DegreesToRadians(ToleranceDeg));
	if (FVector::DotProduct(FrameUp, Up) < CosTolerance)
	{
		return false;
	}

	// Que Z coincida no basta: un marco con ejes no ortogonales pasaría la prueba anterior y
	// produciría deriva al componerse cada frame.
	const FVector FrameForward = Rotation.GetForwardVector();
	const FVector FrameRight = Rotation.GetRightVector();
	const double OrthogonalTolerance = FMath::Sin(FMath::DegreesToRadians(ToleranceDeg));

	return FMath::Abs(FVector::DotProduct(FrameForward, FrameUp)) <= OrthogonalTolerance
		&& FMath::Abs(FVector::DotProduct(FrameRight, FrameUp)) <= OrthogonalTolerance
		&& FMath::Abs(FVector::DotProduct(FrameForward, FrameRight)) <= OrthogonalTolerance;
}
