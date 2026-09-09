#include "AstraeonDiagnostics.h"

#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY(LogAstraeonDiag);

namespace AstraeonDiagnostics
{
	bool IsEnabled()
	{
		// Se resuelve una vez: la línea de comandos no cambia durante la sesión.
		static const bool bEnabled = FParse::Param(FCommandLine::Get(), TEXT("AstraeonDiag"));
		return bEnabled;
	}

	FString DescribeGroundUnder(const AActor& Actor, float TraceDistanceCm)
	{
		const UWorld* World = Actor.GetWorld();
		if (!World)
		{
			return TEXT("ground=<no world>");
		}

		const FVector Start = Actor.GetActorLocation();
		const FVector End = Start - FVector(0.0f, 0.0f, TraceDistanceCm);
		FHitResult Hit;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AstraeonDiagGround), false, &Actor);
		if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
		{
			return FString::Printf(TEXT("ground=NONE within %.0fcm"), TraceDistanceCm);
		}

		return FString::Printf(TEXT("ground=%s/%s z=%.1f drop=%.1f"),
			*GetNameSafe(Hit.GetActor()), *GetNameSafe(Hit.GetComponent()),
			Hit.ImpactPoint.Z, Start.Z - Hit.ImpactPoint.Z);
	}

	FString DescribeBlockingOverlaps(const AActor& Actor, float RadiusCm, float HalfHeightCm)
	{
		const UWorld* World = Actor.GetWorld();
		if (!World)
		{
			return TEXT("overlaps=<no world>");
		}

		TArray<FOverlapResult> Overlaps;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AstraeonDiagOverlap), false, &Actor);
		World->OverlapMultiByChannel(Overlaps, Actor.GetActorLocation(), FQuat::Identity,
			ECC_Pawn, FCollisionShape::MakeCapsule(RadiusCm, HalfHeightCm), QueryParams);

		TArray<FString> Blocking;
		for (const FOverlapResult& Overlap : Overlaps)
		{
			const UPrimitiveComponent* Component = Overlap.GetComponent();
			if (Component && Component->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block)
			{
				Blocking.Add(FString::Printf(TEXT("%s/%s"),
					*GetNameSafe(Overlap.GetActor()), *Component->GetName()));
			}
		}

		// Cero es lo normal de pie sobre el suelo. Dos o más superficies bloqueantes
		// atravesando la cápsula es exactamente lo que produce empujes y vibración.
		return FString::Printf(TEXT("blockingOverlaps=%d [%s]"),
			Blocking.Num(), *FString::Join(Blocking, TEXT(", ")));
	}
}
