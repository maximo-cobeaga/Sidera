#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "AstraeonFirstPersonRigComponent.generated.h"

class UAnimSequence;
class UStaticMesh;
class UStaticMeshComponent;

// Gesto puntual: interrumpe la locomoción durante la duración del clip y vuelve solo.
UENUM()
enum class EAstraeonHandGesture : uint8
{
	None,
	Scan,
	Pulse,
	Drill,
	Hammer,
	Maul,
	Consume,
	Present,
	Interact,
	Grip
};

/**
 * Manos en primera persona más la herramienta que el jugador lleva en la mano.
 *
 * El componente ES la malla de manos (`SK_Human_HandsFP_Blockout`) y cuelga de la cámara,
 * así que sigue el cabeceo de la vista. La herramienta cuelga del hueso `socket_tool_r`,
 * de modo que la sujeción viene de la animación y no de una posición fija en pantalla.
 *
 * La selección de clip es un pequeño estado en C++ sobre `PlayAnimation`, no un
 * AnimBlueprint: es Q1 y evita depender de un asset de Blueprint que ningún generador
 * reproduce. Sin BlendSpace ni transiciones: los clips cortan.
 */
UCLASS(ClassGroup = (Astraeon), meta = (BlueprintSpawnableComponent))
class ASTRAEON_API UAstraeonFirstPersonRigComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	UAstraeonFirstPersonRigComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Dispara un gesto. Si ya hay uno corriendo se reemplaza: el jugador que repite la
	// acción espera ver el gesto otra vez, no que se ignore.
	void PlayGesture(EAstraeonHandGesture Gesture);

	// La malla de cuerpo completo acompaña la locomoción de las manos, pero con SUS clips:
	// vive sobre otro esqueleto. Asignarla arranca su animación, porque puede llegar después
	// de que el rig ya eligió el suyo.
	void SetShadowBodyMesh(USkeletalMeshComponent* BodyMesh);
	void SetFirstPersonVisible(bool bNewVisible);

	UAnimSequence* GetGestureSequence(EAstraeonHandGesture Gesture) const;

private:
	// La herramienta llevada en la mano. Con las manos vacías se muestra el escáner, que
	// según el diseño está disponible desde el primer minuto y no es un ítem de inventario.
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Rig")
	TObjectPtr<UStaticMeshComponent> ToolMesh;

	// Broca del taladro: gira solo mientras el taladro está en la mano.
	UPROPERTY(VisibleAnywhere, Category = "Astraeon|Rig")
	TObjectPtr<UStaticMeshComponent> ToolRotorMesh;

	// Ojo del rig a 1,60 m; elevar 12 cm las manos mantiene el guante dentro del encuadre.
	// El frente importado (+Y) gira hacia +X.
	UPROPERTY(EditAnywhere, Category = "Astraeon|Rig")
	FVector HandsOffsetCm = FVector(0.0f, 0.0f, -148.0f);

	UPROPERTY(EditAnywhere, Category = "Astraeon|Rig")
	FRotator HandsRotation = FRotator(0.0f, -90.0f, 0.0f);

	// Transform de agarre medido en Blender y convertido al marco de Unreal; ver el .cpp.
	UPROPERTY(EditAnywhere, Category = "Astraeon|Rig")
	FTransform ToolGripTransform;

	UPROPERTY(EditAnywhere, Category = "Astraeon|Rig")
	float RotorDegreesPerSecond = 720.0f;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> ShadowBody;

	UPROPERTY()
	TMap<FName, TObjectPtr<UStaticMesh>> ToolMeshesByItemId;

	UPROPERTY()
	TObjectPtr<UStaticMesh> ScannerMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> RotorMesh;

	UPROPERTY()
	TMap<uint8, TObjectPtr<UAnimSequence>> GestureSequences;

	UPROPERTY()
	TObjectPtr<UAnimSequence> IdleSequence;

	UPROPERTY()
	TObjectPtr<UAnimSequence> WalkSequence;

	UPROPERTY()
	TObjectPtr<UAnimSequence> RunSequence;

	UPROPERTY()
	TObjectPtr<UAnimSequence> JumpSequence;

	UPROPERTY()
	TObjectPtr<UAnimSequence> LandSequence;

	// Los clips de arriba viven sobre SKEL_Humanoid_A, el esqueleto de las manos. El cuerpo
	// de sombra es el protagonista, con esqueleto propio de 75 huesos: empujarle una
	// secuencia ajena no produce una pose válida y la malla deja de dibujarse. Por eso el
	// cuerpo tiene su propio juego de clips, y se busca el equivalente al reproducir.
	UPROPERTY()
	TObjectPtr<UAnimSequence> BodyIdleSequence;

	UPROPERTY()
	TObjectPtr<UAnimSequence> BodyWalkSequence;

	UPROPERTY()
	TObjectPtr<UAnimSequence> BodyRunSequence;

	UPROPERTY()
	TObjectPtr<UAnimSequence> BodyJumpSequence;

	UPROPERTY()
	TObjectPtr<UAnimSequence> BodyLandSequence;

	// Clip que se está reproduciendo, para no reiniciarlo en cada frame.
	UPROPERTY()
	TObjectPtr<UAnimSequence> ActiveSequence;

	FName CurrentHandItemId;
	bool bHeldToolInitialized = false;
	bool bFirstPersonVisible = true;
	float GestureSecondsRemaining = 0.0f;
	bool bWasFallingLastFrame = false;
	float LandingSecondsRemaining = 0.0f;
	float RotorAngleDegrees = 0.0f;

	void RefreshHeldTool();
	void UpdateBodyLocomotion();
	UAnimSequence* SelectLocomotionSequence() const;

	// Equivalente del clip de manos en el juego de clips del cuerpo. Devuelve nullptr si no
	// hay correspondencia: entonces el cuerpo se queda en su pose anterior, que es preferible
	// a evaluarlo con una secuencia de otro esqueleto.
	UAnimSequence* BodyCounterpart(UAnimSequence* HandSequence) const;
	void PlaySequence(UAnimSequence* Sequence, bool bLoop);
};
