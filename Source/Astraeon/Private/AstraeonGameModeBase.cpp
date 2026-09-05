#include "AstraeonGameModeBase.h"

#include "AstraeonGameInstance.h"
#include "AstraeonHUD.h"
#include "AstraeonPlayerCharacter.h"
#include "AstraeonPlayerController.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
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
#include "WorldGen/AstraeonRegionMarker.h"
#include "WorldGen/AstraeonRegionMaterializer.h"

AAstraeonGameModeBase::AAstraeonGameModeBase()
{
	DefaultPawnClass = AAstraeonPlayerCharacter::StaticClass();
	PlayerControllerClass = AAstraeonPlayerController::StaticClass();
	HUDClass = AAstraeonHUD::StaticClass();
}

void AAstraeonGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	if (GEngine)
	{
		GEngine->Exec(GetWorld(), TEXT("DisableAllScreenMessages"), *GLog);
	}

	EnsureRuntimeLighting();

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

	const FVector HatchLocationCm(620.0f, 0.0f, 80.0f);
	const FVector InteractionLocationCm(360.0f, 0.0f, 120.0f);
	PlayerCharacter->SetActorLocation(InteractionLocationCm, false, nullptr, ETeleportType::TeleportPhysics);
	PlayerController->SetControlRotation((HatchLocationCm - (InteractionLocationCm + FVector(-10.0f, 0.0f, 64.0f))).Rotation());
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
		AStaticMeshActor* RuntimeSurface = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(0.0f, 0.0f, -50.0f), FRotator::ZeroRotator, SpawnParameters);
		if (RuntimeSurface && RuntimeSurface->GetStaticMeshComponent())
		{
			RuntimeSurface->Tags.Add(TEXT("AstraeonRuntimeSurface"));
			RuntimeSurface->SetActorScale3D(FVector(1200.0f, 1200.0f, 1.0f));
			RuntimeSurface->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
			RuntimeSurface->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
			RuntimeSurface->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			// Dark blue-grey alien rock floor.
			RuntimeSurface->GetStaticMeshComponent()->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.06f, 0.09f, 0.16f));
#if WITH_EDITOR
			RuntimeSurface->SetActorLabel(TEXT("Runtime_ProceduralRegionSurface"));
#endif
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AstraeonRegionMaterialization: Runtime surface actor was not created."));
		}

		const FVector DeploymentLocationCm = UAstraeonRegionMaterializer::GetSurfaceDeploymentLocationCm();
		AStaticMeshActor* DeploymentPad = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(DeploymentLocationCm.X, DeploymentLocationCm.Y, 25.0f), FRotator::ZeroRotator, SpawnParameters);
		if (DeploymentPad && DeploymentPad->GetStaticMeshComponent())
		{
			DeploymentPad->Tags.Add(TEXT("AstraeonRuntimeSurface"));
			DeploymentPad->SetActorScale3D(FVector(8.0f, 8.0f, 0.5f));
			DeploymentPad->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
			DeploymentPad->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
			DeploymentPad->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			// Lighter grey — visually distinct from the ground, reads as a landing platform.
			DeploymentPad->GetStaticMeshComponent()->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.22f, 0.22f, 0.28f));
#if WITH_EDITOR
			DeploymentPad->SetActorLabel(TEXT("Runtime_SurfaceDeploymentPad"));
#endif
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AstraeonRegionMaterialization: Deployment pad actor was not created."));
		}

		// Procedural terrain rocks — scattered around the active play area using the
		// world seed so the layout is deterministic per expedition. They stay tagged
		// AstraeonRuntimeSurface so a region re-materialisation always rebuilds them
		// fresh. Two exclusion zones keep key gameplay points clear:
		//   • 250 cm around the origin (Ítaca console / hatch cluster)
		//   • 400 cm around the surface deployment pad
		{
			const int32 WorldSeed = AstraeonGameInstance->GetCurrentWorldSeed();
			int32 TerrainRng = WorldSeed;
			auto NextFloat = [&TerrainRng]() -> float
			{
				TerrainRng = TerrainRng * 1664525 + 1013904223;
				return static_cast<float>(static_cast<uint16>((TerrainRng >> 8) & 0xFFFF)) / 65535.0f;
			};

			const FVector2D DeployXY(DeploymentLocationCm.X, DeploymentLocationCm.Y);
			constexpr int32 NumRocks = 12;
			for (int32 RockIndex = 0; RockIndex < NumRocks; ++RockIndex)
			{
				const float Angle = (static_cast<float>(RockIndex) / NumRocks) * 2.0f * PI + NextFloat() * 0.9f;
				const float RadiusCm = 350.0f + NextFloat() * 1100.0f; // 3.5 m – 14.5 m
				const float HeightCm = 25.0f + NextFloat() * 130.0f;   // 0.25 m – 1.55 m
				const float FootprintM = 0.6f + NextFloat() * 2.4f;    // 0.6 m – 3.0 m

				const FVector2D RockXY(FMath::Cos(Angle) * RadiusCm, FMath::Sin(Angle) * RadiusCm);
				if (RockXY.Size() < 250.0f || (RockXY - DeployXY).Size() < 400.0f)
				{
					continue;
				}

				const FVector RockLocation(RockXY.X, RockXY.Y, HeightCm * 0.5f);
				AStaticMeshActor* Rock = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), RockLocation, FRotator::ZeroRotator, SpawnParameters);
				if (Rock && Rock->GetStaticMeshComponent())
				{
					Rock->Tags.Add(TEXT("AstraeonRuntimeSurface"));
					Rock->SetActorScale3D(FVector(FootprintM, FootprintM, HeightCm / 100.0f));
					Rock->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
					Rock->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
					Rock->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					// Rocks block pawns for walking but must not intercept ECC_Visibility so
					// interact/scan traces reach resource markers that may sit behind them.
					Rock->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
					// Warm ochre — alien oxidised rock, reads differently from the dark floor.
					Rock->GetStaticMeshComponent()->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.28f, 0.14f, 0.07f));
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AstraeonRegionMaterialization: Cube mesh unavailable; runtime surface was not created."));
	}

	TArray<FAstraeonRegionActorSpec> Specs = UAstraeonRegionMaterializer::BuildItacaActorSpecs();
	Specs.Append(UAstraeonRegionMaterializer::BuildActorSpecs(AstraeonGameInstance->GetCurrentRegionLayout()));
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

	for (const FAstraeonPointOfInterest& PointOfInterest : AstraeonGameInstance->GetCurrentRegionLayout().PointsOfInterest)
	{
		if (PointOfInterest.Type != EAstraeonPointOfInterestType::CreatureSpawn)
		{
			continue;
		}

		const FVector SpawnLocationCm(PointOfInterest.LocationMeters.X * 100.0f, PointOfInterest.LocationMeters.Y * 100.0f, 70.0f);
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AAstraeonCreatureActor* Creature = World->SpawnActor<AAstraeonCreatureActor>(AAstraeonCreatureActor::StaticClass(), SpawnLocationCm, FRotator::ZeroRotator, SpawnParameters);
#if WITH_EDITOR
		if (Creature)
		{
			Creature->SetActorLabel(TEXT("Creature_UmbraGrazer_FirstMob"));
		}
#endif
	}
}
