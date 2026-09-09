#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Building/AstraeonBuildingTypes.h"
#include "Presentation/AstraeonFirstPersonRigComponent.h"
#include "Survival/AstraeonProtectionTypes.h"
#include "AstraeonPlayerCharacter.generated.h"

class AAstraeonRegionMarker;
class UAstraeonGameInstance;
class UAstraeonSuitComponent;
class UAstraeonFirstPersonRigComponent;
class UAstraeonPlanetGravityComponent;
class UCameraComponent;
class USpringArmComponent;
struct FHitResult;

UCLASS()
class ASTRAEON_API AAstraeonPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAstraeonPlayerCharacter();

	void Interact();

	// Alterna primera/tercera persona. En tercera se muestra el cuerpo al propio jugador y
	// se ocultan las manos, que están pensadas para verse pegadas a la cámara. Pública para
	// que la prueba de integración pueda alternar sin simular la tecla.
	UFUNCTION(BlueprintCallable, Category = "Astraeon|Camera")
	void ToggleCameraView();

	UFUNCTION(BlueprintPure, Category = "Astraeon|Camera")
	bool IsThirdPersonView() const { return bThirdPersonView; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Building")
	bool IsBuildModeActive() const { return bBuildModeActive; }

	UFUNCTION(BlueprintPure, Category = "Astraeon|Building")
	EAstraeonStructureType GetSelectedStructureType() const { return SelectedStructureType; }

	// True cuando la vista previa apunta a una superficie válida y hay material suficiente.
	UFUNCTION(BlueprintPure, Category = "Astraeon|Building")
	bool IsPlacementValid() const { return bPlacementValid; }

	// Mientras Ítaca vuela, el personaje queda oculto y sin colisión. Si además siguiera
	// simulando, caería sin suelo y la red anti-caída lo devolvería al punto previo al
	// despegue, apareciendo lejos de la nave recién aterrizada.
	void PrepareForShipFlight();
	void RecoverFromShipFlight(const FVector& LandingLocationCm);

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	// Vista en tercera persona: sirve para ver al protagonista —el traje, la escala, la
	// silueta— que en primera persona sólo se insinúa por la sombra. El brazo hace prueba
	// de colisión para no meter la cámara dentro de la geometría.
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Camera")
	TObjectPtr<USpringArmComponent> ThirdPersonBoom;

	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Camera")
	TObjectPtr<UCameraComponent> ThirdPersonCamera;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Camera")
	bool bThirdPersonView = false;

	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Survival")
	TObjectPtr<UAstraeonSuitComponent> SuitComponent;

	// Manos en primera persona, herramienta en mano y selección de clip. Vive aparte para
	// no mezclar presentación con la lógica de juego del Character.
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Presentation")
	TObjectPtr<UAstraeonFirstPersonRigComponent> FirstPersonRig;

	// Gravedad radial. Inactivo salvo que el mapa tenga un harness planetario, asi que puede
	// viajar montado sin alterar el recorrido plano actual (ADR 0004, spike de la Fase 0).
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Planet")
	TObjectPtr<UAstraeonPlanetGravityComponent> PlanetGravity;

	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Presentation")
	TArray<TObjectPtr<USkeletalMeshComponent>> BodyEquipment;

	// Fantasma de colocación: la pieza que se va a construir, translúcida y sin colisión,
	// siguiendo la superficie apuntada. Construir a ciegas y corregir después sería mucho
	// más costoso que previsualizar.
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Building")
	TObjectPtr<UStaticMeshComponent> BuildPreviewMesh;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Building")
	bool bBuildModeActive = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Building")
	bool bPlacementValid = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Building")
	EAstraeonStructureType SelectedStructureType = EAstraeonStructureType::Wall;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Building")
	float BuildYawDegrees = 0.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Astraeon|Building")
	FVector PreviewLocationCm = FVector::ZeroVector;

	// Last actor location where the character was confirmed to be standing on solid,
	// walkable ground (see RescueFromVoidIfNeeded). Used as a safety net so a fall
	// through missing/late collision - for example right after a SURFACE HATCH
	// teleport - can always be recovered from instead of falling forever.
	FVector LastSafeGroundLocationCm = FVector::ZeroVector;
	bool bHasSafeGroundLocation = false;

	// El Tick del personaje ahora corre por frame para animar el rig de primera persona; la
	// lógica de juego, que no necesita esa frecuencia, se sigue evaluando cada 0,2 s.
	float GameplayTickAccumulatorSeconds = 0.0f;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void StartJump();
	void StopJump();
	void StartSprint();
	void StopSprint();
	void EquipProtectionModule(EAstraeonProtectionModule Protection);
	void EquipNoProtection();
	void EquipRespirator();
	void EquipThermalShield();
	void EquipPressureSeal();
	void CraftPulseCutter();
	void CraftCoreDrill();
	void CraftAtFabricator(FName RecipeId);

	// Las teclas numéricas son contextuales: con la mesa abierta fabrican, y con la mesa
	// cerrada eligen qué se lleva en la mano.
	void HandleNumberKey(int32 Index, FName FabricatorRecipeId);
	void SelectSlotOne();
	void SelectSlotTwo();
	void SelectSlotThree();
	void SelectSlotFour();
	void SelectSlotFive();
	void SelectSlotSix();
	void UseHandItem();
	void PlayHandGesture(EAstraeonHandGesture Gesture);
	void CycleProtectionModule();
	void ScanEnvironment();
	void FireWeapon();
	void CraftSignalResonator();
	AAstraeonRegionMarker* FindFocusedRegionMarker() const;
	AAstraeonRegionMarker* FindBestRegionMarkerInReach(float RadiusCm) const;
	bool InteractWithRegionMarker(AAstraeonRegionMarker& Marker, UAstraeonGameInstance& AstraeonGameInstance, bool& bOutSignalSourceAttempted);
	bool DeployToSurface(UAstraeonGameInstance& AstraeonGameInstance);
	bool EnterItacaThroughHatch(UAstraeonGameInstance& AstraeonGameInstance);
	bool TraceForDeploymentFloor(const FVector& DeploymentLocationCm, FHitResult& OutHit) const;
	void MarkLocationAsSafeGround(const FVector& LocationCm);
	void LogDiagnosticState() const;
	void RescueFromVoidIfNeeded();

	// La mesa es una estación física dentro de Ítaca: alejarse de ella la cierra, en vez
	// de dejar el panel abierto mientras el jugador camina por la superficie.
	void CloseFabricatorIfOutOfReach();

	void ApplyCameraView();
	void LogCameraState() const;

	void ToggleBuildMode();
	void CycleStructureType();
	void RotateStructure();
	void ConfirmPlacement();
	void DemolishAimedStructure();
	void UpdateBuildPreview();
	bool TraceBuildSurface(FVector& OutLocationCm) const;

	// Ítaca presuriza: estar dentro recarga el traje. Y si la salud llega a cero fuera,
	// ARGOS ejecuta un rescate de vuelta a la nave.
	void UpdateHavenAndRecall();
};
