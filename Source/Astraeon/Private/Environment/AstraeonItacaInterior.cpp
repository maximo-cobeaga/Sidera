#include "Environment/AstraeonItacaInterior.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

bool AAstraeonItacaInterior::IsInsideFootprint(const FVector2D& XY, float MarginCm)
{
	return XY.X >= -180-MarginCm && XY.X <= 620+MarginCm && FMath::Abs(XY.Y) <= 300+MarginCm;
}

namespace AstraeonItacaHatch
{
	// Bisagra en la jamba de estribor del hueco de 1,3 m; la hoja gira hacia dentro.
	constexpr float HingeYCm = 65.0f;
	constexpr float OpenAngleDegrees = -95.0f;
	constexpr float SwingDegreesPerSecond = 90.0f;
}

float AAstraeonItacaInterior::GetDeckThicknessCm()
{
	return 12.0f;
}

float AAstraeonItacaInterior::GetHatchOpenAngleDegrees()
{
	return AstraeonItacaHatch::OpenAngleDegrees;
}

void AAstraeonItacaInterior::OpenHatch()
{
	if (bHatchOpen)
	{
		return;
	}

	bHatchOpen = true;
	// Deja de bloquear en cuanto empieza a abrirse: quedarse encerrado contra una hoja a
	// medio girar sería peor que la puerta siempre abierta que esto viene a corregir.
	if (HatchLeafCollision)
	{
		HatchLeafCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AAstraeonItacaInterior::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Target = bHatchOpen ? AstraeonItacaHatch::OpenAngleDegrees : 0.0f;
	if (FMath::IsNearlyEqual(HatchAngleDegrees, Target, 0.01f))
	{
		return;
	}

	HatchAngleDegrees = FMath::FInterpConstantTo(HatchAngleDegrees, Target, DeltaSeconds,
		AstraeonItacaHatch::SwingDegreesPerSecond);
	if (HatchHinge)
	{
		HatchHinge->SetRelativeRotation(FRotator(0.0f, HatchAngleDegrees, 0.0f));
	}
}

AAstraeonItacaInterior::AAstraeonItacaInterior()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RoomRoot"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Floor(TEXT("/Game/Astraeon/Art/Blockouts/Itaca/SM_Itaca_Floor_200_Blockout.SM_Itaca_Floor_200_Blockout"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Wall(TEXT("/Game/Astraeon/Art/Blockouts/Itaca/SM_Itaca_Wall_200_Blockout.SM_Itaca_Wall_200_Blockout"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Frame(TEXT("/Game/Astraeon/Art/Blockouts/Itaca/SM_Itaca_HatchFrame_Blockout.SM_Itaca_HatchFrame_Blockout"));
	auto Box = [this](const FString& Name, FVector Position, FVector Size)
	{
		auto* Collision = CreateDefaultSubobject<UBoxComponent>(*Name);
		Collision->SetupAttachment(RootComponent);
		Collision->SetRelativeLocation(Position);
		Collision->SetBoxExtent(Size * 0.5f);
		Collision->SetCollisionProfileName(TEXT("BlockAll"));
		Collision->SetGenerateOverlapEvents(false);
	};
	auto Mesh = [this](const FString& Name, UStaticMesh* Asset, FVector Position, FRotator Rotation, FVector Scale)
	{
		auto* Component = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Component->SetupAttachment(RootComponent);
		Component->SetStaticMesh(Asset);
		Component->SetRelativeLocation(Position);
		Component->SetRelativeRotation(Rotation);
		Component->SetRelativeScale3D(Scale);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Component;
	};
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Guide(TEXT("/Game/Astraeon/Art/ItacaTerrain/M_Itaca_Guide.M_Itaca_Guide"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Trim(TEXT("/Game/Astraeon/Art/ItacaTerrain/M_Itaca_Trim.M_Itaca_Trim"));
	auto Detail = [&](const FString& Name, FVector Position, FVector Size, UMaterialInterface* Material)
	{
		auto* Component = Mesh(Name, Cube.Object, Position, FRotator::ZeroRotator, Size / 100.0f);
		Component->SetMaterial(0, Material);
		Component->SetCastShadow(false);
	};
	// Thin presentation-only ribs and light strips stay outside the walkable aisle.
	for (int32 Side : {-1, 1})
	{
		Detail(FString::Printf(TEXT("Skirting_%d"), Side), FVector(220, Side*286, 18), FVector(780, 8, 36), Trim.Object);
		Detail(FString::Printf(TEXT("GuideRail_%d"), Side), FVector(220, Side*280, 40), FVector(770, 3, 3), Guide.Object);
		Detail(FString::Printf(TEXT("CeilingStrip_%d"), Side), FVector(220, Side*190, 274), FVector(740, 8, 3), Guide.Object);
		for (int32 Rib=0; Rib<4; ++Rib)
			Detail(FString::Printf(TEXT("ServiceRib_%d_%d"), Side, Rib), FVector(-175+Rib*200, Side*287, 140), FVector(8, 10, 270), Trim.Object);
		Detail(FString::Printf(TEXT("HatchGuide_%d"), Side), FVector(605, Side*81, 120), FVector(3, 4, 192), Guide.Object);
	}
	for (int32 X=0; X<4; ++X)
	{
		const float PositionX = -80+200*X;
		for (int32 Y=0; Y<3; ++Y)
		{
			const FString Id = FString::Printf(TEXT("%d_%d"), X,Y);
			Mesh(TEXT("Floor_")+Id, Floor.Object, FVector(PositionX,-200+200*Y,-12), FRotator::ZeroRotator, FVector::OneVector);
			Mesh(TEXT("Ceiling_")+Id, Floor.Object, FVector(PositionX,-200+200*Y,280), FRotator::ZeroRotator, FVector::OneVector);
		}
		for (int32 Side : {-1,1})
			Mesh(FString::Printf(TEXT("Side_%d_%d"), X,Side), Wall.Object, FVector(PositionX,Side*300,0), FRotator::ZeroRotator, FVector::OneVector);
	}
	for (int32 Y=0; Y<3; ++Y)
		Mesh(FString::Printf(TEXT("Back_%d"),Y), Wall.Object, FVector(-180,-200+200*Y,0), FRotator(0,90,0), FVector::OneVector);
	for (int32 Side : {-1,1})
	{
		Mesh(FString::Printf(TEXT("Front_%d"),Side), Wall.Object, FVector(620,Side*195,0), FRotator(0,90,0), FVector(1.05,1,1));
		Box(FString::Printf(TEXT("FrontCollision_%d"),Side), FVector(620,Side*195,140), FVector(24,210,280));
		Box(FString::Printf(TEXT("JambCollision_%d"),Side), FVector(620,Side*77.5,110), FVector(24,25,220));
	}
	Mesh(TEXT("HatchFrame"), Frame.Object, FVector(620,0,0), FRotator(0,90,0), FVector::OneVector);
	// Hoja de escotilla sobre una bisagra real. La hoja mide 1,3 m en su X local, así que
	// gira 90 grados para cubrir el hueco, que en la estancia corre a lo largo de Y.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Leaf(TEXT("/Game/Astraeon/Art/Blockouts/Itaca/SM_Itaca_HatchLeaf_Blockout.SM_Itaca_HatchLeaf_Blockout"));
	HatchHinge = CreateDefaultSubobject<USceneComponent>(TEXT("HatchHinge"));
	HatchHinge->SetupAttachment(RootComponent);
	HatchHinge->SetRelativeLocation(FVector(620, AstraeonItacaHatch::HingeYCm, 0));
	HatchLeaf = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HatchLeaf"));
	HatchLeaf->SetupAttachment(HatchHinge);
	HatchLeaf->SetStaticMesh(Leaf.Object);
	HatchLeaf->SetRelativeLocation(FVector(0, -AstraeonItacaHatch::HingeYCm, 0));
	HatchLeaf->SetRelativeRotation(FRotator(0, 90, 0));
	HatchLeaf->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HatchLeafCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("HatchLeafCollision"));
	HatchLeafCollision->SetupAttachment(HatchHinge);
	HatchLeafCollision->SetRelativeLocation(FVector(0, -AstraeonItacaHatch::HingeYCm, 110));
	HatchLeafCollision->SetBoxExtent(FVector(6, 65, 110));
	HatchLeafCollision->SetCollisionProfileName(TEXT("BlockAll"));
	// Ignora el canal de visibilidad a propósito: la hoja frena a la cápsula, pero el trazo
	// de interacción tiene que seguir alcanzando el marcador ESCOTILLA que hay detrás.
	HatchLeafCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	HatchLeafCollision->SetGenerateOverlapEvents(false);
	Mesh(TEXT("AboveHatch"), Wall.Object, FVector(620,0,250), FRotator(0,90,0), FVector(0.9,1,30.0/280.0));
	Box(TEXT("HeaderCollision"), FVector(620,0,250), FVector(24,180,60));
	Box(TEXT("FloorCollision"), FVector(220,0,-6), FVector(800,600,12));
	Box(TEXT("CeilingCollision"), FVector(220,0,286), FVector(800,600,12));
	Box(TEXT("BackCollision"), FVector(-180,0,140), FVector(12,600,280));
	Box(TEXT("NorthCollision"), FVector(220,300,140), FVector(800,12,280));
	Box(TEXT("SouthCollision"), FVector(220,-300,140), FVector(800,12,280));
	for (int32 Index=0; Index<2; ++Index)
	{
		auto* Light = CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("CabinLight_%d"), Index));
		Light->SetupAttachment(RootComponent);
		Light->SetRelativeLocation(FVector(50+Index*350,0,240));
		Light->SetIntensity(1800);
		Light->SetAttenuationRadius(700);
		Light->SetLightColor(FLinearColor(0.75,0.87,1.0));
		Light->SetCastShadows(false);
	}
	auto* Note = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HumanNote"));
	Note->SetupAttachment(RootComponent);
	Note->SetRelativeLocation(FVector(-170,0,160));
	Note->SetWorldSize(12);
	Note->SetHorizontalAlignment(EHTA_Center);
	Note->SetText(FText::FromString(TEXT("ITACA / ARCHIVO PERSONAL\nIngenieria - Cuaderno de viaje\nMedir antes de abrir.\nVolver con algo que comprender.")));
	Note->SetTextRenderColor(FColor(210,220,215));
}
