#include "AstraeonHUD.h"

#include "AstraeonGameInstance.h"
#include "AstraeonPlayerController.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
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

	UFont* Font = GEngine->GetMediumFont() ? GEngine->GetMediumFont() : GEngine->GetSmallFont();
	constexpr float X = 36.0f;
	float Y = 36.0f;
	constexpr float LineHeight = 30.0f;

	FCanvasTileItem Background(FVector2D(20.0f, 20.0f), FVector2D(760.0f, 500.0f), FLinearColor(0.0f, 0.0f, 0.0f, 0.62f));
	Background.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Background);

	for (int32 LineIndex = 0; LineIndex < Lines.Num(); ++LineIndex)
	{
		FLinearColor LineColor = LineIndex == 0 ? FLinearColor(0.45f, 0.85f, 1.0f, 1.0f) : FLinearColor::White;
		if (Lines[LineIndex].StartsWith(TEXT("Estado: ")))
		{
			// El feedback de interacción (despliegue, recolección, briefing ARGOS, etc.)
			// se destaca en verde para que el jugador lo note entre el resto del HUD.
			LineColor = FLinearColor(0.65f, 1.0f, 0.45f, 1.0f);
		}
		FCanvasTextItem TextItem(FVector2D(X, Y), FText::FromString(Lines[LineIndex]), Font, LineColor);
		TextItem.EnableShadow(FLinearColor::Black);
		TextItem.Scale = FVector2D(0.95f, 0.95f);
		Canvas->DrawItem(TextItem);
		Y += LineHeight;
	}

	if (AstraeonPlayerController && !AstraeonPlayerController->IsMenuVisible())
	{
		if (const APawn* Pawn = AstraeonPlayerController->GetPawn())
		{
			if (const UAstraeonSuitComponent* Suit = Pawn->FindComponentByClass<UAstraeonSuitComponent>())
			{
				FCanvasTextItem SuitText(FVector2D(X, Y), FText::FromString(FString::Printf(TEXT("Suit O2: %.1f%% | Health: %.1f%% | Move x%.2f"), Suit->GetOxygenPercent(), Suit->GetHealthPercent(), Suit->GetGravitySpeedMultiplier())), Font, FLinearColor(0.70f, 1.0f, 0.70f, 1.0f));
				SuitText.EnableShadow(FLinearColor::Black);
				SuitText.Scale = FVector2D(0.95f, 0.95f);
				Canvas->DrawItem(SuitText);
			}
		}
	}

	// Draw a simple crosshair at screen center: the E/Interact and Left Mouse/Scan
	// traces both fire from the camera along the control rotation (screen center in
	// a standard first-person view), but nothing was ever drawn to show the player
	// where that is, making it very easy to aim just off a small marker and see the
	// interaction silently fail.
	if (AstraeonPlayerController && !AstraeonPlayerController->IsMenuVisible())
	{
		const FVector2D CrosshairCenter(Canvas->SizeX * 0.5f, Canvas->SizeY * 0.5f);
		constexpr float CrosshairHalfSize = 9.0f;
		constexpr float CrosshairGap = 3.0f;
		const FLinearColor CrosshairColor(1.0f, 1.0f, 1.0f, 0.85f);

		FCanvasLineItem CrosshairLines[4] = {
			FCanvasLineItem(CrosshairCenter - FVector2D(CrosshairHalfSize, 0.0f), CrosshairCenter - FVector2D(CrosshairGap, 0.0f)),
			FCanvasLineItem(CrosshairCenter + FVector2D(CrosshairGap, 0.0f), CrosshairCenter + FVector2D(CrosshairHalfSize, 0.0f)),
			FCanvasLineItem(CrosshairCenter - FVector2D(0.0f, CrosshairHalfSize), CrosshairCenter - FVector2D(0.0f, CrosshairGap)),
			FCanvasLineItem(CrosshairCenter + FVector2D(0.0f, CrosshairGap), CrosshairCenter + FVector2D(0.0f, CrosshairHalfSize)),
		};
		for (FCanvasLineItem& CrosshairLine : CrosshairLines)
		{
			CrosshairLine.LineThickness = 2.0f;
			CrosshairLine.SetColor(CrosshairColor);
			Canvas->DrawItem(CrosshairLine);
		}
	}
}

TArray<FString> AAstraeonHUD::BuildMenuLines(const AAstraeonPlayerController* PlayerController)
{
	TArray<FString> Lines;
	Lines.Reserve(7);
	Lines.Add(TEXT("ASTRAEON — La primera señal"));
	Lines.Add(TEXT("Sistemas iniciales listos."));
	Lines.Add(FString::Printf(TEXT("Semilla seleccionada: %d"), PlayerController ? PlayerController->GetSelectedMenuSeed() : 1001));
	Lines.Add(TEXT("RePág/AvPág: cambiar semilla"));
	Lines.Add(TEXT("Intro: nueva partida"));
	Lines.Add(TEXT("F9: continuar partida guardada"));
	Lines.Add(TEXT("Objetivo: explorar, medir, comprender, actuar, registrar."));
	return Lines;
}

TArray<FString> AAstraeonHUD::BuildStatusLines(const UAstraeonGameInstance* GameInstance)
{
	TArray<FString> Lines;
	Lines.Reserve(14);
	Lines.Add(TEXT("ASTRAEON — La primera señal"));

	if (!GameInstance || !GameInstance->HasStartedGame())
	{
		Lines.Add(TEXT("Sesión: esperando nueva partida"));
		return Lines;
	}

	const FAstraeonEnvironmentalSnapshot& Environment = GameInstance->GetCurrentEnvironment();
	Lines.Add(FString::Printf(TEXT("Semilla: %d | Generador: %d"), Environment.WorldSeed, Environment.GeneratorVersion));
	Lines.Add(FString::Printf(TEXT("Gravedad m/s²: %.2f"), Environment.GravityMS2));
	Lines.Add(FString::Printf(TEXT("Temperatura K: %.2f"), Environment.TemperatureKelvin));
	Lines.Add(FString::Printf(TEXT("Presión kPa: %.2f"), Environment.PressureKPa));
	Lines.Add(FString::Printf(TEXT("Respirable: %s | Riesgo: %.2f"), Environment.bBreathable ? TEXT("sí") : TEXT("no"), Environment.EnvironmentalRisk01));
	Lines.Add(FString::Printf(TEXT("Región: %d recursos | %d PDI"), GameInstance->GetCurrentRegionLayout().Resources.Num(), GameInstance->GetCurrentRegionLayout().PointsOfInterest.Num()));
	Lines.Add(FString::Printf(TEXT("Celdas reveladas: %d"), GameInstance->GetRevealedMap().RevealedCells.Num()));
	const TCHAR* ObjectiveText = TEXT("Medir ambiente");
	if (GameInstance->GetObjectiveState() == EAstraeonObjectiveState::GatherResources)
	{
		ObjectiveText = TEXT("Recolectar recursos");
	}
	else if (GameInstance->GetObjectiveState() == EAstraeonObjectiveState::CraftSignalResonator)
	{
		ObjectiveText = TEXT("Fabricar signal_resonator");
	}
	else if (GameInstance->GetObjectiveState() == EAstraeonObjectiveState::ReachSignalSource)
	{
		ObjectiveText = TEXT("Alcanzar fuente de señal");
	}
	else if (GameInstance->GetObjectiveState() == EAstraeonObjectiveState::Completed)
	{
		ObjectiveText = TEXT("Completado");
	}
	Lines.Add(FString::Printf(TEXT("Objetivo: %s"), ObjectiveText));
	Lines.Add(FString::Printf(TEXT("Pista: %s"), *GameInstance->GetObjectiveHint()));
	if (!GameInstance->GetLastFeedbackMessage().IsEmpty())
	{
		Lines.Add(FString::Printf(TEXT("Estado: %s"), *GameInstance->GetLastFeedbackMessage()));
	}
	Lines.Add(FString::Printf(TEXT("Inventario: %d ítem(s)"), GameInstance->GetInventory().Num()));
	Lines.Add(FString::Printf(TEXT("Bitácora: %d entradas"), GameInstance->GetRuntimeLogbookEntries().Num()));
	Lines.Add(TEXT("WASD mover | Ratón mirar | Espacio saltar | Click izq. escanear | E interactuar | C fabricar | F5 guardar"));
	return Lines;
}
