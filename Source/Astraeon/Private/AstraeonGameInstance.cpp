#include "AstraeonGameInstance.h"

#include "Exploration/AstraeonMapRevealLibrary.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Persistence/AstraeonSaveGame.h"
#include "Building/AstraeonBuiltStructure.h"
#include "Survival/AstraeonSuitComponent.h"
#include "Environment/AstraeonItacaInterior.h"
#include "WorldGen/AstraeonTerrainField.h"
#include "WorldGen/AstraeonWorldGenerator.h"
#include "WorldGen/AstraeonWorldProfiles.h"

namespace AstraeonSession
{
	constexpr int32 DefaultWorldSeed = 1001;
	const FName InitialEnvironmentEntryId(TEXT("environment.initial_measurement"));
	const FName ArgosBriefingEntryId(TEXT("argos.first_signal_briefing"));
	const FName SurfaceDeploymentEntryId(TEXT("itaca.surface_deployment"));
	const FName SignalResonatorItemId(TEXT("signal_resonator"));
	const FName SilicateFiberItemId(TEXT("silicate_fiber"));
	const FName FerriteNoduleItemId(TEXT("ferrite_nodule"));
	const FName PulseCutterItemId(TEXT("weapon_pulse_cutter"));
	const FName CoreDrillItemId(TEXT("tool_core_drill"));
	const FName RegolithItemId(TEXT("regolith"));
	const FName BrickItemId(TEXT("regolith_brick"));
	const FName BuildHammerItemId(TEXT("tool_build_hammer"));
	const FName DemolitionMaulItemId(TEXT("tool_demolition_maul"));
	constexpr int32 RegolithPerExtraction = 3;
	const FName RationItemId(TEXT("ration_pack"));
	constexpr float RationNourishmentPercent = 45.0f;
	// Cuatro minutos: castiga arrasar con la fauna sin dejar al jugador sin comida.
	constexpr float CreatureRespawnSeconds = 240.0f;

	EAstraeonSavedObjectiveState ToSavedObjectiveState(EAstraeonObjectiveState State)
	{
		return static_cast<EAstraeonSavedObjectiveState>(static_cast<uint8>(State));
	}

	EAstraeonObjectiveState FromSavedObjectiveState(EAstraeonSavedObjectiveState State)
	{
		return static_cast<EAstraeonObjectiveState>(static_cast<uint8>(State));
	}
}

void UAstraeonGameInstance::StartNewGame(int32 RequestedWorldSeed)
{
	CurrentWorldSeed = NormalizeRequestedSeed(RequestedWorldSeed);
	CurrentEnvironment = UAstraeonWorldProfiles::GetMvpPlanetProfile().Environment;
	// Compatibilidad de HUD, save y diagnóstico: el campo histórico se vuelve metadata de
	// variación, nunca una causa de geografía o de los valores ambientales authored.
	CurrentEnvironment.WorldSeed = CurrentWorldSeed;
	CurrentRegionLayout = UAstraeonWorldProfiles::BuildFixedRegionLayout(CurrentWorldSeed);
	RevealedMap = FAstraeonRevealedMap();
	UAstraeonMapRevealLibrary::RevealRadius(RevealedMap, FVector2D::ZeroVector, 1);
	Inventory.Reset();
	ObjectiveState = EAstraeonObjectiveState::MeasureEnvironment;
	RuntimeLogbookEntries.Reset();
	EquippedProtection = EAstraeonProtectionModule::None;
	// Ítaca se apoya SOBRE la plataforma de terreno, no a su misma cota. Posarla en la
	// altura del suelo dejaba la cara superior de su piso y la del bloque de terreno en el
	// mismo Z: se veía roca en vez de la cubierta y el jugador quedaba atrapado entre las
	// dos superficies. La cota de la plataforma es la misma que usa el generador de relieve.
	ItacaOriginCm = FVector(0.0f, 0.0f,
		AAstraeonTerrainField::GetItacaPadHeightCm(GetCurrentTerrainSeed(), 0.0f, 0.0f)
			+ AAstraeonItacaInterior::GetDeckThicknessCm());
	PlacedStructures.Reset();
	HandItemId = NAME_None;
	HungerPercent = 100.0f;
	CreatureRespawnTimers.Reset();
	LastFeedbackMessage = TEXT("Nueva expedición iniciada. Interactúa con la consola ARGOS para comenzar.");

	FAstraeonLogbookEntry InitialEnvironmentEntry;
	InitialEnvironmentEntry.EntryId = AstraeonSession::InitialEnvironmentEntryId;
	InitialEnvironmentEntry.Title = FText::FromString(TEXT("Medición ambiental inicial"));
	InitialEnvironmentEntry.Summary = FText::Format(
		FText::FromString(TEXT("ARGOS midió gravedad {0} m/s², temperatura {1} K y presión {2} kPa.")),
		FText::AsNumber(CurrentEnvironment.GravityMS2),
		FText::AsNumber(CurrentEnvironment.TemperatureKelvin),
		FText::AsNumber(CurrentEnvironment.PressureKPa));
	InitialEnvironmentEntry.Certainty = EAstraeonDiscoveryCertainty::Measured;
	UpsertRuntimeLogbookEntry(InitialEnvironmentEntry);

	bHasStartedGame = true;
}

int32 UAstraeonGameInstance::GetInventoryItemCount(FName ItemId) const
{
	const int32* Count = Inventory.Find(ItemId);
	return Count ? *Count : 0;
}

bool UAstraeonGameInstance::AddInventoryItem(FName ItemId, int32 Quantity)
{
	if (!bHasStartedGame || ItemId.IsNone() || Quantity <= 0)
	{
		return false;
	}

	Inventory.FindOrAdd(ItemId) += Quantity;
	SetLastFeedbackMessage(FString::Printf(TEXT("Recolectado: %s"), *ItemId.ToString()));
	if (ObjectiveState == EAstraeonObjectiveState::GatherResources && Inventory.Num() >= 3)
	{
		ObjectiveState = EAstraeonObjectiveState::CraftSignalResonator;
	}
	return true;
}

bool UAstraeonGameInstance::ConsumeInventoryItem(FName ItemId, int32 Quantity)
{
	if (!bHasStartedGame || ItemId.IsNone() || Quantity <= 0)
	{
		return false;
	}

	int32* Count = Inventory.Find(ItemId);
	if (!Count || *Count < Quantity)
	{
		return false;
	}

	*Count -= Quantity;
	if (*Count <= 0)
	{
		Inventory.Remove(ItemId);
	}
	return true;
}

bool UAstraeonGameInstance::CanCraftSignalResonator() const
{
	if (!bHasStartedGame || CurrentRegionLayout.Resources.Num() < 3)
	{
		return false;
	}

	const FName SignatureResourceId = CurrentRegionLayout.Resources[2].ResourceId;
	return GetInventoryItemCount(AstraeonSession::SilicateFiberItemId) >= 1
		&& GetInventoryItemCount(AstraeonSession::FerriteNoduleItemId) >= 1
		&& GetInventoryItemCount(SignatureResourceId) >= 1;
}

bool UAstraeonGameInstance::CraftSignalResonator()
{
	if (!CanCraftSignalResonator())
	{
		return false;
	}

	const FName SignatureResourceId = CurrentRegionLayout.Resources[2].ResourceId;
	ConsumeInventoryItem(AstraeonSession::SilicateFiberItemId, 1);
	ConsumeInventoryItem(AstraeonSession::FerriteNoduleItemId, 1);
	ConsumeInventoryItem(SignatureResourceId, 1);
	AddInventoryItem(AstraeonSession::SignalResonatorItemId, 1);
	ObjectiveState = EAstraeonObjectiveState::ReachSignalSource;

	SetLastFeedbackMessage(TEXT("Fabricado: signal_resonator. Sigue el marcador SEÑAL e interactúa con E."));

	FAstraeonLogbookEntry CraftingEntry;
	CraftingEntry.EntryId = TEXT("recipe.signal_resonator");
	CraftingEntry.Title = FText::FromString(TEXT("Resonador de señal"));
	CraftingEntry.Summary = FText::FromString(TEXT("Un resonador de campo ajustado con propiedades locales; debería estabilizar el camino de la señal desconocida."));
	CraftingEntry.Certainty = EAstraeonDiscoveryCertainty::Confirmed;
	UpsertRuntimeLogbookEntry(CraftingEntry);
	return true;
}

int32 UAstraeonGameInstance::RevealMapAroundLocationMeters(FVector2D LocationMeters, int32 RadiusCells)
{
	if (!bHasStartedGame)
	{
		return 0;
	}

	return UAstraeonMapRevealLibrary::RevealRadius(RevealedMap, LocationMeters, RadiusCells);
}

bool UAstraeonGameInstance::ScanCurrentEnvironment()
{
	if (!bHasStartedGame)
	{
		return false;
	}

	FAstraeonLogbookEntry ConfirmedEnvironmentEntry;
	ConfirmedEnvironmentEntry.EntryId = AstraeonSession::InitialEnvironmentEntryId;
	ConfirmedEnvironmentEntry.Title = FText::FromString(TEXT("Escaneo ambiental confirmado"));
	ConfirmedEnvironmentEntry.Summary = FText::Format(
		FText::FromString(TEXT("Confirmación del escáner: gravedad {0} m/s², temperatura {1} K, presión {2} kPa, respirable {3}.")),
		FText::AsNumber(CurrentEnvironment.GravityMS2),
		FText::AsNumber(CurrentEnvironment.TemperatureKelvin),
		FText::AsNumber(CurrentEnvironment.PressureKPa),
		CurrentEnvironment.bBreathable ? FText::FromString(TEXT("sí")) : FText::FromString(TEXT("no")));
	ConfirmedEnvironmentEntry.Certainty = EAstraeonDiscoveryCertainty::Confirmed;
	UpsertRuntimeLogbookEntry(ConfirmedEnvironmentEntry);
	if (ObjectiveState == EAstraeonObjectiveState::MeasureEnvironment)
	{
		ObjectiveState = EAstraeonObjectiveState::GatherResources;
	}
	SetLastFeedbackMessage(TEXT("Escaneo ambiental confirmado. Busca cubos VERDES con etiqueta RECURSO y recógelos con E."));
	return true;
}

bool UAstraeonGameInstance::RecordCreatureScan(const FAstraeonCreatureProfile& CreatureProfile)
{
	if (!bHasStartedGame || CreatureProfile.SpeciesId.IsNone())
	{
		return false;
	}

	FAstraeonLogbookEntry CreatureEntry;
	CreatureEntry.EntryId = FName(*FString::Printf(TEXT("creature.%s"), *CreatureProfile.SpeciesId.ToString()));
	CreatureEntry.Title = CreatureProfile.DisplayName;
	CreatureEntry.Summary = CreatureProfile.ScannerSummary;
	CreatureEntry.Certainty = EAstraeonDiscoveryCertainty::Observed;
	UpsertRuntimeLogbookEntry(CreatureEntry);
	SetLastFeedbackMessage(TEXT("Organismo observado y añadido a la bitácora."));
	return true;
}

bool UAstraeonGameInstance::RecordMarkerScan(FName MarkerId, bool bIsResource)
{
	if (!bHasStartedGame || MarkerId.IsNone())
	{
		return false;
	}

	FAstraeonLogbookEntry Entry;

	if (bIsResource)
	{
		Entry.EntryId = FName(*FString::Printf(TEXT("resource.%s"), *MarkerId.ToString()));
		Entry.Title = FText::Format(
			FText::FromString(TEXT("Material analizado: {0}")),
			FText::FromName(MarkerId));
		Entry.Summary = FText::FromString(TEXT("El escáner confirma una veta explotable. Sus propiedades resonantes lo hacen apto para el ensamblado del resonador."));
		Entry.Certainty = EAstraeonDiscoveryCertainty::Measured;
		SetLastFeedbackMessage(FString::Printf(TEXT("Material analizado: %s. Recógelo con E."), *MarkerId.ToString()));
	}
	else if (MarkerId == TEXT("signal_source"))
	{
		Entry.EntryId = TEXT("signal.source_survey");
		Entry.Title = FText::FromString(TEXT("Estructura de la fuente de señal"));
		Entry.Summary = FText::FromString(TEXT("La emisión es de banda estrecha y periodo estable: no es geológica. La estructura responde al barrido, pero el patrón sólo se estabiliza con un resonador sintonizado."));
		Entry.Certainty = EAstraeonDiscoveryCertainty::Inferred;
		SetLastFeedbackMessage(TEXT("Fuente de señal barrida. Necesitas un signal_resonator para resolverla."));
	}
	else if (MarkerId == TEXT("minor_geologic_anomaly"))
	{
		Entry.EntryId = TEXT("anomaly.minor_geologic");
		Entry.Title = FText::FromString(TEXT("Anomalía geológica menor"));
		Entry.Summary = FText::FromString(TEXT("Una veta mineral con una orientación que no corresponde al estrato circundante. A esta distancia sólo se observa; acércate e inspecciónala con E para medirla."));
		Entry.Certainty = EAstraeonDiscoveryCertainty::Observed;
		SetLastFeedbackMessage(TEXT("Anomalía observada. Acércate e inspecciónala con E."));
	}
	else
	{
		return false;
	}

	UpsertRuntimeLogbookEntry(Entry);
	return true;
}

FName UAstraeonGameInstance::GetProtectionItemId(EAstraeonProtectionModule Protection)
{
	switch (Protection)
	{
	case EAstraeonProtectionModule::Respirator:
		return TEXT("module_respirator");
	case EAstraeonProtectionModule::ThermalShield:
		return TEXT("module_thermal_shield");
	case EAstraeonProtectionModule::PressureSeal:
		return TEXT("module_pressure_seal");
	default:
		return NAME_None;
	}
}

FName UAstraeonGameInstance::GetRationItemId() { return AstraeonSession::RationItemId; }

bool UAstraeonGameInstance::IsHandheldItem(FName ItemId)
{
	const FString Name = ItemId.ToString();
	return Name.StartsWith(TEXT("tool_"))
		|| Name.StartsWith(TEXT("weapon_"))
		|| ItemId == AstraeonSession::RationItemId;
}

TArray<FName> UAstraeonGameInstance::GetHotbarItems() const
{
	TArray<FName> Handheld;
	for (const TPair<FName, int32>& Entry : Inventory)
	{
		if (Entry.Value > 0 && IsHandheldItem(Entry.Key))
		{
			Handheld.Add(Entry.Key);
		}
	}

	// Orden estable: el mapa de inventario no garantiza orden, y una barra rápida que
	// reordena sus ranuras sola sería inusable.
	Handheld.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
	return Handheld;
}

bool UAstraeonGameInstance::SelectHotbarSlot(int32 SlotIndex)
{
	const TArray<FName> Handheld = GetHotbarItems();
	if (!Handheld.IsValidIndex(SlotIndex))
	{
		return false;
	}

	HandItemId = Handheld[SlotIndex];
	SetLastFeedbackMessage(FString::Printf(TEXT("En mano: %s"), *HandItemId.ToString()));
	return true;
}

bool UAstraeonGameInstance::UseHandItem()
{
	if (!bHasStartedGame || HandItemId.IsNone() || GetInventoryItemCount(HandItemId) <= 0)
	{
		return false;
	}

	if (HandItemId == AstraeonSession::RationItemId)
	{
		ConsumeInventoryItem(AstraeonSession::RationItemId, 1);
		Nourish(AstraeonSession::RationNourishmentPercent);
		SetLastFeedbackMessage(FString::Printf(TEXT("Ración consumida. Saciedad: %.0f%%."), HungerPercent));

		if (GetInventoryItemCount(AstraeonSession::RationItemId) <= 0)
		{
			HandItemId = NAME_None;
		}
		return true;
	}

	SetLastFeedbackMessage(TEXT("Ese objeto se usa con su propio gesto, no con F."));
	return false;
}

FName UAstraeonGameInstance::GetRegolithItemId() { return AstraeonSession::RegolithItemId; }
FName UAstraeonGameInstance::GetBrickItemId() { return AstraeonSession::BrickItemId; }
FName UAstraeonGameInstance::GetBuildHammerItemId() { return AstraeonSession::BuildHammerItemId; }
FName UAstraeonGameInstance::GetDemolitionMaulItemId() { return AstraeonSession::DemolitionMaulItemId; }

bool UAstraeonGameInstance::HasBuildHammer() const
{
	return GetInventoryItemCount(AstraeonSession::BuildHammerItemId) > 0;
}

bool UAstraeonGameInstance::HasDemolitionMaul() const
{
	return GetInventoryItemCount(AstraeonSession::DemolitionMaulItemId) > 0;
}

bool UAstraeonGameInstance::ExtractRegolith()
{
	if (!bHasStartedGame)
	{
		return false;
	}

	// El taladro sirve para dos cosas: abrir vetas profundas y sacar regolito del suelo.
	// Darle un segundo uso evita inventar otra herramienta para el mismo gesto.
	if (!IsHolding(AstraeonSession::CoreDrillItemId))
	{
		SetLastFeedbackMessage(GetInventoryItemCount(AstraeonSession::CoreDrillItemId) > 0
			? TEXT("Lleva el taladro en la mano para extraer regolito (teclas 1-6).")
			: TEXT("Necesitas el taladro de núcleo para extraer regolito."));
		return false;
	}

	AddInventoryItem(AstraeonSession::RegolithItemId, AstraeonSession::RegolithPerExtraction);
	SetLastFeedbackMessage(FString::Printf(TEXT("Regolito extraído (+%d). Conviértelo en ladrillos en la mesa."),
		AstraeonSession::RegolithPerExtraction));
	return true;
}

bool UAstraeonGameInstance::CanPlaceStructure(EAstraeonStructureType Type) const
{
	return bHasStartedGame
		&& HasBuildHammer()
		&& GetInventoryItemCount(AstraeonSession::BrickItemId) >= AAstraeonBuiltStructure::GetStructureBrickCost(Type);
}

bool UAstraeonGameInstance::PlaceStructure(const FAstraeonPlacedStructure& Placement)
{
	if (!CanPlaceStructure(Placement.Type))
	{
		SetLastFeedbackMessage(HasBuildHammer()
			? TEXT("Faltan ladrillos para esta pieza.")
			: TEXT("Necesitas el martillo de obra para construir."));
		return false;
	}

	ConsumeInventoryItem(AstraeonSession::BrickItemId, AAstraeonBuiltStructure::GetStructureBrickCost(Placement.Type));
	PlacedStructures.Add(Placement);
	SetLastFeedbackMessage(FString::Printf(TEXT("%s construido. Ladrillos restantes: %d."),
		*AAstraeonBuiltStructure::DescribeStructure(Placement.Type),
		GetInventoryItemCount(AstraeonSession::BrickItemId)));
	return true;
}

bool UAstraeonGameInstance::DemolishStructureAt(const FVector& LocationCm, float ToleranceCm)
{
	if (!bHasStartedGame)
	{
		return false;
	}

	if (!HasDemolitionMaul())
	{
		SetLastFeedbackMessage(TEXT("Necesitas la maza de demolición para derribar."));
		return false;
	}

	const int32 Index = PlacedStructures.IndexOfByPredicate([&LocationCm, ToleranceCm](const FAstraeonPlacedStructure& Candidate)
	{
		return FVector::Dist(Candidate.LocationCm, LocationCm) <= ToleranceCm;
	});

	if (Index == INDEX_NONE)
	{
		return false;
	}

	const EAstraeonStructureType DemolishedType = PlacedStructures[Index].Type;
	PlacedStructures.RemoveAt(Index);

	const int32 Refund = AAstraeonBuiltStructure::GetStructureRefund(DemolishedType);
	AddInventoryItem(AstraeonSession::BrickItemId, Refund);
	SetLastFeedbackMessage(FString::Printf(TEXT("%s demolido. Recuperaste %d ladrillo(s)."),
		*AAstraeonBuiltStructure::DescribeStructure(DemolishedType), Refund));
	return true;
}

bool UAstraeonGameInstance::IsCriticalPathResource(FName ItemId) const
{
	// Los tres primeros recursos del layout son el recorrido crítico; el resto son vetas
	// profundas opcionales.
	for (int32 Index = 0; Index < FMath::Min(3, CurrentRegionLayout.Resources.Num()); ++Index)
	{
		if (CurrentRegionLayout.Resources[Index].ResourceId == ItemId)
		{
			return true;
		}
	}

	return false;
}

bool UAstraeonGameInstance::RecordEmergencyRecall()
{
	if (!bHasStartedGame)
	{
		return false;
	}

	// Se pierde la carga opcional. El equipo fabricado (herramientas, arma, módulos,
	// resonador) sobrevive: perderlo obligaría a refabricar y podría dejar al jugador sin
	// forma de avanzar.
	TArray<FName> LostItems;
	for (const TPair<FName, int32>& Entry : Inventory)
	{
		const FString ItemName = Entry.Key.ToString();
		const bool bIsCraftedEquipment = ItemName.StartsWith(TEXT("module_"))
			|| ItemName.StartsWith(TEXT("tool_"))
			|| ItemName.StartsWith(TEXT("weapon_"))
			|| Entry.Key == AstraeonSession::SignalResonatorItemId;

		if (!bIsCraftedEquipment && !IsCriticalPathResource(Entry.Key))
		{
			LostItems.Add(Entry.Key);
		}
	}

	for (const FName& LostItem : LostItems)
	{
		Inventory.Remove(LostItem);
	}

	FAstraeonLogbookEntry RecallEntry;
	RecallEntry.EntryId = TEXT("itaca.emergency_recall");
	RecallEntry.Title = FText::FromString(TEXT("Rescate de emergencia"));
	RecallEntry.Summary = FText::FromString(TEXT("El traje llegó a fallo estructural y ARGOS ejecutó una recuperación automática hacia Ítaca. La carga suelta quedó en el terreno; el equipo fabricado se conservó."));
	RecallEntry.Certainty = EAstraeonDiscoveryCertainty::Confirmed;
	UpsertRuntimeLogbookEntry(RecallEntry);

	SetLastFeedbackMessage(LostItems.IsEmpty()
		? TEXT("Rescate de emergencia: ARGOS te devolvió a Ítaca.")
		: FString::Printf(TEXT("Rescate de emergencia: perdiste %d tipo(s) de carga suelta."), LostItems.Num()));
	return true;
}

FName UAstraeonGameInstance::GetPulseCutterItemId()
{
	return AstraeonSession::PulseCutterItemId;
}

bool UAstraeonGameInstance::HasPulseCutter() const
{
	return GetInventoryItemCount(AstraeonSession::PulseCutterItemId) > 0;
}

bool UAstraeonGameInstance::RecordCreatureKill(const FAstraeonCreatureProfile& CreatureProfile)
{
	if (!bHasStartedGame || CreatureProfile.SpeciesId.IsNone())
	{
		return false;
	}

	if (!CreatureProfile.HarvestItemId.IsNone() && CreatureProfile.HarvestQuantity > 0)
	{
		AddInventoryItem(CreatureProfile.HarvestItemId, CreatureProfile.HarvestQuantity);
	}

	FAstraeonLogbookEntry KillEntry;
	KillEntry.EntryId = FName(*FString::Printf(TEXT("creature.%s.harvest"), *CreatureProfile.SpeciesId.ToString()));
	KillEntry.Title = FText::Format(FText::FromString(TEXT("Disección de campo: {0}")), CreatureProfile.DisplayName);
	KillEntry.Summary = FText::FromString(TEXT("Ejemplar abatido y muestreado. El tejido conserva la firma mineral de la región; ARGOS lo registra como dato, no como trofeo."));
	KillEntry.Certainty = EAstraeonDiscoveryCertainty::Measured;
	UpsertRuntimeLogbookEntry(KillEntry);

	SetLastFeedbackMessage(FString::Printf(TEXT("%s abatido. Muestra recogida."), *CreatureProfile.DisplayName.ToString()));
	return true;
}

void UAstraeonGameInstance::RecordCreatureDeath(FName SpawnPointId)
{
	if (SpawnPointId.IsNone())
	{
		return;
	}

	CreatureRespawnTimers.Add(SpawnPointId, AstraeonSession::CreatureRespawnSeconds);
}

bool UAstraeonGameInstance::IsCreatureSpawnPopulated(FName SpawnPointId) const
{
	return !CreatureRespawnTimers.Contains(SpawnPointId);
}

TArray<FName> UAstraeonGameInstance::AdvanceCreatureRespawns(float DeltaSeconds)
{
	TArray<FName> RepopulatedSpawnPoints;
	if (DeltaSeconds <= 0.0f || CreatureRespawnTimers.IsEmpty())
	{
		return RepopulatedSpawnPoints;
	}

	for (TPair<FName, float>& Timer : CreatureRespawnTimers)
	{
		Timer.Value -= DeltaSeconds;
		if (Timer.Value <= 0.0f)
		{
			RepopulatedSpawnPoints.Add(Timer.Key);
		}
	}

	for (const FName& SpawnPointId : RepopulatedSpawnPoints)
	{
		CreatureRespawnTimers.Remove(SpawnPointId);
	}

	return RepopulatedSpawnPoints;
}

int32 UAstraeonGameInstance::GetResourceNodeQuantity(FName ResourceId) const
{
	const FAstraeonResourceNode* Node = CurrentRegionLayout.Resources.FindByPredicate([ResourceId](const FAstraeonResourceNode& Candidate)
	{
		return Candidate.ResourceId == ResourceId;
	});

	return Node ? FMath::Max(1, Node->Quantity) : 1;
}

TArray<FAstraeonCraftingRecipe> UAstraeonGameInstance::GetCraftingRecipes() const
{
	TArray<FAstraeonCraftingRecipe> Recipes;

	auto MakeInput = [](FName ItemId, int32 Quantity)
	{
		FAstraeonRecipeInput Input;
		Input.ItemId = ItemId;
		Input.Quantity = Quantity;
		return Input;
	};

	if (CurrentRegionLayout.Resources.Num() >= 3)
	{
		FAstraeonCraftingRecipe Resonator;
		Resonator.RecipeId = TEXT("signal_resonator");
		Resonator.DisplayName = FText::FromString(TEXT("Resonador de señal"));
		Resonator.Description = FText::FromString(TEXT("Obligatorio para resolver la señal. La mesa protege sus insumos."));
		Resonator.Inputs = {
			MakeInput(AstraeonSession::SilicateFiberItemId, 1),
			MakeInput(AstraeonSession::FerriteNoduleItemId, 1),
			MakeInput(CurrentRegionLayout.Resources[2].ResourceId, 1)
		};
		Resonator.OutputItemId = AstraeonSession::SignalResonatorItemId;
		Recipes.Add(Resonator);
	}

	FAstraeonCraftingRecipe Respirator;
	Respirator.RecipeId = TEXT("module_respirator");
	Respirator.DisplayName = FText::FromString(TEXT("Respirador"));
	Respirator.Description = FText::FromString(TEXT("Frena la pérdida de oxígeno en atmósfera irrespirable."));
	Respirator.Inputs = { MakeInput(AstraeonSession::SilicateFiberItemId, 1) };
	Respirator.OutputItemId = GetProtectionItemId(EAstraeonProtectionModule::Respirator);
	Recipes.Add(Respirator);

	FAstraeonCraftingRecipe ThermalShield;
	ThermalShield.RecipeId = TEXT("module_thermal_shield");
	ThermalShield.DisplayName = FText::FromString(TEXT("Aislante térmico"));
	ThermalShield.Description = FText::FromString(TEXT("Mitiga el daño por temperatura extrema."));
	ThermalShield.Inputs = { MakeInput(AstraeonSession::FerriteNoduleItemId, 1) };
	ThermalShield.OutputItemId = GetProtectionItemId(EAstraeonProtectionModule::ThermalShield);
	Recipes.Add(ThermalShield);

	FAstraeonCraftingRecipe PressureSeal;
	PressureSeal.RecipeId = TEXT("module_pressure_seal");
	PressureSeal.DisplayName = FText::FromString(TEXT("Sellado de presión"));
	PressureSeal.Description = FText::FromString(TEXT("Mitiga el daño por presión extrema."));
	PressureSeal.Inputs = {
		MakeInput(AstraeonSession::SilicateFiberItemId, 1),
		MakeInput(AstraeonSession::FerriteNoduleItemId, 1)
	};
	PressureSeal.OutputItemId = GetProtectionItemId(EAstraeonProtectionModule::PressureSeal);
	Recipes.Add(PressureSeal);

	FAstraeonCraftingRecipe Cutter;
	Cutter.RecipeId = TEXT("weapon_pulse_cutter");
	Cutter.DisplayName = FText::FromString(TEXT("Cortadora de pulso"));
	Cutter.Description = FText::FromString(TEXT("Herramienta de corte reconvertida en arma. Click derecho dispara."));
	Cutter.Inputs = {
		MakeInput(AstraeonSession::FerriteNoduleItemId, 1),
		MakeInput(AstraeonSession::SilicateFiberItemId, 1)
	};
	Cutter.OutputItemId = AstraeonSession::PulseCutterItemId;
	Recipes.Add(Cutter);

	FAstraeonCraftingRecipe CoreDrill;
	CoreDrill.RecipeId = TEXT("tool_core_drill");
	CoreDrill.DisplayName = FText::FromString(TEXT("Taladro de núcleo"));
	CoreDrill.Description = FText::FromString(TEXT("Abre vetas profundas que la mano no alcanza. Basta con llevarlo encima."));
	CoreDrill.Inputs = {
		MakeInput(AstraeonSession::FerriteNoduleItemId, 2),
		MakeInput(AstraeonSession::SilicateFiberItemId, 1)
	};
	CoreDrill.OutputItemId = AstraeonSession::CoreDrillItemId;
	Recipes.Add(CoreDrill);

	FAstraeonCraftingRecipe Ration;
	Ration.RecipeId = TEXT("ration_pack");
	Ration.DisplayName = FText::FromString(TEXT("Raciones"));
	Ration.Description = FText::FromString(TEXT("Biomasa procesada. Llévalas en la mano y come con F."));
	Ration.Inputs = { MakeInput(TEXT("biomass_sample"), 2) };
	Ration.OutputItemId = AstraeonSession::RationItemId;
	Ration.OutputQuantity = 2;
	Recipes.Add(Ration);

	FAstraeonCraftingRecipe Brick;
	Brick.RecipeId = TEXT("regolith_brick");
	Brick.DisplayName = FText::FromString(TEXT("Ladrillos de regolito"));
	Brick.Description = FText::FromString(TEXT("Regolito compactado. Material de obra: se gasta al construir."));
	Brick.Inputs = { MakeInput(AstraeonSession::RegolithItemId, 3) };
	Brick.OutputItemId = AstraeonSession::BrickItemId;
	Brick.OutputQuantity = 2;
	Recipes.Add(Brick);

	FAstraeonCraftingRecipe BuildHammer;
	BuildHammer.RecipeId = TEXT("tool_build_hammer");
	BuildHammer.DisplayName = FText::FromString(TEXT("Martillo de obra"));
	BuildHammer.Description = FText::FromString(TEXT("Habilita el modo construcción (B)."));
	BuildHammer.Inputs = {
		MakeInput(AstraeonSession::FerriteNoduleItemId, 1),
		MakeInput(AstraeonSession::RegolithItemId, 2)
	};
	BuildHammer.OutputItemId = AstraeonSession::BuildHammerItemId;
	Recipes.Add(BuildHammer);

	FAstraeonCraftingRecipe DemolitionMaul;
	DemolitionMaul.RecipeId = TEXT("tool_demolition_maul");
	DemolitionMaul.DisplayName = FText::FromString(TEXT("Maza de demolición"));
	DemolitionMaul.Description = FText::FromString(TEXT("Derriba construcciones y recupera parte del material."));
	DemolitionMaul.Inputs = {
		MakeInput(AstraeonSession::FerriteNoduleItemId, 2),
		MakeInput(AstraeonSession::SilicateFiberItemId, 1)
	};
	DemolitionMaul.OutputItemId = AstraeonSession::DemolitionMaulItemId;
	Recipes.Add(DemolitionMaul);

	return Recipes;
}

bool UAstraeonGameInstance::CanCraftRecipe(const FAstraeonCraftingRecipe& Recipe, FString& OutReason) const
{
	if (!bHasStartedGame)
	{
		OutReason = TEXT("sin sesión activa");
		return false;
	}

	for (const FAstraeonRecipeInput& Input : Recipe.Inputs)
	{
		if (GetInventoryItemCount(Input.ItemId) < Input.Quantity)
		{
			OutReason = FString::Printf(TEXT("falta %s"), *Input.ItemId.ToString());
			return false;
		}
	}

	// El recorrido crítico no puede quedar bloqueado por fabricar accesorios: si gastar
	// esto dejaría el resonador fuera de alcance, la mesa lo impide y lo explica.
	if (Recipe.RecipeId != TEXT("signal_resonator")
		&& GetInventoryItemCount(AstraeonSession::SignalResonatorItemId) <= 0
		&& CurrentRegionLayout.Resources.Num() >= 3)
	{
		const FName SignatureResourceId = CurrentRegionLayout.Resources[2].ResourceId;
		for (const FAstraeonRecipeInput& Input : Recipe.Inputs)
		{
			const bool bResonatorNeedsIt = Input.ItemId == AstraeonSession::SilicateFiberItemId
				|| Input.ItemId == AstraeonSession::FerriteNoduleItemId
				|| Input.ItemId == SignatureResourceId;
			if (bResonatorNeedsIt && GetInventoryItemCount(Input.ItemId) - Input.Quantity < 1)
			{
				OutReason = FString::Printf(TEXT("reservado para el resonador (%s)"), *Input.ItemId.ToString());
				return false;
			}
		}
	}

	OutReason.Reset();
	return true;
}

bool UAstraeonGameInstance::CraftRecipe(FName RecipeId)
{
	const TArray<FAstraeonCraftingRecipe> Recipes = GetCraftingRecipes();
	const FAstraeonCraftingRecipe* Recipe = Recipes.FindByPredicate([RecipeId](const FAstraeonCraftingRecipe& Candidate)
	{
		return Candidate.RecipeId == RecipeId;
	});

	if (!Recipe)
	{
		return false;
	}

	FString Reason;
	if (!CanCraftRecipe(*Recipe, Reason))
	{
		SetLastFeedbackMessage(FString::Printf(TEXT("No se puede fabricar %s: %s."), *Recipe->DisplayName.ToString(), *Reason));
		return false;
	}

	if (Recipe->RecipeId == TEXT("signal_resonator"))
	{
		return CraftSignalResonator();
	}

	for (const FAstraeonRecipeInput& Input : Recipe->Inputs)
	{
		ConsumeInventoryItem(Input.ItemId, Input.Quantity);
	}
	AddInventoryItem(Recipe->OutputItemId, Recipe->OutputQuantity);

	SetLastFeedbackMessage(FString::Printf(TEXT("Fabricado: %s. Equípalo con 1/2/3."), *Recipe->DisplayName.ToString()));

	FAstraeonLogbookEntry CraftingEntry;
	CraftingEntry.EntryId = FName(*FString::Printf(TEXT("recipe.%s"), *Recipe->RecipeId.ToString()));
	CraftingEntry.Title = Recipe->DisplayName;
	CraftingEntry.Summary = Recipe->Description;
	CraftingEntry.Certainty = EAstraeonDiscoveryCertainty::Confirmed;
	UpsertRuntimeLogbookEntry(CraftingEntry);
	return true;
}

void UAstraeonGameInstance::SetItacaOriginCm(const FVector& OriginCm)
{
	ItacaOriginCm = OriginCm;
}

bool UAstraeonGameInstance::EquipProtection(EAstraeonProtectionModule Protection)
{
	if (!bHasStartedGame)
	{
		return false;
	}

	if (Protection == EAstraeonProtectionModule::None)
	{
		EquippedProtection = Protection;
		SetLastFeedbackMessage(TEXT("Módulo de protección retirado."));
		return true;
	}

	// Los módulos ya no son gratuitos: hay que fabricarlos en la mesa de Ítaca.
	const FName ModuleItemId = GetProtectionItemId(Protection);
	if (GetInventoryItemCount(ModuleItemId) <= 0)
	{
		SetLastFeedbackMessage(FString::Printf(TEXT("No tienes %s. Fabrícalo en la mesa de Ítaca."),
			*UAstraeonSuitComponent::DescribeProtection(Protection)));
		return false;
	}

	EquippedProtection = Protection;

	const FString ProtectionName = UAstraeonSuitComponent::DescribeProtection(Protection);
	const bool bMatchesHazard = UAstraeonSuitComponent::RecommendProtection(CurrentEnvironment) == Protection;
	SetLastFeedbackMessage(bMatchesHazard
		? FString::Printf(TEXT("%s activo. Compensa la amenaza dominante."), *ProtectionName)
		: FString::Printf(TEXT("%s activo, pero no cubre la amenaza dominante de esta región."), *ProtectionName));
	return true;
}

bool UAstraeonGameInstance::RecordAnomalyInspection()
{
	if (!bHasStartedGame)
	{
		return false;
	}

	FAstraeonLogbookEntry Entry;
	Entry.EntryId = TEXT("anomaly.minor_geologic");
	Entry.Title = FText::FromString(TEXT("Anomalía geológica menor (medida)"));
	Entry.Summary = FText::FromString(TEXT("Medida de cerca: la veta está magnetizada en la misma frecuencia que la señal desconocida. Algo la alineó. La región recuerda un evento que ARGOS no registró."));
	Entry.Certainty = EAstraeonDiscoveryCertainty::Measured;
	UpsertRuntimeLogbookEntry(Entry);
	SetLastFeedbackMessage(TEXT("Anomalía medida: comparte frecuencia con la señal. Registrado en bitácora (L)."));
	return true;
}

FString UAstraeonGameInstance::GetObjectiveHint() const
{
	switch (ObjectiveState)
	{
	case EAstraeonObjectiveState::MeasureEnvironment:
		if (!HasRuntimeLogbookEntry(AstraeonSession::ArgosBriefingEntryId))
		{
			return TEXT("Interactúa con la consola ARGOS (E) para recibir el briefing de la primera señal.");
		}
		if (!HasRuntimeLogbookEntry(AstraeonSession::SurfaceDeploymentEntryId))
		{
			return TEXT("Usa la ESCOTILLA (E) para despliegue controlado, luego escanea con Click Izq.");
		}
		return TEXT("Escanea el ambiente con Click Izq. para confirmar datos de supervivencia.");
	case EAstraeonObjectiveState::GatherResources:
		return TEXT("Busca cubos VERDES con etiqueta RECURSO dispersos en el área y recoge cada uno con E.");
	case EAstraeonObjectiveState::CraftSignalResonator:
		return TEXT("Presiona C para fabricar signal_resonator con las muestras recogidas.");
	case EAstraeonObjectiveState::ReachSignalSource:
		return TEXT("Sigue el marcador SEÑAL e interactúa con E para estabilizarlo.");
	case EAstraeonObjectiveState::Completed:
		return TEXT("Primera señal resuelta. Guarda con F5 o revisa las entradas de la bitácora.");
	default:
		return TEXT("Continúa explorando, midiendo, actuando y registrando.");
	}
}

void UAstraeonGameInstance::SetLastFeedbackMessage(const FString& Message)
{
	LastFeedbackMessage = Message;
}

void UAstraeonGameInstance::RecordArgosBriefing()
{
	if (!bHasStartedGame)
	{
		return;
	}

	FAstraeonLogbookEntry ArgosEntry;
	ArgosEntry.EntryId = AstraeonSession::ArgosBriefingEntryId;
	ArgosEntry.Title = FText::FromString(TEXT("Briefing de ARGOS"));
	ArgosEntry.Summary = FText::FromString(TEXT("ARGOS aisló una señal anómala de banda estrecha. Mide la región, recoge materiales resonantes, fabrica un signal_resonator y luego confirma la fuente."));
	ArgosEntry.Certainty = EAstraeonDiscoveryCertainty::Confirmed;
	UpsertRuntimeLogbookEntry(ArgosEntry);
	SetLastFeedbackMessage(TEXT("Briefing de ARGOS registrado. Usa la ESCOTILLA a continuación."));
}

bool UAstraeonGameInstance::RecordSurfaceDeployment()
{
	if (!bHasStartedGame)
	{
		return false;
	}

	FAstraeonLogbookEntry DeploymentEntry;
	DeploymentEntry.EntryId = AstraeonSession::SurfaceDeploymentEntryId;
	DeploymentEntry.Title = FText::FromString(TEXT("Despliegue controlado en superficie"));
	DeploymentEntry.Summary = FText::FromString(TEXT("La escotilla de superficie de Ítaca cicló correctamente. ARGOS ahora trata la región cercana como zona de expedición activa."));
	DeploymentEntry.Certainty = EAstraeonDiscoveryCertainty::Observed;
	UpsertRuntimeLogbookEntry(DeploymentEntry);
	SetLastFeedbackMessage(TEXT("Despliegue en superficie registrado. Escanea el ambiente con Click Izq."));
	return true;
}

bool UAstraeonGameInstance::TryResolveSignalSource()
{
	if (!bHasStartedGame || GetInventoryItemCount(AstraeonSession::SignalResonatorItemId) <= 0)
	{
		return false;
	}

	ObjectiveState = EAstraeonObjectiveState::Completed;

	FAstraeonLogbookEntry SignalEntry;
	SignalEntry.EntryId = TEXT("signal.first_source");
	SignalEntry.Title = FText::FromString(TEXT("La primera señal"));
	SignalEntry.Summary = FText::FromString(TEXT("El resonador estabilizó la señal desconocida. El origen está confirmado, pero su creador sigue siendo un misterio."));
	SignalEntry.Certainty = EAstraeonDiscoveryCertainty::Confirmed;
	UpsertRuntimeLogbookEntry(SignalEntry);
	SetLastFeedbackMessage(TEXT("Fuente de señal resuelta. Vertical slice completo."));
	return true;
}

bool UAstraeonGameInstance::SaveCurrentGame(const FString& SlotName, int32 UserIndex) const
{
	if (!bHasStartedGame)
	{
		return false;
	}

	UAstraeonSaveGame* SaveSnapshot = CreateSaveSnapshot();
	return SaveSnapshot && UGameplayStatics::SaveGameToSlot(SaveSnapshot, SlotName, UserIndex);
}

bool UAstraeonGameInstance::LoadSavedGame(const FString& SlotName, int32 UserIndex)
{
	USaveGame* LoadedObject = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
	const UAstraeonSaveGame* LoadedSave = Cast<UAstraeonSaveGame>(LoadedObject);
	if (!LoadedSave)
	{
		return false;
	}

	CurrentWorldSeed = LoadedSave->SaveGameVersion >= 2 ? LoadedSave->ContentSeed : LoadedSave->WorldSeed;
	CurrentEnvironment = LoadedSave->Environment;
	CurrentRegionLayout = LoadedSave->RegionLayout;
	if (LoadedSave->SaveGameVersion >= 2)
	{
		CurrentRegionLayout.PlanetProfileId = LoadedSave->PlanetProfileId;
		CurrentRegionLayout.RegionProfileId = LoadedSave->RegionProfileId;
		CurrentRegionLayout.ContentSeed = LoadedSave->ContentSeed;
	}
	else
	{
		// Una partida anterior conserva su mundo tal como se guardó. Se etiqueta como legado
		// y queda lista para regrabarse con versión 2, sin teletransportar ni regenerar POIs.
		CurrentRegionLayout.PlanetProfileId = TEXT("legacy_generated_planet");
		CurrentRegionLayout.RegionProfileId = TEXT("legacy_generated_region");
		CurrentRegionLayout.ContentSeed = CurrentWorldSeed;
	}
	RevealedMap = LoadedSave->RevealedMap;
	Inventory = LoadedSave->Inventory;
	ObjectiveState = AstraeonSession::FromSavedObjectiveState(LoadedSave->ObjectiveState);
	RuntimeLogbookEntries = LoadedSave->LogbookEntries;
	EquippedProtection = LoadedSave->EquippedProtection;
	ItacaOriginCm = LoadedSave->ItacaOriginCm;
	PlacedStructures = LoadedSave->PlacedStructures;
	HandItemId = LoadedSave->HandItemId;
	HungerPercent = LoadedSave->HungerPercent;
	CreatureRespawnTimers = LoadedSave->CreatureRespawnTimers;
	bHasStartedGame = true;
	return true;
}

UAstraeonSaveGame* UAstraeonGameInstance::CreateSaveSnapshot() const
{
	UAstraeonSaveGame* SaveSnapshot = Cast<UAstraeonSaveGame>(UGameplayStatics::CreateSaveGameObject(UAstraeonSaveGame::StaticClass()));
	if (!SaveSnapshot)
	{
		return nullptr;
	}

	SaveSnapshot->WorldSeed = CurrentWorldSeed;
	SaveSnapshot->ContentSeed = CurrentWorldSeed;
	SaveSnapshot->PlanetProfileId = CurrentRegionLayout.PlanetProfileId;
	SaveSnapshot->RegionProfileId = CurrentRegionLayout.RegionProfileId;
	SaveSnapshot->SaveGameVersion = 2;
	SaveSnapshot->GeneratorVersion = CurrentEnvironment.GeneratorVersion;
	SaveSnapshot->Environment = CurrentEnvironment;
	SaveSnapshot->RegionLayout = CurrentRegionLayout;
	SaveSnapshot->RevealedMap = RevealedMap;
	SaveSnapshot->Inventory = Inventory;
	SaveSnapshot->ObjectiveState = AstraeonSession::ToSavedObjectiveState(ObjectiveState);
	SaveSnapshot->LogbookEntries = RuntimeLogbookEntries;
	SaveSnapshot->EquippedProtection = EquippedProtection;
	SaveSnapshot->ItacaOriginCm = ItacaOriginCm;
	SaveSnapshot->PlacedStructures = PlacedStructures;
	SaveSnapshot->HandItemId = HandItemId;
	SaveSnapshot->HungerPercent = HungerPercent;
	SaveSnapshot->CreatureRespawnTimers = CreatureRespawnTimers;

	if (const UWorld* World = GetWorld())
	{
		if (const APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			if (const APawn* Pawn = PlayerController->GetPawn())
			{
				SaveSnapshot->PlayerTransform = Pawn->GetActorTransform();
			}
		}
	}

	return SaveSnapshot;
}

int32 UAstraeonGameInstance::GetCurrentTerrainSeed() const
{
	// La seed de contenido deriva el relieve regional. El generador aplica zonas de garantía
	// antes de materializarlo, por lo que variar la seed nunca puede invalidar Ítaca o rutas.
	return CurrentWorldSeed;
}

void UAstraeonGameInstance::UpsertRuntimeLogbookEntry(const FAstraeonLogbookEntry& Entry)
{
	if (Entry.EntryId.IsNone())
	{
		return;
	}

	for (FAstraeonLogbookEntry& Existing : RuntimeLogbookEntries)
	{
		if (Existing.EntryId == Entry.EntryId)
		{
			Existing = Entry;
			return;
		}
	}

	RuntimeLogbookEntries.Add(Entry);
}

bool UAstraeonGameInstance::HasRuntimeLogbookEntry(FName EntryId) const
{
	return RuntimeLogbookEntries.ContainsByPredicate([EntryId](const FAstraeonLogbookEntry& Entry)
	{
		return Entry.EntryId == EntryId;
	});
}

int32 UAstraeonGameInstance::NormalizeRequestedSeed(int32 RequestedWorldSeed)
{
	return RequestedWorldSeed == 0 ? AstraeonSession::DefaultWorldSeed : RequestedWorldSeed;
}
