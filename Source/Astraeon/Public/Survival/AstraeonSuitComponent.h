#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Survival/AstraeonProtectionTypes.h"
#include "WorldGen/AstraeonEnvironmentTypes.h"
#include "AstraeonSuitComponent.generated.h"

UCLASS(ClassGroup = (Astraeon), meta = (BlueprintSpawnableComponent))
class ASTRAEON_API UAstraeonSuitComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAstraeonSuitComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void ApplyEnvironment(const FAstraeonEnvironmentalSnapshot& Environment);

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Survival")
	void ApplyHazardDamage(float DamagePercent);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	float GetOxygenPercent() const { return OxygenPercent; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	float GetHealthPercent() const { return HealthPercent; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	float GetGravitySpeedMultiplier() const { return GravitySpeedMultiplier; }

	// Sin esto, la salud llegaba a cero y no pasaba nada: el daño ambiental, la amenaza de
	// las criaturas y los módulos de protección no tenían ninguna consecuencia final.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	bool IsIncapacitated() const { return HealthPercent <= 0.0f; }

	// Dentro de Ítaca el traje recarga en vez de consumirse: la nave es el refugio, y por
	// eso vale la pena volver a ella.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	bool IsInHaven() const { return bInHaven; }

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Survival")
	void SetInHaven(bool bNewInHaven) { bInHaven = bNewInHaven; }

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Survival")
	void RestoreAfterRecall();

	// El hambre vive en el GameInstance, que es la fuente de verdad de la sesión y lo que
	// se persiste; el traje sólo lo consume tick a tick.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	static float GetHungerLossPerSecond();

	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	static float GetStarvationDamagePerSecond();

	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	static float GetHavenRecoveryPercentPerSecond();

	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	static float ComputeGravitySpeedMultiplier(float GravityMS2);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	static float ComputeOxygenConsumptionPercentPerSecond(const FAstraeonEnvironmentalSnapshot& Environment, EAstraeonProtectionModule Protection = EAstraeonProtectionModule::None);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	static float ComputeEnvironmentalDamagePercentPerSecond(const FAstraeonEnvironmentalSnapshot& Environment, EAstraeonProtectionModule Protection = EAstraeonProtectionModule::None);

	// Amenazas activas del ambiente, en el orden en que conviene compensarlas. Alimenta el
	// HUD para que la medición se traduzca en una decisión legible.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	static TArray<FString> DescribeActiveHazards(const FAstraeonEnvironmentalSnapshot& Environment);

	// Módulo que mejor compensa la amenaza dominante del ambiente medido.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	static EAstraeonProtectionModule RecommendProtection(const FAstraeonEnvironmentalSnapshot& Environment);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	static FString DescribeProtection(EAstraeonProtectionModule Protection);

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Survival", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float OxygenPercent = 100.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Survival", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float HealthPercent = 100.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Survival")
	float GravitySpeedMultiplier = 1.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Survival")
	bool bInHaven = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Survival")
	FAstraeonEnvironmentalSnapshot ActiveEnvironment;
};
