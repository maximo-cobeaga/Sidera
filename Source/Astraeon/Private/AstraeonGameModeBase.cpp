#include "AstraeonGameModeBase.h"

#include "AstraeonGameInstance.h"
#include "AstraeonHUD.h"
#include "AstraeonPlayerCharacter.h"
#include "AstraeonPlayerController.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Building/AstraeonBuiltStructure.h"
#include "Creatures/AstraeonCreatureActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Tests/AstraeonItacaInputSmoke.h"
#include "Tests/AstraeonRegionArtSmoke.h"
#include "Environment/AstraeonItacaInterior.h"
#include "WorldGen/AstraeonRegionMarker.h"
#include "WorldGen/AstraeonRegionMaterializer.h"
#include "WorldGen/AstraeonTerrainField.h"
#include "WorldGen/AstraeonTerrainSurfacePrototype.h"
#include "WorldGen/AstraeonWorldProfiles.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AAstraeonGameModeBase::AAstraeonGameModeBase()
{
	// Tick lento: sólo lo usa el reloj de repoblado de fauna, no hace falta por frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f;

	DefaultPawnClass = AAstraeonPlayerCharacter::StaticClass();
	PlayerControllerClass = AAstraeonPlayerController::StaticClass();
	HUDClass = AAstraeonHUD::StaticClass();
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Geology(TEXT("/Game/Astraeon/Art/ItacaTerrain/M_Terrain_Geology.M_Terrain_Geology"));
	TerrainGeologyMaterial = Geology.Object;
}

void AAstraeonGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	if (GEngine)
	{
		GEngine->Exec(GetWorld(), TEXT("DisableAllScreenMessages"), *GLog);
	}

	TArray<AActor*> Interiors;
	UGameplayStatics::GetAllActorsOfClass(this, AAstraeonItacaInterior::StaticClass(), Interiors);
	if (Interiors.IsEmpty()) GetWorld()->SpawnActor<AAstraeonItacaInterior>();
	EnsureRuntimeLighting();
	if (FParse::Param(FCommandLine::Get(), TEXT("AstraeonSmokeRegionArt")))
	{
		GetWorld()->SpawnActor<AAstraeonRegionArtSmoke>();
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("AstraeonSmokeItacaInput")))
	{
		GetWorld()->SpawnActor<AAstraeonItacaInputSmoke>();
	}

	// The MVP starts at a minimal C++ menu. Region materialization happens after
	// StartSelectedNewGame or ContinueSavedGame in AAstraeonPlayerController.
	RunCriticalPathSmokeIfRequested();
}

void AAstraeonGameModeBase::RunCriticalPathSmokeIfRequested()
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("AstraeonAutoSmokeCriticalPath")))
	{
		return;
	}

	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	if (!AstraeonGameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("AstraeonCriticalPathSmoke: MissingGameInstance"));
		FPlatformMisc::RequestExit(false);
		return;
	}

	int32 RequestedSeed = 0;
	FParse::Value(FCommandLine::Get(), TEXT("AstraeonSeed="), RequestedSeed);
	AstraeonGameInstance->StartNewGame(RequestedSeed);
	MaterializeCurrentRegion();

	AstraeonGameInstance->RecordArgosBriefing();
	const bool bDeployed = RunSurfaceHatchInteractionSmoke();
	const bool bScanned = AstraeonGameInstance->ScanCurrentEnvironment();
	const FAstraeonRegionLayout& Layout = AstraeonGameInstance->GetCurrentRegionLayout();
	for (const FAstraeonResourceNode& Resource : Layout.Resources)
	{
		AstraeonGameInstance->AddInventoryItem(Resource.ResourceId, 1);
	}
	const bool bCrafted = AstraeonGameInstance->CraftSignalResonator();
	const bool bResolved = AstraeonGameInstance->TryResolveSignalSource();
	const FString SlotName(TEXT("AstraeonAutoSmokeCriticalPath"));
	UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	const bool bSaved = AstraeonGameInstance->SaveCurrentGame(SlotName, 0);

	UAstraeonGameInstance* LoadedGame = NewObject<UAstraeonGameInstance>();
	const bool bLoaded = LoadedGame->LoadSavedGame(SlotName, 0);
	const bool bLoadedResolved = bLoaded && LoadedGame->IsSignalResolved();
	UGameplayStatics::DeleteGameInSlot(SlotName, 0);

	UE_LOG(LogTemp, Display, TEXT("AstraeonCriticalPathSmoke: Deployed=%s Scanned=%s Crafted=%s Resolved=%s Saved=%s Loaded=%s LoadedResolved=%s Seed=%d"),
		bDeployed ? TEXT("true") : TEXT("false"),
		bScanned ? TEXT("true") : TEXT("false"),
		bCrafted ? TEXT("true") : TEXT("false"),
		bResolved ? TEXT("true") : TEXT("false"),
		bSaved ? TEXT("true") : TEXT("false"),
		bLoaded ? TEXT("true") : TEXT("false"),
		bLoadedResolved ? TEXT("true") : TEXT("false"),
		AstraeonGameInstance->GetCurrentWorldSeed());

	FPlatformMisc::RequestExit(!(bDeployed && bScanned && bCrafted && bResolved && bSaved && bLoadedResolved));
}

bool AAstraeonGameModeBase::RunSurfaceHatchInteractionSmoke()
{
	UWorld* World = GetWorld();
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	AAstraeonPlayerCharacter* PlayerCharacter = Cast<AAstraeonPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!World || !AstraeonGameInstance || !PlayerCharacter || !PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("AstraeonCriticalPathSmoke: SurfaceHatchInteractionUnavailable"));
		return false;
	}

	// Stand close enough for the proximity fallback, but aim horizontally above the
	// short temporary hatch cube. This reproduces the manual failure mode where a
	// point line trace can miss even though the player is clearly beside the hatch.
	const FVector InteractionLocationCm(360.0f, 0.0f, 120.0f);
	PlayerCharacter->SetActorLocation(InteractionLocationCm, false, nullptr, ETeleportType::TeleportPhysics);
	PlayerController->SetControlRotation(FRotator::ZeroRotator);
	PlayerCharacter->Interact();

	const bool bRecordedDeployment = AstraeonGameInstance->GetRuntimeLogbookEntries().ContainsByPredicate([](const FAstraeonLogbookEntry& Entry)
	{
		return Entry.EntryId == TEXT("itaca.surface_deployment");
	});
	const FVector DeploymentLocationCm = UAstraeonRegionMaterializer::GetSurfaceDeploymentLocationCm();
	const FVector PlayerLocationCm = PlayerCharacter->GetActorLocation();
	const bool bReachedSurfaceXY = FVector2D(PlayerLocationCm.X, PlayerLocationCm.Y).Equals(FVector2D(DeploymentLocationCm.X, DeploymentLocationCm.Y), 1.0f);

	FHitResult FloorHit;
	FCollisionQueryParams FloorQueryParams(SCENE_QUERY_STAT(AstraeonSurfaceHatchSmokeFloor), false, PlayerCharacter);
	const FVector FloorTraceStart = PlayerCharacter->GetActorLocation();
	const FVector FloorTraceEnd = FloorTraceStart - FVector(0.0f, 0.0f, 300.0f);
	const bool bHasBlockingFloor = World->LineTraceSingleByChannel(FloorHit, FloorTraceStart, FloorTraceEnd, ECC_Visibility, FloorQueryParams);
	const bool bStandingAboveFloor = bHasBlockingFloor && PlayerLocationCm.Z > FloorHit.ImpactPoint.Z;

	return bRecordedDeployment && bReachedSurfaceXY && bStandingAboveFloor;
}

void AAstraeonGameModeBase::EnsureRuntimeLighting()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ADirectionalLight* RuntimeSun = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(0.0f, 0.0f, 1200.0f), FRotator(-45.0f, -35.0f, 0.0f), SpawnParameters);
	if (RuntimeSun && RuntimeSun->GetLightComponent())
	{
		RuntimeSun->Tags.Add(TEXT("AstraeonRuntimeLighting"));
		RuntimeSun->SetMobility(EComponentMobility::Movable);
		RuntimeSun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
		RuntimeSun->GetLightComponent()->SetIntensity(8.0f);
		RuntimeSun->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.82f, 0.60f));
#if WITH_EDITOR
		RuntimeSun->SetActorLabel(TEXT("Runtime_Sun_KeyLight"));
#endif
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AstraeonLighting: Runtime directional light was not created."));
	}

	ASkyLight* RuntimeSky = World->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector(0.0f, 0.0f, 900.0f), FRotator::ZeroRotator, SpawnParameters);
	if (RuntimeSky && RuntimeSky->GetLightComponent())
	{
		RuntimeSky->Tags.Add(TEXT("AstraeonRuntimeLighting"));
		RuntimeSky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
		RuntimeSky->GetLightComponent()->SetIntensity(2.0f);
		RuntimeSky->GetLightComponent()->RecaptureSky();
#if WITH_EDITOR
		RuntimeSky->SetActorLabel(TEXT("Runtime_Sky_AmbientLight"));
#endif
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AstraeonLighting: Runtime sky light was not created."));
	}

	// Atmospheric haze: keeps the area immediately beyond the materialised surface boundary
	// visible as haze rather than the plain render background colour.
	AExponentialHeightFog* RuntimeFog = World->SpawnActor<AExponentialHeightFog>(AExponentialHeightFog::StaticClass(), FVector(0.0f, 0.0f, -100.0f), FRotator::ZeroRotator, SpawnParameters);
	if (RuntimeFog)
	{
		RuntimeFog->Tags.Add(TEXT("AstraeonRuntimeLighting"));
		if (UExponentialHeightFogComponent* FogComponent = RuntimeFog->GetComponent())
		{
			FogComponent->SetFogDensity(0.04f);
			FogComponent->SetStartDistance(800.0f);
			FogComponent->SetFogHeightFalloff(0.15f);
		}
#if WITH_EDITOR
		RuntimeFog->SetActorLabel(TEXT("Runtime_AtmosphericHaze"));
#endif
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AstraeonLighting: Runtime atmospheric fog was not created."));
	}
}

void AAstraeonGameModeBase::SetItacaInteriorHidden(bool bInteriorHidden)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> Interiors;
	UGameplayStatics::GetAllActorsOfClass(World, AAstraeonItacaInterior::StaticClass(), Interiors);
	for (AActor* Interior : Interiors)
	{
		Interior->SetActorHiddenInGame(bInteriorHidden);
		Interior->SetActorEnableCollision(!bInteriorHidden);
	}

	// Las consolas y la escotilla forman parte de la estancia: si quedaran visibles
	// flotando en el aire durante el vuelo, la nave parecería haber dejado piezas atrás.
	TArray<AActor*> Markers;
	UGameplayStatics::GetAllActorsOfClass(World, AAstraeonRegionMarker::StaticClass(), Markers);
	for (AActor* MarkerActor : Markers)
	{
		const AAstraeonRegionMarker* Marker = Cast<AAstraeonRegionMarker>(MarkerActor);
		if (!Marker)
		{
			continue;
		}

		// La lista enumeraba tres de las cuatro piezas y se olvidaba de la mesa de
		// fabricación, que se quedaba en el suelo del mapa mientras la nave volaba.
		// Preguntar por el conjunto de estaciones evita que la próxima que se añada
		// vuelva a quedarse afuera por omisión.
		const FName MarkerId = Marker->GetMarkerId();
		if (AAstraeonRegionMarker::BelongsToItacaInterior(MarkerId))
		{
			MarkerActor->SetActorHiddenInGame(bInteriorHidden);
			MarkerActor->SetActorEnableCollision(!bInteriorHidden);
		}
	}
}

void AAstraeonGameModeBase::MaterializeCurrentRegion()
{
	UWorld* World = GetWorld();
	const UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	if (!World || !AstraeonGameInstance || !AstraeonGameInstance->HasStartedGame())
	{
		return;
	}

	TArray<AActor*> ExistingMarkers;
	UGameplayStatics::GetAllActorsOfClass(World, AAstraeonRegionMarker::StaticClass(), ExistingMarkers);
	for (AActor* ExistingMarker : ExistingMarkers)
	{
		ExistingMarker->Destroy();
	}

	// Las obras se vuelven a instanciar desde el SaveGame más abajo; sin este barrido se
	// duplicarían en cada aterrizaje.
	TArray<AActor*> ExistingStructures;
	UGameplayStatics::GetAllActorsOfClass(World, AAstraeonBuiltStructure::StaticClass(), ExistingStructures);
	for (AActor* ExistingStructure : ExistingStructures)
	{
		ExistingStructure->Destroy();
	}

	TArray<AActor*> ExistingCreatures;
	UGameplayStatics::GetAllActorsOfClass(World, AAstraeonCreatureActor::StaticClass(), ExistingCreatures);
	for (AActor* ExistingCreature : ExistingCreatures)
	{
		ExistingCreature->Destroy();
	}

	TArray<AActor*> ExistingRuntimeSurfaces;
	UGameplayStatics::GetAllActorsWithTag(World, TEXT("AstraeonRuntimeSurface"), ExistingRuntimeSurfaces);
	for (AActor* ExistingRuntimeSurface : ExistingRuntimeSurfaces)
	{
		ExistingRuntimeSurface->Destroy();
	}

	if (UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		// Region top is -2 cm, below the cabin floor at 0: no coplanar flicker indoors.
		AStaticMeshActor* RuntimeSurface = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(0.0f, 0.0f, -52.0f), FRotator::ZeroRotator, SpawnParameters);
		if (RuntimeSurface && RuntimeSurface->GetStaticMeshComponent())
		{
			// Created after BeginPlay from the menu: Static components reject SetStaticMesh.
			RuntimeSurface->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
			RuntimeSurface->Tags.Add(TEXT("AstraeonRuntimeSurface"));
			RuntimeSurface->SetActorScale3D(FVector(1200.0f, 1200.0f, 1.0f));
			RuntimeSurface->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
			RuntimeSurface->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
			RuntimeSurface->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			// Dark blue-grey alien rock floor.
			if (TerrainGeologyMaterial) RuntimeSurface->GetStaticMeshComponent()->SetMaterial(0, TerrainGeologyMaterial);
#if WITH_EDITOR
			RuntimeSurface->SetActorLabel(TEXT("Runtime_ProceduralRegionSurface"));
#endif
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AstraeonRegionMaterialization: Runtime surface actor was not created."));
		}

		// Las 12 rocas procedurales se eliminaron. Databan de antes del campo de relieve y su
		// anillo se centraba en el ORIGEN DEL MUNDO, no en Ítaca, así que al empezar la partida
		// rodeaban la nave como un muro de bloques planos a 3,5-14,5 m. Su Z tampoco consultaba
		// el terreno: se calculaba contra Z=0, de modo que sobre relieve quedaban medio
		// enterradas. El relieve por baldosas ya aporta el paisaje que ellas intentaban dar.

	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AstraeonRegionMaterialization: Cube mesh unavailable; runtime surface was not created."));
	}

	// La estancia y sus consolas viajan con la nave: se colocan alrededor del origen actual
	// de Ítaca, que cambia cada vez que el jugador aterriza en otro punto de la región.
	const FVector ItacaOrigin = AstraeonGameInstance->GetItacaOriginCm();
	TArray<AActor*> Interiors;
	UGameplayStatics::GetAllActorsOfClass(World, AAstraeonItacaInterior::StaticClass(), Interiors);
	for (AActor* Interior : Interiors)
	{
		Interior->SetActorLocation(ItacaOrigin);
	}

	TArray<FAstraeonRegionActorSpec> Specs = UAstraeonRegionMaterializer::BuildItacaActorSpecs(ItacaOrigin);
	Specs.Append(UAstraeonRegionMaterializer::BuildActorSpecs(AstraeonGameInstance->GetCurrentRegionLayout()));

	// Relieve del terreno. Se construye después de conocer dónde está todo lo jugable para
	// dejar esos puntos llanos: las colinas son el paisaje, no un obstáculo que entierre
	// una veta o encierre a Ítaca.
	{
		// El contexto lo construye el materializador: la seed se validó contra exactamente
		// esta superficie, así que rearmarlo aquí a mano abriría la puerta a validar una
		// región y materializar otra.
		FActorSpawnParameters TerrainSpawnParameters;
		TerrainSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FAstraeonTerrainSurfaceContext& SurfaceContext = AstraeonGameInstance->GetSurfaceContext();
		AAstraeonTerrainSurfacePrototype* TerrainSurface = World->SpawnActor<AAstraeonTerrainSurfacePrototype>(
			AAstraeonTerrainSurfacePrototype::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, TerrainSpawnParameters);
		if (TerrainSurface)
		{
			TerrainSurface->Tags.Add(TEXT("AstraeonRuntimeSurface"));
			TerrainSurface->BuildPrototype(SurfaceContext);
#if WITH_EDITOR
			TerrainSurface->SetActorLabel(TEXT("Runtime_ProceduralTerrainSurface"));
#endif
		}

		// Los marcadores de región se posan sobre el relieve. Aplanar el terreno bajo cada
		// uno producía un borde de acantilado alrededor del claro; consultar la altura y
		// apoyarlos encima resuelve el mismo problema sin deformar el paisaje.
		for (FAstraeonRegionActorSpec& Spec : Specs)
		{
			const bool bBelongsToItaca = Spec.ActorId.ToString().StartsWith(TEXT("itaca_"));
			if (!bBelongsToItaca)
			{
				Spec.LocationCm.Z += AAstraeonTerrainField::SampleSurface(SurfaceContext,
					FVector2D(Spec.LocationCm.X, Spec.LocationCm.Y)).HeightCm;
			}
		}
	}
	for (const FAstraeonRegionActorSpec& Spec : Specs)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AAstraeonRegionMarker* Marker = World->SpawnActor<AAstraeonRegionMarker>(AAstraeonRegionMarker::StaticClass(), Spec.LocationCm, FRotator::ZeroRotator, SpawnParameters);
		if (Marker)
		{
			Marker->ApplySpec(Spec);
		}
	}

	// Lo construido por el jugador se reconstruye desde el SaveGame: la región se rehace
	// entera cada vez que Ítaca aterriza, así que las obras no pueden vivir sólo como
	// actores en el mundo.
	for (const FAstraeonPlacedStructure& Placement : AstraeonGameInstance->GetPlacedStructures())
	{
		FActorSpawnParameters StructureSpawnParameters;
		StructureSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AAstraeonBuiltStructure* Structure = World->SpawnActor<AAstraeonBuiltStructure>(
			AAstraeonBuiltStructure::StaticClass(), Placement.LocationCm, Placement.Rotation, StructureSpawnParameters))
		{
			Structure->ApplyPlacement(Placement);
		}
	}

	for (const FAstraeonPointOfInterest& PointOfInterest : AstraeonGameInstance->GetCurrentRegionLayout().PointsOfInterest)
	{
		if (PointOfInterest.Type != EAstraeonPointOfInterestType::CreatureSpawn)
		{
			continue;
		}

		// Un nido cazado hace poco queda vacío hasta que vence su reloj de repoblado: antes
		// bastaba despegar y aterrizar para tener toda la fauna viva otra vez.
		if (!AstraeonGameInstance->IsCreatureSpawnPopulated(PointOfInterest.PointId))
		{
			continue;
		}

		SpawnCreatureAtPoint(PointOfInterest.PointId, PointOfInterest.LocationMeters);
	}
}

AAstraeonCreatureActor* AAstraeonGameModeBase::SpawnCreatureAtPoint(FName SpawnPointId, const FVector2D& LocationMeters)
{
	UWorld* World = GetWorld();
	UAstraeonGameInstance* AstraeonGameInstance = World ? World->GetGameInstance<UAstraeonGameInstance>() : nullptr;
	if (!AstraeonGameInstance)
	{
		return nullptr;
	}

	const FVector2D CreatureXY = LocationMeters * 100.0f;
	const FVector SpawnLocationCm(
		CreatureXY.X,
		CreatureXY.Y,
		70.0f + AstraeonGameInstance->GetSurfaceHeightCm(CreatureXY));

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AAstraeonCreatureActor* Creature = World->SpawnActor<AAstraeonCreatureActor>(
		AAstraeonCreatureActor::StaticClass(), SpawnLocationCm, FRotator::ZeroRotator, SpawnParameters);
	if (Creature)
	{
		Creature->SetSpawnPointId(SpawnPointId);
#if WITH_EDITOR
		Creature->SetActorLabel(*FString::Printf(TEXT("Creature_UmbraGrazer_%s"), *SpawnPointId.ToString()));
#endif
	}

	return Creature;
}

void AAstraeonGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UWorld* World = GetWorld();
	UAstraeonGameInstance* AstraeonGameInstance = World ? World->GetGameInstance<UAstraeonGameInstance>() : nullptr;
	if (!AstraeonGameInstance || !AstraeonGameInstance->HasStartedGame())
	{
		return;
	}

	const TArray<FName> RepopulatedSpawnPoints = AstraeonGameInstance->AdvanceCreatureRespawns(DeltaSeconds);
	if (RepopulatedSpawnPoints.IsEmpty())
	{
		return;
	}

	for (const FAstraeonPointOfInterest& PointOfInterest : AstraeonGameInstance->GetCurrentRegionLayout().PointsOfInterest)
	{
		if (PointOfInterest.Type == EAstraeonPointOfInterestType::CreatureSpawn
			&& RepopulatedSpawnPoints.Contains(PointOfInterest.PointId))
		{
			SpawnCreatureAtPoint(PointOfInterest.PointId, PointOfInterest.LocationMeters);
		}
	}
}
