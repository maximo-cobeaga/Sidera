#pragma once

#include "CoreMinimal.h"
#include "Creatures/AstraeonCreatureTypes.h"
#include "GameFramework/Actor.h"
#include "AstraeonCreatureActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class ASTRAEON_API AAstraeonCreatureActor : public AActor
{
	GENERATED_BODY()

public:
	AAstraeonCreatureActor();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Creatures")
	void ConfigureCreature(const FAstraeonCreatureProfile& NewProfile);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Creatures")
	const FAstraeonCreatureProfile& GetCreatureProfile() const { return Profile; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Creatures")
	EAstraeonCreatureAwarenessState GetAwarenessState() const { return AwarenessState; }

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Creatures")
	void UpdateAwarenessFromPlayerDistanceMeters(float DistanceMeters);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Creatures")
	static EAstraeonCreatureAwarenessState EvaluateAwarenessState(float DistanceMeters, const FAstraeonCreatureProfile& Profile);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Creatures")
	static FVector ComputePatrolOffsetCm(float PhaseSeconds, float PatrolRadiusMeters);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Creatures")
	static FVector ComputeStateVisualScale(EAstraeonCreatureAwarenessState State);

private:
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Creatures")
	TObjectPtr<UStaticMeshComponent> CreatureMesh;

	UPROPERTY(EditAnywhere, Category = "Astraeon|Creatures")
	FAstraeonCreatureProfile Profile;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Creatures")
	EAstraeonCreatureAwarenessState AwarenessState = EAstraeonCreatureAwarenessState::Patrolling;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Creatures")
	float PatrolPhaseSeconds = 0.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Creatures")
	FVector PatrolOriginCm = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Creatures")
	bool bHasPatrolOrigin = false;
};
