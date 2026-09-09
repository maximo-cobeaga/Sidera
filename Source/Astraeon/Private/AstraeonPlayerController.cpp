#include "AstraeonPlayerController.h"

#include "AstraeonGameInstance.h"
#include "AstraeonGameModeBase.h"
#include "AstraeonPlayerCharacter.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "AstraeonDiagnostics.h"
#include "Environment/AstraeonItacaInterior.h"
#include "Ship/AstraeonShipPawn.h"
#include "Survival/AstraeonSuitComponent.h"
#include "WorldGen/AstraeonRegionMaterializer.h"
#include "WorldGen/AstraeonTerrainField.h"

AAstraeonPlayerController::AAstraeonPlayerController()
{
	bShowMouseCursor = false;
}

void AAstraeonPlayerController::BeginPlay()
{
	Super::BeginPlay();

	int32 CommandLineSeed = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("AstraeonSeed="), CommandLineSeed))
	{
		SelectedMenuSeed = UAstraeonGameInstance::NormalizeRequestedSeed(CommandLineSeed);
	}
}

void AAstraeonPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	check(InputComponent);
	InputComponent->BindAction(TEXT("StartGame"), IE_Pressed, this, &AAstraeonPlayerController::StartSelectedNewGame);
	InputComponent->BindAction(TEXT("ContinueGame"), IE_Pressed, this, &AAstraeonPlayerController::ContinueSavedGame);
	InputComponent->BindAction(TEXT("SaveGame"), IE_Pressed, this, &AAstraeonPlayerController::SaveCurrentGame);
	InputComponent->BindAction(TEXT("SeedUp"), IE_Pressed, this, &AAstraeonPlayerController::IncreaseSelectedSeed);
	InputComponent->BindAction(TEXT("SeedDown"), IE_Pressed, this, &AAstraeonPlayerController::DecreaseSelectedSeed);
	InputComponent->BindAction(TEXT("ToggleLogbook"), IE_Pressed, this, &AAstraeonPlayerController::ToggleLogbook);
	InputComponent->BindAction(TEXT("ToggleInventory"), IE_Pressed, this, &AAstraeonPlayerController::ToggleInventory);
}

void AAstraeonPlayerController::ToggleInventory()
{
	if (bMenuVisible)
	{
		return;
	}

	bInventoryVisible = !bInventoryVisible;
	if (bInventoryVisible)
	{
		bLogbookVisible = false;
		bFabricatorOpen = false;
	}
}

void AAstraeonPlayerController::ToggleLogbook()
{
	if (bMenuVisible)
	{
		return;
	}

	bLogbookVisible = !bLogbookVisible;
	if (bLogbookVisible)
	{
		bFabricatorOpen = false;
		bInventoryVisible = false;
	}
}

void AAstraeonPlayerController::SetFabricatorOpen(bool bOpen)
{
	bFabricatorOpen = bOpen;
	if (bOpen)
	{
		bLogbookVisible = false;
		bInventoryVisible = false;
	}
}

void AAstraeonPlayerController::StartSelectedNewGame()
{
	// Sin esta guarda, pulsar Intro durante la partida reiniciaba la expedición entera y
	// borraba el progreso en silencio.
	if (!bMenuVisible)
	{
		return;
	}

	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	if (!AstraeonGameInstance)
	{
		return;
	}

	AstraeonGameInstance->StartNewGame(SelectedMenuSeed);
	bMenuVisible = false;
	ApplySessionToRuntime();
	SetInputMode(FInputModeGameOnly());
	if (auto* SpawnedPlayerCharacter = Cast<AAstraeonPlayerCharacter>(GetPawn()))
	{
		// Relativo al origen de Ítaca, no a Z=100 fijo: con relieve la estancia se apoya a
		// la cota de su plataforma, y aparecer a 100 cm del cero del mundo dejaba al
		// jugador dentro del bloque de terreno.
		const FVector ItacaOrigin = AstraeonGameInstance->GetItacaOriginCm();
		SpawnedPlayerCharacter->SetActorLocation(ItacaOrigin + FVector(220.0f, 0.0f, 110.0f),
			false, nullptr, ETeleportType::TeleportPhysics);
		SpawnedPlayerCharacter->GetCharacterMovement()->StopMovementImmediately();
		SetControlRotation(FRotator::ZeroRotator);
	}
}

void AAstraeonPlayerController::ContinueSavedGame()
{
	if (!bMenuVisible)
	{
		return;
	}

	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	if (!AstraeonGameInstance)
	{
		return;
	}

	if (AstraeonGameInstance->LoadSavedGame())
	{
		bMenuVisible = false;
		ApplySessionToRuntime();
	}
}

void AAstraeonPlayerController::SaveCurrentGame()
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	const bool bSaved = !bMenuVisible && AstraeonGameInstance && AstraeonGameInstance->SaveCurrentGame();
	if (AstraeonGameInstance)
	{
		AstraeonGameInstance->SetLastFeedbackMessage(bSaved ? TEXT("Partida guardada.") : TEXT("Inicia o continúa una partida antes de guardar."));
	}
}

void AAstraeonPlayerController::IncreaseSelectedSeed()
{
	if (bMenuVisible)
	{
		++SelectedMenuSeed;
	}
}

void AAstraeonPlayerController::DecreaseSelectedSeed()
{
	if (bMenuVisible)
	{
		SelectedMenuSeed = FMath::Max(1, SelectedMenuSeed - 1);
	}
}

bool AAstraeonPlayerController::BeginShipFlight()
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	UWorld* World = GetWorld();
	if (!AstraeonGameInstance || !World || !AstraeonGameInstance->HasStartedGame() || ShipPawn)
	{
		return false;
	}

	APawn* CurrentPawn = GetPawn();
	if (!CurrentPawn)
	{
		return false;
	}

	// La nave nace sobre el techo de la estancia para que el despegue se lea como que
	// Ítaca entera se levanta, no como aparecer dentro de la geometría.
	const FVector LiftOffLocation = AstraeonGameInstance->GetItacaOriginCm() + FVector(220.0f, 0.0f, 900.0f);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ShipPawn = World->SpawnActor<AAstraeonShipPawn>(AAstraeonShipPawn::StaticClass(), LiftOffLocation, FRotator::ZeroRotator, SpawnParameters);
	if (!ShipPawn)
	{
		return false;
	}

	GroundedPawn = CurrentPawn;
	if (AAstraeonPlayerCharacter* GroundedCharacter = Cast<AAstraeonPlayerCharacter>(CurrentPawn))
	{
		GroundedCharacter->PrepareForShipFlight();
	}

	Possess(ShipPawn);
	SetControlRotation(FRotator::ZeroRotator);

	if (AAstraeonGameModeBase* AstraeonGameMode = World->GetAuthGameMode<AAstraeonGameModeBase>())
	{
		AstraeonGameMode->SetItacaInteriorHidden(true);
	}

	UE_LOG(LogAstraeonDiag, Log, TEXT("LIFTOFF itacaOrigin=(%.0f,%.0f,%.1f) shipSpawn=(%.0f,%.0f,%.1f)"),
		AstraeonGameInstance->GetItacaOriginCm().X, AstraeonGameInstance->GetItacaOriginCm().Y, AstraeonGameInstance->GetItacaOriginCm().Z,
		LiftOffLocation.X, LiftOffLocation.Y, LiftOffLocation.Z);

	AstraeonGameInstance->SetLastFeedbackMessage(TEXT("Ítaca en vuelo. WASD desplaza, Espacio/Ctrl altura, Shift acelera, E aterriza."));
	return true;
}

bool AAstraeonPlayerController::RequestShipLanding()
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	UWorld* World = GetWorld();
	if (!AstraeonGameInstance || !World || !ShipPawn || !GroundedPawn)
	{
		return false;
	}

	if (!ShipPawn->CanLandHere())
	{
		AstraeonGameInstance->SetLastFeedbackMessage(TEXT("Descenso rechazado: baja más y reduce la velocidad antes de aterrizar."));
		return false;
	}

	// El punto de aterrizaje pasa a ser el nuevo origen de Ítaca: la estancia, la consola
	// ARGOS, el pilotaje y la escotilla se remateralizan alrededor de la nave posada.
	// La estancia se posa sobre la meseta que el terreno deja bajo ella, no en Z=0: con
	// relieve, aterrizar a cota cero la habría enterrado.
	const FVector ShipLocation = ShipPawn->GetActorLocation();
	const float LandedGroundZ = AAstraeonTerrainField::GetItacaPadHeightCm(
		AstraeonGameInstance->GetCurrentTerrainSeed(), ShipLocation.X - 220.0f, ShipLocation.Y);
	// La cubierta se apoya sobre el terreno en vez de quedar a su misma cota. Posarla justo
	// en LandedGroundZ dejaba la cara superior del piso de la estancia y la cara superior
	// del bloque de terreno en el mismo Z: z-fighting visible y dos superficies de colisión
	// coincidentes bajo los pies. El resto del mundo asumía que Ítaca vivía en Z=0, donde
	// el problema no se notaba.
	const FVector LandedOrigin(ShipLocation.X - 220.0f, ShipLocation.Y,
		LandedGroundZ + AAstraeonItacaInterior::GetDeckThicknessCm());
	AstraeonGameInstance->SetItacaOriginCm(LandedOrigin);

	APawn* PawnToRestore = GroundedPawn;
	ShipPawn->Destroy();
	ShipPawn = nullptr;
	GroundedPawn = nullptr;

	Possess(PawnToRestore);

	// La estancia se reubica antes de devolver al jugador, para que el suelo bajo él ya
	// exista cuando reaparezca.
	if (AAstraeonGameModeBase* AstraeonGameMode = World->GetAuthGameMode<AAstraeonGameModeBase>())
	{
		AstraeonGameMode->SetItacaInteriorHidden(false);
		AstraeonGameMode->MaterializeCurrentRegion();
	}

	// El jugador reaparece dentro de la estancia ya reubicada, no bajo la nave, y mirando
	// hacia el interior: dejarlo encarando la ESCOTILLA hacía que el siguiente E lo
	// desplegara a la superficie sin querer, justo después de aterrizar.
	const FVector InteriorSpawn = LandedOrigin + FVector(220.0f, 0.0f, 110.0f);
	if (AAstraeonPlayerCharacter* LandedCharacter = Cast<AAstraeonPlayerCharacter>(PawnToRestore))
	{
		LandedCharacter->RecoverFromShipFlight(InteriorSpawn);
	}
	else
	{
		PawnToRestore->SetActorHiddenInGame(false);
		PawnToRestore->SetActorEnableCollision(true);
		PawnToRestore->SetActorLocation(InteriorSpawn, false, nullptr, ETeleportType::TeleportPhysics);
	}
	SetControlRotation(FRotator(0.0f, 180.0f, 0.0f));

	AstraeonGameInstance->RevealMapAroundLocationMeters(FVector2D(LandedOrigin.X, LandedOrigin.Y) / 100.0f, 2);
	AstraeonGameInstance->SetLastFeedbackMessage(TEXT("Ítaca posada. La estancia se reubicó en el punto de aterrizaje."));

	// Se registra siempre: aterrizar es el momento en que se reconstruye medio mundo, y es
	// donde aparecen los síntomas más difíciles de reproducir a mano.
	UE_LOG(LogAstraeonDiag, Log,
		TEXT("LANDING ship=(%.0f,%.0f,%.1f) terrainZ=%.1f landedOrigin=(%.0f,%.0f,%.1f) ")
		TEXT("playerSpawn=(%.0f,%.0f,%.1f) deck=%.1f"),
		ShipLocation.X, ShipLocation.Y, ShipLocation.Z, LandedGroundZ,
		LandedOrigin.X, LandedOrigin.Y, LandedOrigin.Z,
		InteriorSpawn.X, InteriorSpawn.Y, InteriorSpawn.Z,
		AAstraeonItacaInterior::GetDeckThicknessCm());
	if (const AActor* RestoredActor = PawnToRestore)
	{
		UE_LOG(LogAstraeonDiag, Log, TEXT("LANDING settled %s | %s"),
			*AstraeonDiagnostics::DescribeGroundUnder(*RestoredActor, 30000.0f),
			*AstraeonDiagnostics::DescribeBlockingOverlaps(*RestoredActor, 42.0f, 96.0f));
	}
	return true;
}

void AAstraeonPlayerController::ApplySessionToRuntime()
{
	if (APawn* ControlledPawn = GetPawn())
	{
		if (UAstraeonSuitComponent* SuitComponent = ControlledPawn->FindComponentByClass<UAstraeonSuitComponent>())
		{
			if (const UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>())
			{
				SuitComponent->ApplyEnvironment(AstraeonGameInstance->GetCurrentEnvironment());
			}
		}
	}

	if (AAstraeonGameModeBase* AstraeonGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AAstraeonGameModeBase>() : nullptr)
	{
		AstraeonGameMode->MaterializeCurrentRegion();
	}
}
