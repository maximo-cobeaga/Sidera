#include "Planet/Gravity/AstraeonPlanetGravityHarness.h"

#include "AstraeonDiagnostics.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace AstraeonPlanetHarness
{
	// La esfera básica del motor mide 100 cm de diámetro, es decir 50 cm de radio.
	constexpr double EngineSphereRadiusCm = 50.0;
}

AAstraeonPlanetGravityHarness::AAstraeonPlanetGravityHarness()
{
	PrimaryActorTick.bCanEverTick = false;

	// La raíz es una escena vacía y el visual cuelga de ella. Con la malla como raíz, el centro
	// del planeta y la escala de la esfera quedan acoplados: mover o escalar una cosa cambiaría
	// la otra, y `GetPlanetCenterCm()` dejaría de significar lo que dice.
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SphereMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SphereMesh"));
	SphereMesh->SetupAttachment(SceneRoot);

	SphereMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SphereMesh->SetCollisionProfileName(TEXT("BlockAll"));
	// Movable y no Static: el radio es un dato editable y reescalar un componente estático fuera
	// de la construcción es un error del motor, no una advertencia.
	SphereMesh->SetMobility(EComponentMobility::Movable);
}

void AAstraeonPlanetGravityHarness::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyRadiusToMesh();
}

void AAstraeonPlanetGravityHarness::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogAstraeonDiag, Log, TEXT("[Planet] Harness activo: centro=%s radio=%.0f cm (%.1f km) g=%.2f m/s2"),
		*GetPlanetCenterCm().ToString(), PlanetRadiusCm, PlanetRadiusCm / 100000.0, SurfaceGravityMS2);
}

void AAstraeonPlanetGravityHarness::ApplyRadiusToMesh()
{
	if (!SphereMesh)
	{
		return;
	}

	// La malla se carga aquí y no con ConstructorHelpers: cargarla en el constructor la deja
	// asignada en el CDO, y el editor calcula la colocación de un actor nuevo a partir de los
	// bounds del CDO. Con una esfera de kilómetros ahí dentro, esa colocación revienta.
	if (!SphereMesh->GetStaticMesh())
	{
		if (UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")))
		{
			SphereMesh->SetStaticMesh(Sphere);
		}
	}

	// Relativa, no mundial: la malla cuelga de la raíz y su escala no debe depender de dónde
	// esté el actor.
	const double Scale = FMath::Max(PlanetRadiusCm, 1.0) / AstraeonPlanetHarness::EngineSphereRadiusCm;
	SphereMesh->SetRelativeScale3D(FVector(Scale));
}

FVector AAstraeonPlanetGravityHarness::GetSurfacePointCm(const FVector& DirectionFromCenter, double HeightAboveSurfaceCm) const
{
	const FVector Direction = DirectionFromCenter.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return GetPlanetCenterCm();
	}

	return GetPlanetCenterCm() + Direction * (PlanetRadiusCm + HeightAboveSurfaceCm);
}

AAstraeonPlanetGravityHarness* AAstraeonPlanetGravityHarness::FindActiveHarness(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	AAstraeonPlanetGravityHarness* Found = nullptr;
	int32 Count = 0;

	for (TActorIterator<AAstraeonPlanetGravityHarness> It(World); It; ++It)
	{
		if (!Found)
		{
			Found = *It;
		}
		++Count;
	}

	// Varios cuerpos gravitatorios activos a la vez es exactamente lo que §4.9 prohíbe para el
	// gameplay cercano. Se avisa en vez de elegir en silencio, que es como se descubre tarde.
	if (Count > 1)
	{
		UE_LOG(LogAstraeonDiag, Warning,
			TEXT("[Planet] %d harness en el mundo; el gameplay cercano debe usar un solo cuerpo. Se usa '%s'."),
			Count, *Found->GetName());
	}

	return Found;
}
