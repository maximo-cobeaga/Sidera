#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstraeonItacaInputSmoke.generated.h"

/** Development smoke drives key events across frames, never calls Interact directly. */
UCLASS()
class ASTRAEON_API AAstraeonItacaInputSmoke : public AActor
{

	GENERATED_BODY()
public:
	AAstraeonItacaInputSmoke();
	virtual void Tick(float DeltaSeconds) override;
private:
	int32 Stage = 0;
	float Elapsed = 0;
	void Finish(bool bPassed, const FString& Reason);
};
