#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstraeonPlanetCardinalSmoke.generated.h"

UCLASS()
class AAstraeonPlanetCardinalSmoke : public AActor
{
	GENERATED_BODY()
public:
	AAstraeonPlanetCardinalSmoke();
	virtual void Tick(float DeltaSeconds) override;
private:
	TArray<FVector> Directions;
	int32 Site=-1;
	float Elapsed=0;
	float Startup=0;
	bool bStarted=false;
	bool bSawFlight=false;
	bool bDone=false;
	double MaxLiftCm=0;
	FVector Ground=FVector::ZeroVector;
	void Finish(bool bPassed,const FString& Reason);
};
