#include "Tests/AstraeonPlanetPatchLODSmoke.h"
#include "AstraeonPlayerCharacter.h"
#include "AstraeonPlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformMisc.h"
#include "Planet/AstraeonPlanetRuntime.h"
#include "Planet/Gravity/AstraeonPlanetGravityComponent.h"
#include "Planet/LOD/AstraeonPlanetLODManager.h"
#include "UnrealClient.h"

namespace
{
	constexpr double LowAboveGroundCm = 10000.0; // Below 100 m the finest level is expected.
	constexpr float FirstCoverTimeoutSeconds = 60.f;
	struct FShot { float Seconds; const TCHAR* Name; };
	const FShot Shots[] = {{30.f, TEXT("PatchLOD_Corner")}, {100.f, TEXT("PatchLOD_Orbit")}, {150.f, TEXT("PatchLOD_Edge")}};
}

AAstraeonPlanetPatchLODSmoke::AAstraeonPlanetPatchLODSmoke()
{
	PrimaryActorTick.bCanEverTick = true;
	// After the runtime and the camera manager: audit what this frame will show.
	PrimaryActorTick.TickGroup = TG_PostPhysics;
}

void AAstraeonPlanetPatchLODSmoke::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDone) return;
	Startup += DeltaSeconds;
	auto* PC = Cast<AAstraeonPlayerController>(GetWorld()->GetFirstPlayerController());
	auto* Character = PC ? Cast<AAstraeonPlayerCharacter>(PC->GetPawn()) : nullptr;
	auto* Planet = AAstraeonPlanetRuntime::FindActive(GetWorld());
	if (!PC || !Character || !Planet)
	{
		if (Startup > 10.f) Finish(false, TEXT("Missing player or runtime"));
		return;
	}
	if (!Planet->IsUsingPatches()) { Finish(false, TEXT("Runtime is not rendering patches")); return; }
	if (!bStarted) { PC->StartSelectedNewGame(); bStarted = true; return; }
	if (!bFlying)
	{
		// Nothing stands on this planet: freeze the pawn and look through a scripted camera.
		Character->GetCharacterMovement()->DisableMovement();
		if (auto* Gravity = Character->FindComponentByClass<UAstraeonPlanetGravityComponent>()) Gravity->ClearPlanetBody();
		Character->SetActorEnableCollision(false);
		Character->SetActorHiddenInGame(true);
		Camera = GetWorld()->SpawnActor<ACameraActor>();
		Camera->GetCameraComponent()->SetConstraintAspectRatio(false);
		Camera->GetCameraComponent()->SetFieldOfView(90.f);
		PC->SetViewTarget(Camera);
		const double R = Planet->RadiusCm;
		const auto Along = [R](const FVector& From, const FVector& Tangent, double ArcCm)
		{
			return From * FMath::Cos(ArcCm / R) + Tangent * FMath::Sin(ArcCm / R);
		};
		// Corner of +X/+Y/+Z crossed along (1,-1,0); later the +X/+Z edge followed along +Y.
		const FVector Corner = FVector(1, 1, 1).GetSafeNormal(), CornerPath = FVector(1, -1, 0).GetSafeNormal();
		const FVector Edge = FVector(1, 0, 1).GetSafeNormal(), EdgePath(0, 1, 0);
		Route = {
			{0.f, Along(Corner, CornerPath, -200000.0), 2000.0},
			{40.f, Corner, 2000.0},
			{60.f, Along(Corner, CornerPath, 100000.0), 2000.0},
			{80.f, Along(Corner, CornerPath, 150000.0), 500000.0},
			{100.f, Along(Corner, CornerPath, 300000.0), 30000000.0},
			{120.f, Edge, 500000.0},
			{135.f, Along(Edge, EdgePath, -100000.0), 2000.0},
			{175.f, Along(Edge, EdgePath, 100000.0), 2000.0},
			{185.f, Along(Edge, EdgePath, 100000.0), 2000.0},
		};
		Place(0.f, DeltaSeconds);
		bFlying = true;
		return;
	}
	const auto* Patches = Planet->GetPatchManager();
	if (Patches->GetVisibleCount() == 0)
	{
		// The route clock starts with the first cover; until then the camera waits at the start.
		Place(0.f, DeltaSeconds);
		if (Startup > FirstCoverTimeoutSeconds) Finish(false, TEXT("First cover never appeared"));
		return;
	}
	if (Elapsed == 0.f)
		UE_LOG(LogTemp, Display, TEXT("PlanetPatchLOD: first cover after %.2f s visible=%d"), Startup, Patches->GetVisibleCount());
	Elapsed += DeltaSeconds;
	Place(Elapsed, DeltaSeconds);
	Audit();
	if (bDone) return;
	if (NextShot < UE_ARRAY_COUNT(Shots) && Elapsed >= Shots[NextShot].Seconds)
		FScreenshotRequest::RequestScreenshot(Shots[NextShot++].Name, true, false);
	if (Elapsed < Route.Last().Seconds) return;

	const auto& S = Patches->GetStats();
	UE_LOG(LogTemp, Display, TEXT("PlanetPatchLOD: frames=%d auditorias=%d visibles_max=%d componentes_max=%d delta_max=%d frames_delta2=%d (%.2f%%)"),
		Frames, Audits, MaxVisible, MaxComponents, MaxDelta, FramesDeltaTwo, Frames > 0 ? 100.0 * FramesDeltaTwo / Frames : 0.0);
	UE_LOG(LogTemp, Display, TEXT("PlanetPatchLOD: bajo 100 m frames=%d con patch mas fino=%d (%.1f%%) peor_lod=%d | velocidad max=%.0f m/s"),
		LowFrames, LowFramesFinest, LowFrames > 0 ? 100.0 * LowFramesFinest / LowFrames : 0.0,
		WorstLowLod == MAX_int32 ? -1 : WorstLowLod, MaxSpeedCmS / 100.0);
	UE_LOG(LogTemp, Display, TEXT("PlanetPatchLOD: pedidos=%d confirmados=%d relevos=%d obsoletos=%d cancelados=%d retirados=%d fallos=%d"),
		S.Requests, S.Commits, S.Relays, S.StaleDrops, S.Cancels, S.Removals, S.Failures);
	UE_LOG(LogTemp, Display, TEXT("PlanetPatchLOD: trabajo de patches en game thread media=%.3f ms max=%.2f ms frames>%.0f ms=%d"),
		Planet->GetMeanPatchWorkMs(), Planet->GetMaxPatchWorkMs(), AAstraeonPlanetRuntime::PatchWorkBudgetMs,
		Planet->GetPatchWorkFramesOverBudget());
	FString Failures;
	if (!Patches->IsSettled()) Failures += TEXT("no asienta con la camara quieta; ");
	if (S.Failures > 0) Failures += FString::Printf(TEXT("%d patches fallidos; "), S.Failures);
	if (MaxComponents > 2 * FAstraeonPlanetLODSettings::MaxAllowedPatches)
		Failures += FString::Printf(TEXT("%d componentes, sin cota; "), MaxComponents);
	Finish(Failures.IsEmpty(), Failures.IsEmpty() ? TEXT("OK") : Failures);
}

void AAstraeonPlanetPatchLODSmoke::Place(float Seconds, float DeltaSeconds)
{
	const auto* Planet = AAstraeonPlanetRuntime::FindActive(GetWorld());
	int32 I = 0;
	while (I + 2 < Route.Num() && Seconds >= Route[I + 1].Seconds) ++I;
	const FKey& A = Route[I];
	const FKey& B = Route[I + 1];
	const double Alpha = FMath::Clamp((Seconds - A.Seconds) / FMath::Max(B.Seconds - A.Seconds, UE_KINDA_SMALL_NUMBER), 0.0, 1.0);
	const FVector From = A.Direction.GetSafeNormal(), To = B.Direction.GetSafeNormal();
	const FVector Up = FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(From, To), Alpha).RotateVector(From);
	// Geometric altitude: 20 m to 300 km spends time at every order of magnitude.
	const double AboveGround = FMath::Exp(FMath::Lerp(FMath::Loge(A.AboveGroundCm), FMath::Loge(B.AboveGroundCm), Alpha));
	const FVector Position = Planet->GetSurfacePointCm(Up, AboveGround);
	// Motion relative to the planet: local frame shifts move the world under the camera.
	const FVector BodyPosition = Position - Planet->GetActorLocation();

	FVector Motion = BodyPosition - LastPosition;
	if (DeltaSeconds > 0.f && !LastPosition.IsZero()) MaxSpeedCmS = FMath::Max(MaxSpeedCmS, Motion.Size() / DeltaSeconds);
	Motion -= (Motion | Up) * Up;
	if (Motion.SizeSquared() > 1.0) LastForward = Motion.GetSafeNormal();
	else if (LastForward.IsZero()) LastForward = (To - (To | Up) * Up).GetSafeNormal();
	if (LastForward.IsNearlyZero()) LastForward = FVector::CrossProduct(Up, FVector::UpVector).GetSafeNormal();
	LastForward = (LastForward - (LastForward | Up) * Up).GetSafeNormal();
	LastPosition = BodyPosition;
	// Near the ground look ahead at the horizon; from orbit look down at the planet.
	const double Height = FMath::Clamp(FMath::Loge(AboveGround / 2000.0) / FMath::Loge(30000000.0 / 2000.0), 0.0, 1.0);
	const double Pitch = FMath::DegreesToRadians(FMath::Lerp(12.0, 85.0, Height));
	const FVector Look = LastForward * FMath::Cos(Pitch) - Up * FMath::Sin(Pitch);
	Camera->SetActorLocationAndRotation(Position, FRotationMatrix::MakeFromXZ(Look, Up).Rotator());

	if (AboveGround >= LowAboveGroundCm || !Planet->GetPatchManager() || Planet->GetPatchManager()->GetVisibleCount() == 0) return;
	// Which level is on screen under the camera: walk up from the finest until a visible patch.
	const auto Definition = Planet->GetDefinition();
	const uint8 Finest = FAstraeonPlanetLODManager::FinestAllowedLod(Definition, {});
	++LowFrames;
	for (int32 Lod = Finest; Lod >= 0; --Lod)
	{
		FAstraeonPlanetPatchAddress Under;
		if (!FAstraeonPlanetPatchAddress::TryFromDirection(Definition.BodyId, Up, uint8(Lod), Under)) break;
		if (!Planet->GetPatchManager()->IsVisible(Under)) continue;
		LowFramesFinest += Lod == Finest;
		WorstLowLod = FMath::Min(WorstLowLod, Lod);
		break;
	}
}

void AAstraeonPlanetPatchLODSmoke::Audit()
{
	const auto* Planet = AAstraeonPlanetRuntime::FindActive(GetWorld());
	const auto* Patches = Planet->GetPatchManager();
	++Frames;
	MaxComponents = FMath::Max(MaxComponents, Planet->GetPatchComponentCount());
	// The screen only changes on a relay, so the full audit runs then and not every frame:
	// otherwise the smoke's own cost would pollute the frame times it reports.
	if (Patches->GetStats().Relays != LastRelays)
	{
		LastRelays = Patches->GetStats().Relays;
		const auto Visible = Patches->GetVisible();
		FString Reason;
		if (!FAstraeonPlanetLODManager::ValidatePartition(Visible, &Reason))
		{
			Finish(false, FString::Printf(TEXT("pantalla con agujero o solape en t=%.2f s: %s"), Elapsed, *Reason));
			return;
		}
		++Audits;
		MaxVisible = FMath::Max(MaxVisible, Visible.Num());
		CurrentDelta = FAstraeonPlanetLODManager::MaxNeighborLodDelta(Visible);
		MaxDelta = FMath::Max(MaxDelta, CurrentDelta);
	}
	FramesDeltaTwo += CurrentDelta >= 2;
}

void AAstraeonPlanetPatchLODSmoke::Finish(bool bPassed, const FString& Reason)
{
	bDone = true;
	UE_LOG(LogTemp, Display, TEXT("PlanetPatchLOD: RESULTADO=%s %s"), bPassed ? TEXT("OK") : TEXT("FALLO"), *Reason);
	FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
}
