#include "Creatures/AstraeonCreatureActor.h"

#include "AstraeonGameInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Survival/AstraeonSuitComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "WorldGen/AstraeonTerrainField.h"
#include "Planet/AstraeonPlanetRuntime.h"

namespace AstraeonCreature
{
	// Persecución algo más lenta que el sprint del jugador (900): escapar corriendo tiene
	// que ser una salida válida, si no cualquier encuentro es una muerte segura.
	constexpr float ChaseSpeedCms = 760.0f;
	constexpr float FleeSpeedCms = 620.0f;
	constexpr float PatrolSpeedCms = 180.0f;
	constexpr float ContactDamageRangeCm = 260.0f;

	// Envolvente del proxy anterior, ahora invisible: 1,4 x 0,8 x 0,7 m apoyado en el suelo.
	const FVector CollisionScale(1.4f, 0.8f, 0.7f);
	constexpr float BodyCentreHeightCm = 35.0f;

	// Ciclo de pastoreo: cuatro segundos quieta de cada diez.
	constexpr float GrazeCycleSeconds = 10.0f;
	constexpr float GrazeHoldSeconds = 4.0f;

	const TCHAR* ArtPath = TEXT("/Game/Astraeon/Art/Blockouts/Creatures/");

	template <typename AssetType>
	AssetType* Load(const TCHAR* Name)
	{
		const FString Reference = FString::Printf(TEXT("%s%s.%s"), ArtPath, Name, Name);
		ConstructorHelpers::FObjectFinder<AssetType> Finder(*Reference);
		return Finder.Succeeded() ? Finder.Object : nullptr;
	}
}

AAstraeonCreatureActor::AAstraeonCreatureActor()
{
	PrimaryActorTick.bCanEverTick = true;

	CreatureCollision = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CreatureCollision"));
	RootComponent = CreatureCollision;
	CreatureCollision->SetCollisionProfileName(TEXT("BlockAll"));
	// La esfera sigue siendo el cuerpo a efectos de disparo, escaneo y contacto; deja de
	// serlo sólo a efectos de lo que se ve.
	CreatureCollision->SetVisibility(false);
	CreatureCollision->SetRelativeLocation(FVector(0.0f, 0.0f, AstraeonCreature::BodyCentreHeightCm));
	CreatureCollision->SetRelativeScale3D(AstraeonCreature::CollisionScale);
	// No se puede subir encima de un animal. Además de ser lo razonable, impide que la
	// criatura se vuelva la base de movimiento del jugador: como se mueve y gira cada frame,
	// arrastraba y hacía patinar al que se le subía encima.
	CreatureCollision->CanCharacterStepUpOn = ECB_No;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		CreatureCollision->SetStaticMesh(SphereMesh.Object);
	}

	CreatureArt = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CreatureArt"));
	CreatureArt->SetupAttachment(CreatureCollision);
	// Deshace la escala y el alzado del envolvente: el arte va a escala 1 y con el pivote
	// en las patas, que es donde lo dejó el generador.
	CreatureArt->SetAbsolute(false, false, true);
	CreatureArt->SetRelativeLocation(FVector(0.0f, 0.0f, -AstraeonCreature::BodyCentreHeightCm
		/ AstraeonCreature::CollisionScale.Z));
	// El frente del rig queda en +Y tras la importación; los actores miran a +X.
	CreatureArt->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	CreatureArt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CreatureArt->SetGenerateOverlapEvents(false);

	if (USkeletalMesh* Base = AstraeonCreature::Load<USkeletalMesh>(TEXT("SK_Creature_UmbraGrazer_Blockout")))
	{
		VariantMeshes.Add(Base);
		CreatureArt->SetSkeletalMeshAsset(Base);
	}
	if (USkeletalMesh* Plated = AstraeonCreature::Load<USkeletalMesh>(TEXT("SK_Creature_UmbraGrazer_Plated_Blockout")))
	{
		VariantMeshes.Add(Plated);
	}

	GrazeSequence = AstraeonCreature::Load<UAnimSequence>(TEXT("AN_Creature_Graze_Blockout"));
	WalkSequence = AstraeonCreature::Load<UAnimSequence>(TEXT("AN_Creature_Walk_Blockout"));
	AlertSequence = AstraeonCreature::Load<UAnimSequence>(TEXT("AN_Creature_Alert_Blockout"));
	ThreatenSequence = AstraeonCreature::Load<UAnimSequence>(TEXT("AN_Creature_Threaten_Blockout"));
	FleeSequence = AstraeonCreature::Load<UAnimSequence>(TEXT("AN_Creature_Flee_Blockout"));
	HitSequence = AstraeonCreature::Load<UAnimSequence>(TEXT("AN_Creature_Hit_Blockout"));
	DeathSequence = AstraeonCreature::Load<UAnimSequence>(TEXT("AN_Creature_Death_Blockout"));
}

FVector AAstraeonCreatureActor::GetBodyCenterCm() const
{
	// Along the actor's own up: world Z in the flat region, the local vertical on a planet.
	return GetActorLocation() + GetActorUpVector() * AstraeonCreature::BodyCentreHeightCm;
}

void AAstraeonCreatureActor::SetPlanetaryIdentity(FName EntityId, const FVector& InHomeDirection)
{
	bPlanetary = true;
	HomeDirection = InHomeDirection.GetSafeNormal();
	SetSpawnPointId(EntityId);
}

FVector AAstraeonCreatureActor::GetPlanetDirection() const
{
	const AAstraeonPlanetRuntime* Planet = bPlanetary ? AAstraeonPlanetRuntime::FindActive(GetWorld()) : nullptr;
	return Planet ? (GetActorLocation() - Planet->GetActorLocation()).GetSafeNormal() : HomeDirection;
}

bool AAstraeonCreatureActor::IsGrazingAtPhase(float PhaseSeconds)
{
	using namespace AstraeonCreature;
	return FMath::Fmod(FMath::Max(0.0f, PhaseSeconds), GrazeCycleSeconds) < GrazeHoldSeconds;
}

void AAstraeonCreatureActor::SetSpawnPointId(FName NewSpawnPointId)
{
	SpawnPointId = NewSpawnPointId;

	// La variante se elige por el nido, no al azar: la misma seed devuelve el mismo animal
	// en el mismo sitio partida tras partida.
	if (VariantMeshes.Num() > 1 && !SpawnPointId.IsNone())
	{
		const uint32 Hash = GetTypeHash(SpawnPointId);
		const int32 Index = static_cast<int32>(Hash % static_cast<uint32>(VariantMeshes.Num()));
		CreatureArt->SetSkeletalMeshAsset(VariantMeshes[Index]);
		// Cambiar la malla descarta la pose actual: hay que volver a pedir el clip.
		ActiveSequence = nullptr;
	}
}

void AAstraeonCreatureActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bPlanetary)
	{
		TickOnPlanet(FMath::Max(DeltaSeconds, 0.0f));
		return;
	}

	if (!bHasPatrolOrigin)
	{
		PatrolOriginCm = GetActorLocation();
		bHasPatrolOrigin = true;
	}

	const float SafeDelta = FMath::Max(DeltaSeconds, 0.0f);
	PatrolPhaseSeconds += SafeDelta;
	OneShotSecondsRemaining = FMath::Max(0.0f, OneShotSecondsRemaining - SafeDelta);

	const UAstraeonGameInstance* AstraeonGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UAstraeonGameInstance>() : nullptr;

	// Un cadáver ya no persigue, ni hiere, ni cambia de estado: sólo se queda donde cayó
	// hasta que termina su clip.
	if (IsDead())
	{
		FVector RestingLocation = GetActorLocation();
		RestingLocation.Z = AstraeonGameInstance ? AstraeonGameInstance->GetSurfaceHeightCm(FVector2D(RestingLocation.X, RestingLocation.Y)) : RestingLocation.Z;
		SetActorLocation(RestingLocation);
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (PlayerPawn)
	{
		const float DistanceMeters = FVector::Distance(PlayerPawn->GetActorLocation(), GetBodyCenterCm()) / 100.0f;
		UpdateAwarenessFromPlayerDistanceMeters(DistanceMeters);
	}

	// Antes la criatura sólo giraba en un círculo fijo y jamás se acercaba: con el combate
	// letal ya implementado, eso volvía trivial tanto pelear como cazar. Ahora persigue
	// cuando amenaza y huye cuando se desinteresa; patrullar queda para cuando te ignora.
	const FVector CurrentLocation = GetActorLocation();
	FVector TargetLocation = CurrentLocation;
	float SpeedCms = 0.0f;
	const bool bGrazing = AwarenessState == EAstraeonCreatureAwarenessState::Patrolling
		&& IsGrazingAtPhase(PatrolPhaseSeconds);

	if (PlayerPawn && AwarenessState == EAstraeonCreatureAwarenessState::Threatening)
	{
		TargetLocation = PlayerPawn->GetActorLocation();
		SpeedCms = AstraeonCreature::ChaseSpeedCms;
	}
	else if (PlayerPawn && AwarenessState == EAstraeonCreatureAwarenessState::Disengaging)
	{
		const FVector AwayDirection = (CurrentLocation - PlayerPawn->GetActorLocation()).GetSafeNormal2D();
		TargetLocation = CurrentLocation + AwayDirection * 1000.0f;
		SpeedCms = AstraeonCreature::FleeSpeedCms;
	}
	else if (!bGrazing)
	{
		TargetLocation = PatrolOriginCm + ComputePatrolOffsetCm(PatrolPhaseSeconds, Profile.PatrolRadiusMeters);
		SpeedCms = AstraeonCreature::PatrolSpeedCms;
	}

	FVector NextLocation = CurrentLocation;
	const FVector ToTarget = (TargetLocation - CurrentLocation).GetSafeNormal2D();
	if (SpeedCms > 0.0f && !ToTarget.IsNearlyZero())
	{
		NextLocation += ToTarget * SpeedCms * SafeDelta;
		// Mira hacia donde se mueve: sin esto, perseguir se leía como deslizarse de costado.
		SetActorRotation(FRotator(0.0f, ToTarget.Rotation().Yaw, 0.0f));
	}

	// Se apoya sobre el relieve. El pivote de la malla está en las patas, así que la cota
	// del terreno es directamente la del actor; el vaivén del cuerpo lo trae la animación,
	// que antes había que falsear con un seno sobre la posición.
	// Se apoya en la superficie materializada, no en la capa de suelo desnuda: donde hay
	// montaña permitida, el suelo pelado queda metros por debajo de lo que se ve.
	NextLocation.Z = AstraeonGameInstance ? AstraeonGameInstance->GetSurfaceHeightCm(FVector2D(NextLocation.X, NextLocation.Y)) : NextLocation.Z;
	SetActorLocation(NextLocation);
	UpdatePresentationAndContact(PlayerPawn, bGrazing, SafeDelta);
}

void AAstraeonCreatureActor::TickOnPlanet(float SafeDelta)
{
	const AAstraeonPlanetRuntime* Planet = AAstraeonPlanetRuntime::FindActive(GetWorld());
	if (!Planet) return;
	PatrolPhaseSeconds += SafeDelta;
	OneShotSecondsRemaining = FMath::Max(0.0f, OneShotSecondsRemaining - SafeDelta);
	FVector Up = (GetActorLocation() - Planet->GetActorLocation()).GetSafeNormal();
	if (Up.IsNearlyZero()) Up = HomeDirection;
	if (IsDead())
	{
		SetActorLocation(Planet->GetSurfacePointCm(Up, 0.0));
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (PlayerPawn)
	{
		UpdateAwarenessFromPlayerDistanceMeters(FVector::Distance(PlayerPawn->GetActorLocation(), GetBodyCenterCm()) / 100.0f);
	}
	const bool bGrazing = AwarenessState == EAstraeonCreatureAwarenessState::Patrolling && IsGrazingAtPhase(PatrolPhaseSeconds);

	// Same behaviour as in the flat region, with "the plane" replaced by the tangent plane.
	FVector Target = GetActorLocation();
	float SpeedCms = 0.0f;
	if (PlayerPawn && AwarenessState == EAstraeonCreatureAwarenessState::Threatening)
	{
		Target = PlayerPawn->GetActorLocation(); SpeedCms = AstraeonCreature::ChaseSpeedCms;
	}
	else if (PlayerPawn && AwarenessState == EAstraeonCreatureAwarenessState::Disengaging)
	{
		Target = GetActorLocation() * 2.0 - PlayerPawn->GetActorLocation(); SpeedCms = AstraeonCreature::FleeSpeedCms;
	}
	else if (!bGrazing)
	{
		// The patrol circle lives on the tangent plane of the home direction, so it stays put.
		const FVector Reference = FMath::Abs(HomeDirection.Z) < 0.9 ? FVector(0, 0, 1) : FVector(1, 0, 0);
		const FVector A = FVector::CrossProduct(Reference, HomeDirection).GetSafeNormal(), B = FVector::CrossProduct(HomeDirection, A);
		const FVector Offset = ComputePatrolOffsetCm(PatrolPhaseSeconds, Profile.PatrolRadiusMeters);
		Target = Planet->GetSurfacePointCm((HomeDirection + (A * Offset.X + B * Offset.Y) / Planet->RadiusCm).GetSafeNormal(), 0.0);
		SpeedCms = AstraeonCreature::PatrolSpeedCms;
	}
	FVector Along = Target - GetActorLocation();
	Along -= Up * FVector::DotProduct(Along, Up);
	FVector Forward = GetActorForwardVector();
	if (SpeedCms > 0.0f && Along.SizeSquared() > 1.0)
	{
		const double Step = FMath::Min<double>(SpeedCms * SafeDelta, Along.Size());
		Forward = Along.GetSafeNormal();
		Up = (Up + Forward * (Step / Planet->RadiusCm)).GetSafeNormal();
	}
	// Feet on the surface, body along the local vertical, facing where it goes.
	SetActorLocation(Planet->GetSurfacePointCm(Up, 0.0));
	Forward -= Up * FVector::DotProduct(Forward, Up);
	if (Forward.IsNearlyZero()) Forward = FVector::CrossProduct(Up, FMath::Abs(Up.Z) < 0.9 ? FVector(0, 0, 1) : FVector(1, 0, 0));
	SetActorRotation(FRotationMatrix::MakeFromXZ(Forward.GetSafeNormal(), Up).ToQuat());
	UpdatePresentationAndContact(PlayerPawn, bGrazing, SafeDelta);
}

void AAstraeonCreatureActor::UpdatePresentationAndContact(APawn* PlayerPawn, bool bGrazing, float SafeDelta)
{
	if (OneShotSecondsRemaining <= 0.0f)
	{
		PlaySequence(SelectStateSequence(bGrazing), true);
	}

	// El daño ahora exige contacto real, no sólo estar dentro del radio de amenaza: si no,
	// la criatura hería sin siquiera haberse acercado.
	if (PlayerPawn && AwarenessState == EAstraeonCreatureAwarenessState::Threatening)
	{
		const float ContactDistanceCm = FVector::Dist(PlayerPawn->GetActorLocation(), GetBodyCenterCm());
		if (ContactDistanceCm <= AstraeonCreature::ContactDamageRangeCm)
		{
			if (UAstraeonSuitComponent* SuitComponent = PlayerPawn->FindComponentByClass<UAstraeonSuitComponent>())
			{
				SuitComponent->ApplyHazardDamage(Profile.ThreatDamagePercentPerSecond * SafeDelta);
			}
		}
	}
}

UAnimSequence* AAstraeonCreatureActor::SelectStateSequence(bool bGrazing) const
{
	switch (AwarenessState)
	{
	case EAstraeonCreatureAwarenessState::Alert:
		return AlertSequence;
	case EAstraeonCreatureAwarenessState::Threatening:
		return ThreatenSequence;
	case EAstraeonCreatureAwarenessState::Disengaging:
		return FleeSequence;
	case EAstraeonCreatureAwarenessState::Patrolling:
	default:
		return bGrazing ? GrazeSequence : WalkSequence;
	}
}

void AAstraeonCreatureActor::PlaySequence(UAnimSequence* Sequence, bool bLoop)
{
	if (!Sequence || Sequence == ActiveSequence || !CreatureArt)
	{
		return;
	}

	ActiveSequence = Sequence;
	CreatureArt->PlayAnimation(Sequence, bLoop);
}

void AAstraeonCreatureActor::PlayOneShot(UAnimSequence* Sequence)
{
	if (!Sequence)
	{
		return;
	}

	OneShotSecondsRemaining = Sequence->GetPlayLength();
	// Forzar el reinicio: recibir dos impactos seguidos tiene que verse dos veces.
	ActiveSequence = nullptr;
	PlaySequence(Sequence, false);
}

void AAstraeonCreatureActor::ConfigureCreature(const FAstraeonCreatureProfile& NewProfile)
{
	Profile = NewProfile;
	Health = FMath::Max(1.0f, Profile.MaxHealth);
}

bool AAstraeonCreatureActor::ApplyWeaponDamage(float DamageAmount)
{
	if (DamageAmount <= 0.0f || IsDead())
	{
		return false;
	}

	Health = FMath::Max(0.0f, Health - DamageAmount);

	// Herirla la vuelve hostil aunque estuviera pastando tranquila.
	if (!IsDead() && AwarenessState != EAstraeonCreatureAwarenessState::Threatening)
	{
		AwarenessState = EAstraeonCreatureAwarenessState::Threatening;
	}

	PlayOneShot(IsDead() ? DeathSequence : HitSequence);
	return IsDead();
}

void AAstraeonCreatureActor::BeginDeathSequence()
{
	// Deja de estorbar de inmediato: un cadáver no debe bloquear el paso ni recibir otro
	// disparo mientras se desploma.
	SetActorEnableCollision(false);
	PlayOneShot(DeathSequence);

	UWorld* World = GetWorld();
	if (!World)
	{
		Destroy();
		return;
	}

	const float CorpseSeconds = DeathSequence ? DeathSequence->GetPlayLength() : 0.0f;
	if (CorpseSeconds <= 0.0f)
	{
		Destroy();
		return;
	}

	World->GetTimerManager().SetTimer(CorpseTimerHandle, FTimerDelegate::CreateWeakLambda(this,
		[this]() { Destroy(); }), CorpseSeconds, false);
}

float AAstraeonCreatureActor::GetHealthPercent() const
{
	const float MaxHealth = FMath::Max(1.0f, Profile.MaxHealth);
	return (Health / MaxHealth) * 100.0f;
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
