#include "AstraeonGameInstance.h"

#include "Exploration/AstraeonMapRevealLibrary.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Persistence/AstraeonSaveGame.h"
#include "WorldGen/AstraeonWorldGenerator.h"

namespace AstraeonSession
{
	constexpr int32 DefaultWorldSeed = 1001;
	const FName InitialEnvironmentEntryId(TEXT("environment.initial_measurement"));
	const FName ArgosBriefingEntryId(TEXT("argos.first_signal_briefing"));
	const FName SurfaceDeploymentEntryId(TEXT("itaca.surface_deployment"));
	const FName SignalResonatorItemId(TEXT("signal_resonator"));
	const FName SilicateFiberItemId(TEXT("silicate_fiber"));
	const FName FerriteNoduleItemId(TEXT("ferrite_nodule"));

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
	CurrentEnvironment = UAstraeonWorldGenerator::GenerateEnvironment(CurrentWorldSeed);
	CurrentRegionLayout = UAstraeonWorldGenerator::GenerateRegionLayout(CurrentWorldSeed);
	RevealedMap = FAstraeonRevealedMap();
	UAstraeonMapRevealLibrary::RevealRadius(RevealedMap, FVector2D::ZeroVector, 1);
	Inventory.Reset();
	ObjectiveState = EAstraeonObjectiveState::MeasureEnvironment;
	RuntimeLogbookEntries.Reset();
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

	CurrentWorldSeed = LoadedSave->WorldSeed;
	CurrentEnvironment = LoadedSave->Environment;
	CurrentRegionLayout = LoadedSave->RegionLayout;
	RevealedMap = LoadedSave->RevealedMap;
	Inventory = LoadedSave->Inventory;
	ObjectiveState = AstraeonSession::FromSavedObjectiveState(LoadedSave->ObjectiveState);
	RuntimeLogbookEntries = LoadedSave->LogbookEntries;
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
	SaveSnapshot->GeneratorVersion = CurrentEnvironment.GeneratorVersion;
	SaveSnapshot->Environment = CurrentEnvironment;
	SaveSnapshot->RegionLayout = CurrentRegionLayout;
	SaveSnapshot->RevealedMap = RevealedMap;
	SaveSnapshot->Inventory = Inventory;
	SaveSnapshot->ObjectiveState = AstraeonSession::ToSavedObjectiveState(ObjectiveState);
	SaveSnapshot->LogbookEntries = RuntimeLogbookEntries;

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
