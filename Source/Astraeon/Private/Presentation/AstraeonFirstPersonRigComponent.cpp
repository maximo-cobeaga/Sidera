#include "Presentation/AstraeonFirstPersonRigComponent.h"

#include "AstraeonGameInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace AstraeonFirstPersonRig
{
	const TCHAR* HumanPath = TEXT("/Game/Astraeon/Art/Blockouts/Human/");
	const TCHAR* ToolsPath = TEXT("/Game/Astraeon/Art/Blockouts/Tools/");
	const TCHAR* PlayerPath = TEXT("/Game/Astraeon/Characters/Player/Optimized/");

	// Por debajo de esto el personaje está efectivamente quieto; por encima del umbral de
	// carrera usa el clip corto. Los dos valores salen de las velocidades reales del
	// Character (520 cm/s caminando, 900 cm/s corriendo).
	constexpr float StandingSpeedCms = 10.0f;
	constexpr float RunSpeedThresholdCms = 620.0f;
	constexpr float LandingSeconds = 0.6f;

	template <typename AssetType>
	AssetType* Load(const FString& ObjectPath)
	{
		ConstructorHelpers::FObjectFinder<AssetType> Finder(*ObjectPath);
		return Finder.Succeeded() ? Finder.Object : nullptr;
	}

	FString MeshRef(const TCHAR* Directory, const TCHAR* Name)
	{
		return FString::Printf(TEXT("%s%s.%s"), Directory, Name, Name);
	}
}

UAstraeonFirstPersonRigComponent::UAstraeonFirstPersonRigComponent()
{
	using namespace AstraeonFirstPersonRig;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// Sólo el dueño ve las manos; la sombra la proyecta el cuerpo completo, no estas manos
	// flotantes, que si arrojaran sombra dibujarían dos brazos sueltos en el suelo.
	SetOnlyOwnerSee(true);
	SetCastShadow(false);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	// Sin esto las manos se cullean al mirar hacia abajo, porque los bounds del rig
	// completo quedan mayormente detrás de la cámara.
	bUseAttachParentBound = false;
	SetBoundsScale(4.0f);

	if (USkeletalMesh* Hands = Load<USkeletalMesh>(MeshRef(HumanPath, TEXT("SK_Human_HandsFP_Blockout"))))
	{
		SetSkeletalMeshAsset(Hands);
	}

	ToolMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandToolMesh"));
	ToolMesh->SetupAttachment(this, TEXT("socket_tool_r"));
	ToolMesh->SetOnlyOwnerSee(true);
	ToolMesh->SetCastShadow(false);
	ToolMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ToolMesh->SetGenerateOverlapEvents(false);
	ToolMesh->SetAbsolute(false, false, true);

	ToolRotorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandToolRotorMesh"));
	ToolRotorMesh->SetupAttachment(ToolMesh);
	ToolRotorMesh->SetOnlyOwnerSee(true);
	ToolRotorMesh->SetCastShadow(false);
	ToolRotorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ToolRotorMesh->SetGenerateOverlapEvents(false);
	ToolRotorMesh->SetVisibility(false);

	// Agarre medido en Blender (`attachment.matrix_local_blender_m` del reporte de
	// validación), expresado en el espacio del hueso socket_tool_r. Unreal importa el FBX
	// con (x,y,z) -> (x,-y,z), así que la matriz equivalente es C*M*C con C = diag(1,-1,1),
	// y Unreal usa vectores fila, por lo que se guarda transpuesta. Metros -> centímetros.
	const FMatrix GripMatrix(
		FPlane(0.0f, 1.0f, 0.0f, 0.0f),
		FPlane(0.3499177f, 0.0f, -0.9367805f, 0.0f),
		FPlane(-0.9367805f, 0.0f, -0.3499177f, 0.0f),
		FPlane(1.92455f, -2.6f, -5.15229f, 1.0f));
	ToolGripTransform = FTransform(GripMatrix);

	// Offset de la broca respecto del cuerpo del taladro (`drill_rotor.offset_m`), con el
	// mismo espejo en Y.
	ToolRotorMesh->SetRelativeLocation(FVector(0.0f, 24.0f, 14.0f));

	ScannerMesh = Load<UStaticMesh>(MeshRef(ToolsPath, TEXT("SM_Tool_Scanner_Blockout")));
	RotorMesh = Load<UStaticMesh>(MeshRef(ToolsPath, TEXT("SM_Tool_CoreDrillRotor_Blockout")));

	// Los ids son claves de guardado y de recetas: la presentación se cuelga de ellos, no
	// al revés (ver ASSETS_PENDIENTES_DISENO_ANIMACION.md §8).
	const TPair<const TCHAR*, const TCHAR*> ToolsByItem[] = {
		{TEXT("weapon_pulse_cutter"), TEXT("SM_Tool_PulseCutter_Blockout")},
		{TEXT("tool_core_drill"), TEXT("SM_Tool_CoreDrill_Blockout")},
		{TEXT("tool_build_hammer"), TEXT("SM_Tool_BuildHammer_Blockout")},
		{TEXT("tool_demolition_maul"), TEXT("SM_Tool_DemolitionMaul_Blockout")},
		{TEXT("ration_pack"), TEXT("SM_Item_RationPack_Blockout")},
		{TEXT("signal_resonator"), TEXT("SM_Item_SignalResonator_Blockout")}};
	for (const TPair<const TCHAR*, const TCHAR*>& Entry : ToolsByItem)
	{
		if (UStaticMesh* Mesh = Load<UStaticMesh>(MeshRef(ToolsPath, Entry.Value)))
		{
			ToolMeshesByItemId.Add(FName(Entry.Key), Mesh);
		}
	}

	// Clips del protagonista, sobre su propio esqueleto, para el cuerpo de sombra.
	BodyIdleSequence = Load<UAnimSequence>(MeshRef(PlayerPath, TEXT("AN_Astraeon_Player_All_Armature_AN_Player_Idle")));
	BodyWalkSequence = Load<UAnimSequence>(MeshRef(PlayerPath, TEXT("AN_Astraeon_Player_All_Armature_AN_Player_Walk_F")));
	BodyRunSequence = Load<UAnimSequence>(MeshRef(PlayerPath, TEXT("AN_Astraeon_Player_All_Armature_AN_Player_Run_F")));
	BodyJumpSequence = Load<UAnimSequence>(MeshRef(PlayerPath, TEXT("AN_Astraeon_Player_All_Armature_AN_Player_Jump_Loop")));
	BodyLandSequence = Load<UAnimSequence>(MeshRef(PlayerPath, TEXT("AN_Astraeon_Player_All_Armature_AN_Player_Jump_Land")));

	IdleSequence = Load<UAnimSequence>(MeshRef(HumanPath, TEXT("AN_HandsFP_Idle")));
	WalkSequence = Load<UAnimSequence>(MeshRef(HumanPath, TEXT("AN_HandsFP_Walk")));
	RunSequence = Load<UAnimSequence>(MeshRef(HumanPath, TEXT("AN_HandsFP_Run")));
	JumpSequence = Load<UAnimSequence>(MeshRef(HumanPath, TEXT("AN_HandsFP_Jump")));
	LandSequence = Load<UAnimSequence>(MeshRef(HumanPath, TEXT("AN_HandsFP_Land")));

	const TPair<EAstraeonHandGesture, const TCHAR*> Gestures[] = {
		{EAstraeonHandGesture::Scan, TEXT("AN_Human_Scan_Tool_Blockout")},
		{EAstraeonHandGesture::Pulse, TEXT("AN_Human_Pulse_Tool_Blockout")},
		{EAstraeonHandGesture::Drill, TEXT("AN_Human_Drill_Tool_Blockout")},
		{EAstraeonHandGesture::Hammer, TEXT("AN_Human_Hammer_Tool_Blockout")},
		{EAstraeonHandGesture::Maul, TEXT("AN_Human_Maul_Tool_Blockout")},
		{EAstraeonHandGesture::Consume, TEXT("AN_Human_Consume_Tool_Blockout")},
		{EAstraeonHandGesture::Present, TEXT("AN_Human_Present_Tool_Blockout")}};
	for (const TPair<EAstraeonHandGesture, const TCHAR*>& Entry : Gestures)
	{
		if (UAnimSequence* Sequence = Load<UAnimSequence>(MeshRef(ToolsPath, Entry.Value)))
		{
			GestureSequences.Add(static_cast<uint8>(Entry.Key), Sequence);
		}
	}
	// Interactuar y agarrar salen del lote humano, no del de herramientas.
	if (UAnimSequence* Sequence = Load<UAnimSequence>(MeshRef(HumanPath, TEXT("AN_Human_Interact_Blockout"))))
	{
		GestureSequences.Add(static_cast<uint8>(EAstraeonHandGesture::Interact), Sequence);
	}
	if (UAnimSequence* Sequence = Load<UAnimSequence>(MeshRef(HumanPath, TEXT("AN_Human_Grip_Blockout"))))
	{
		GestureSequences.Add(static_cast<uint8>(EAstraeonHandGesture::Grip), Sequence);
	}
}

void UAstraeonFirstPersonRigComponent::BeginPlay()
{
	Super::BeginPlay();

	SetRelativeLocation(HandsOffsetCm);
	SetRelativeRotation(HandsRotation);
	// El socket hereda la escala 100 de la raíz FBX. El agarre está documentado en cm:
	// convertir su offset a unidades locales del socket y conservar la malla a escala 1.
	FTransform LocalGrip = ToolGripTransform;
	const FVector RootScale = GetSkeletalMeshAsset()->GetRefSkeleton().GetRefBonePose()[0].GetScale3D();
	LocalGrip.SetTranslation(LocalGrip.GetTranslation() / RootScale);
	ToolMesh->SetRelativeTransform(LocalGrip);
	ToolMesh->RegisterComponent();
	ToolRotorMesh->RegisterComponent();
	ToolRotorMesh->SetStaticMesh(RotorMesh);

	CurrentHandItemId = NAME_None;
	RefreshHeldTool();
	PlaySequence(IdleSequence, true);
}

void UAstraeonFirstPersonRigComponent::SetFirstPersonVisible(bool bNewVisible)
{
	bFirstPersonVisible = bNewVisible;
	SetVisibility(bNewVisible);
	ToolMesh->SetVisibility(bNewVisible);
	ToolRotorMesh->SetVisibility(bNewVisible && CurrentHandItemId == TEXT("tool_core_drill"));
}

void UAstraeonFirstPersonRigComponent::PlayGesture(EAstraeonHandGesture Gesture)
{
	UAnimSequence* Sequence = GetGestureSequence(Gesture);
	if (!Sequence)
	{
		return;
	}

	GestureSecondsRemaining = Sequence->GetPlayLength();
	// Forzar el reinicio: repetir la acción tiene que volver a verse.
	ActiveSequence = nullptr;
	PlaySequence(Sequence, false);
}

UAnimSequence* UAstraeonFirstPersonRigComponent::GetGestureSequence(EAstraeonHandGesture Gesture) const
{
	const TObjectPtr<UAnimSequence>* Found = GestureSequences.Find(static_cast<uint8>(Gesture));
	return Found ? Found->Get() : nullptr;
}

void UAstraeonFirstPersonRigComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	RefreshHeldTool();

	if (ToolRotorMesh->IsVisible())
	{
		RotorAngleDegrees = FMath::Fmod(RotorAngleDegrees + RotorDegreesPerSecond * DeltaSeconds, 360.0f);
		ToolRotorMesh->SetRelativeRotation(FRotator(RotorAngleDegrees, 0.0f, 0.0f));
	}

	const ACharacter* OwningCharacter = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* Movement = OwningCharacter ? OwningCharacter->GetCharacterMovement() : nullptr;
	const bool bFalling = Movement && Movement->IsFalling();
	if (bWasFallingLastFrame && !bFalling)
	{
		LandingSecondsRemaining = AstraeonFirstPersonRig::LandingSeconds;
	}
	bWasFallingLastFrame = bFalling;
	LandingSecondsRemaining = FMath::Max(0.0f, LandingSecondsRemaining - DeltaSeconds);
	UpdateBodyLocomotion();

	if (GestureSecondsRemaining > 0.0f)
	{
		GestureSecondsRemaining -= DeltaSeconds;
		return;
	}

	UAnimSequence* Desired = SelectLocomotionSequence();
	if (Desired != ActiveSequence)
	{
		PlaySequence(Desired, Desired == IdleSequence || Desired == WalkSequence || Desired == RunSequence);
	}
}

UAnimSequence* UAstraeonFirstPersonRigComponent::SelectLocomotionSequence() const
{
	using namespace AstraeonFirstPersonRig;

	const ACharacter* OwningCharacter = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* Movement = OwningCharacter ? OwningCharacter->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		return IdleSequence;
	}

	if (Movement->IsFalling())
	{
		return JumpSequence ? JumpSequence : IdleSequence;
	}
	if (LandingSecondsRemaining > 0.0f && LandSequence)
	{
		return LandSequence;
	}

	const float SpeedCms = Movement->Velocity.Size2D();
	if (SpeedCms >= RunSpeedThresholdCms && RunSequence)
	{
		return RunSequence;
	}
	if (SpeedCms >= StandingSpeedCms && WalkSequence)
	{
		return WalkSequence;
	}
	return IdleSequence;
}

void UAstraeonFirstPersonRigComponent::PlaySequence(UAnimSequence* Sequence, bool bLoop)
{
	if (!Sequence || Sequence == ActiveSequence)
	{
		return;
	}

	ActiveSequence = Sequence;
	PlayAnimation(Sequence, bLoop);
}

void UAstraeonFirstPersonRigComponent::UpdateBodyLocomotion()
{
	// Sigue actualizándose durante un gesto de manos y cuando se oculta el rig FP.
	if (!ShadowBody)
	{
		return;
	}
	UAnimSequence* BodySequence = BodyCounterpart(SelectLocomotionSequence());
	if (!BodySequence)
	{
		return;
	}
	const USkeletalMesh* BodyMesh = ShadowBody->GetSkeletalMeshAsset();
	if (BodyMesh && BodyMesh->GetSkeleton() == BodySequence->GetSkeleton())
	{
		const UAnimSingleNodeInstance* Instance = ShadowBody->GetSingleNodeInstance();
		if (!Instance || Instance->GetAnimationAsset() != BodySequence)
		{
			ShadowBody->PlayAnimation(BodySequence, BodySequence != BodyLandSequence);
		}
	}
}

void UAstraeonFirstPersonRigComponent::SetShadowBodyMesh(USkeletalMeshComponent* BodyMesh)
{
	ShadowBody = BodyMesh;
	// El cuerpo se asigna desde el BeginPlay del personaje, que puede correr después del de
	// este componente. Sin este arranque quedaría en pose de referencia hasta el primer
	// cambio de locomoción.
	UAnimSequence* BodySequence = BodyCounterpart(ActiveSequence ? ActiveSequence : IdleSequence);
	if (!ShadowBody || !BodySequence)
	{
		return;
	}
	const USkeletalMesh* Mesh = ShadowBody->GetSkeletalMeshAsset();
	if (Mesh && Mesh->GetSkeleton() == BodySequence->GetSkeleton())
	{
		ShadowBody->PlayAnimation(BodySequence, true);
	}
}

UAnimSequence* UAstraeonFirstPersonRigComponent::BodyCounterpart(UAnimSequence* HandSequence) const
{
	if (HandSequence == IdleSequence) return BodyIdleSequence;
	if (HandSequence == WalkSequence) return BodyWalkSequence;
	if (HandSequence == RunSequence) return BodyRunSequence;
	if (HandSequence == JumpSequence) return BodyJumpSequence;
	if (HandSequence == LandSequence) return BodyLandSequence;
	// Los gestos son de manos: el cuerpo mantiene su locomoción en lugar de congelarse.
	return nullptr;
}

void UAstraeonFirstPersonRigComponent::RefreshHeldTool()
{
	const UAstraeonGameInstance* AstraeonGameInstance = GetOwner()
		? GetOwner()->GetGameInstance<UAstraeonGameInstance>()
		: nullptr;
	const FName HeldItemId = AstraeonGameInstance ? AstraeonGameInstance->GetHandItemId() : NAME_None;
	if (bHeldToolInitialized && HeldItemId == CurrentHandItemId)
	{
		return;
	}

	CurrentHandItemId = HeldItemId;
	bHeldToolInitialized = true;
	const TObjectPtr<UStaticMesh>* Found = ToolMeshesByItemId.Find(HeldItemId);
	// Con las manos vacías queda el escáner: es la herramienta que el diseño da por
	// disponible desde el primer minuto y no ocupa una ranura de inventario.
	ToolMesh->SetStaticMesh(Found ? Found->Get() : ScannerMesh.Get());
	ToolRotorMesh->SetVisibility(bFirstPersonVisible && HeldItemId == TEXT("tool_core_drill"));
}
