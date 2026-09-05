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

	FAstraeonLogbookEntry InitialEnvironmentEntry;
	InitialEnvironmentEntry.EntryId = AstraeonSession::InitialEnvironmentEntryId;
	InitialEnvironmentEntry.Title = FText::FromString(TEXT("Initial environmental measurement"));
	InitialEnvironmentEntry.Summary = FText::Format(
		FText::FromString(TEXT("ARGOS measured gravity {0} m/s^2, temperature {1} K and pressure {2} kPa.")),
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

	FAstraeonLogbookEntry CraftingEntry;
	CraftingEntry.EntryId = TEXT("recipe.signal_resonator");
	CraftingEntry.Title = FText::FromString(TEXT("Signal resonator"));
	CraftingEntry.Summary = FText::FromString(TEXT("A field-made resonator tuned with local material properties; it should stabilize the unknown signal path."));
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
	ConfirmedEnvironmentEntry.Title = FText::FromString(TEXT("Confirmed environmental scan"));
	ConfirmedEnvironmentEntry.Summary = FText::Format(
		FText::FromString(TEXT("Scanner confirmation: gravity {0} m/s^2, temperature {1} K, pressure {2} kPa, breathable {3}.")),
		FText::AsNumber(CurrentEnvironment.GravityMS2),
		FText::AsNumber(CurrentEnvironment.TemperatureKelvin),
		FText::AsNumber(CurrentEnvironment.PressureKPa),
		CurrentEnvironment.bBreathable ? FText::FromString(TEXT("yes")) : FText::FromString(TEXT("no")));
	ConfirmedEnvironmentEntry.Certainty = EAstraeonDiscoveryCertainty::Confirmed;
	UpsertRuntimeLogbookEntry(ConfirmedEnvironmentEntry);
	if (ObjectiveState == EAstraeonObjectiveState::MeasureEnvironment)
	{
		ObjectiveState = EAstraeonObjectiveState::GatherResources;
	}
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
	return true;
}

FString UAstraeonGameInstance::GetObjectiveHint() const
{
	switch (ObjectiveState)
	{
	case EAstraeonObjectiveState::MeasureEnvironment:
		return TEXT("Use the ARGOS console, then scan the environment with Left Mouse.");
	case EAstraeonObjectiveState::GatherResources:
		return TEXT("Find three labeled resources and collect each one with E.");
	case EAstraeonObjectiveState::CraftSignalResonator:
		return TEXT("Press C to craft signal_resonator from the collected samples.");
	case EAstraeonObjectiveState::ReachSignalSource:
		return TEXT("Follow the SIGNAL marker and interact with E to stabilize it.");
	case EAstraeonObjectiveState::Completed:
		return TEXT("First signal resolved. Save with F5 or review the logbook entries.");
	default:
		return TEXT("Continue exploring, measuring, acting, and recording.");
	}
}

void UAstraeonGameInstance::RecordArgosBriefing()
{
	if (!bHasStartedGame)
	{
		return;
	}

	FAstraeonLogbookEntry ArgosEntry;
	ArgosEntry.EntryId = TEXT("argos.first_signal_briefing");
	ArgosEntry.Title = FText::FromString(TEXT("ARGOS briefing"));
	ArgosEntry.Summary = FText::FromString(TEXT("ARGOS isolated an anomalous narrow-band signal. Measure the region, collect resonant materials, craft a signal_resonator, then confirm the source."));
	ArgosEntry.Certainty = EAstraeonDiscoveryCertainty::Confirmed;
	UpsertRuntimeLogbookEntry(ArgosEntry);
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
	SignalEntry.Title = FText::FromString(TEXT("The first signal"));
	SignalEntry.Summary = FText::FromString(TEXT("The resonator stabilized the unknown signal. The origin is confirmed, but its maker remains unresolved."));
	SignalEntry.Certainty = EAstraeonDiscoveryCertainty::Confirmed;
	UpsertRuntimeLogbookEntry(SignalEntry);
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

int32 UAstraeonGameInstance::NormalizeRequestedSeed(int32 RequestedWorldSeed)
{
	return RequestedWorldSeed == 0 ? AstraeonSession::DefaultWorldSeed : RequestedWorldSeed;
}
