#include "AstraeonGameModeBase.h"

#include "AstraeonGameInstance.h"
#include "AstraeonHUD.h"
#include "AstraeonPlayerCharacter.h"
#include "AstraeonPlayerController.h"
#include "Creatures/AstraeonCreatureActor.h"
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

	UE_LOG(LogTemp, Display, TEXT("AstraeonCriticalPathSmoke: Scanned=%s Crafted=%s Resolved=%s Saved=%s Loaded=%s LoadedResolved=%s Seed=%d"),
		bScanned ? TEXT("true") : TEXT("false"),
		bCrafted ? TEXT("true") : TEXT("false"),
		bResolved ? TEXT("true") : TEXT("false"),
		bSaved ? TEXT("true") : TEXT("false"),
		bLoaded ? TEXT("true") : TEXT("false"),
		bLoadedResolved ? TEXT("true") : TEXT("false"),
		AstraeonGameInstance->GetCurrentWorldSeed());

	FPlatformMisc::RequestExit(!(bScanned && bCrafted && bResolved && bSaved && bLoadedResolved));
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

	FAstraeonRegionActorSpec ArgosConsoleSpec;
	ArgosConsoleSpec.ActorId = TEXT("itaca_argos_console");
	ArgosConsoleSpec.Kind = EAstraeonRegionActorKind::PointOfInterest;
	ArgosConsoleSpec.LocationCm = FVector(250.0f, 0.0f, 80.0f);
	ArgosConsoleSpec.Scale = FVector(0.6f, 0.6f, 1.2f);
	if (AAstraeonRegionMarker* ArgosConsole = World->SpawnActor<AAstraeonRegionMarker>(AAstraeonRegionMarker::StaticClass(), ArgosConsoleSpec.LocationCm, FRotator::ZeroRotator, FActorSpawnParameters()))
	{
		ArgosConsole->ApplySpec(ArgosConsoleSpec);
#if WITH_EDITOR
		ArgosConsole->SetActorLabel(TEXT("POI_Itaca_ARGOS_Console"));
#endif
	}

	const TArray<FAstraeonRegionActorSpec> Specs = UAstraeonRegionMaterializer::BuildActorSpecs(AstraeonGameInstance->GetCurrentRegionLayout());
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
