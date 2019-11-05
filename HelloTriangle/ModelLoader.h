#pragma once
#include <rapidjson/fwd.h>
#include <vector>

struct SModelInformation;
struct SVertex;

class CModelLoader
{
public:
	static void GetSceneHierarchy(const char* filename, std::vector<SModelInformation>& outVecModelInformation);
	static void LoadModel(const SModelInformation& modelInformation, std::vector<SVertex>& outVecVertex, std::vector<uint32_t>& outVecIndex);
};
