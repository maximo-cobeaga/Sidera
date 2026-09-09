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

	// Un módulo mitiga su amenaza pero nunca vuelve inmune: sigue habiendo un costo
	// residual, así que un ambiente muy hostil se siente hostil incluso bien equipado.
	constexpr float RespiratorOxygenLossPerSecond = 0.012f;
	constexpr float SealedPressureDamagePerSecond = 0.025f;
	constexpr float ShieldedThermalDamagePerSecond = 0.015f;
	constexpr float SuffocationDamagePerSecond = 2.5f;
	constexpr float HavenRecoveryPercentPerSecond = 6.0f;

	// ~20 minutos de juego para vaciarse desde lleno: obliga a cazar de vez en cuando sin
	// convertir la partida en una carrera contra el reloj.
	constexpr float HungerLossPerSecond = 0.085f;
	constexpr float StarvationDamagePerSecond = 1.2f;
	constexpr float RationNourishmentPercent = 45.0f;

	constexpr float MinSafePressureKPa = 20.0f;
	constexpr float MaxSafePressureKPa = 130.0f;
	constexpr float MinSafeTemperatureKelvin = 235.0f;
	constexpr float MaxSafeTemperatureKelvin = 325.0f;

	bool HasPressureHazard(const FAstraeonEnvironmentalSnapshot& Environment)
	{
		return Environment.PressureKPa < MinSafePressureKPa || Environment.PressureKPa > MaxSafePressureKPa;
	}

	bool HasThermalHazard(const FAstraeonEnvironmentalSnapshot& Environment)
	{
		return Environment.TemperatureKelvin < MinSafeTemperatureKelvin || Environment.TemperatureKelvin > MaxSafeTemperatureKelvin;
	}
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

	const UAstraeonGameInstance* AstraeonGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UAstraeonGameInstance>() : nullptr;
	const EAstraeonProtectionModule Protection = AstraeonGameInstance
		? AstraeonGameInstance->GetEquippedProtection()
		: EAstraeonProtectionModule::None;

	// El hambre corre también dentro de Ítaca: el refugio da aire y cura heridas, pero no
	// alimenta. Comer exige haber cazado, y eso es lo que sostiene el bucle a largo plazo.
	float StarvationDamage = 0.0f;
	if (UAstraeonGameInstance* MutableGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UAstraeonGameInstance>() : nullptr)
	{
		MutableGameInstance->ConsumeHunger(AstraeonSuit::HungerLossPerSecond * DeltaTime);
		StarvationDamage = MutableGameInstance->IsStarving() ? AstraeonSuit::StarvationDamagePerSecond : 0.0f;
	}

	if (bInHaven)
	{
		// Ítaca presuriza y repone: el oxígeno deja de ser una cuenta atrás sin retorno.
		const float Recovery = AstraeonSuit::HavenRecoveryPercentPerSecond * DeltaTime;
		OxygenPercent = FMath::Clamp(OxygenPercent + Recovery, 0.0f, 100.0f);
		// Pasar hambre impide recuperarse incluso a cubierto: no se puede vivir en la nave.
		HealthPercent = FMath::Clamp(HealthPercent + (Recovery - StarvationDamage * DeltaTime), 0.0f, 100.0f);
		return;
	}

	OxygenPercent = FMath::Clamp(OxygenPercent - ComputeOxygenConsumptionPercentPerSecond(ActiveEnvironment, Protection) * DeltaTime, 0.0f, 100.0f);

	// Quedarse sin oxígeno asfixia: si no, el medidor bajaba a cero sin ninguna consecuencia.
	const float SuffocationDamage = OxygenPercent <= 0.0f ? AstraeonSuit::SuffocationDamagePerSecond : 0.0f;
	const float TotalDamage = ComputeEnvironmentalDamagePercentPerSecond(ActiveEnvironment, Protection)
		+ SuffocationDamage
		+ StarvationDamage;
	HealthPercent = FMath::Clamp(HealthPercent - TotalDamage * DeltaTime, 0.0f, 100.0f);
}

float UAstraeonSuitComponent::GetHungerLossPerSecond()
{
	return AstraeonSuit::HungerLossPerSecond;
}

float UAstraeonSuitComponent::GetStarvationDamagePerSecond()
{
	return AstraeonSuit::StarvationDamagePerSecond;
}

void UAstraeonSuitComponent::RestoreAfterRecall()
{
	// ARGOS estabiliza al superviviente, pero no lo deja como nuevo: se vuelve en pie y con
	// aire, no descansado.
	OxygenPercent = 60.0f;
	HealthPercent = 45.0f;
}

float UAstraeonSuitComponent::GetHavenRecoveryPercentPerSecond()
{
	return AstraeonSuit::HavenRecoveryPercentPerSecond;
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

float UAstraeonSuitComponent::ComputeOxygenConsumptionPercentPerSecond(const FAstraeonEnvironmentalSnapshot& Environment, EAstraeonProtectionModule Protection)
{
	if (Environment.bBreathable)
	{
		return AstraeonSuit::BreathableOxygenLossPerSecond;
	}

	return Protection == EAstraeonProtectionModule::Respirator
		? AstraeonSuit::RespiratorOxygenLossPerSecond
		: AstraeonSuit::SealedSuitOxygenLossPerSecond;
}

float UAstraeonSuitComponent::ComputeEnvironmentalDamagePercentPerSecond(const FAstraeonEnvironmentalSnapshot& Environment, EAstraeonProtectionModule Protection)
{
	float DamagePerSecond = 0.0f;

	if (AstraeonSuit::HasPressureHazard(Environment))
	{
		DamagePerSecond += Protection == EAstraeonProtectionModule::PressureSeal
			? AstraeonSuit::SealedPressureDamagePerSecond
			: AstraeonSuit::VacuumOrLowPressureDamagePerSecond;
	}

	if (AstraeonSuit::HasThermalHazard(Environment))
	{
		DamagePerSecond += Protection == EAstraeonProtectionModule::ThermalShield
			? AstraeonSuit::ShieldedThermalDamagePerSecond
			: AstraeonSuit::ThermalDamagePerSecond;
	}

	return DamagePerSecond;
}

TArray<FString> UAstraeonSuitComponent::DescribeActiveHazards(const FAstraeonEnvironmentalSnapshot& Environment)
{
	TArray<FString> Hazards;

	if (AstraeonSuit::HasPressureHazard(Environment))
	{
		Hazards.Add(FString::Printf(TEXT("presión %.0f kPa"), Environment.PressureKPa));
	}

	if (AstraeonSuit::HasThermalHazard(Environment))
	{
		Hazards.Add(FString::Printf(TEXT("temperatura %.0f K"), Environment.TemperatureKelvin));
	}

	if (!Environment.bBreathable)
	{
		Hazards.Add(TEXT("atmósfera irrespirable"));
	}

	return Hazards;
}

EAstraeonProtectionModule UAstraeonSuitComponent::RecommendProtection(const FAstraeonEnvironmentalSnapshot& Environment)
{
	// Se prioriza por costo real: la presión hace más daño por segundo que la térmica, y
	// el oxígeno sólo se vuelve crítico cuando no hay ninguna amenaza que cause daño.
	if (AstraeonSuit::HasPressureHazard(Environment))
	{
		return EAstraeonProtectionModule::PressureSeal;
	}

	if (AstraeonSuit::HasThermalHazard(Environment))
	{
		return EAstraeonProtectionModule::ThermalShield;
	}

	if (!Environment.bBreathable)
	{
		return EAstraeonProtectionModule::Respirator;
	}

	return EAstraeonProtectionModule::None;
}

FString UAstraeonSuitComponent::DescribeProtection(EAstraeonProtectionModule Protection)
{
	switch (Protection)
	{
	case EAstraeonProtectionModule::Respirator:
		return TEXT("Respirador");
	case EAstraeonProtectionModule::ThermalShield:
		return TEXT("Aislante térmico");
	case EAstraeonProtectionModule::PressureSeal:
		return TEXT("Sellado de presión");
	default:
		return TEXT("sin módulo");
	}
}
