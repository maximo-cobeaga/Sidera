#pragma once

#include "CoreMinimal.h"
#include "Planet/AstraeonPlanetDefinition.h"
#include "Planet/Surface/AstraeonPlanetRegionRelief.h"

// Consulta pura de la superficie de referencia. La entrada geometrica es una direccion
// planetaria global, no una UV local: dos caras que comparten borde reciben el mismo valor.
//
// El relieve son DOS CAPAS con propositos distintos, portadas del dominio plano
// (`AstraeonTerrainField`) conservando su logica y sus constantes:
//
//   • Suelo: amplitud contenida repartida en decenas de metros, de modo que el escalon
//     entre dos muestras vecinas queda por debajo del limite de paso. Es piso, y siempre
//     se camina.
//   • Montanas: formaciones aisladas y altas que NO se pretende escalar. Son barreras y
//     puntos de referencia, y estan EXENTAS del limite de escalon a proposito.
//
// Mezclar ambas en una sola curva fue el error que el dominio plano ya habia pagado: subir
// la amplitud para tener paisaje volvia el suelo intransitable, y bajarla para poder
// caminar dejaba el mundo liso.
//
// Las constantes son invariantes de escala humana y valen igual en la esfera que en el
// plano: el radio del planeta cambia cuanta superficie hay, no que es un escalon caminable.
struct ASTRAEON_API FAstraeonPlanetSurface
{
	// 3: dos capas. La 2 era una sola capa de +-180 cm, sin suelo ni montanas separados.
	static constexpr int32 GeneratorVersion = 3;

	// Espaciado al que se mide el transito: entre dos muestras la malla es un plano, no la
	// curva del ruido, asi que es la resolucion a la que el jugador realmente pisa.
	static constexpr double TileSizeCm = 700.0;
	static constexpr double MaxWalkableStepCm = 45.0;
	static constexpr double FlatSpotRadiusCm = 2600.0;
	// Las montanas se suprimen de golpe mas lejos que el claro del suelo. Un corte brusco es
	// aceptable porque una montana ya es un muro: se lee como pared de roca, no como error.
	static constexpr double MountainClearanceCm = 9000.0;
	static constexpr double GroundMaxHeightCm = 260.0;
	static constexpr double MountainMaxHeightCm = 5200.0;
	static constexpr double MaxReliefCm = GroundMaxHeightCm + MountainMaxHeightCm;

	// Un cuerpo que no da la vuelta en varias celdas de montana no tiene donde poner una:
	// el muestreo entero cae dentro de una sola celda de ruido y la capa deja de significar
	// nada. Por debajo de ese tamano el planeta es todo suelo caminable, que es exactamente
	// lo que necesita el banco de locomocion de 200 m de radio.
	static constexpr double MinMountainCellsAround = 8.0;
	static bool HasMountainLayer(const FAstraeonPlanetDefinition& Planet);

	// Una definicion invalida, una version desconocida o una direccion no normalizable
	// devuelven NaN, nunca suelo liso: un cero silencioso se lee como terreno valido.
	static double SampleGroundHeightCm(const FAstraeonPlanetDefinition& Planet, const FVector& Direction);
	static double SampleMountainHeightCm(const FAstraeonPlanetDefinition& Planet, const FVector& Direction);
	// LA altura del cuerpo: las dos capas, mas el relieve de su region si la definicion lo trae.
	// Malla, colision, validador y consultas de juego pasan todas por aqui.
	static double SampleRadialHeightCm(const FAstraeonPlanetDefinition& Planet, const FVector& Direction);
	// Las dos capas editadas por una region: meseta bajo Itaca, claros y exclusiones. Fuera del
	// cono de influencia devuelve exactamente las capas del cuerpo.
	static double SampleRegionHeightCm(const FAstraeonPlanetDefinition& Planet, const FAstraeonPlanetRegionRelief& Relief,
		const FVector& Direction);

	// Claro para estructuras rigidas que no pueden seguir el relieve. Es una MESETA a la
	// altura del suelo LOCAL, no un pozo excavado hasta el nivel del mar: cavar dejaba un
	// crater y obligaba a un radio enorme para que la rampa fuese caminable.
	static double SampleClearedGroundHeightCm(const FAstraeonPlanetDefinition& Planet, const FVector& Direction,
		const TArray<FVector>& FlatSpotDirections);

	static FVector SampleRadialNormal(const FAstraeonPlanetDefinition& Planet, const FVector& Direction);

	// Distancia sobre la superficie entre dos direcciones, en centimetros.
	static double ArcDistanceCm(const FAstraeonPlanetDefinition& Planet, const FVector& A, const FVector& B);

	// Exclusion por radio explicito. Deducirla de la rampa del claro la dejo sin efecto una
	// vez y una montana acabo tapando la escotilla de la nave.
	static bool IsWithinAnySpot(const FAstraeonPlanetDefinition& Planet, const FVector& Direction,
		const TArray<FVector>& SpotDirections, double RadiusCm);
};
