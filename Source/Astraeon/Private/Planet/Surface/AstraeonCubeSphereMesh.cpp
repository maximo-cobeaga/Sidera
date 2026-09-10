#include "Planet/Surface/AstraeonCubeSphereMesh.h"
#include "Planet/Surface/AstraeonPlanetSurface.h"

bool FAstraeonCubeSphereMesh::BuildFace(const FAstraeonPlanetDefinition& Planet,
	EAstraeonPlanetFace Face, int32 Quads, FAstraeonCubeSphereMesh& Out)
{
	Out = {};
	if (!Planet.IsValid() || Quads < 4 || Quads > 128 || uint8(Face) >= 6
		|| Planet.GeneratorVersion != FAstraeonPlanetSurface::GeneratorVersion) return false;
	Out.OriginBodyCm = FAstraeonPlanetCoordinates::FaceUvToDirection(Face, FVector2D::ZeroVector)*Planet.RadiusCm;
	for (int32 Y=0; Y<=Quads; ++Y)
	for (int32 X=0; X<=Quads; ++X)
	{
		const FVector2D Uv(-1.0+2.0*X/Quads, -1.0+2.0*Y/Quads);
		const FVector Dir = FAstraeonPlanetCoordinates::FaceUvToDirection(Face, Uv);
		const double Height = FAstraeonPlanetSurface::SampleRadialHeightCm(Planet, Dir);
		Out.Vertices.Add(Dir*(Planet.RadiusCm+Height)-Out.OriginBodyCm);
		Out.Normals.Add(FAstraeonPlanetSurface::SampleRadialNormal(Planet, Dir));
		Out.UVs.Add((Uv+FVector2D(1,1))*0.5);
	}
	for (int32 Y=0; Y<Quads; ++Y)
	for (int32 X=0; X<Quads; ++X)
	{
		const int32 A=Y*(Quads+1)+X, B=A+1, C=A+Quads+1, D=C+1;
		// Clockwise viewed from outside, consistent with the existing PMC terrain backend.
		Out.Indices.Append({A,C,B,B,C,D});
	}
	return true;
}
