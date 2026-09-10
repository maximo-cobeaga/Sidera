#pragma once

#include "CoreMinimal.h"
#include "Creatures/AstraeonCreatureTypes.h"
#include "GameFramework/Actor.h"
#include "AstraeonCreatureActor.generated.h"

class UAnimSequence;
class USkeletalMesh;
class USkeletalMeshComponent;
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

	// El pivote de la malla está en las patas, así que la ubicación del actor ya no es el
	// centro del cuerpo. Distancias de percepción y contacto siguen midiéndose al centro,
	// que es donde estaba antes el actor: cambiar la referencia habría movido los umbrales.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Creatures")
	FVector GetBodyCenterCm() const;

	// Mientras pasta se queda quieta cada tanto, en vez de dar vueltas sin parar. Sin esto
	// el clip de pastar no se vería nunca y la región se leía como un carrusel.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Creatures")
	static bool IsGrazingAtPhase(float PhaseSeconds);

	// Devuelve true si el disparo la mató. La criatura no se destruye sola: quien la mata
	// decide qué hacer con el cuerpo y con lo que deja.
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Creatures")
	bool ApplyWeaponDamage(float DamageAmount);

	// Deja el cadáver a la vista durante el clip de muerte y recién después lo retira.
	// Destruir en el mismo frame del disparo hacía imposible ver que algo murió.
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Creatures")
	void BeginDeathSequence();

	UFUNCTION(BlueprintPure, Category = "Astraeon|Creatures")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Creatures")
	bool IsDead() const { return Health <= 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Creatures")
	float GetHealthPercent() const;

	// Nido del que salió, para que al morir el mundo recuerde cuál quedó vacío.
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Creatures")
	void SetSpawnPointId(FName NewSpawnPointId);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Creatures")
	FName GetSpawnPointId() const { return SpawnPointId; }

	// A creature of a planet: its id is its entity id and its home is a direction on the body.
	// It walks the tangent plane and stands on the surface along the local vertical; the flat
	// region behaviour is untouched.
	void SetPlanetaryIdentity(FName EntityId, const FVector& HomeDirection);
	bool IsPlanetary() const { return bPlanetary; }
	FVector GetPlanetDirection() const;

private:
	void TickOnPlanet(float SafeDelta);
	void UpdatePresentationAndContact(APawn* PlayerPawn, bool bGrazing, float SafeDelta);
	bool bPlanetary = false;
	FVector HomeDirection = FVector(0, 0, 1);
	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Creatures")
	FName SpawnPointId;

	// Envolvente invisible de colisión e interacción. Conserva las dimensiones del proxy
	// anterior para no mover el alcance del disparo ni el del daño por contacto: el arte
	// cambia la presentación, no lo que el jugador puede tocar.
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Creatures")
	TObjectPtr<UStaticMeshComponent> CreatureCollision;

	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Creatures")
	TObjectPtr<USkeletalMeshComponent> CreatureArt;

	UPROPERTY(EditAnywhere, Category = "Astraeon|Creatures")
	FAstraeonCreatureProfile Profile;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Creatures")
	EAstraeonCreatureAwarenessState AwarenessState = EAstraeonCreatureAwarenessState::Patrolling;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Creatures")
	float Health = 100.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Creatures")
	float PatrolPhaseSeconds = 0.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Creatures")
	FVector PatrolOriginCm = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Creatures")
	bool bHasPatrolOrigin = false;

	UPROPERTY()
	TArray<TObjectPtr<USkeletalMesh>> VariantMeshes;

	UPROPERTY()
	TObjectPtr<UAnimSequence> GrazeSequence;

	UPROPERTY()
	TObjectPtr<UAnimSequence> WalkSequence;

	UPROPERTY()
	TObjectPtr<UAnimSequence> AlertSequence;

	UPROPERTY()
	TObjectPtr<UAnimSequence> ThreatenSequence;

	UPROPERTY()
	TObjectPtr<UAnimSequence> FleeSequence;

	UPROPERTY()
	TObjectPtr<UAnimSequence> HitSequence;

	UPROPERTY()
	TObjectPtr<UAnimSequence> DeathSequence;

	UPROPERTY()
	TObjectPtr<UAnimSequence> ActiveSequence;

	FTimerHandle CorpseTimerHandle;
	float OneShotSecondsRemaining = 0.0f;

	UAnimSequence* SelectStateSequence(bool bGrazing) const;
	void PlaySequence(UAnimSequence* Sequence, bool bLoop);
	void PlayOneShot(UAnimSequence* Sequence);
};
