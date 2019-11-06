#include "Common.h"
#include "CommonStructs.h"
#include "ModelLoader.h"
#include "FileReader.h"
#include "GameObject.h"

#include <rapidjson/document.h>
using namespace rapidjson;

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>


void CModelLoader::GetSceneHierarchy(const char* filename, GameObjectVecPtrs& goPtrs)
{
	CFileReader fileReader(filename);
	std::vector<char> out;
	fileReader.ReadFile(out);

	// Parse the scene hierarchy
	const auto sceneHierarchy = std::string(out.begin(), out.end());
	Document sceneDoc;
	sceneDoc.Parse(sceneHierarchy.data());

	const auto& sceneObj = sceneDoc["Scene"];
	goPtrs.reserve(sceneObj.Size());
	
	for(size_t index = 0; index != sceneObj.Size(); ++index)
	{
		const auto& model = sceneObj[static_cast<rapidjson::SizeType>(index)];
		const auto file = model["Filename"].GetString();
		const auto name = model["Name"].GetString();
		const auto transform = model["Transform"].GetObject();
		const auto position = transform["Position"].GetArray();
		const auto rotation = transform["Rotation"].GetArray();
		const auto scale = transform["Scale"].GetArray();
		const auto objectPosition = glm::vec3(position[0].GetFloat(), 
											  position[1].GetFloat(), 
											  position[2].GetFloat());
		const auto objectRotation = glm::vec3(rotation[0].GetFloat(),
											  rotation[1].GetFloat(),
											  rotation[2].GetFloat());
		const auto objectScale = glm::vec3(scale[0].GetFloat(),
										   scale[1].GetFloat(), 
										   scale[2].GetFloat());

		SObjectInformation objectInformation(file, name);
		STransform objectTransform(objectPosition, objectRotation, objectScale);
		GameObjectUPtr uptr(new CStaticGameObject(objectInformation, objectTransform));
		goPtrs.push_back(std::move(uptr));
	}
}

void CModelLoader::LoadModel(const SObjectInformation& modelInformation,
							 std::vector<SVertex>& outVecVertex, 
							 std::vector<uint32_t>& outVecIndex)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> vecShape;
	std::vector<tinyobj::material_t> vecMaterial;
	std::string warn, error;

	
	const auto result = tinyobj::LoadObj(&attrib, &vecShape, &vecMaterial, &warn, &error, modelInformation.FileName.c_str());

	if(!warn.empty())
		std::cout << warn << std::endl;
	if(!error.empty())
		std::cout << error << std::endl;
	if(!result)
		throw std::runtime_error("Failed to load the object");


	outVecVertex.reserve(attrib.GetVertices().size());

	for(const auto& shape : vecShape)
	{
		for(const auto& index : shape.mesh.indices)
		{
			SVertex vertex = {};
			vertex.Position.x = attrib.vertices[3 * index.vertex_index + 0];
			vertex.Position.y = attrib.vertices[3 * index.vertex_index + 1];
			vertex.Position.z = attrib.vertices[3 * index.vertex_index + 2];

			vertex.TextureCoords.x = attrib.texcoords[2 * index.texcoord_index + 0];
			vertex.TextureCoords.y = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1];
			
			outVecVertex.push_back(vertex);
			outVecIndex.push_back(static_cast<uint32_t>(outVecIndex.size()));
		}
	}
}

void CModelLoader::LoadModel(const SObjectInformation& objectInformation, SModelInformation& modelInfo)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> vecShape;
	std::vector<tinyobj::material_t> vecMaterial;
	std::string warn, error;


	const auto result = tinyobj::LoadObj(&attrib, &vecShape, &vecMaterial, &warn, &error, objectInformation.FileName.c_str());

	if (!warn.empty())
		std::cout << warn << std::endl;
	if (!error.empty())
		std::cout << error << std::endl;
	if (!result)
		throw std::runtime_error("Failed to load the object");


	modelInfo.VecVertex.reserve(attrib.GetVertices().size());
	// TODO: Find a better way to do the loading
	for (const auto& shape : vecShape)
	{
		for (const auto& index : shape.mesh.indices)
		{
			SVertex vertex = {};
			vertex.Position.x = attrib.vertices[3 * index.vertex_index + 0];
			vertex.Position.y = attrib.vertices[3 * index.vertex_index + 1];
			vertex.Position.z = attrib.vertices[3 * index.vertex_index + 2];

			vertex.TextureCoords.x = attrib.texcoords[2 * index.texcoord_index + 0];
			vertex.TextureCoords.y = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1];

			modelInfo.VecVertex.push_back(vertex);
			modelInfo.VecIndex.push_back(static_cast<uint32_t>(modelInfo.VecIndex.size()));
		}
	}
}
