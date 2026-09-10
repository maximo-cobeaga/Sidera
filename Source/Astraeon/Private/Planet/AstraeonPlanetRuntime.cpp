#include "Planet/AstraeonPlanetRuntime.h"
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
	// Dos búferes de colisión, no uno. Ver `PrepareCollision`: el relevo se construye antes de
	// retirar el que sostiene al jugador.
	for (int32 I=0; I<2; ++I)
	{
		auto* Near = CreateDefaultSubobject<UProceduralMeshComponent>(
			I==0 ? TEXT("NearCollision") : TEXT("NearCollisionRelay"));
		Near->SetupAttachment(RootComponent);
		Near->SetCollisionProfileName(TEXT("BlockAll"));
		Near->SetVisibility(false);
		Near->bUseAsyncCooking = false; // tiny bounded proof; worker pipeline belongs to Phase 2
		if (I==0) NearCollision = Near; else NearCollisionRelay = Near;
	}
	NearCollisionRelay->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
	if (!GetDefinition().IsValid() || FaceQuads<4 || FaceQuads>128 || !FMath::IsPowerOfTwo(FaceQuads)
		|| !GetActorScale3D().Equals(FVector::OneVector) || !GetActorQuat().Equals(FQuat::Identity))
	{
		UE_LOG(LogTemp, Error, TEXT("PlanetRuntime: invalid definition/grid/transform; radius is data, body axes fixed"));
		return false;
	}
	FaceData.SetNum(6);
	for (int32 I=0; I<6; ++I)
	{
		if (!FAstraeonCubeSphereMesh::BuildFace(GetDefinition(), EAstraeonPlanetFace(I), FaceQuads, FaceData[I])) return false;
		const auto& D=FaceData[I];
		Faces[I]->SetRelativeLocation(D.OriginBodyCm);
		Faces[I]->CreateMeshSection(0,D.Vertices,D.Indices,D.Normals,D.UVs,TArray<FColor>(),TArray<FProcMeshTangent>(),false);
		if (SurfaceMaterial) Faces[I]->SetMaterial(0,SurfaceMaterial);
	}
	return PrepareCollision(FVector(0,0,1),true);
}

bool AAstraeonPlanetRuntime::PrepareCollision(const FVector& Direction, bool bForce)
{
	FVector Unit;
	if (FaceData.Num()!=6 || !FAstraeonPlanetCoordinates::TryNormalizeDirection(Direction,Unit)) return false;
	if (!bForce && FVector::DotProduct(Unit,CollisionDirection)>FMath::Cos(0.5/FaceQuads)) return true;
	// Select nearby cells from all faces: seam crossings include both neighbours.
	// Collision uses EXACTLY the rendered triangles, with an origin close to the player.
	const FVector Origin=Unit*RadiusCm;
	TArray<FVector> V;
	TArray<int32> Indices;
	const double MinDot=FMath::Cos(6.0/FaceQuads);
	for (const auto& D:FaceData)
	for (int32 Cell=0; Cell<D.Indices.Num(); Cell+=6)
	{
		FVector Center=FVector::ZeroVector;
		for (int32 J=0; J<6; ++J) Center+=D.Vertices[D.Indices[Cell+J]]+D.OriginBodyCm;
		if (FVector::DotProduct(Center.GetSafeNormal(),Unit)<MinDot) continue;
		for (int32 J=0; J<6; ++J)
		{
			Indices.Add(V.Num());
			V.Add(D.Vertices[D.Indices[Cell+J]]+D.OriginBodyCm-Origin);
		}
	}
	if (V.IsEmpty()) return false;
	// Re-centering collision preserves world triangles; it is NOT a moving platform.
	//
	// Con un solo componente esto costaba dos frames sin suelo bajo el jugador: había que
	// despegarlo (`SetBase(nullptr)`), mover el componente y recocer su cuerpo físico en el
	// sitio. Medido el 2026-09-10 sobre 40 s de caminata: 55 reconstrucciones y 53 caídas de
	// la velocidad a 0, que el selector de animación leía como `Idle` y le reiniciaban el
	// ciclo de paso al jugador ~1,3 veces por segundo.
	//
	// Con dos búferes el relevo se construye completo y se activa ANTES de retirar el
	// saliente, y el personaje se pasa de uno a otro en vez de quedarse sin base.
	UProceduralMeshComponent* Incoming = ActiveCollisionBuffer==0 ? NearCollisionRelay : NearCollision;
	UProceduralMeshComponent* Outgoing = ActiveCollisionBuffer==0 ? NearCollision : NearCollisionRelay;
	Incoming->SetRelativeLocation(Origin);
	Incoming->CreateMeshSection(0,V,Indices,TArray<FVector>(),TArray<FVector2D>(),TArray<FColor>(),TArray<FProcMeshTangent>(),true);
	Incoming->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
		if (It->GetMovementBase()==Outgoing) It->SetBase(Incoming);
	Outgoing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Outgoing->ClearAllMeshSections();
	ActiveCollisionBuffer = 1-ActiveCollisionBuffer;
	CollisionDirection=Unit; CollisionTriangles=Indices.Num()/3; ++CollisionRebuilds;
	return true;
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
	if (!Rebuild()) return;
	UE_LOG(LogTemp,Display,TEXT("PlanetRuntime: Ready radius_cm=%.0f seed=%d faces=6 triangles=%d collision=%d"),
		RadiusCm,BodySeed,6*FaceQuads*FaceQuads*2,CollisionTriangles);
}

void AAstraeonPlanetRuntime::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const auto* PC=GetWorld()->GetFirstPlayerController();
	if (PC && PC->GetPawn())
	{
		if (auto* Character=Cast<ACharacter>(PC->GetPawn()))
			Character->GetCharacterMovement()->AddTickPrerequisiteActor(this);
		PrepareCollision(PC->GetPawn()->GetActorLocation()-GetActorLocation());
	}
}

AAstraeonPlanetRuntime* AAstraeonPlanetRuntime::FindActive(const UWorld* World)
{
	if (!World) return nullptr;
	for (TActorIterator<AAstraeonPlanetRuntime> It(World); It; ++It) return *It;
	return nullptr;
}
