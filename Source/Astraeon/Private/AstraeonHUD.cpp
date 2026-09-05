#include "AstraeonHUD.h"

#include "AstraeonGameInstance.h"
#include "AstraeonPlayerController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Survival/AstraeonSuitComponent.h"
#include "WorldGen/AstraeonEnvironmentTypes.h"

void AAstraeonHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas || !GEngine)
	{
		return;
	}

	const AAstraeonPlayerController* AstraeonPlayerController = Cast<AAstraeonPlayerController>(GetOwningPlayerController());
	const UAstraeonGameInstance* AstraeonGameInstance = GetGameInstance<UAstraeonGameInstance>();
	const TArray<FString> Lines = (AstraeonPlayerController && AstraeonPlayerController->IsMenuVisible())
		? BuildMenuLines(AstraeonPlayerController)
		: BuildStatusLines(AstraeonGameInstance);

	UFont* Font = GEngine->GetSmallFont();
	constexpr float X = 32.0f;
	float Y = 32.0f;
	constexpr float LineHeight = 18.0f;

	for (const FString& Line : Lines)
	{
		Canvas->DrawText(Font, Line, X, Y);
		Y += LineHeight;
	}

	if (AstraeonPlayerController && !AstraeonPlayerController->IsMenuVisible())
	{
		if (const APawn* Pawn = AstraeonPlayerController->GetPawn())
		{
			if (const UAstraeonSuitComponent* Suit = Pawn->FindComponentByClass<UAstraeonSuitComponent>())
			{
				Canvas->DrawText(Font, FString::Printf(TEXT("Suit O2: %.1f%% | Health: %.1f%% | Move x%.2f"), Suit->GetOxygenPercent(), Suit->GetHealthPercent(), Suit->GetGravitySpeedMultiplier()), X, Y);
			}
		}
	}
}

TArray<FString> AAstraeonHUD::BuildMenuLines(const AAstraeonPlayerController* PlayerController)
{
	TArray<FString> Lines;
	Lines.Reserve(7);
	Lines.Add(TEXT("ASTRAEON — La primera señal"));
	Lines.Add(TEXT("Initial systems are ready."));
	Lines.Add(FString::Printf(TEXT("Selected seed: %d"), PlayerController ? PlayerController->GetSelectedMenuSeed() : 1001));
	Lines.Add(TEXT("PageUp/PageDown: change seed"));
	Lines.Add(TEXT("Enter: start new game"));
	Lines.Add(TEXT("F9: continue saved game"));
	Lines.Add(TEXT("Goal: explore, measure, understand, act, record."));
	return Lines;
}

TArray<FString> AAstraeonHUD::BuildStatusLines(const UAstraeonGameInstance* GameInstance)
{
	TArray<FString> Lines;
	Lines.Reserve(8);
	Lines.Add(TEXT("ASTRAEON — La primera señal / bootstrap"));

	if (!GameInstance || !GameInstance->HasStartedGame())
	{
		Lines.Add(TEXT("Session: pending new game"));
		return Lines;
	}

	const FAstraeonEnvironmentalSnapshot& Environment = GameInstance->GetCurrentEnvironment();
	Lines.Add(FString::Printf(TEXT("Seed: %d | Generator: %d"), Environment.WorldSeed, Environment.GeneratorVersion));
	Lines.Add(FString::Printf(TEXT("GravityMS2: %.2f"), Environment.GravityMS2));
	Lines.Add(FString::Printf(TEXT("TemperatureKelvin: %.2f"), Environment.TemperatureKelvin));
	Lines.Add(FString::Printf(TEXT("PressureKPa: %.2f"), Environment.PressureKPa));
	Lines.Add(FString::Printf(TEXT("Breathable: %s | Risk: %.2f"), Environment.bBreathable ? TEXT("yes") : TEXT("no"), Environment.EnvironmentalRisk01));
	Lines.Add(FString::Printf(TEXT("Region: %d resources | %d POIs"), GameInstance->GetCurrentRegionLayout().Resources.Num(), GameInstance->GetCurrentRegionLayout().PointsOfInterest.Num()));
	Lines.Add(FString::Printf(TEXT("Map revealed cells: %d"), GameInstance->GetRevealedMap().RevealedCells.Num()));
	const TCHAR* ObjectiveText = TEXT("Measure environment");
	if (GameInstance->GetObjectiveState() == EAstraeonObjectiveState::GatherResources)
	{
		ObjectiveText = TEXT("Gather resources");
	}
	else if (GameInstance->GetObjectiveState() == EAstraeonObjectiveState::CraftSignalResonator)
	{
		ObjectiveText = TEXT("Craft signal_resonator");
	}
	else if (GameInstance->GetObjectiveState() == EAstraeonObjectiveState::ReachSignalSource)
	{
		ObjectiveText = TEXT("Reach signal source");
	}
	else if (GameInstance->GetObjectiveState() == EAstraeonObjectiveState::Completed)
	{
		ObjectiveText = TEXT("Completed");
	}
	Lines.Add(FString::Printf(TEXT("Objective: %s"), ObjectiveText));
	Lines.Add(FString::Printf(TEXT("Hint: %s"), *GameInstance->GetObjectiveHint()));
	Lines.Add(FString::Printf(TEXT("Inventory stacks: %d"), GameInstance->GetInventory().Num()));
	Lines.Add(FString::Printf(TEXT("Logbook entries: %d"), GameInstance->GetRuntimeLogbookEntries().Num()));
	Lines.Add(TEXT("Controls: WASD move | Mouse look | Space jump | Left mouse scan | E interact | C craft | F5 save"));
	return Lines;
}
