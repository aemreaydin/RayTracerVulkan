#pragma once
#include <rapidjson/fwd.h>
#include <vector>

#include "TypeAliases.h"

struct SModelInformation;
struct SVertex;
struct STransform;

class CModelLoader
{
public:
	static void GetSceneHierarchy(const char* filename, GameObjectVecPtrs& goPtrs);
	static void LoadModel(const SObjectInformation& modelInformation, std::vector<SVertex>& outVecVertex, std::vector<uint32_t>& outVecIndex);
	static void LoadModel(const SObjectInformation &objectInformation, SModelInformation& modelInfo);
};
