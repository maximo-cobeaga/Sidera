#pragma once

#include "CoreMinimal.h"
#include "Building/AstraeonBuildingTypes.h"
#include "Crafting/AstraeonCraftingTypes.h"
#include "Creatures/AstraeonCreatureTypes.h"
#include "Engine/GameInstance.h"
#include "Exploration/AstraeonMapTypes.h"
#include "Knowledge/AstraeonLogbookTypes.h"
#include "Persistence/AstraeonSaveGame.h"
#include "Survival/AstraeonProtectionTypes.h"
#include "WorldGen/AstraeonEnvironmentTypes.h"
#include "WorldGen/AstraeonRegionTypes.h"
#include "WorldGen/AstraeonTerrainTraversal.h"
#include "AstraeonGameInstance.generated.h"

class AAstraeonCreatureActor;
class UAstraeonRuntimeStateManager;

UENUM(BlueprintType)
enum class EAstraeonObjectiveState : uint8
{
	MeasureEnvironment UMETA(DisplayName = "Measure Environment"),
	GatherResources UMETA(DisplayName = "Gather Resources"),
	CraftSignalResonator UMETA(DisplayName = "Craft Signal Resonator"),
	ReachSignalSource UMETA(DisplayName = "Reach Signal Source"),
	Completed UMETA(DisplayName = "Completed")
};

UCLASS()
class ASTRAEON_API UAstraeonGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Session")
	void StartNewGame(int32 RequestedWorldSeed = 0);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Session")
	int32 GetCurrentWorldSeed() const { return CurrentWorldSeed; }

	// Nombre explícito para nuevas llamadas. GetCurrentWorldSeed se conserva temporalmente
	// para compatibilidad: ya es seed de variación, no identidad/geografía del mundo.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Session")
	int32 GetCurrentContentSeed() const { return CurrentWorldSeed; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Session")
	FName GetCurrentPlanetProfileId() const { return CurrentRegionLayout.PlanetProfileId; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Session")
	FName GetCurrentRegionProfileId() const { return CurrentRegionLayout.RegionProfileId; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	int32 GetCurrentTerrainSeed() const;

	// Superficie efectiva de la región: la misma que se materializa y sobre la que se
	// validó el tránsito. Criaturas, obras y diagnóstico consultan aquí y no la capa de
	// suelo desnuda, que ignora claros, exclusiones de montaña y la plataforma de Ítaca.
	const FAstraeonTerrainSurfaceContext& GetSurfaceContext() const;

	UFUNCTION(BlueprintPure, Category = "Astraeon|WorldGen")
	float GetSurfaceHeightCm(const FVector2D& PointCm) const;

	// Estado de la última resolución de seed de relieve. Lo consulta el smoke para exigir
	// que una partida normal no dependa de la variante segura.
	const FAstraeonTerrainSeedResolution& GetTerrainSeedResolution() const;

	UFUNCTION(BlueprintPure, Category = "Astraeon|Session")
	const FAstraeonEnvironmentalSnapshot& GetCurrentEnvironment() const { return CurrentEnvironment; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Session")
	const FAstraeonRegionLayout& GetCurrentRegionLayout() const { return CurrentRegionLayout; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Session")
	const TArray<FAstraeonLogbookEntry>& GetRuntimeLogbookEntries() const { return RuntimeLogbookEntries; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Map")
	const FAstraeonRevealedMap& GetRevealedMap() const { return RevealedMap; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Inventory")
	const TMap<FName, int32>& GetInventory() const { return Inventory; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Inventory")
	int32 GetInventoryItemCount(FName ItemId) const;

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Inventory")
	bool AddInventoryItem(FName ItemId, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Inventory")
	bool ConsumeInventoryItem(FName ItemId, int32 Quantity = 1);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Crafting")
	bool CanCraftSignalResonator() const;

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Crafting")
	bool CraftSignalResonator();

	// Recetas de la mesa de fabricación. Se construyen en runtime porque el recurso
	// característico depende de la seed de la región.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Crafting")
	TArray<FAstraeonCraftingRecipe> GetCraftingRecipes() const;

	UFUNCTION(BlueprintPure, Category = "Astraeon|Crafting")
	bool CanCraftRecipe(const FAstraeonCraftingRecipe& Recipe, FString& OutReason) const;

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Crafting")
	bool CraftRecipe(FName RecipeId);

	// Id de inventario del módulo de protección, para exigir que se haya fabricado antes
	// de poder equiparlo.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Crafting")
	static FName GetProtectionItemId(EAstraeonProtectionModule Protection);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Combat")
	static FName GetPulseCutterItemId();

	UFUNCTION(BlueprintPure, Category = "Astraeon|Combat")
	bool HasPulseCutter() const;

	// Registra una criatura abatida: entrega lo que deja y anota el hallazgo.
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Combat")
	bool RecordCreatureKill(const FAstraeonCreatureProfile& CreatureProfile);

	// --- Fauna ---
	// Cazar es la fuente de comida, así que la fauna tiene que ser renovable: la muerte
	// sobrevive al aterrizaje (antes bastaba despegar para tenerlas todas vivas de nuevo)
	// pero el nido se repuebla solo pasado un rato.
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Creatures")
	void RecordCreatureDeath(FName SpawnPointId);

	// The one entry point for a creature that died by the player's hand: a planetary creature
	// becomes a delta at its place on the body, a flat-region one starts its nest clock.
	// Until P2.6 nothing called `RecordCreatureDeath`, so every nest was always repopulated.
	bool RecordCreatureDefeat(const AAstraeonCreatureActor* Creature);

	// Persistent changes to every planet. Created on first use; reset by a new session.
	UAstraeonRuntimeStateManager* GetPlanetState();
	const UAstraeonRuntimeStateManager* GetPlanetState() const;

	// Where the last loaded save left the player, as body + direction + altitude + heading.
	bool GetLoadedPlanetLocation(FName& OutBodyId, FVector& OutDirection, double& OutAltitudeCm, FVector& OutForward) const;

	UFUNCTION(BlueprintPure, Category = "Astraeon|Creatures")
	bool IsCreatureSpawnPopulated(FName SpawnPointId) const;

	// Descuenta el reloj de repoblado y devuelve los nidos que acaban de quedar libres.
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Creatures")
	TArray<FName> AdvanceCreatureRespawns(float DeltaSeconds);

	// Rescate de emergencia al caer incapacitado. Cuesta la carga opcional (muestras y
	// vetas profundas) pero nunca los insumos del recorrido crítico ni el equipo fabricado:
	// morir debe doler sin poder dejar la partida sin salida.
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Survival")
	bool RecordEmergencyRecall();

	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	bool IsCriticalPathResource(FName ItemId) const;

	// --- Construcción ---
	// Lo construido se guarda como datos y se reconstruye al materializar la región, porque
	// la región se rehace cada vez que Ítaca aterriza.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Building")
	const TArray<FAstraeonPlacedStructure>& GetPlacedStructures() const { return PlacedStructures; }

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Building")
	bool PlaceStructure(const FAstraeonPlacedStructure& Placement);

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Building")
	bool DemolishStructureAt(const FVector& LocationCm, float ToleranceCm = 50.0f);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Building")
	bool CanPlaceStructure(EAstraeonStructureType Type) const;

	UFUNCTION(BlueprintPure, Category = "Astraeon|Building")
	static FName GetRegolithItemId();

	UFUNCTION(BlueprintPure, Category = "Astraeon|Building")
	static FName GetBrickItemId();

	UFUNCTION(BlueprintPure, Category = "Astraeon|Building")
	static FName GetBuildHammerItemId();

	UFUNCTION(BlueprintPure, Category = "Astraeon|Building")
	static FName GetDemolitionMaulItemId();

	UFUNCTION(BlueprintPure, Category = "Astraeon|Building")
	bool HasBuildHammer() const;

	UFUNCTION(BlueprintPure, Category = "Astraeon|Building")
	bool HasDemolitionMaul() const;

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Building")
	bool ExtractRegolith();

	// --- Objeto en mano ---
	// Tener la herramienta en el inventario ya no basta: hay que llevarla en la mano. Eso
	// obliga a elegir, que es lo que convierte el inventario en decisión y no en lista.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Hand")
	FName GetHandItemId() const { return HandItemId; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Hand")
	bool IsHolding(FName ItemId) const { return !ItemId.IsNone() && HandItemId == ItemId && GetInventoryItemCount(ItemId) > 0; }

	// Los objetos que se pueden llevar en la mano, en orden estable, para la barra rápida.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Hand")
	TArray<FName> GetHotbarItems() const;

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Hand")
	bool SelectHotbarSlot(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Hand")
	static bool IsHandheldItem(FName ItemId);

	// Usa lo que se tenga en la mano. Hoy sólo las raciones tienen un uso directo; el resto
	// de las herramientas se usan con su propio gesto (disparar, construir, extraer).
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Hand")
	bool UseHandItem();

	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	static FName GetRationItemId();

	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	float GetHungerPercent() const { return HungerPercent; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	bool IsStarving() const { return HungerPercent <= 0.0f; }

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Survival")
	void ConsumeHunger(float Amount) { HungerPercent = FMath::Clamp(HungerPercent - Amount, 0.0f, 100.0f); }

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Survival")
	void Nourish(float Amount) { HungerPercent = FMath::Clamp(HungerPercent + Amount, 0.0f, 100.0f); }

	// Cuánto rinde una veta al recolectarla, según la declara el generador de región.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Inventory")
	int32 GetResourceNodeQuantity(FName ResourceId) const;

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Narrative")
	void RecordArgosBriefing();

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Narrative")
	bool RecordSurfaceDeployment();

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Map")
	int32 RevealMapAroundLocationMeters(FVector2D LocationMeters, int32 RadiusCells = 1);

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Scanning")
	bool ScanCurrentEnvironment();

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Scanning")
	bool RecordCreatureScan(const FAstraeonCreatureProfile& CreatureProfile);

	// El MVP (§3.5) exige que el escáner identifique ambiente, recursos, mob y señal/estructura.
	// Ambiente y mob ya tenían camino propio; esto cubre recursos, fuente de señal y anomalía.
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Scanning")
	bool RecordMarkerScan(FName MarkerId, bool bIsResource);

	// Inspección cercana de la anomalía (E). Sube la certeza de Observada a Medida.
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Scanning")
	bool RecordAnomalyInspection();

	// Ítaca es la nave: al aterrizar en otro punto, la estancia y sus marcadores se
	// remateralizan alrededor de este origen en vez de quedar clavados en (0,0).
	UFUNCTION(BlueprintPure, Category = "Astraeon|Ship")
	FVector GetItacaOriginCm() const { return ItacaOriginCm; }

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Ship")
	void SetItacaOriginCm(const FVector& OriginCm);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Survival")
	EAstraeonProtectionModule GetEquippedProtection() const { return EquippedProtection; }

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Survival")
	bool EquipProtection(EAstraeonProtectionModule Protection);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Session")
	bool HasStartedGame() const { return bHasStartedGame; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Objectives")
	EAstraeonObjectiveState GetObjectiveState() const { return ObjectiveState; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Objectives")
	FString GetObjectiveHint() const;

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Feedback")
	void SetLastFeedbackMessage(const FString& Message);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Feedback")
	FString GetLastFeedbackMessage() const { return LastFeedbackMessage; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Objectives")
	bool IsSignalResolved() const { return ObjectiveState == EAstraeonObjectiveState::Completed; }

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Objectives")
	bool TryResolveSignalSource();

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Persistence")
	bool SaveCurrentGame(const FString& SlotName = TEXT("AstraeonAutosave"), int32 UserIndex = 0) const;

	UFUNCTION(BlueprintCallable, Category = "Astraeon|Persistence")
	bool LoadSavedGame(const FString& SlotName = TEXT("AstraeonAutosave"), int32 UserIndex = 0);

	UFUNCTION(BlueprintPure, Category = "Astraeon|Persistence")
	UAstraeonSaveGame* CreateSaveSnapshot() const;

	static int32 NormalizeRequestedSeed(int32 RequestedWorldSeed);

private:
	// Resuelve la seed de relieve y cachea el contexto. Es perezoso y mutable porque los
	// consumidores consultan la superficie desde funciones const, y rehacerlo en cada
	// consulta costaría reconstruir las listas de claros y exclusiones por criatura y frame.
	void EnsureTerrainSurface() const;
	void InvalidateTerrainSurface();

	mutable bool bTerrainSurfaceReady = false;
	mutable FAstraeonTerrainSurfaceContext CachedSurfaceContext;
	mutable FAstraeonTerrainSeedResolution CachedSeedResolution;

	void UpsertRuntimeLogbookEntry(const FAstraeonLogbookEntry& Entry);
	bool HasRuntimeLogbookEntry(FName EntryId) const;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Session")
	bool bHasStartedGame = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Session")
	int32 CurrentWorldSeed = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Session")
	FAstraeonEnvironmentalSnapshot CurrentEnvironment;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Session")
	FAstraeonRegionLayout CurrentRegionLayout;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Map")
	FAstraeonRevealedMap RevealedMap;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Inventory")
	TMap<FName, int32> Inventory;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Objectives")
	EAstraeonObjectiveState ObjectiveState = EAstraeonObjectiveState::MeasureEnvironment;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Ship")
	FVector ItacaOriginCm = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Building")
	TArray<FAstraeonPlacedStructure> PlacedStructures;

	// Nido -> segundos que faltan para que vuelva a haber una criatura ahí.
	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Creatures")
	TMap<FName, float> CreatureRespawnTimers;

	UPROPERTY()
	TObjectPtr<UAstraeonRuntimeStateManager> PlanetState;

	bool bHasLoadedPlanetLocation = false;
	FName LoadedPlanetBodyId;
	FVector LoadedPlayerDirection = FVector(0, 0, 1);
	double LoadedPlayerAltitudeCm = 0.0;
	FVector LoadedPlayerForward = FVector(1, 0, 0);

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Hand")
	FName HandItemId;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Survival", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float HungerPercent = 100.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Survival")
	EAstraeonProtectionModule EquippedProtection = EAstraeonProtectionModule::None;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Feedback")
	FString LastFeedbackMessage;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Session")
	TArray<FAstraeonLogbookEntry> RuntimeLogbookEntries;
};
