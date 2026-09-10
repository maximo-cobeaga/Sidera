#include "Tests/AstraeonPlanetCardinalSmoke.h"
#include "AstraeonPlayerCharacter.h"
#include "AstraeonPlayerController.h"
#include "Planet/AstraeonPlanetRuntime.h"
#include "Planet/Coordinates/AstraeonPlanetFrame.h"
#include "Planet/Gravity/AstraeonPlanetGravityComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"

AAstraeonPlanetCardinalSmoke::AAstraeonPlanetCardinalSmoke()
{
	PrimaryActorTick.bCanEverTick=true;
	PrimaryActorTick.TickGroup=TG_PostPhysics;
	// Six face centres, twelve edges, eight corners. Test every sign combination.
	for (int32 X=-1; X<=1; ++X)
	for (int32 Y=-1; Y<=1; ++Y)
	for (int32 Z=-1; Z<=1; ++Z)
		if (X || Y || Z) Directions.Add(FVector(X,Y,Z).GetSafeNormal());
}

void AAstraeonPlanetCardinalSmoke::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDone) return;
	Startup+=DeltaSeconds;
	auto* PC=Cast<AAstraeonPlayerController>(GetWorld()->GetFirstPlayerController());
	auto* Character=PC ? Cast<AAstraeonPlayerCharacter>(PC->GetPawn()) : nullptr;
	auto* Planet=AAstraeonPlanetRuntime::FindActive(GetWorld());
	if (!PC || !Character || !Planet)
	{
		if (Startup>10) Finish(false,TEXT("Missing player or runtime"));
		return;
	}
	if (!bStarted) { PC->StartSelectedNewGame(); bStarted=true; return; }
	auto* Move=Character->GetCharacterMovement();
	auto* Gravity=Character->FindComponentByClass<UAstraeonPlanetGravityComponent>();
	if (Site<0 || Elapsed>=4.0f)
	{
		if (Site>=0)
		{
			if (!bSawFlight || MaxLiftCm<20 || !Move->IsMovingOnGround())
			{
				Finish(false,FString::Printf(TEXT("site=%d flight=%d lift=%.2f landed=%d"),Site,bSawFlight,MaxLiftCm,Move->IsMovingOnGround())); return;
			}
			UE_LOG(LogTemp,Display,TEXT("PlanetCardinals: site=%d PASS lift=%.2f collision=%d"),Site,MaxLiftCm,Planet->GetCollisionTriangleCount());
		}
		if (++Site>=Directions.Num()) { Finish(true,TEXT("26 sites, jump and landing verified")); return; }
		Elapsed=0; bSawFlight=false; MaxLiftCm=0;
		const FVector Up=Directions[Site];
		if (!Planet->PrepareCollision(Up,true)) { Finish(false,TEXT("Collision missing")); return; }
		FHitResult Hit;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(PlanetCardinalGround),true,Character);
		const FVector Center=Planet->GetActorLocation();
		// A 5 cm sphere, not a ray. Collision is one mesh per patch since P2.4, and a ray aimed
		// exactly down a seam or through the vertex three meshes share can slip through the
		// sub-millimetre float gap between them. The player is a capsule and never can.
		const FVector From=Center+Up*(Planet->RadiusCm*1.03), To=Center+Up*(Planet->RadiusCm*0.97);
		if (!GetWorld()->SweepSingleByChannel(Hit,From,To,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(5.f),Query))
		{
			Finish(false,FString::Printf(TEXT("No ground under test direction site=%d collision_patches=%d triangles=%d"),
				Site,Planet->GetCollisionPatchCount(),Planet->GetCollisionTriangleCount()));
			return;
		}
		Ground=Hit.ImpactPoint;
		Character->StopJumping(); Move->StopMovementImmediately();
		Gravity->ClearPlanetBody();
		Character->SetActorLocation(Ground+Up*(Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+3.0),false,nullptr,ETeleportType::TeleportPhysics);
		Character->SetActorRotation(FAstraeonPlanetFrame::AlignToUp(FQuat::Identity,Up),ETeleportType::TeleportPhysics);
		Gravity->SetPlanetBody(Center,Planet->RadiusCm,float(Planet->GravityMS2));
		Move->SetMovementMode(MOVE_Falling);
		return;
	}
	Elapsed+=DeltaSeconds;
	const FVector Up=Gravity->GetUpVector();
	if (Elapsed>0.5 && Elapsed<2.5) Character->AddMovementInput(Character->GetActorForwardVector(),1.0);
	if (Elapsed>=1.0 && Elapsed<1.15) Character->Jump();
	if (Elapsed>=1.15) Character->StopJumping();
	if (Elapsed>1.0)
	{
		bSawFlight |= Move->IsFalling();
		MaxLiftCm=FMath::Max(MaxLiftCm,FVector::DotProduct(Character->GetActorLocation()-Ground,Directions[Site])-Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	}
	if (Elapsed>0.5)
	{
		if (FVector::DotProduct(Character->GetActorUpVector(),Up)<0.99)
		{ Finish(false,TEXT("Capsule lost radial orientation")); return; }
		const FVector CameraUp=PC->PlayerCameraManager->GetCameraRotation().Quaternion().GetUpVector();
		if (FVector::DotProduct(CameraUp,Up)<0.98)
		{ Finish(false,TEXT("Camera lost radial orientation")); return; }
	}
	if (Site==0 && Elapsed>2.9 && Elapsed-DeltaSeconds<=2.9)
		FScreenshotRequest::RequestScreenshot(TEXT("PlanetCardinalLab"),true,false);
}

void AAstraeonPlanetCardinalSmoke::Finish(bool bPassed,const FString& Reason)
{
	bDone=true;
	UE_LOG(LogTemp,Display,TEXT("PlanetCardinals: %s %s"),bPassed?TEXT("PASS"):TEXT("FAIL"),*Reason);
	FPlatformMisc::RequestExitWithStatus(false,bPassed?0:1);
}
