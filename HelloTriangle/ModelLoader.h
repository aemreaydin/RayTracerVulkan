#pragma once
#include <rapidjson/fwd.h>
#include <vector>

struct SObjectInformation;
struct SModelInformation;
struct SVertex;

class CModelLoader
{
public:
	static void GetSceneHierarchy(const char* filename, std::vector<SObjectInformation>& outVecModelInformation);
	static void LoadModel(const SObjectInformation& modelInformation, std::vector<SVertex>& outVecVertex, std::vector<uint32_t>& outVecIndex);
	static void LoadModel(SObjectInformation& objectInformation);
};
