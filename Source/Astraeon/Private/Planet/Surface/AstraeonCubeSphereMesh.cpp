#include "Planet/Surface/AstraeonCubeSphereMesh.h"
#include "Planet/Patches/AstraeonPlanetPatchMesh.h"

bool FAstraeonCubeSphereMesh::BuildFace(const FAstraeonPlanetDefinition& Planet,
	EAstraeonPlanetFace Face, int32 Quads, FAstraeonCubeSphereMesh& Out)
{
	Out = {};
	FAstraeonPlanetPatchAddress Address;
	Address.BodyId = Planet.BodyId; Address.Face = Face;
	FAstraeonPlanetPatchBuildOptions Options;
	Options.Quads = Quads; Options.SkirtDepthCm = 0.0;
	FAstraeonPlanetPatchBuildResult Result;
	if (FAstraeonPlanetPatchMesh::Build(Planet, Address, 1, Options, Result) != EAstraeonPatchBuildStatus::Success) return false;
	Out.OriginBodyCm = Result.OriginBodyCm;
	Out.Vertices = MoveTemp(Result.Vertices); Out.Normals = MoveTemp(Result.Normals);
	Out.UVs = MoveTemp(Result.UVs); Out.Indices = MoveTemp(Result.Indices);
	return true;
}
