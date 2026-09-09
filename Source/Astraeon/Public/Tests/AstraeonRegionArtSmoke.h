#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstraeonRegionArtSmoke.generated.h"

class AAstraeonRegionMarker;

/** Opt-in Development smoke: inspect real seeded markers and exercise scanner/collection. */
UCLASS()
class ASTRAEON_API AAstraeonRegionArtSmoke : public AActor
{
	GENERATED_BODY()
public:
	AAstraeonRegionArtSmoke();
	virtual void Tick(float DeltaSeconds) override;
private:
	int32 Stage = 0;
	float Elapsed = 0;
	FName CurrentId;
	bool bCurrentResource = false;
	bool bCurrentToolGated = false;
	UPROPERTY()
	TArray<TObjectPtr<AAstraeonRegionMarker>> Markers;
	void Finish(bool bPassed, const FString& Reason);
};
