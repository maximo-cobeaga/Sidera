#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Creatures/AstraeonCreatureActor.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonCreatureArtTest,
	"Astraeon.Art.Creature.PresentationPreservesGameplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstraeonCreatureArtTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Transient integration world"), World)) return false;

	AAstraeonCreatureActor* Creature = World->SpawnActor<AAstraeonCreatureActor>();
	if (!TestNotNull(TEXT("Creature spawns"), Creature)) return false;

	USkeletalMeshComponent* Art = Creature->FindComponentByClass<USkeletalMeshComponent>();
	if (!TestNotNull(TEXT("Creature art component exists"), Art)) return false;
	USkeletalMesh* Mesh = Art->GetSkeletalMeshAsset();
	if (!TestNotNull(TEXT("Creature mesh loads from Content"), Mesh)) return false;
	USkeleton* Skeleton = Mesh->GetSkeleton();
	if (!TestNotNull(TEXT("Creature is skinned to a skeleton"), Skeleton)) return false;
	TestEqual(TEXT("SKEL_Quadruped_A keeps its 37 bones"), Skeleton->GetReferenceSkeleton().GetNum(), 37);
	TestEqual(TEXT("Root bone survives the FBX round trip"),
		Skeleton->GetReferenceSkeleton().GetBoneName(0), FName(TEXT("root")));
	// Cuatro cadenas y cola: si el rig se recorta, la marcha y la caída dejan de existir.
	for (const TCHAR* Bone : {TEXT("upperleg_f_l"), TEXT("lowerleg_f_r"), TEXT("foot_b_l"),
		TEXT("tail_04"), TEXT("jaw"), TEXT("socket_horn_r")})
	{
		TestTrue(FString::Printf(TEXT("Rig keeps %s"), Bone),
			Skeleton->GetReferenceSkeleton().FindBoneIndex(FName(Bone)) != INDEX_NONE);
	}

	// El arte no puede quedar en el camino de un disparo, un escaneo ni un empujón: la
	// colisión sigue siendo el envolvente invisible que ya existía.
	TestEqual(TEXT("Art carries no collision"), Art->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	UStaticMeshComponent* Envelope = Cast<UStaticMeshComponent>(Creature->GetRootComponent());
	if (TestNotNull(TEXT("Interaction envelope is still the root"), Envelope))
	{
		TestEqual(TEXT("Envelope still blocks"), Envelope->GetCollisionEnabled(), ECollisionEnabled::QueryAndPhysics);
		TestFalse(TEXT("The old proxy sphere is hidden"), Envelope->IsVisible());
		TestTrue(TEXT("Envelope keeps the 1.4 x 0.8 x 0.7 m silhouette"),
			Envelope->GetRelativeScale3D().Equals(FVector(1.4f, 0.8f, 0.7f), 0.001f));
	}
	// El pivote pasó a las patas, así que las distancias se miden al centro del cuerpo.
	TestTrue(TEXT("Body centre sits above the feet"),
		FMath::IsNearlyEqual(Creature->GetBodyCenterCm().Z - Creature->GetActorLocation().Z, 35.0f, 0.01f));

	// Cada estado de conciencia tiene que tener su clip, o el estado no se ve.
	const TCHAR* Clips[] = {TEXT("Graze"), TEXT("Walk"), TEXT("Alert"),
		TEXT("Threaten"), TEXT("Flee"), TEXT("Hit"), TEXT("Death")};
	for (const TCHAR* Clip : Clips)
	{
		const FString Path = FString::Printf(
			TEXT("/Game/Astraeon/Art/Blockouts/Creatures/AN_Creature_%s_Blockout.AN_Creature_%s_Blockout"), Clip, Clip);
		UAnimSequence* Sequence = LoadObject<UAnimSequence>(nullptr, *Path);
		if (!TestNotNull(FString::Printf(TEXT("%s clip loads from Content"), Clip), Sequence)) continue;
		TestEqual(TEXT("Clip targets the shared skeleton"), Sequence->GetSkeleton(), Skeleton);
		TestTrue(TEXT("Clip has duration"), Sequence->GetPlayLength() > 0.0f);
	}

	// AC-06 pide al menos una variante visual, y tiene que ser estable: la misma seed
	// devuelve el mismo animal en el mismo nido partida tras partida.
	USkeletalMesh* Variant = LoadObject<USkeletalMesh>(nullptr,
		TEXT("/Game/Astraeon/Art/Blockouts/Creatures/SK_Creature_UmbraGrazer_Plated_Blockout.SK_Creature_UmbraGrazer_Plated_Blockout"));
	if (TestNotNull(TEXT("Variant mesh loads from Content"), Variant))
	{
		TestEqual(TEXT("Variant shares the skeleton"), Variant->GetSkeleton(), Skeleton);
	}
	Creature->SetSpawnPointId(TEXT("creature_spawn_0"));
	USkeletalMesh* First = Art->GetSkeletalMeshAsset();
	Creature->SetSpawnPointId(TEXT("creature_spawn_0"));
	TestEqual(TEXT("The same nest always yields the same variant"), Art->GetSkeletalMeshAsset(), First);

	// Matar tiene que dejar un cadáver a la vista, no un actor que desaparece en el frame.
	Creature->ApplyWeaponDamage(1000.0f);
	TestTrue(TEXT("A lethal hit reads as dead"), Creature->IsDead());
	Creature->BeginDeathSequence();
	TestFalse(TEXT("A corpse stops blocking"), Creature->GetActorEnableCollision());
	TestFalse(TEXT("A corpse is not destroyed on the shot's own frame"), Creature->IsActorBeingDestroyed());

	World->DestroyWorld(false);
	return true;
}

#endif
