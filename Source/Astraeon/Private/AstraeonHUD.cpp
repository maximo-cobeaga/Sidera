#include "AstraeonHUD.h"

#include "AstraeonGameInstance.h"
#include "AstraeonPlayerController.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"
#include "AstraeonPlayerCharacter.h"
#include "Building/AstraeonBuiltStructure.h"
#include "Ship/AstraeonShipPawn.h"
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
	const bool bMenuVisible = AstraeonPlayerController && AstraeonPlayerController->IsMenuVisible();
	const bool bFlying = AstraeonPlayerController && !bMenuVisible && AstraeonPlayerController->IsFlyingShip();
	const AAstraeonPlayerCharacter* AstraeonCharacter = AstraeonPlayerController
		? Cast<AAstraeonPlayerCharacter>(AstraeonPlayerController->GetPawn())
		: nullptr;
	const bool bBuilding = !bMenuVisible && !bFlying && AstraeonCharacter && AstraeonCharacter->IsBuildModeActive();
	const bool bFabricatorOpen = AstraeonPlayerController && !bMenuVisible && !bFlying && !bBuilding && AstraeonPlayerController->IsFabricatorOpen();
	const bool bInventoryVisible = AstraeonPlayerController && !bMenuVisible && !bFlying && !bBuilding && !bFabricatorOpen && AstraeonPlayerController->IsInventoryVisible();
	const bool bLogbookVisible = AstraeonPlayerController && !bMenuVisible && !bFlying && !bBuilding && !bFabricatorOpen && !bInventoryVisible && AstraeonPlayerController->IsLogbookVisible();
	const TArray<FString> Lines = bMenuVisible
		? BuildMenuLines(AstraeonPlayerController)
		: bFlying
			? BuildFlightLines(AstraeonPlayerController->GetShipPawn(), AstraeonGameInstance)
			: bBuilding
				? BuildBuildModeLines(AstraeonCharacter, AstraeonGameInstance)
				: bFabricatorOpen
				? BuildFabricatorLines(AstraeonGameInstance)
				: bInventoryVisible
					? BuildInventoryLines(AstraeonGameInstance)
					: bLogbookVisible
						? BuildLogbookLines(AstraeonGameInstance)
						: BuildStatusLines(AstraeonGameInstance);

	UFont* Font = GEngine->GetMediumFont() ? GEngine->GetMediumFont() : GEngine->GetSmallFont();
	constexpr float X = 36.0f;
	float Y = 36.0f;
	constexpr float LineHeight = 30.0f;

	// El panel se ajusta al contenido: la vista de bitácora y las líneas de protección
	// crecen según la partida, y un alto fijo dejaba texto fuera del fondo oscuro.
	const float BackgroundHeight = FMath::Max(120.0f, (Lines.Num() + 2) * LineHeight);
	FCanvasTileItem Background(FVector2D(20.0f, 20.0f), FVector2D(760.0f, BackgroundHeight), FLinearColor(0.0f, 0.0f, 0.0f, 0.62f));
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
				// O2 y salud críticos se pintan en rojo: son la única advertencia antes del
				// rescate de emergencia, que cuesta la carga suelta.
				const float Hunger = AstraeonGameInstance ? AstraeonGameInstance->GetHungerPercent() : 100.0f;
				const bool bCritical = Suit->GetOxygenPercent() <= 15.0f || Suit->GetHealthPercent() <= 25.0f || Hunger <= 15.0f;
				const FLinearColor SuitColor = Suit->IsInHaven()
					? FLinearColor(0.55f, 0.85f, 1.0f, 1.0f)
					: (bCritical ? FLinearColor(1.0f, 0.35f, 0.30f, 1.0f) : FLinearColor(0.70f, 1.0f, 0.70f, 1.0f));
				const FString SuitStatus = FString::Printf(TEXT("O2: %.0f%% | Salud: %.0f%% | Saciedad: %.0f%% | Mov x%.2f%s"),
					Suit->GetOxygenPercent(),
					Suit->GetHealthPercent(),
					Hunger,
					Suit->GetGravitySpeedMultiplier(),
					Hunger <= 0.0f ? TEXT("  [INANICIÓN — come con F]")
						: (Suit->IsInHaven() ? TEXT("  [ÍTACA: recargando]")
							: (bCritical ? TEXT("  [CRÍTICO]") : TEXT(""))));
				FCanvasTextItem SuitText(FVector2D(X, Y), FText::FromString(SuitStatus), Font, SuitColor);
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
	Lines.Add(TEXT("F10: continuar partida guardada"));
	Lines.Add(TEXT("Objetivo: explorar, medir, comprender, actuar, registrar."));
	return Lines;
}

FString AAstraeonHUD::DescribeItem(FName ItemId)
{
	static const TMap<FName, FString> ItemNames = {
		{TEXT("silicate_fiber"), TEXT("Fibra de silicato")},
		{TEXT("ferrite_nodule"), TEXT("Nódulo de ferrita")},
		{TEXT("cryo_ferrite_vein"), TEXT("Ferrita criogénica")},
		{TEXT("resonant_quartz_vein"), TEXT("Cuarzo resonante")},
		{TEXT("biomass_sample"), TEXT("Muestra de biomasa")},
		{TEXT("regolith"), TEXT("Regolito")},
		{TEXT("regolith_brick"), TEXT("Ladrillo de regolito")},
		{TEXT("signal_resonator"), TEXT("Resonador de señal")},
		{TEXT("weapon_pulse_cutter"), TEXT("Cortadora de pulso")},
		{TEXT("tool_core_drill"), TEXT("Taladro de núcleo")},
		{TEXT("tool_build_hammer"), TEXT("Martillo de obra")},
		{TEXT("tool_demolition_maul"), TEXT("Maza de demolición")},
		{TEXT("module_respirator"), TEXT("Respirador")},
		{TEXT("module_thermal_shield"), TEXT("Aislante térmico")},
		{TEXT("module_pressure_seal"), TEXT("Sellado de presión")},
		{TEXT("ration_pack"), TEXT("Ración")}
	};

	if (const FString* Found = ItemNames.Find(ItemId))
	{
		return *Found;
	}

	// Un recurso característico de seed que no esté en la tabla se muestra con su id, que
	// sigue siendo legible, en vez de esconderse.
	return ItemId.ToString();
}

TArray<FString> AAstraeonHUD::BuildBuildModeLines(const AAstraeonPlayerCharacter* PlayerCharacter, const UAstraeonGameInstance* GameInstance)
{
	TArray<FString> Lines;
	Lines.Add(TEXT("MODO CONSTRUCCIÓN — B sale"));

	if (!PlayerCharacter || !GameInstance)
	{
		return Lines;
	}

	const EAstraeonStructureType Selected = PlayerCharacter->GetSelectedStructureType();
	const int32 Cost = AAstraeonBuiltStructure::GetStructureBrickCost(Selected);
	const int32 Bricks = GameInstance->GetInventoryItemCount(UAstraeonGameInstance::GetBrickItemId());

	Lines.Add(FString::Printf(TEXT("Pieza: %s   (Q cambia)"), *AAstraeonBuiltStructure::DescribeStructure(Selected)));
	Lines.Add(FString::Printf(TEXT("Coste: %d ladrillo(s)   |   Tienes: %d"), Cost, Bricks));
	Lines.Add(TEXT("R rota 45°  |  Click izq. coloca  |  Click der. demuele"));
	Lines.Add(PlayerCharacter->IsPlacementValid()
		? TEXT("LISTO PARA COLOCAR")
		: (Bricks < Cost ? TEXT("SIN MATERIAL SUFICIENTE") : TEXT("APUNTA A UNA SUPERFICIE")));
	Lines.Add(FString::Printf(TEXT("Construcciones levantadas: %d"), GameInstance->GetPlacedStructures().Num()));

	if (!GameInstance->HasDemolitionMaul())
	{
		Lines.Add(TEXT("Sin maza de demolición: no puedes derribar todavía."));
	}

	if (!GameInstance->GetLastFeedbackMessage().IsEmpty())
	{
		Lines.Add(FString::Printf(TEXT("Estado: %s"), *GameInstance->GetLastFeedbackMessage()));
	}
	return Lines;
}

TArray<FString> AAstraeonHUD::BuildInventoryLines(const UAstraeonGameInstance* GameInstance)
{
	TArray<FString> Lines;
	Lines.Add(TEXT("INVENTARIO — I cierra"));

	if (!GameInstance)
	{
		return Lines;
	}

	const TMap<FName, int32>& Inventory = GameInstance->GetInventory();
	if (Inventory.IsEmpty())
	{
		Lines.Add(TEXT("Vacío. Escanea con Click Izq. y recoge vetas con E."));
		return Lines;
	}

	// Cuatro categorías en vez de dos: con construcción y herramientas, una lista plana de
	// ids crudos dejó de ser legible. El nombre se muestra en castellano y con la cantidad
	// alineada, para poder leer de un vistazo si alcanza el material.
	TArray<FString> Materials;
	TArray<FString> BuildingLines;
	TArray<FString> Tools;
	TArray<FString> Equipment;

	for (const TPair<FName, int32>& Entry : Inventory)
	{
		if (Entry.Value <= 0)
		{
			continue;
		}

		const FString ItemId = Entry.Key.ToString();
		const FString Line = FString::Printf(TEXT("   %-26s %4d"), *DescribeItem(Entry.Key), Entry.Value);

		if (ItemId.StartsWith(TEXT("tool_")) || ItemId.StartsWith(TEXT("weapon_")))
		{
			Tools.Add(Line);
		}
		else if (ItemId.StartsWith(TEXT("module_")) || Entry.Key == TEXT("signal_resonator"))
		{
			Equipment.Add(Line);
		}
		else if (Entry.Key == UAstraeonGameInstance::GetBrickItemId() || Entry.Key == UAstraeonGameInstance::GetRegolithItemId())
		{
			BuildingLines.Add(Line);
		}
		else if (Entry.Key == UAstraeonGameInstance::GetRationItemId())
		{
			Equipment.Add(Line);
		}
		else
		{
			Materials.Add(Line);
		}
	}

	Materials.Sort();
	BuildingLines.Sort();
	Tools.Sort();
	Equipment.Sort();

	auto AppendSection = [&Lines](const TCHAR* Title, const TArray<FString>& Section)
	{
		if (!Section.IsEmpty())
		{
			Lines.Add(FString::Printf(TEXT("── %s"), Title));
			Lines.Append(Section);
		}
	};

	AppendSection(TEXT("MATERIALES"), Materials);
	AppendSection(TEXT("CONSTRUCCIÓN"), BuildingLines);
	AppendSection(TEXT("HERRAMIENTAS"), Tools);
	AppendSection(TEXT("EQUIPO"), Equipment);

	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("Protección equipada: %s"),
		*UAstraeonSuitComponent::DescribeProtection(GameInstance->GetEquippedProtection())));

	if (GameInstance->GetInventoryItemCount(TEXT("signal_resonator")) <= 0)
	{
		Lines.Add(TEXT("Nota: la mesa reserva los insumos del resonador hasta fabricarlo."));
	}
	return Lines;
}

TArray<FString> AAstraeonHUD::BuildFabricatorLines(const UAstraeonGameInstance* GameInstance)
{
	TArray<FString> Lines;
	Lines.Add(TEXT("MESA DE FABRICACIÓN — E cierra"));

	if (!GameInstance)
	{
		return Lines;
	}

	const TArray<FAstraeonCraftingRecipe> Recipes = GameInstance->GetCraftingRecipes();
	// Las teclas siguen el mismo orden que las de equipar: 0 resonador, 1/2/3 módulos.
	const TCHAR* RecipeKeys[] = { TEXT("0"), TEXT("1"), TEXT("2"), TEXT("3"), TEXT("4"), TEXT("5") };

	for (int32 Index = 0; Index < Recipes.Num(); ++Index)
	{
		const FAstraeonCraftingRecipe& Recipe = Recipes[Index];

		FString InputsText;
		for (const FAstraeonRecipeInput& Input : Recipe.Inputs)
		{
			if (!InputsText.IsEmpty())
			{
				InputsText += TEXT(" + ");
			}
			InputsText += FString::Printf(TEXT("%dx %s"), Input.Quantity, *Input.ItemId.ToString());
		}

		FString Reason;
		const bool bCanCraft = GameInstance->CanCraftRecipe(Recipe, Reason);
		const FString KeyText = Recipes.IsValidIndex(Index) && Index < UE_ARRAY_COUNT(RecipeKeys) ? RecipeKeys[Index] : TEXT("-");

		Lines.Add(FString::Printf(TEXT("[%s] %s — %s%s"),
			*KeyText,
			*Recipe.DisplayName.ToString(),
			*InputsText,
			bCanCraft ? TEXT("") : *FString::Printf(TEXT("  (%s)"), *Reason)));
	}

	Lines.Add(FString::Printf(TEXT("Inventario: %d ítem(s)"), GameInstance->GetInventory().Num()));
	if (!GameInstance->GetLastFeedbackMessage().IsEmpty())
	{
		Lines.Add(FString::Printf(TEXT("Estado: %s"), *GameInstance->GetLastFeedbackMessage()));
	}
	return Lines;
}

TArray<FString> AAstraeonHUD::BuildFlightLines(const AAstraeonShipPawn* ShipPawn, const UAstraeonGameInstance* GameInstance)
{
	TArray<FString> Lines;
	Lines.Add(TEXT("ÍTACA — EN VUELO"));

	if (!ShipPawn)
	{
		return Lines;
	}

	const float AltitudeCm = ShipPawn->GetAltitudeCm();
	Lines.Add(AltitudeCm < 0.0f
		? FString(TEXT("Altitud: sin suelo bajo la nave"))
		: FString::Printf(TEXT("Altitud: %.0f m"), AltitudeCm / 100.0f));
	Lines.Add(FString::Printf(TEXT("Velocidad: %.0f km/h"), ShipPawn->GetSpeedKmH()));

	if (ShipPawn->IsAtAltitudeCeiling())
	{
		Lines.Add(FString::Printf(TEXT("TECHO ATMOSFÉRICO (%.0f m): ARGOS no registra ningún destino fuera de esta región."),
			AAstraeonShipPawn::GetAltitudeCeilingCm() / 100.0f));
	}

	Lines.Add(ShipPawn->CanLandHere()
		? FString(TEXT("Listo para aterrizar: E posa Ítaca aquí."))
		: FString(TEXT("Para aterrizar: desciende y reduce la velocidad, luego E.")));
	Lines.Add(TEXT("WASD desplaza | Ratón orienta | Espacio sube | Ctrl baja | Shift acelera"));

	if (GameInstance && !GameInstance->GetLastFeedbackMessage().IsEmpty())
	{
		Lines.Add(FString::Printf(TEXT("Estado: %s"), *GameInstance->GetLastFeedbackMessage()));
	}
	return Lines;
}

TArray<FString> AAstraeonHUD::BuildLogbookLines(const UAstraeonGameInstance* GameInstance)
{
	TArray<FString> Lines;
	Lines.Add(TEXT("BITÁCORA — L para cerrar"));

	if (!GameInstance || GameInstance->GetRuntimeLogbookEntries().IsEmpty())
	{
		Lines.Add(TEXT("Sin entradas todavía."));
		return Lines;
	}

	for (const FAstraeonLogbookEntry& Entry : GameInstance->GetRuntimeLogbookEntries())
	{
		Lines.Add(FString::Printf(TEXT("• %s"), *Entry.Title.ToString()));
		Lines.Add(FString::Printf(TEXT("   %s"), *Entry.Summary.ToString()));
	}
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
	Lines.Add(FString::Printf(TEXT("Variación: %d | Región: %s"), GameInstance->GetCurrentContentSeed(),
		*GameInstance->GetCurrentRegionProfileId().ToString()));
	Lines.Add(FString::Printf(TEXT("Gravedad m/s²: %.2f"), Environment.GravityMS2));
	Lines.Add(FString::Printf(TEXT("Temperatura K: %.2f"), Environment.TemperatureKelvin));
	Lines.Add(FString::Printf(TEXT("Presión kPa: %.2f"), Environment.PressureKPa));
	Lines.Add(FString::Printf(TEXT("Respirable: %s | Riesgo: %.2f"), Environment.bBreathable ? TEXT("sí") : TEXT("no"), Environment.EnvironmentalRisk01));

	// La medición sólo sirve si el jugador puede leer qué lo está dañando y con qué
	// compensarlo: por eso amenazas y protección van juntas, debajo de los datos.
	const TArray<FString> Hazards = UAstraeonSuitComponent::DescribeActiveHazards(Environment);
	Lines.Add(Hazards.IsEmpty()
		? FString(TEXT("Amenazas: ninguna activa"))
		: FString::Printf(TEXT("Amenazas: %s"), *FString::Join(Hazards, TEXT(" | "))));

	const EAstraeonProtectionModule Equipped = GameInstance->GetEquippedProtection();
	const EAstraeonProtectionModule Recommended = UAstraeonSuitComponent::RecommendProtection(Environment);
	FString ProtectionLine = FString::Printf(TEXT("Protección: %s"), *UAstraeonSuitComponent::DescribeProtection(Equipped));
	if (Recommended != EAstraeonProtectionModule::None && Equipped != Recommended)
	{
		ProtectionLine += FString::Printf(TEXT("  → conviene: %s"), *UAstraeonSuitComponent::DescribeProtection(Recommended));
	}
	Lines.Add(ProtectionLine);
	Lines.Add(TEXT("Módulos: 1 respirador | 2 aislante térmico | 3 sellado de presión | 0 ninguno"));
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
	// Barra rápida: lo que se puede llevar en la mano, con la ranura activa marcada. Sin
	// esto el jugador no sabría qué tiene empuñado ni con qué tecla cambiarlo.
	const TArray<FName> Hotbar = GameInstance->GetHotbarItems();
	if (Hotbar.IsEmpty())
	{
		Lines.Add(TEXT("Manos vacías. Fabrica herramientas en la mesa de Ítaca."));
	}
	else
	{
		FString HotbarLine;
		for (int32 Index = 0; Index < Hotbar.Num() && Index < 6; ++Index)
		{
			const bool bSelected = Hotbar[Index] == GameInstance->GetHandItemId();
			HotbarLine += FString::Printf(TEXT("%s[%d] %s%s   "),
				bSelected ? TEXT(">") : TEXT(" "),
				Index + 1,
				*DescribeItem(Hotbar[Index]),
				bSelected ? TEXT(" <") : TEXT(""));
		}
		Lines.Add(HotbarLine);
	}

	Lines.Add(GameInstance->GetHandItemId().IsNone()
		? FString(TEXT("En mano: nada"))
		: FString::Printf(TEXT("En mano: %s"), *DescribeItem(GameInstance->GetHandItemId())));

	Lines.Add(GameInstance->HasPulseCutter()
		? TEXT("WASD | Shift correr | Click izq. escanear | Click der. disparar | E interactuar | V cámara | I inventario | L bitácora | F6 guardar")
		: TEXT("WASD | Shift correr | Espacio saltar | Click izq. escanear | E interactuar | V cámara | I inventario | L bitácora | F6 guardar"));
	return Lines;
}
