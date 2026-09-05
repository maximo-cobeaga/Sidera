#pragma once

#include "CoreMinimal.h"
#include "AstraeonEnvironmentTypes.generated.h"

USTRUCT(BlueprintType)
struct ASTRAEON_API FAstraeonAtmosphereFractions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Science")
	float Oxygen = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Science")
	float Nitrogen = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Science")
	float CarbonDioxide = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Science")
	float Argon = 0.0f;
};

USTRUCT(BlueprintType)
struct ASTRAEON_API FAstraeonEnvironmentalSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Science")
	int32 WorldSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Science")
	int32 GeneratorVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Science")
	float GravityMS2 = 9.81f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Science")
	float TemperatureKelvin = 293.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Science")
	float PressureKPa = 101.325f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Science")
	FAstraeonAtmosphereFractions Atmosphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Science")
	bool bBreathable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Astraeon|Science", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EnvironmentalRisk01 = 0.0f;
};
