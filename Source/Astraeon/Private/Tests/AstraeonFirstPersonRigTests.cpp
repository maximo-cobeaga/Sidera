#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AstraeonPlayerCharacter.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/Skeleton.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Presentation/AstraeonFirstPersonRigComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonFirstPersonRigTest,
	"Astraeon.Art.Character.FirstPersonRigIsWired",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonFirstPersonRigTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Transient integration world"), World)) return false;

	AAstraeonPlayerCharacter* Character = World->SpawnActor<AAstraeonPlayerCharacter>();
	if (!TestNotNull(TEXT("Character spawns"), Character)) return false;

	UAstraeonFirstPersonRigComponent* Rig = Character->FindComponentByClass<UAstraeonFirstPersonRigComponent>();
	if (!TestNotNull(TEXT("First person rig exists"), Rig)) return false;

	// Las manos cuelgan de la cámara: sin eso no siguen el cabeceo de la vista.
	TestTrue(TEXT("Rig hangs from the first person camera"),
		Cast<UCameraComponent>(Rig->GetAttachParent()) != nullptr);

	USkeletalMesh* Hands = Rig->GetSkeletalMeshAsset();
	if (!TestNotNull(TEXT("Hands mesh loads from Content"), Hands)) return false;
	USkeleton* Skeleton = Hands->GetSkeleton();
	if (!TestNotNull(TEXT("Hands are skinned to a skeleton"), Skeleton)) return false;
	TestEqual(TEXT("SKEL_Humanoid_A keeps its 57 bones"), Skeleton->GetReferenceSkeleton().GetNum(), 57);
	TestEqual(TEXT("Root bone survives the FBX round trip"),
		Skeleton->GetReferenceSkeleton().GetBoneName(0), FName(TEXT("root")));

	// Regression: mesh bounds stayed 183 cm while every animated bone collapsed to 1/100.
	// Evaluate the runtime component, including its compressed animation, at several times.
	auto CheckAnimatedScale = [this](USkeletalMeshComponent* Component, UAnimSequence* Sequence)
	{
		if (!Sequence) return;
		Component->PlayAnimation(Sequence, false);
		for (float Fraction : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
		{
			Component->SetPosition(Sequence->GetPlayLength() * Fraction, false);
			Component->TickAnimation(0.0f, false);
			Component->RefreshBoneTransforms();
			const float HeadHeightCm = Component->GetSocketLocation(TEXT("head")).Z
				- Component->GetSocketLocation(TEXT("root")).Z;
			TestTrue(FString::Printf(TEXT("%s animated head height %.2f cm"), *Sequence->GetName(), HeadHeightCm),
				HeadHeightCm > 65.0f && HeadHeightCm < 220.0f);
		}
	};

	// El cuerpo completo sólo existe para la sombra propia: si el dueño lo viera, el
	// jugador se encontraría dentro de su propia malla.
	// La vista de tercera persona existe para poder ver al protagonista, que en primera
	// sólo se insinúa por la sombra. Al alternar tienen que moverse tres cosas a la vez:
	// qué cámara está activa, si el dueño ve su propio cuerpo, y si las manos —pegadas a la
	// cámara de primera— siguen dibujándose.
	UCameraComponent* FirstPerson = nullptr;
	UCameraComponent* ThirdPerson = nullptr;
	for (UActorComponent* Component : Character->GetComponents())
	{
		if (UCameraComponent* Camera = Cast<UCameraComponent>(Component))
		{
			if (Camera->GetName() == TEXT("FirstPersonCamera")) FirstPerson = Camera;
			else if (Camera->GetName() == TEXT("ThirdPersonCamera")) ThirdPerson = Camera;
		}
	}
	if (TestNotNull(TEXT("First person camera exists"), FirstPerson)
		&& TestNotNull(TEXT("Third person camera exists"), ThirdPerson))
	{
		// En un mundo transitorio que nunca arranca, la activación automática de los
		// componentes no es determinista, así que el estado inicial se comprueba por la
		// bandera del personaje y no por `IsActive`. A partir del primer toggle sí,
		// porque `ApplyCameraView` llama `SetActive` explícitamente en ambas.
		TestFalse(TEXT("Starts in first person"), Character->IsThirdPersonView());
		TestFalse(TEXT("Third person camera does not auto activate"), ThirdPerson->bAutoActivate);
		Character->ToggleCameraView();
		TestTrue(TEXT("Toggle flips the view flag"), Character->IsThirdPersonView());
		TestTrue(TEXT("Toggle activates the third person camera"),
			ThirdPerson->IsActive() && !FirstPerson->IsActive());
		TestFalse(TEXT("Third person shows the body to its own player"),
			Character->GetMesh()->bOwnerNoSee);
		TestFalse(TEXT("Third person hides the first person hands"), Rig->IsVisible());
		Character->ToggleCameraView();
		TestTrue(TEXT("Toggling back returns to first person"),
			FirstPerson->IsActive() && !ThirdPerson->IsActive());
		TestTrue(TEXT("First person hides the body again"), Character->GetMesh()->bOwnerNoSee);
		TestTrue(TEXT("First person shows the hands again"), Rig->IsVisible());
	}

	USkeletalMeshComponent* Body = Character->GetMesh();
	if (!TestNotNull(TEXT("Character keeps its body mesh component"), Body)) return false;
	// El cuerpo de sombra y las manos usan esqueletos distintos a propósito: el
	// protagonista trae el suyo de 75 huesos y las manos siguen sobre SKEL_Humanoid_A,
	// de 57. Antes esto se cubría exigiendo que compartieran esqueleto, pero esa
	// igualdad no era el requisito real — sólo una forma indirecta de comprobarlo. Lo
	// que de verdad importa es que cada gesto resuelva sobre el esqueleto que anima las
	// manos (se verifica más abajo) y que el cuerpo proyecte una silueta a escala. Ver
	// Docs/PENDIENTE_PROTAGONISTA.md, P9.
	if (USkeletalMesh* BodyMesh = Body->GetSkeletalMeshAsset())
	{
		TestTrue(TEXT("Body mesh loads from Content"), true);
		if (USkeleton* BodySkeleton = BodyMesh->GetSkeleton())
		{
			TestEqual(TEXT("Body skeleton keeps root as its first bone"),
				BodySkeleton->GetReferenceSkeleton().GetBoneName(0), FName(TEXT("root")));
		}
		else
		{
			AddError(TEXT("Body mesh is not skinned to a skeleton"));
		}
		// El cuerpo NUNCA debe recibir un clip del esqueleto de las manos: evaluarlo con una
		// secuencia ajena lo deja sin pose válida y deja de dibujarse. Este fue el defecto
		// que hacía que el jugador viera "un cuerpo vacío" en tercera persona.
		// En el mundo transitorio el BeginPlay del personaje no corre, así que se engancha
		// el cuerpo a mano: es justamente la llamada que arranca su animación.
		Rig->SetShadowBodyMesh(Body);
		if (UAnimSingleNodeInstance* BodyAnim = Body->GetSingleNodeInstance())
		{
			UAnimationAsset* Playing = BodyAnim->GetAnimationAsset();
			if (TestNotNull(TEXT("Body receives an animation of its own"), Playing))
			{
				TestNotEqual(TEXT("Body is not driven by the hands skeleton"),
					Playing->GetSkeleton(), Skeleton);
				TestEqual(TEXT("Body clip belongs to the body skeleton"),
					Playing->GetSkeleton(), BodyMesh->GetSkeleton());
			}
		}
		else
		{
			AddError(TEXT("Body has no animation instance after being wired as shadow body"));
		}
		for (const TCHAR* Clip : { TEXT("Idle"), TEXT("Walk_F"), TEXT("Run_F"), TEXT("Jump_Loop"), TEXT("Jump_Land") })
		{
			const FString Path = FString::Printf(TEXT("/Game/Astraeon/Characters/Player/Optimized/AN_Astraeon_Player_All_Armature_AN_Player_%s"), Clip);
			UAnimSequence* Sequence = LoadObject<UAnimSequence>(nullptr, *Path);
			if (TestNotNull(TEXT("Body locomotion loads"), Sequence)) CheckAnimatedScale(Body, Sequence);
		}

		// La sombra sólo sirve si la figura mide lo que mide el personaje.
		const float BodyHeightCm = BodyMesh->GetBounds().BoxExtent.Z * 2.0f;
		TestTrue(FString::Printf(TEXT("Body reads at human scale (%.1f cm)"), BodyHeightCm),
			BodyHeightCm > 170.0f && BodyHeightCm < 195.0f);
		// Raíz en los pies: la malla baja media cápsula para apoyar en el suelo.
		const float HalfHeight = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		TestEqual(TEXT("Body root sits at the capsule floor"),
			static_cast<float>(Body->GetRelativeLocation().Z), -HalfHeight, 0.5f);
	}
	else
	{
		AddError(TEXT("Body mesh failed to load from Content"));
	}
	TestTrue(TEXT("Owner does not see their own body"), Body->bOwnerNoSee);
	TestTrue(TEXT("Hidden body still casts a shadow"), Body->bCastHiddenShadow);
	TestFalse(TEXT("Hands do not cast a second pair of arms"), Rig->CastShadow);

	// Cada gesto tiene que resolver a un clip real sobre ese mismo esqueleto, o la acción
	// se dispara y no se ve nada.
	const EAstraeonHandGesture Gestures[] = {
		EAstraeonHandGesture::Scan, EAstraeonHandGesture::Pulse, EAstraeonHandGesture::Drill,
		EAstraeonHandGesture::Hammer, EAstraeonHandGesture::Maul, EAstraeonHandGesture::Consume,
		EAstraeonHandGesture::Present, EAstraeonHandGesture::Interact, EAstraeonHandGesture::Grip};
	for (const EAstraeonHandGesture Gesture : Gestures)
	{
		UAnimSequence* Sequence = Rig->GetGestureSequence(Gesture);
		if (!TestNotNull(TEXT("Gesture resolves to a clip"), Sequence)) continue;
		TestEqual(TEXT("Gesture clip targets the shared skeleton"), Sequence->GetSkeleton(), Skeleton);
		TestTrue(TEXT("Gesture clip has duration"), Sequence->GetPlayLength() > 0.0f);
		CheckAnimatedScale(Rig, Sequence);
	}
	TestNull(TEXT("The empty gesture stays empty"), Rig->GetGestureSequence(EAstraeonHandGesture::None));

	// La herramienta cuelga del hueso de agarre, no de una posición fija en pantalla.
	UStaticMeshComponent* Tool = nullptr;
	for (USceneComponent* Child : Rig->GetAttachChildren())
	{
		if (Child && Child->GetFName() == TEXT("HandToolMesh")) Tool = Cast<UStaticMeshComponent>(Child);
	}
	if (TestNotNull(TEXT("Hand tool component exists"), Tool))
	{
		TestEqual(TEXT("Tool hangs from the grip bone"), Tool->GetAttachSocketName(), FName(TEXT("socket_tool_r")));
		TestEqual(TEXT("Tool cannot block a trace"), Tool->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	}

	// Los clips de locomoción y las mallas de herramienta se cargan por ruta: si alguien
	// mueve o renombra un paquete, esto lo detecta antes que una sesión manual.
	const TCHAR* RequiredAssets[] = {
		TEXT("/Game/Astraeon/Art/Blockouts/Human/AN_Human_Idle_Blockout.AN_Human_Idle_Blockout"),
		TEXT("/Game/Astraeon/Art/Blockouts/Human/AN_Human_Walk_Blockout.AN_Human_Walk_Blockout"),
		TEXT("/Game/Astraeon/Art/Blockouts/Human/AN_Human_Run_Blockout.AN_Human_Run_Blockout"),
		TEXT("/Game/Astraeon/Art/Blockouts/Human/AN_Human_Jump_Blockout.AN_Human_Jump_Blockout"),
		TEXT("/Game/Astraeon/Art/Blockouts/Human/AN_Human_Land_Blockout.AN_Human_Land_Blockout")};
	for (const TCHAR* AssetPath : RequiredAssets)
	{
		UAnimSequence* Sequence = LoadObject<UAnimSequence>(nullptr, AssetPath);
		if (TestNotNull(TEXT("Locomotion clip loads from Content"), Sequence))
		{
			TestEqual(TEXT("Locomotion clip targets the shared skeleton"), Sequence->GetSkeleton(), Skeleton);
			CheckAnimatedScale(Rig, Sequence);
		}
	}

	// Un ítem sin malla se vería como manos vacías sosteniendo aire; los ids son claves de
	// guardado, así que la presentación tiene que cubrirlos todos.
	const TCHAR* HeldItemMeshes[] = {
		TEXT("/Game/Astraeon/Art/Blockouts/Tools/SM_Tool_Scanner_Blockout.SM_Tool_Scanner_Blockout"),
		TEXT("/Game/Astraeon/Art/Blockouts/Tools/SM_Tool_PulseCutter_Blockout.SM_Tool_PulseCutter_Blockout"),
		TEXT("/Game/Astraeon/Art/Blockouts/Tools/SM_Tool_CoreDrill_Blockout.SM_Tool_CoreDrill_Blockout"),
		TEXT("/Game/Astraeon/Art/Blockouts/Tools/SM_Tool_BuildHammer_Blockout.SM_Tool_BuildHammer_Blockout"),
		TEXT("/Game/Astraeon/Art/Blockouts/Tools/SM_Tool_DemolitionMaul_Blockout.SM_Tool_DemolitionMaul_Blockout"),
		TEXT("/Game/Astraeon/Art/Blockouts/Tools/SM_Item_RationPack_Blockout.SM_Item_RationPack_Blockout"),
		TEXT("/Game/Astraeon/Art/Blockouts/Tools/SM_Item_SignalResonator_Blockout.SM_Item_SignalResonator_Blockout"),
		TEXT("/Game/Astraeon/Art/Blockouts/Tools/SM_Tool_CoreDrillRotor_Blockout.SM_Tool_CoreDrillRotor_Blockout")};
	for (const TCHAR* AssetPath : HeldItemMeshes)
	{
		TestNotNull(TEXT("Held item mesh loads from Content"), LoadObject<UStaticMesh>(nullptr, AssetPath));
	}

	// BeginPlay must initialize the default scanner even when the held item is NAME_None.
	Character->DispatchBeginPlay();
	if (Tool)
	{
		TestTrue(TEXT("Tool is registered for rendering"), Tool->IsRegistered());
		TestNotNull(TEXT("New game has a visible scanner mesh"), Tool->GetStaticMesh().Get());
		TestTrue(TEXT("Static tool does not inherit FBX bone scale 100"), Tool->GetComponentScale().Equals(FVector::OneVector, 0.001f));
	}
	Rig->TickAnimation(0.0f, false);
	Rig->RefreshBoneTransforms();
	if (FirstPerson)
	{
		const FVector HandInView = FirstPerson->GetComponentTransform().InverseTransformPosition(Rig->GetSocketLocation(TEXT("hand_r")));
		TestTrue(TEXT("Ready hand is ahead of the eye and within the vertical field of view"),
			HandInView.X > 20 && FMath::Abs(HandInView.Z / HandInView.X) < 0.56f);
	}
	int32 EquipmentCount = 0;
	for (UActorComponent* Component : Character->GetComponents())
	{
		USkeletalMeshComponent* Part = Cast<USkeletalMeshComponent>(Component);
		if (!Part || Part == Body || Part == Rig) continue;
		++EquipmentCount;
		TestNotNull(TEXT("Equipment mesh loads"), Part->GetSkeletalMeshAsset());
		TestTrue(TEXT("Equipment follows the body pose"), Part->LeaderPoseComponent.Get() == Body);
		TestTrue(TEXT("Equipment hidden from first person owner"), Part->bOwnerNoSee);
		Character->ToggleCameraView();
		TestFalse(TEXT("Equipment visible in third person"), Part->bOwnerNoSee);
		Character->ToggleCameraView();
	}
	TestEqual(TEXT("Helmet, backpack and wrist computer mounted"), EquipmentCount, 3);
	// A hand gesture must not freeze body locomotion or expose a drill rotor in FP.
	Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Character->GetCharacterMovement()->Velocity = FVector(800, 0, 0);
	Rig->PlayGesture(EAstraeonHandGesture::Scan);
	Rig->TickComponent(0.016f, LEVELTICK_All, nullptr);
	if (UAnimSingleNodeInstance* Instance = Body->GetSingleNodeInstance())
	{
		TestTrue(TEXT("Body runs while hands scan"), Instance->GetAnimationAsset()->GetName().EndsWith(TEXT("Run_F")));
	}
	for (USceneComponent* Child : Tool->GetAttachChildren())
	{
		TestFalse(TEXT("Empty hand scanner never displays the drill rotor"), Child->IsVisible());
	}
	World->DestroyWorld(false);
	return true;
}

#endif
