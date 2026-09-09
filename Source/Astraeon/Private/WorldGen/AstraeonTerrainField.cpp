#include "WorldGen/AstraeonTerrainField.h"

#include "Environment/AstraeonItacaInterior.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace AstraeonTerrain
{
	// El terreno es de bloques: el desnivel entre dos tiles vecinos es un escalón vertical,
	// no una rampa. De ahí que el relieve se construya en DOS CAPAS con propósitos
	// distintos, en vez de una sola que intente ser paisaje y piso a la vez:
	//
	//   • Suelo ondulado: amplitud baja repartida entre decenas de tiles, de modo que cada
	//     escalón queda por debajo del límite de paso. Es el piso, y siempre se camina.
	//   • Montañas: formaciones aisladas y altas que NO se pretende escalar. Son barreras y
	//     puntos de referencia, se rodean a pie y se sobrevuelan con Ítaca.
	//
	// Mezclar ambas en una única curva fue el error anterior: subir la altura para tener
	// paisaje volvía el suelo intransitable, y bajarla para poder caminar dejaba el mundo
	// plano. Separadas, cada una puede ir a su escala.
	constexpr float TileSizeCm = 700.0f;

	// Separación de la malla continua. Validar el tránsito a esta misma resolución mide lo
	// que el jugador pisa: entre dos vértices la malla es un plano, no la curva del ruido.
	constexpr float SurfaceSpacingCm = 400.0f;
	constexpr float FieldRadiusCm = 32000.0f;
	constexpr float MaxWalkableStepCm = 45.0f;

	// Capa de suelo. Amplitud contenida a propósito: es piso, no paisaje. El drama lo
	// aportan las montañas, que no se pisan.
	constexpr float GroundMaxHeightCm = 260.0f;
	constexpr float GroundCoarseCellCm = 46000.0f;
	constexpr float GroundFineCellCm = 19000.0f;

	// Capa de montañas. El umbral alto las vuelve escasas: son excepciones en el paisaje.
	constexpr float MountainMaxHeightCm = 5200.0f;
	constexpr float MountainCellCm = 72000.0f;
	constexpr float MountainThreshold = 0.62f;

	// Aplanar el suelo sólo tiene sentido bajo Ítaca, que es una estancia rígida de 8 × 6 m
	// y no puede seguir el relieve. El llano nivela contra la altura del suelo LOCAL, no
	// contra Z=0: por eso puede ser chico. Cavar hasta cero exigía un radio enorme para que
	// la rampa fuese caminable, y dejaba un cráter alrededor de la nave.
	//
	// El resto de los marcadores NO aplana nada: se apoyan sobre el terreno consultando la
	// altura, que es para lo que la función de altura es pura y consultable.
	constexpr float FlatSpotRadiusCm = 2600.0f;

	// Las montañas se suprimen de golpe cerca del juego. Un corte brusco es aceptable
	// porque una montaña ya es un muro: se lee como pared de roca, no como error.
	constexpr float MountainClearanceCm = 9000.0f;

	// Por debajo de esto no se instancia el bloque. Debe quedar bajo el límite de paso: si
	// no, el primer bloque de cada loma es un escalón imposible contra el suelo desnudo.
	constexpr float MinimumTileHeightCm = 20.0f;

	float HashToUnitFloat(int32 X, int32 Y, int32 Seed)
	{
		uint32 Hash = static_cast<uint32>(X) * 374761393u
			+ static_cast<uint32>(Y) * 668265263u
			+ static_cast<uint32>(Seed) * 2246822519u;
		Hash = (Hash ^ (Hash >> 13)) * 1274126177u;
		Hash ^= Hash >> 16;
		return static_cast<float>(Hash & 0xFFFFFF) / static_cast<float>(0xFFFFFF);
	}

	// Ruido de valor con interpolación suave: barato, determinista y sin dependencias.
	float ValueNoise(int32 Seed, float XCm, float YCm, float CellCm)
	{
		const float CellX = XCm / CellCm;
		const float CellY = YCm / CellCm;
		const int32 X0 = FMath::FloorToInt(CellX);
		const int32 Y0 = FMath::FloorToInt(CellY);
		const float TX = FMath::SmoothStep(0.0f, 1.0f, CellX - X0);
		const float TY = FMath::SmoothStep(0.0f, 1.0f, CellY - Y0);

		const float TopLeft = HashToUnitFloat(X0, Y0, Seed);
		const float TopRight = HashToUnitFloat(X0 + 1, Y0, Seed);
		const float BottomLeft = HashToUnitFloat(X0, Y0 + 1, Seed);
		const float BottomRight = HashToUnitFloat(X0 + 1, Y0 + 1, Seed);

		return FMath::Lerp(FMath::Lerp(TopLeft, TopRight, TX), FMath::Lerp(BottomLeft, BottomRight, TX), TY);
	}
}

AAstraeonTerrainField::AAstraeonTerrainField()
{
	PrimaryActorTick.bCanEverTick = false;

	TerrainTiles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TerrainTiles"));
	RootComponent = TerrainTiles;
	TerrainTiles->SetMobility(EComponentMobility::Movable);
	TerrainTiles->SetCollisionProfileName(TEXT("BlockAll"));
	TerrainTiles->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TerrainTiles->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		TerrainTiles->SetStaticMesh(CubeMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Geology(TEXT("/Game/Astraeon/Art/ItacaTerrain/M_Terrain_Geology.M_Terrain_Geology"));
	if (Geology.Succeeded()) TerrainTiles->SetMaterial(0, Geology.Object);
}

float AAstraeonTerrainField::GetGroundHeightCm(int32 WorldSeed, float XCm, float YCm)
{
	const float Coarse = AstraeonTerrain::ValueNoise(WorldSeed, XCm, YCm, AstraeonTerrain::GroundCoarseCellCm);
	const float Fine = AstraeonTerrain::ValueNoise(WorldSeed + 7919, XCm, YCm, AstraeonTerrain::GroundFineCellCm);

	// La octava fina sólo matiza: sin esto el suelo queda ondulado y sin lectura.
	const float Combined = FMath::Clamp(Coarse * 0.85f + Fine * 0.15f, 0.0f, 1.0f);

	// El exponente se mantiene bajo porque también multiplica la pendiente: subirlo vuelve
	// a generar escalones infranqueables entre tiles vecinos.
	return FMath::Pow(Combined, 1.2f) * AstraeonTerrain::GroundMaxHeightCm;
}

float AAstraeonTerrainField::GetMountainHeightCm(int32 WorldSeed, float XCm, float YCm)
{
	const float Mask = AstraeonTerrain::ValueNoise(WorldSeed + 31337, XCm, YCm, AstraeonTerrain::MountainCellCm);
	if (Mask <= AstraeonTerrain::MountainThreshold)
	{
		return 0.0f;
	}

	// Normalizar sobre el umbral y elevar al cuadrado da faldas que nacen suaves desde el
	// suelo y sólo se empinan cerca de la cima: se lee como montaña y no como meseta.
	const float Above = (Mask - AstraeonTerrain::MountainThreshold) / (1.0f - AstraeonTerrain::MountainThreshold);
	return Above * Above * AstraeonTerrain::MountainMaxHeightCm;
}

float AAstraeonTerrainField::GetFieldRadiusCm()
{
	return AstraeonTerrain::FieldRadiusCm;
}

float AAstraeonTerrainField::GetSurfaceSpacingCm()
{
	return AstraeonTerrain::SurfaceSpacingCm;
}

float AAstraeonTerrainField::SampleHeightCm(const FAstraeonTerrainSurfaceContext& Context, const FVector2D& PointCm)
{
	const FVector2D LocalToItacaCm = PointCm - Context.ItacaOriginCm;
	if (AAstraeonItacaInterior::IsInsideFootprint(LocalToItacaCm))
	{
		return Context.ItacaPadHeightCm;
	}

	const float GroundHeightCm = GetClearedGroundHeightCm(Context.WorldSeed, PointCm, Context.GroundFlatSpotsCm);
	const bool bMountainAllowed = !IsWithinAnySpot(PointCm, Context.MountainKeepOutCm, AstraeonTerrain::MountainClearanceCm);
	return GroundHeightCm + (bMountainAllowed ? GetMountainHeightCm(Context.WorldSeed, PointCm.X, PointCm.Y) : 0.0f);
}

FAstraeonTerrainSurfaceSample AAstraeonTerrainField::SampleSurface(const FAstraeonTerrainSurfaceContext& Context,
	const FVector2D& PointCm)
{
	FAstraeonTerrainSurfaceSample Result;
	const float RadiusCm = GetFieldRadiusCm();
	if (FVector2D::DistSquared(PointCm, Context.CenterCm) > FMath::Square(RadiusCm))
	{
		return Result;
	}

	Result.HeightCm = SampleHeightCm(Context, PointCm);
	// Diferencias centrales sobre la misma función que crea la malla: la normal describe
	// el terreno final y no la altura base previa a claros o exclusiones.
	constexpr float NormalSampleOffsetCm = 100.0f;
	const float Left = SampleHeightCm(Context, PointCm - FVector2D(NormalSampleOffsetCm, 0.0f));
	const float Right = SampleHeightCm(Context, PointCm + FVector2D(NormalSampleOffsetCm, 0.0f));
	const float Down = SampleHeightCm(Context, PointCm - FVector2D(0.0f, NormalSampleOffsetCm));
	const float Up = SampleHeightCm(Context, PointCm + FVector2D(0.0f, NormalSampleOffsetCm));
	Result.Normal = FVector(-(Right - Left) / (2.0f * NormalSampleOffsetCm),
		-(Up - Down) / (2.0f * NormalSampleOffsetCm), 1.0f).GetSafeNormal();
	Result.bIsValid = true;
	return Result;
}

float AAstraeonTerrainField::GetHeightCm(int32 WorldSeed, float XCm, float YCm)
{
	return GetGroundHeightCm(WorldSeed, XCm, YCm) + GetMountainHeightCm(WorldSeed, XCm, YCm);
}

float AAstraeonTerrainField::ComputeClearanceScale(const FVector2D& PointCm, const TArray<FVector2D>& FlatSpotsCm, float RadiusCm)
{
	// Devuelve 0 en el centro del claro y 1 al borde, con transición suave. Antes los tiles
	// se descartaban de golpe dentro del radio, lo que dejaba el claro en el fondo de un
	// acantilado vertical del alto del terreno vecino. Escalar en vez de recortar convierte
	// ese borde en una rampa.
	float Scale = 1.0f;
	for (const FVector2D& FlatSpot : FlatSpotsCm)
	{
		const float Distance = FVector2D::Distance(PointCm, FlatSpot);
		Scale = FMath::Min(Scale, FMath::SmoothStep(0.0f, RadiusCm, Distance));
	}
	return Scale;
}

float AAstraeonTerrainField::GetTileSizeCm()
{
	return AstraeonTerrain::TileSizeCm;
}

float AAstraeonTerrainField::GetFlatSpotRadiusCm()
{
	return AstraeonTerrain::FlatSpotRadiusCm;
}

float AAstraeonTerrainField::GetMaxWalkableStepCm()
{
	return AstraeonTerrain::MaxWalkableStepCm;
}

float AAstraeonTerrainField::GetMountainClearanceCm()
{
	return AstraeonTerrain::MountainClearanceCm;
}

int32 AAstraeonTerrainField::GetTileCount() const
{
	return TerrainTiles ? TerrainTiles->GetInstanceCount() : 0;
}

float AAstraeonTerrainField::GetItacaPadHeightCm(int32 WorldSeed, float XCm, float YCm)
{
	// Nunca por debajo del umbral de instanciado: si la plataforma quedara más baja, su
	// baldosa se descartaría y la estancia se apoyaría sobre un agujero.
	return FMath::Max(GetGroundHeightCm(WorldSeed, XCm, YCm), AstraeonTerrain::MinimumTileHeightCm);
}

void AAstraeonTerrainField::BuildTerrain(int32 WorldSeed, const FVector2D& CenterCm, const TArray<FVector2D>& GroundFlatSpotsCm, const TArray<FVector2D>& MountainKeepOutCm, float ItacaPadHeightCm)
{
	if (!TerrainTiles)
	{
		return;
	}

	TerrainTiles->ClearInstances();

	const int32 TilesPerSide = FMath::CeilToInt(AstraeonTerrain::FieldRadiusCm / AstraeonTerrain::TileSizeCm);

	// Las instancias se acumulan y se entregan de una sola vez. Añadirlas de a una hacía
	// que el componente recocinara la colisión en cada llamada, y como la región se
	// remateraliza al empezar la partida y en cada aterrizaje, eso era un tirón visible.
	TArray<FTransform> TileTransforms;
	TileTransforms.Reserve((2 * TilesPerSide + 1) * (2 * TilesPerSide + 1));

	for (int32 TileX = -TilesPerSide; TileX <= TilesPerSide; ++TileX)
	{
		for (int32 TileY = -TilesPerSide; TileY <= TilesPerSide; ++TileY)
		{
			const FVector2D TileCenter(
				CenterCm.X + TileX * AstraeonTerrain::TileSizeCm,
				CenterCm.Y + TileY * AstraeonTerrain::TileSizeCm);

			const float GroundHeightCm = GetClearedGroundHeightCm(WorldSeed, TileCenter, GroundFlatSpotsCm);

			// Ninguna montaña puede nacer sobre un punto jugable. Antes esto comparaba la
			// rampa contra cero, y como SmoothStep devuelve algo mayor que cero para
			// cualquier distancia no nula, la exclusión no excluía nada: podía crecer una
			// montaña justo delante de la escotilla y tapar la salida de la nave.
			const bool bMountainAllowed = !IsWithinAnySpot(TileCenter, MountainKeepOutCm, AstraeonTerrain::MountainClearanceCm);
			const float MountainHeightCm = bMountainAllowed ? GetMountainHeightCm(WorldSeed, TileCenter.X, TileCenter.Y) : 0.0f;

			// Una baldosa mide 7 m y la estancia 8 x 6 m: aunque su centro caiga fuera de la
			// huella, el bloque entra dentro por el borde. Nace en Z=0 y sube, así que su cara
			// superior aparecía como suelo rocoso dentro de Ítaca y como una pared afuera, y
			// dejaba al jugador atrapado entre ese bloque y el piso de la estancia. Bajo la
			// huella, más media baldosa de margen, el terreno es una plataforma plana a la
			// cota sobre la que se apoya la cubierta.
			const bool bUnderItaca = AAstraeonItacaInterior::IsInsideFootprint(
				TileCenter - CenterCm, AstraeonTerrain::TileSizeCm * 0.5f);

			const float HeightCm = bUnderItaca ? ItacaPadHeightCm : GroundHeightCm + MountainHeightCm;
			if (!bUnderItaca && HeightCm < AstraeonTerrain::MinimumTileHeightCm)
			{
				// Las lomas mínimas no se instancian: no se ven y cuestan colisión. El
				// umbral está por debajo del escalón caminable para que el primer bloque
				// de cada loma no sea un peldaño imposible contra el suelo desnudo.
				continue;
			}

			// El cubo base mide 100 cm, así que la escala es directamente metros. Cada
			// bloque nace en Z=0 y sube: nunca hunde el terreno bajo la placa de suelo.
			FTransform TileTransform;
			TileTransform.SetLocation(FVector(TileCenter.X, TileCenter.Y, HeightCm * 0.5f));
			TileTransform.SetScale3D(FVector(
				AstraeonTerrain::TileSizeCm / 100.0f,
				AstraeonTerrain::TileSizeCm / 100.0f,
				HeightCm / 100.0f));
			TileTransforms.Add(TileTransform);
		}
	}

	TerrainTiles->AddInstances(TileTransforms, false, true);
}

float AAstraeonTerrainField::GetClearedGroundHeightCm(int32 WorldSeed, const FVector2D& PointCm, const TArray<FVector2D>& FlatSpotsCm)
{
	// El llano es una MESETA a la altura del suelo local, no un pozo excavado hasta Z=0.
	// Cavar hasta cero dejaba un cráter enorme alrededor de la nave, visible al despegar, y
	// obligaba a un radio gigante para que la rampa resultara caminable. Nivelar contra la
	// altura local deja un desnivel mínimo y un claro pequeño.
	float HeightCm = GetGroundHeightCm(WorldSeed, PointCm.X, PointCm.Y);
	for (const FVector2D& FlatSpot : FlatSpotsCm)
	{
		const float Distance = FVector2D::Distance(PointCm, FlatSpot);
		if (Distance >= AstraeonTerrain::FlatSpotRadiusCm)
		{
			continue;
		}

		const float PlateauHeight = GetGroundHeightCm(WorldSeed, FlatSpot.X, FlatSpot.Y);
		const float Blend = FMath::SmoothStep(0.0f, AstraeonTerrain::FlatSpotRadiusCm, Distance);
		HeightCm = FMath::Lerp(PlateauHeight, HeightCm, Blend);
	}
	return HeightCm;
}

bool AAstraeonTerrainField::IsWithinAnySpot(const FVector2D& PointCm, const TArray<FVector2D>& SpotsCm, float RadiusCm)
{
	const float RadiusSquared = FMath::Square(RadiusCm);
	for (const FVector2D& Spot : SpotsCm)
	{
		if (FVector2D::DistSquared(PointCm, Spot) < RadiusSquared)
		{
			return true;
		}
	}
	return false;
}
