#include "AstraeonPlayerController.h"

#include "AstraeonGameInstance.h"
#include "AstraeonGameModeBase.h"
#include "Engine/Engine.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Survival/AstraeonSuitComponent.h"

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
}

void AAstraeonPlayerController::StartSelectedNewGame()
{
	UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	if (!AstraeonGameInstance)
	{
		return;
	}

	AstraeonGameInstance->StartNewGame(SelectedMenuSeed);
	bMenuVisible = false;
	ApplySessionToRuntime();
}

void AAstraeonPlayerController::ContinueSavedGame()
{
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
	const UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	const bool bSaved = !bMenuVisible && AstraeonGameInstance && AstraeonGameInstance->SaveCurrentGame();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.0f, bSaved ? FColor::Green : FColor::Yellow, bSaved ? TEXT("Game saved.") : TEXT("Start or continue a game before saving."));
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
