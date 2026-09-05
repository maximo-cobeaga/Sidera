#include "Survival/AstraeonSuitComponent.h"

#include "AstraeonGameInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace AstraeonSuit
{
	constexpr float EarthGravityMS2 = 9.81f;
	constexpr float MinGravitySpeedMultiplier = 0.62f;
	constexpr float MaxGravitySpeedMultiplier = 1.18f;
	constexpr float SealedSuitOxygenLossPerSecond = 0.045f;
	constexpr float BreathableOxygenLossPerSecond = 0.008f;
	constexpr float VacuumOrLowPressureDamagePerSecond = 0.18f;
	constexpr float ThermalDamagePerSecond = 0.10f;
}

UAstraeonSuitComponent::UAstraeonSuitComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAstraeonSuitComponent::BeginPlay()
{
	Super::BeginPlay();

	const UAstraeonGameInstance* AstraeonGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UAstraeonGameInstance>() : nullptr;
	if (AstraeonGameInstance && AstraeonGameInstance->HasStartedGame())
	{
		ApplyEnvironment(AstraeonGameInstance->GetCurrentEnvironment());
	}
}

void UAstraeonSuitComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DeltaTime <= 0.0f)
	{
		return;
	}

	OxygenPercent = FMath::Clamp(OxygenPercent - ComputeOxygenConsumptionPercentPerSecond(ActiveEnvironment) * DeltaTime, 0.0f, 100.0f);
	HealthPercent = FMath::Clamp(HealthPercent - ComputeEnvironmentalDamagePercentPerSecond(ActiveEnvironment) * DeltaTime, 0.0f, 100.0f);
}

void UAstraeonSuitComponent::ApplyHazardDamage(float DamagePercent)
{
	if (DamagePercent <= 0.0f)
	{
		return;
	}

	HealthPercent = FMath::Clamp(HealthPercent - DamagePercent, 0.0f, 100.0f);
}

void UAstraeonSuitComponent::ApplyEnvironment(const FAstraeonEnvironmentalSnapshot& Environment)
{
	ActiveEnvironment = Environment;
	GravitySpeedMultiplier = ComputeGravitySpeedMultiplier(Environment.GravityMS2);

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
		{
			Movement->MaxWalkSpeed = 520.0f * GravitySpeedMultiplier;
			Movement->JumpZVelocity = 420.0f / FMath::Max(0.85f, Environment.GravityMS2 / AstraeonSuit::EarthGravityMS2);
		}
	}
}

float UAstraeonSuitComponent::ComputeGravitySpeedMultiplier(float GravityMS2)
{
	const float GravityRatio = GravityMS2 / AstraeonSuit::EarthGravityMS2;
	const float RawMultiplier = 1.0f + ((1.0f - GravityRatio) * 0.35f);
	return FMath::Clamp(RawMultiplier, AstraeonSuit::MinGravitySpeedMultiplier, AstraeonSuit::MaxGravitySpeedMultiplier);
}

float UAstraeonSuitComponent::ComputeOxygenConsumptionPercentPerSecond(const FAstraeonEnvironmentalSnapshot& Environment)
{
	return Environment.bBreathable ? AstraeonSuit::BreathableOxygenLossPerSecond : AstraeonSuit::SealedSuitOxygenLossPerSecond;
}

float UAstraeonSuitComponent::ComputeEnvironmentalDamagePercentPerSecond(const FAstraeonEnvironmentalSnapshot& Environment)
{
	float DamagePerSecond = 0.0f;

	if (Environment.PressureKPa < 20.0f || Environment.PressureKPa > 130.0f)
	{
		DamagePerSecond += AstraeonSuit::VacuumOrLowPressureDamagePerSecond;
	}

	if (Environment.TemperatureKelvin < 235.0f || Environment.TemperatureKelvin > 325.0f)
	{
		DamagePerSecond += AstraeonSuit::ThermalDamagePerSecond;
	}

	return DamagePerSecond;
}
