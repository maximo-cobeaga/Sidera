#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstraeonLocalFrameSubsystem.generated.h"

// Local reference frame for planetary scale. When the view drifts farther than a threshold from
// the world origin, the origin moves to it: physics, animation and near rendering keep working
// with small numbers while the planet is hundreds of kilometres wide.
//
// Persistent state never depends on this: it is stored as body + direction + altitude. Anything
// that caches an absolute world position must shift it in `ApplyWorldOffset`.
//
// Chaos does not shift its scene natively (`SupportsOriginShifting` is false in UE 5.7): the
// engine teleports every physics body instead. On a planet that is the collision ring and a few
// characters, so the cost is bounded; it is measured in TL_14.
UCLASS()
class ASTRAEON_API UAstraeonLocalFrameSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	// 5 km: float spacing there is ~0.5 mm, far below anything a player can see or feel.
	static constexpr double DefaultShiftThresholdCm = 500000.0;

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override;

	bool IsEnabled() const { return bEnabled; }
	double GetShiftThresholdCm() const { return ShiftThresholdCm; }
	int32 GetShiftCount() const { return ShiftCount; }
	// Where the current world origin sits in the coordinates the level was authored in.
	FIntVector GetOriginCm() const;
	// Farthest the view has been from the world origin at the end of a frame.
	double GetMaxViewDistanceCm() const { return MaxViewDistanceCm; }

private:
	bool bEnabled = false;
	double ShiftThresholdCm = DefaultShiftThresholdCm;
	int32 ShiftCount = 0;
	double MaxViewDistanceCm = 0.0;
};
