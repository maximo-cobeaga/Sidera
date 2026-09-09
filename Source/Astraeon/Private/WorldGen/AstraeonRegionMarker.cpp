#include "WorldGen/AstraeonRegionMarker.h"

#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"

AAstraeonRegionMarker::AAstraeonRegionMarker()
{
	PrimaryActorTick.bCanEverTick = false;

	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
	RootComponent = MarkerMesh;
	MarkerMesh->SetCollisionProfileName(TEXT("BlockAll"));
	ConsoleCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("ConsoleCollision"));
	ConsoleCollision->SetupAttachment(RootComponent);
	ConsoleCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ConsoleCollision->SetGenerateOverlapEvents(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Console(TEXT("/Game/Astraeon/Art/Blockouts/Itaca/SM_Itaca_ARGOSConsole_Blockout.SM_Itaca_ARGOSConsole_Blockout"));
	ItacaConsoleMesh = Console.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PilotConsole(TEXT("/Game/Astraeon/Art/Blockouts/Itaca/SM_Itaca_PilotConsole_Blockout.SM_Itaca_PilotConsole_Blockout"));
	ItacaPilotConsoleMesh = PilotConsole.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Fabricator(TEXT("/Game/Astraeon/Art/Blockouts/Itaca/SM_Itaca_Fabricator_Blockout.SM_Itaca_Fabricator_Blockout"));
	ItacaFabricatorMesh = Fabricator.Object;
	RegionArt = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RegionArt"));
	RegionArt->SetupAttachment(RootComponent);
	RegionArt->SetAbsolute(false, false, true);
	RegionArt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RegionArt->SetGenerateOverlapEvents(false);
	const TPair<FName, FString> ArtAssets[] = {
		{TEXT("silicate_fiber"), TEXT("SM_Resource_SilicateFiber_Blockout")},
		{TEXT("ferrite_nodule"), TEXT("SM_Resource_FerriteNodule_Blockout")},
		{TEXT("seed_resource"), TEXT("SM_Resource_SeedCrystal_Blockout")},
		{TEXT("cryo_ferrite_vein"), TEXT("SM_Resource_CryoVein_Blockout")},
		{TEXT("resonant_quartz_vein"), TEXT("SM_Resource_QuartzVein_Blockout")},
		{TEXT("signal_source"), TEXT("SM_Signal_Source_Blockout")},
		{TEXT("minor_geologic_anomaly"), TEXT("SM_Anomaly_Strata_Blockout")}
	};
	for (const auto& Asset : ArtAssets)
	{
		const FString Path = FString::Printf(TEXT("/Game/Astraeon/Art/Blockouts/Region/%s.%s"), *Asset.Value, *Asset.Value);
		ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(*Path);
		if (Mesh.Succeeded()) RegionArtMeshes.Add(Asset.Key, Mesh.Object);
	}

	MarkerLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("MarkerLabel"));
	MarkerLabel->SetupAttachment(RootComponent);
	MarkerLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	MarkerLabel->SetHorizontalAlignment(EHTA_Center);
	MarkerLabel->SetVerticalAlignment(EVRTA_TextCenter);
	MarkerLabel->SetWorldSize(42.0f);
	MarkerLabel->SetTextRenderColor(FColor::White);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		MarkerMesh->SetStaticMesh(CubeMesh.Object);
	}
}

bool AAstraeonRegionMarker::IsItacaStation(FName ActorId)
{
	return ActorId == TEXT("itaca_argos_console")
		|| ActorId == TEXT("itaca_pilot_console")
		|| ActorId == TEXT("itaca_fabricator");
}

bool AAstraeonRegionMarker::BelongsToItacaInterior(FName ActorId)
{
	return IsItacaStation(ActorId) || ActorId == TEXT("itaca_surface_hatch");
}

void AAstraeonRegionMarker::ApplySpec(const FAstraeonRegionActorSpec& Spec)
{
	MarkerId = Spec.ActorId;
	MarkerKind = Spec.Kind;
	RequiredToolId = Spec.RequiredToolId;
	SetActorLocation(Spec.LocationCm);
	SetActorScale3D(Spec.Scale);
	MarkerMesh->SetVisibility(true);
	MarkerMesh->SetCastShadow(true);
	RegionArt->SetStaticMesh(nullptr);
	RegionArt->SetVisibility(false);
	ConsoleCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (MarkerId == TEXT("itaca_argos_console") && ItacaConsoleMesh)
	{
		SetActorLocation(Spec.LocationCm);
		SetActorScale3D(FVector::OneVector);
		MarkerMesh->SetStaticMesh(ItacaConsoleMesh);
		MarkerMesh->SetMaterial(0, ItacaConsoleMesh->GetMaterial(0));
		MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ConsoleCollision->SetRelativeLocation(FVector(0,0,60));
		ConsoleCollision->SetBoxExtent(FVector(40,30,60));
		ConsoleCollision->SetCollisionProfileName(TEXT("BlockAll"));
		ConsoleCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MarkerLabel->SetRelativeLocation(FVector(-45,0,145));
		MarkerLabel->SetRelativeRotation(FRotator(0,180,0));
		MarkerLabel->SetWorldSize(20);
	}
	else if ((MarkerId == TEXT("itaca_pilot_console") && ItacaPilotConsoleMesh)
		|| (MarkerId == TEXT("itaca_fabricator") && ItacaFabricatorMesh))
	{
		// Mismo tratamiento que ARGOS: la malla se apoya en el suelo a escala 1 y la caja
		// de colisión reproduce su silueta, en vez de escalar un cubo del motor.
		const bool bPilot = MarkerId == TEXT("itaca_pilot_console");
		UStaticMesh* StationMesh = bPilot ? ItacaPilotConsoleMesh : ItacaFabricatorMesh;
		const FVector HalfSize = bPilot ? FVector(47.5f, 35.0f, 62.5f) : FVector(55.0f, 37.5f, 57.5f);
		SetActorLocation(Spec.LocationCm);
		SetActorScale3D(FVector::OneVector);
		MarkerMesh->SetStaticMesh(StationMesh);
		MarkerMesh->SetMaterial(0, StationMesh->GetMaterial(0));
		MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ConsoleCollision->SetRelativeLocation(FVector(0, 0, HalfSize.Z));
		ConsoleCollision->SetBoxExtent(HalfSize);
		ConsoleCollision->SetCollisionProfileName(TEXT("BlockAll"));
		ConsoleCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MarkerLabel->SetRelativeLocation(FVector(-45, 0, HalfSize.Z * 2.0f + 25.0f));
		MarkerLabel->SetRelativeRotation(FRotator(0, 180, 0));
		MarkerLabel->SetWorldSize(20);
	}
	else if (MarkerId == TEXT("itaca_surface_hatch"))
	{
		// Invisible interaction target fills the opening but never blocks the capsule.
		SetActorLocation(FVector(Spec.LocationCm.X, Spec.LocationCm.Y, Spec.LocationCm.Z + 110.0f));
		SetActorScale3D(FVector(0.24,1.3,2.2));
		MarkerMesh->SetVisibility(false);
		MarkerMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		MarkerMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		MarkerMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		MarkerLabel->SetAbsolute(false, false, true);
		MarkerLabel->SetRelativeLocation(FVector(-70,0,65));
		MarkerLabel->SetRelativeRotation(FRotator(0,180,0));
		MarkerLabel->SetWorldSize(20);
	}

	FName ArtId = MarkerId;
	if (Spec.Kind == EAstraeonRegionActorKind::Resource &&
		(ArtId == TEXT("cryosalt_shard") || ArtId == TEXT("vesicle_resin") ||
		 ArtId == TEXT("basalt_glass") || ArtId == TEXT("magnetite_thread")))
	{
		ArtId = TEXT("seed_resource");
	}
	if (const TObjectPtr<UStaticMesh>* ArtMesh = RegionArtMeshes.Find(ArtId))
	{
		// Keep the existing root proxy for interaction/collision and stable actor/save transforms.
		// Imported art uses metres -> centimetres, unit world scale and a ground pivot.
		RegionArt->SetStaticMesh(*ArtMesh);
		RegionArt->SetWorldScale3D(FVector::OneVector);
		constexpr float RegionalMarkerCenterHeightCm = 60.0f;
		RegionArt->SetRelativeLocation(FVector(0, 0, -RegionalMarkerCenterHeightCm / FMath::Max(Spec.Scale.Z, KINDA_SMALL_NUMBER)));
		RegionArt->SetVisibility(true);
		MarkerMesh->SetVisibility(false);
		MarkerMesh->SetCastShadow(false);
	}
	const FColor MarkerColor = BuildMarkerColor(Spec.ActorId, Spec.Kind);
	// Teñir el material de una estación borraría la paleta del kit de Ítaca.
	if (!IsItacaStation(MarkerId))
		MarkerMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(MarkerColor.R / 255.0f, MarkerColor.G / 255.0f, MarkerColor.B / 255.0f));
	MarkerLabel->SetText(BuildMarkerLabel(Spec.ActorId, Spec.Kind));
	MarkerLabel->SetTextRenderColor(MarkerColor);
#if WITH_EDITOR
	SetActorLabel(FString::Printf(TEXT("Region_%s_%s"), Spec.Kind == EAstraeonRegionActorKind::Resource ? TEXT("Resource") : TEXT("POI"), *Spec.ActorId.ToString()));
#endif
}

FText AAstraeonRegionMarker::BuildMarkerLabel(FName ActorId, EAstraeonRegionActorKind Kind)
{
	if (ActorId == TEXT("itaca_argos_console"))
	{
		return FText::FromString(TEXT("ARGOS"));
	}

	if (ActorId == TEXT("itaca_surface_hatch"))
	{
		return FText::FromString(TEXT("ESCOTILLA"));
	}

	if (ActorId == TEXT("itaca_pilot_console"))
	{
		return FText::FromString(TEXT("PILOTAJE"));
	}

	if (ActorId == TEXT("itaca_fabricator"))
	{
		return FText::FromString(TEXT("FABRICACIÓN"));
	}

	if (ActorId == TEXT("signal_source"))
	{
		return FText::FromString(TEXT("SEÑAL"));
	}

	if (ActorId == TEXT("minor_geologic_anomaly"))
	{
		return FText::FromString(TEXT("ANOMALÍA"));
	}

	if (Kind == EAstraeonRegionActorKind::Resource)
	{
		// Las vetas profundas llevan su propio rótulo: el jugador tiene que poder decidir
		// si vale la pena caminar hasta allá antes de tener el taladro.
		if (ActorId.ToString().EndsWith(TEXT("_vein")))
		{
			return FText::FromString(FString::Printf(TEXT("VETA PROFUNDA\n%s"), *ActorId.ToString()));
		}
		return FText::FromString(FString::Printf(TEXT("RECURSO\n%s"), *ActorId.ToString()));
	}

	return FText::FromName(ActorId);
}

FColor AAstraeonRegionMarker::BuildMarkerColor(FName ActorId, EAstraeonRegionActorKind Kind)
{
	if (ActorId == TEXT("itaca_argos_console"))
	{
		return FColor(80, 180, 255);
	}

	if (ActorId == TEXT("itaca_surface_hatch"))
	{
		return FColor(120, 255, 220);
	}

	if (ActorId == TEXT("itaca_pilot_console"))
	{
		return FColor(255, 210, 90);
	}

	if (ActorId == TEXT("itaca_fabricator"))
	{
		return FColor(170, 140, 255);
	}

	if (ActorId == TEXT("signal_source"))
	{
		return FColor(255, 80, 220);
	}

	if (ActorId == TEXT("minor_geologic_anomaly"))
	{
		return FColor(255, 180, 40);
	}

	if (Kind == EAstraeonRegionActorKind::Resource)
	{
		return ActorId.ToString().EndsWith(TEXT("_vein"))
			? FColor(90, 200, 255)
			: FColor(80, 255, 120);
	}

	return FColor::White;
}
