#include "Creatures/AstraeonCreatureActor.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Survival/AstraeonSuitComponent.h"
#include "UObject/ConstructorHelpers.h"

AAstraeonCreatureActor::AAstraeonCreatureActor()
{
	PrimaryActorTick.bCanEverTick = true;

	CreatureMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CreatureMesh"));
	RootComponent = CreatureMesh;
	CreatureMesh->SetCollisionProfileName(TEXT("BlockAll"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		CreatureMesh->SetStaticMesh(SphereMesh.Object);
	}

	SetActorScale3D(FVector(1.4f, 0.8f, 0.7f));
}

void AAstraeonCreatureActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bHasPatrolOrigin)
	{
		PatrolOriginCm = GetActorLocation();
		bHasPatrolOrigin = true;
	}

	PatrolPhaseSeconds += FMath::Max(DeltaSeconds, 0.0f);
	const FVector PatrolOffset = ComputePatrolOffsetCm(PatrolPhaseSeconds, Profile.PatrolRadiusMeters);
	FVector Location = PatrolOriginCm + PatrolOffset;
	Location.Z = PatrolOriginCm.Z + FMath::Sin(PatrolPhaseSeconds * 1.2f) * 10.0f;
	SetActorLocation(Location);

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (PlayerPawn)
	{
		const float DistanceMeters = FVector::Distance(PlayerPawn->GetActorLocation(), GetActorLocation()) / 100.0f;
		UpdateAwarenessFromPlayerDistanceMeters(DistanceMeters);
		SetActorScale3D(ComputeStateVisualScale(AwarenessState));

		if (AwarenessState == EAstraeonCreatureAwarenessState::Threatening)
		{
			if (UAstraeonSuitComponent* SuitComponent = PlayerPawn->FindComponentByClass<UAstraeonSuitComponent>())
			{
				SuitComponent->ApplyHazardDamage(Profile.ThreatDamagePercentPerSecond * FMath::Max(DeltaSeconds, 0.0f));
			}
		}
	}
}

void AAstraeonCreatureActor::ConfigureCreature(const FAstraeonCreatureProfile& NewProfile)
{
	Profile = NewProfile;
}

void AAstraeonCreatureActor::UpdateAwarenessFromPlayerDistanceMeters(float DistanceMeters)
{
	AwarenessState = EvaluateAwarenessState(DistanceMeters, Profile);
}

FVector AAstraeonCreatureActor::ComputePatrolOffsetCm(float PhaseSeconds, float PatrolRadiusMeters)
{
	const float RadiusCm = FMath::Max(0.0f, PatrolRadiusMeters) * 100.0f;
	const float AngleRadians = PhaseSeconds * 0.35f;
	return FVector(FMath::Cos(AngleRadians) * RadiusCm, FMath::Sin(AngleRadians) * RadiusCm, 0.0f);
}

FVector AAstraeonCreatureActor::ComputeStateVisualScale(EAstraeonCreatureAwarenessState State)
{
	switch (State)
	{
	case EAstraeonCreatureAwarenessState::Alert:
		return FVector(1.55f, 0.95f, 0.78f);
	case EAstraeonCreatureAwarenessState::Threatening:
		return FVector(1.75f, 1.08f, 0.88f);
	case EAstraeonCreatureAwarenessState::Disengaging:
		return FVector(1.35f, 0.82f, 0.68f);
	case EAstraeonCreatureAwarenessState::Patrolling:
	default:
		return FVector(1.4f, 0.8f, 0.7f);
	}
}

EAstraeonCreatureAwarenessState AAstraeonCreatureActor::EvaluateAwarenessState(float DistanceMeters, const FAstraeonCreatureProfile& Profile)
{
	if (DistanceMeters <= Profile.ThreatRadiusMeters)
	{
		return EAstraeonCreatureAwarenessState::Threatening;
	}

	if (DistanceMeters <= Profile.AlertRadiusMeters)
	{
		return EAstraeonCreatureAwarenessState::Alert;
	}

	if (DistanceMeters <= Profile.AlertRadiusMeters * 1.35f)
	{
		return EAstraeonCreatureAwarenessState::Disengaging;
	}

	return EAstraeonCreatureAwarenessState::Patrolling;
}
