#include "Planet/AstraeonPlanetRuntime.h"
#include "Planet/Collision/AstraeonPlanetCollisionRing.h"
#include "Planet/State/AstraeonPlanetEntities.h"
#include "Planet/State/AstraeonRuntimeStateManager.h"
#include "AstraeonGameInstance.h"
#include "Creatures/AstraeonCreatureActor.h"
#include "Planet/Surface/AstraeonPlanetSurface.h"
#include "Components/SceneComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Camera/PlayerCameraManager.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

namespace
{
	// Provisional, measured in TL_12: selection cadence and uploads per frame.
	constexpr float SelectionIntervalSeconds = 0.1f;
	constexpr int32 MaxCommitsPerTick = 2;
	constexpr int32 MaxBuilderQuads = 128;
	// The ring is recomputed only after this much movement: it is 40 m wide.
	constexpr double RingRecenterCm = 100.0;
}

AAstraeonPlanetRuntime::AAstraeonPlanetRuntime()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("PlanetRoot")));
	for (int32 I=0; I<6; ++I)
	{
		auto* Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(*FString::Printf(TEXT("Face%d"), I));
		Mesh->SetupAttachment(RootComponent);
		Mesh->SetCollisionProfileName(TEXT("NoCollision"));
		Faces.Add(Mesh);
	}
}

FAstraeonPlanetDefinition AAstraeonPlanetRuntime::GetDefinition() const
{
	FAstraeonPlanetDefinition P;
	P.BodyId = TEXT("planet_cube_sphere_lab"); P.RadiusCm=RadiusCm;
	P.SurfaceGravityMS2=GravityMS2;
	// Lab mass is derived to avoid contradictory mass/gravity data. Not Khepri canon.
	P.MassKg = GravityMS2*FMath::Square(RadiusCm/100.0)/6.67430e-11;
	P.BodySeed=BodySeed; P.WorldSeed=BodySeed;
	return P;
}

void AAstraeonPlanetRuntime::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

bool AAstraeonPlanetRuntime::Rebuild()
{
	const auto P=GetDefinition();
	if (!P.IsValid() || FaceQuads<4 || FaceQuads>MaxBuilderQuads || !FMath::IsPowerOfTwo(FaceQuads)
		|| !GetActorScale3D().Equals(FVector::OneVector) || !GetActorQuat().Equals(FQuat::Identity))
	{
		UE_LOG(LogTemp, Error, TEXT("PlanetRuntime: invalid definition/grid/transform; radius is data, body axes fixed"));
		return false;
	}
	FaceData.SetNum(6);
	for (int32 I=0; I<6; ++I)
	{
		if (!FAstraeonCubeSphereMesh::BuildFace(P, EAstraeonPlanetFace(I), FaceQuads, FaceData[I])) return false;
		// The bootstrap image: retired in the frame the first patch cover shows.
		const auto& D=FaceData[I];
		Faces[I]->SetRelativeLocation(D.OriginBodyCm);
		Faces[I]->CreateMeshSection(0,D.Vertices,D.Indices,D.Normals,D.UVs,TArray<FColor>(),TArray<FProcMeshTangent>(),false);
		if (SurfaceMaterial) Faces[I]->SetMaterial(0,SurfaceMaterial);
	}
	bFacesRetired=false;
	// A different definition invalidates every collision patch; the ring rebuilds on demand.
	ResetCollision();
	return true;
}

FAstraeonPlanetPatchBuildOptions AAstraeonPlanetRuntime::CollisionOptions() const
{
	// Same grid as the rendered patches; no skirts, which are visual cover and never ground.
	FAstraeonPlanetPatchBuildOptions Options;
	Options.Quads=LODSettings.Quads; Options.SkirtDepthCm=0.0;
	return Options;
}

void AAstraeonPlanetRuntime::CommitCollision(const FAstraeonPlanetPatchBuildResult& Mesh)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Astraeon_PlanetCollision_Commit);
	if (CollisionLive.Contains(Mesh.Address)) return;
	UProceduralMeshComponent* Component = CollisionFree.IsEmpty() ? nullptr : CollisionFree.Pop(EAllowShrinking::No);
	if (!Component)
	{
		Component=NewObject<UProceduralMeshComponent>(this);
		Component->SetupAttachment(RootComponent);
		Component->SetCollisionProfileName(TEXT("BlockAll"));
		Component->SetVisibility(false);
		Component->SetCanEverAffectNavigation(false);
		Component->bUseAsyncCooking=false; // Built ahead of the player: the cook is ready on commit.
		Component->RegisterComponent();
		CollisionComponents.Add(Component);
	}
	Component->SetRelativeLocation(Mesh.OriginBodyCm);
	Component->CreateMeshSection(0,Mesh.Vertices,Mesh.Indices,TArray<FVector>(),TArray<FVector2D>(),
		TArray<FColor>(),TArray<FProcMeshTangent>(),true);
	Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionLive.Add(Mesh.Address,Component);
	CollisionTriangles+=Mesh.Indices.Num()/3;
	++CollisionRebuilds;
}

void AAstraeonPlanetRuntime::RemoveCollision(const FAddress& Address)
{
	UProceduralMeshComponent* Component=nullptr;
	if (!CollisionLive.RemoveAndCopyValue(Address,Component)) return;
	CollisionTriangles-=Component->GetProcMeshSection(0) ? Component->GetProcMeshSection(0)->ProcIndexBuffer.Num()/3 : 0;
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->ClearAllMeshSections();
	CollisionFree.Add(Component);
}

void AAstraeonPlanetRuntime::ResetCollision()
{
	TArray<FAddress> Live;
	CollisionLive.GetKeys(Live);
	for (const auto& A:Live) RemoveCollision(A);
	for (const auto& Item:CollisionPending) if (CollisionStreaming) CollisionStreaming->Release(Item.Key);
	CollisionPending.Reset(); CollisionWant.Reset(); CollisionKeep.Reset();
	CollisionRingCenter=FVector::ZeroVector; CollisionTriangles=0;
}

bool AAstraeonPlanetRuntime::BuildCollisionNow(const FAddress& Address)
{
	if (CollisionLive.Contains(Address)) return true;
	if (CollisionStreaming && CollisionPending.Remove(Address)) CollisionStreaming->Release(Address);
	FAstraeonPlanetPatchBuildResult Mesh;
	if (FAstraeonPlanetPatchMesh::Build(GetDefinition(),Address,1,CollisionOptions(),Mesh)!=EAstraeonPatchBuildStatus::Success)
	{
		UE_LOG(LogTemp,Error,TEXT("PlanetRuntime: collision patch failed to build; the player has no ground here"));
		return false;
	}
	CommitCollision(Mesh);
	return true;
}

bool AAstraeonPlanetRuntime::PrepareCollision(const FVector& Direction, bool bForce)
{
	if (!bNearCollision) return false;
	const auto P=GetDefinition();
	const uint8 Finest=FAstraeonPlanetLODManager::FinestAllowedLod(P,LODSettings);
	TArray<FAddress> Ring;
	if (!FAstraeonPlanetCollisionRing::Select(P,Finest,Direction,FAstraeonPlanetCollisionRing::RadiusCm,Ring)) return false;
	bool bUnder=BuildCollisionNow(Ring[0]);
	if (bForce) for (int32 I=1; I<Ring.Num(); ++I) BuildCollisionNow(Ring[I]);
	CollisionRingCenter=FVector::ZeroVector; // Force the next tick to recompute around the new spot.
	return bUnder;
}

void AAstraeonPlanetRuntime::UpdateCollision(const FVector& PawnBodyCm)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Astraeon_PlanetCollision_Update);
	const auto P=GetDefinition();
	const uint8 Finest=FAstraeonPlanetLODManager::FinestAllowedLod(P,LODSettings);
	FVector Direction;
	if (!FAstraeonPlanetCoordinates::TryNormalizeDirection(PawnBodyCm,Direction)) return;
	LastPawnDirection=Direction;
	if (CollisionRingCenter.IsZero() || FVector::DistSquared(Direction,CollisionRingCenter)*FMath::Square(RadiusCm)>FMath::Square(RingRecenterCm))
	{
		CollisionRingCenter=Direction;
		FAstraeonPlanetCollisionRing::Select(P,Finest,Direction,FAstraeonPlanetCollisionRing::RadiusCm,CollisionWant);
		FAstraeonPlanetCollisionRing::Select(P,Finest,Direction,FAstraeonPlanetCollisionRing::KeepRadiusCm,CollisionKeep);
	}
	const TSet<FAddress> Keep(CollisionKeep);
	// Results from workers: only what is still wanted becomes ground.
	FAstraeonPlanetPatchCompletion Done;
	while (CollisionStreaming->TryTakeCompleted(Done))
	{
		const uint64* Pending=CollisionPending.Find(Done.Address);
		if (!Pending || *Pending!=Done.BuildRevision || !CollisionStreaming->IsCurrent(Done.Address,Done.BuildRevision)) continue;
		CollisionStreaming->Release(Done.Address);
		CollisionPending.Remove(Done.Address);
		if (Done.Status==EAstraeonPatchBuildStatus::Success && Keep.Contains(Done.Address)) CommitCollision(Done.Mesh);
		else if (Done.Status!=EAstraeonPatchBuildStatus::Success)
			UE_LOG(LogTemp,Error,TEXT("PlanetRuntime: collision patch build failed (status %d)"),int32(Done.Status));
	}
	// The patch under the player has ground now, never later. Not being ready is counted.
	FAddress Under;
	if (FAstraeonPlanetPatchAddress::TryFromDirection(P.BodyId,Direction,Finest,Under) && !CollisionLive.Contains(Under))
	{
		++CollisionEmergencyBuilds;
		if (!BuildCollisionNow(Under)) ++CollisionMissingFrames;
	}
	for (const auto& A:CollisionWant)
	{
		if (CollisionLive.Contains(A) || CollisionPending.Contains(A)) continue;
		uint64 Revision=0;
		const auto Status=CollisionStreaming->Request(P,A,CollisionOptions(),Revision);
		if (Status==EAstraeonPatchRequestStatus::AtCapacity) break;
		if (Status==EAstraeonPatchRequestStatus::Accepted) CollisionPending.Add(A,Revision);
		else UE_LOG(LogTemp,Error,TEXT("PlanetRuntime: collision request rejected (status %d)"),int32(Status));
	}
	// Leaving the keep radius frees the patch, unless something still stands on it.
	TArray<FAddress> Leaving;
	for (const auto& Item:CollisionLive) if (!Keep.Contains(Item.Key)) Leaving.Add(Item.Key);
	for (const auto& A:Leaving)
	{
		bool bStoodOn=false;
		for (TActorIterator<ACharacter> It(GetWorld()); It && !bStoodOn; ++It) bStoodOn=It->GetMovementBase()==CollisionLive[A];
		if (!bStoodOn) RemoveCollision(A);
	}
	TArray<FAddress> Stale;
	for (const auto& Item:CollisionPending) if (!Keep.Contains(Item.Key)) Stale.Add(Item.Key);
	for (const auto& A:Stale) { CollisionStreaming->Release(A); CollisionPending.Remove(A); }
}

FVector AAstraeonPlanetRuntime::GetSurfacePointCm(FVector Direction, double AltitudeCm) const
{
	const FVector Unit=Direction.GetSafeNormal();
	return GetActorLocation()+Unit*(RadiusCm+FAstraeonPlanetSurface::SampleRadialHeightCm(GetDefinition(),Unit)+AltitudeCm);
}

void AAstraeonPlanetRuntime::BeginPlay()
{
	Super::BeginPlay();
	// Global KillZ is not a valid boundary on a sphere: it deletes the southern hemisphere.
	GetWorld()->GetWorldSettings()->bEnableWorldBoundsChecks=false;
	// Overrides are test-only, documented and logged; same map proves radius is a datum.
	FParse::Value(FCommandLine::Get(),TEXT("AstraeonPlanetRadiusCm="),RadiusCm);
	Streaming=MakeUnique<FAstraeonPlanetStreamingManager>();
	Backend=MakeUnique<FAstraeonPlanetProceduralPatchBackend>(*GetRootComponent(),SurfaceMaterial);
	Patches=MakeUnique<FAstraeonPlanetPatchManager>(*Streaming,*Backend);
	if (bNearCollision) CollisionStreaming=MakeUnique<FAstraeonPlanetStreamingManager>();
	bSpawnFauna|=FParse::Param(FCommandLine::Get(),TEXT("AstraeonPlanetFauna"));
	if (!Rebuild()) return;
	UE_LOG(LogTemp,Display,TEXT("PlanetRuntime: Ready radius_cm=%.0f seed=%d finest_lod=%d near_collision=%d"),
		RadiusCm,BodySeed,FAstraeonPlanetLODManager::FinestAllowedLod(GetDefinition(),LODSettings),bNearCollision);
}

void AAstraeonPlanetRuntime::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Patches.IsValid())
	{
		const auto& S=Patches->GetStats();
		UE_LOG(LogTemp,Display,TEXT("PlanetRuntime: patches requests=%d commits=%d relays=%d stale=%d failures=%d cancels=%d visible=%d components=%d ground_mismatch=%d/%d work_ms mean=%.3f max=%.2f over_%.0fms=%d"),
			S.Requests,S.Commits,S.Relays,S.StaleDrops,S.Failures,S.Cancels,Patches->GetVisibleCount(),Backend->GetComponentCount(),
			GroundMismatchFrames,GroundCheckedFrames,GetMeanPatchWorkMs(),MaxPatchWorkMs,PatchWorkBudgetMs,PatchWorkFramesOverBudget);
		UE_LOG(LogTemp,Display,TEXT("PlanetRuntime: collision patches built=%d emergency=%d missing_frames=%d live=%d"),
			CollisionRebuilds,CollisionEmergencyBuilds,CollisionMissingFrames,CollisionLive.Num());
		// The manager releases through the queue and backend, so it goes first.
		Patches->Reset();
	}
	ResetCollision();
	Patches.Reset(); Backend.Reset(); Streaming.Reset(); CollisionStreaming.Reset();
	Super::EndPlay(Reason);
}

void AAstraeonPlanetRuntime::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const double Start=FPlatformTime::Seconds();
	const auto* PC=GetWorld()->GetFirstPlayerController();
	if (PC && PC->GetPawn())
	{
		if (auto* Character=Cast<ACharacter>(PC->GetPawn()))
			Character->GetCharacterMovement()->AddTickPrerequisiteActor(this);
		if (bNearCollision && CollisionStreaming) UpdateCollision(PC->GetPawn()->GetActorLocation()-GetActorLocation());
		// Creatures need ground: the LOD-only lab (TL_12) has none, so it has no fauna either.
		if (bNearCollision && bSpawnFauna) UpdateEntities(DeltaSeconds,PC->GetPawn()->GetActorLocation()-GetActorLocation());
	}
	if (Patches.IsValid()) UpdatePatches(DeltaSeconds);
	const double Ms=(FPlatformTime::Seconds()-Start)*1000.0;
	MaxPatchWorkMs=FMath::Max(MaxPatchWorkMs,Ms); SumPatchWorkMs+=Ms; ++PatchWorkFrames;
	PatchWorkFramesOverBudget+=Ms>PatchWorkBudgetMs;
}

int32 AAstraeonPlanetRuntime::GetEntityActorCount() const
{
	int32 Count=0;
	for (const auto& Item:EntityActors) Count+=Item.Value.Actor.IsValid();
	return Count;
}

AAstraeonCreatureActor* AAstraeonPlanetRuntime::FindEntityActor(FName EntityId) const
{
	const FEntityActor* Found=EntityActors.Find(EntityId);
	return Found ? Found->Actor.Get() : nullptr;
}

void AAstraeonPlanetRuntime::UpdateEntities(float DeltaSeconds, const FVector& PawnBodyCm)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Astraeon_PlanetEntities_Update);
	UAstraeonGameInstance* Game=GetGameInstance<UAstraeonGameInstance>();
	SinceEntityUpdate+=DeltaSeconds;
	// A menu is not a session: nothing lives on the planet until one starts.
	if (!Game || !Game->HasStartedGame() || SinceEntityUpdate<0.25f) return;
	SinceEntityUpdate=0.f;
	const auto P=GetDefinition();
	TArray<FAddress> Load, Keep;
	if (!FAstraeonPlanetEntities::CellsNear(P,PawnBodyCm,EntityRadiusCm,Load)
		|| !FAstraeonPlanetEntities::CellsNear(P,PawnBodyCm,EntityKeepRadiusCm,Keep)) return;
	const UAstraeonRuntimeStateManager* State=Game->GetPlanetState();
	// Leaving: out of the keep ring, or gone (a corpse removes itself).
	const TSet<FAddress> KeepSet(Keep);
	for (auto It=EntityActors.CreateIterator(); It; ++It)
	{
		AAstraeonCreatureActor* Actor=It->Value.Actor.Get();
		if (!Actor) { It.RemoveCurrent(); continue; }
		if (!KeepSet.Contains(It->Value.Cell) && !Actor->IsDead())
		{
			Actor->Destroy(); ++EntityDespawns; It.RemoveCurrent();
		}
	}
	// Arriving: what the seed places here, minus what the player changed.
	TArray<FAstraeonPlanetEntitySpawn> Spawns;
	for (const FAddress& Cell:Load)
	{
		FAstraeonPlanetEntities::CreaturesInCell(P,Cell,Spawns);
		for (const FAstraeonPlanetEntitySpawn& Spawn:Spawns)
		{
			if (EntityActors.Contains(Spawn.EntityId) || State->IsDefeated(Spawn.EntityId)) continue;
			const FVector Up=Spawn.Direction;
			const FVector Forward=FVector::CrossProduct(Up,FMath::Abs(Up.Z)<0.9 ? FVector(0,0,1) : FVector(1,0,0)).GetSafeNormal();
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AAstraeonCreatureActor* Creature=GetWorld()->SpawnActor<AAstraeonCreatureActor>(AAstraeonCreatureActor::StaticClass(),
				GetSurfacePointCm(Up,0.0),FRotationMatrix::MakeFromXZ(Forward,Up).Rotator(),Params);
			if (!Creature) continue;
			Creature->SetPlanetaryIdentity(Spawn.EntityId,Up);
			EntityActors.Add(Spawn.EntityId,{Creature,Cell});
			++EntitySpawns;
		}
	}
}

FVector AAstraeonPlanetRuntime::ObserverBodyCm() const
{
	// Detail follows what is seen: the camera, which is the pawn's eyes in play and a scripted
	// camera in TL_12. Its location is last frame's, one frame of lag the prediction absorbs.
	// A camera inside the planet (or none) means nobody looks at the surface yet.
	const auto* PC=GetWorld()->GetFirstPlayerController();
	const FVector Camera = PC && PC->PlayerCameraManager
		? PC->PlayerCameraManager->GetCameraLocation()-GetActorLocation()
		: FVector::ZeroVector;
	if (Camera.Size()>=RadiusCm*0.5) return Camera;
	if (PC && PC->GetPawn()) return PC->GetPawn()->GetActorLocation()-GetActorLocation();
	return LastPawnDirection*(RadiusCm+1000.0);
}

void AAstraeonPlanetRuntime::UpdatePatches(float DeltaSeconds)
{
	const auto P=GetDefinition();
	const FVector Observer=ObserverBodyCm();
	SinceSelection+=DeltaSeconds;
	if (Patches->GetTarget().IsEmpty() || SinceSelection>=SelectionIntervalSeconds)
	{
		// Velocity over the whole interval, not one frame: steadier, and any source of motion.
		FAstraeonPlanetLODView View; View.ObserverBodyCm=Observer;
		View.VelocityBodyCmS = Patches->GetTarget().IsEmpty() ? FVector::ZeroVector : (Observer-ObserverAtSelection)/SinceSelection;
		SinceSelection=0.f; ObserverAtSelection=Observer;
		FAstraeonPlanetLODSelection Selection;
		if (!FAstraeonPlanetLODManager::Select(P,View,LODSettings,Selection)
			|| !Patches->SetTarget(P,Selection.Leaves,LODSettings.Quads,Observer))
			UE_LOG(LogTemp,Error,TEXT("PlanetRuntime: LOD selection rejected at observer=%s"),*Observer.ToString());
	}
	Patches->Update(MaxCommitsPerTick);
	if (!bFacesRetired && Patches->GetVisibleCount()>0)
	{
		for (UProceduralMeshComponent* Face:Faces) Face->ClearAllMeshSections();
		bFacesRetired=true;
		UE_LOG(LogTemp,Display,TEXT("PlanetRuntime: first patch cover visible=%d; fixed faces retired"),Patches->GetVisibleCount());
	}
	const auto* PC=GetWorld()->GetFirstPlayerController();
	if (bFacesRetired && bNearCollision && PC && PC->GetPawn())
	{
		FAstraeonPlanetPatchAddress Under;
		if (FAstraeonPlanetPatchAddress::TryFromDirection(P.BodyId,PC->GetPawn()->GetActorLocation()-GetActorLocation(),
			FAstraeonPlanetLODManager::FinestAllowedLod(P,LODSettings),Under))
		{
			++GroundCheckedFrames;
			if (!Patches->IsVisible(Under)) ++GroundMismatchFrames;
		}
	}
}

AAstraeonPlanetRuntime* AAstraeonPlanetRuntime::FindActive(const UWorld* World)
{
	if (!World) return nullptr;
	for (TActorIterator<AAstraeonPlanetRuntime> It(World); It; ++It) return *It;
	return nullptr;
}
