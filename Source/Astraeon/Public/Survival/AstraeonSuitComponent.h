#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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

	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	static float ComputeGravitySpeedMultiplier(float GravityMS2);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	static float ComputeOxygenConsumptionPercentPerSecond(const FAstraeonEnvironmentalSnapshot& Environment);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	static float ComputeEnvironmentalDamagePercentPerSecond(const FAstraeonEnvironmentalSnapshot& Environment);

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Survival", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float OxygenPercent = 100.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Survival", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float HealthPercent = 100.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Survival")
	float GravitySpeedMultiplier = 1.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Survival")
	FAstraeonEnvironmentalSnapshot ActiveEnvironment;
};
