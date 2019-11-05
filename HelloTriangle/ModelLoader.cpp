#include "Common.h"
#include "ModelLoader.h"
#include "FileReader.h"

#include <rapidjson/document.h>
using namespace rapidjson;

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>


void CModelLoader::GetSceneHierarchy(const char* filename, std::vector<SModelInformation>& outVecModelInformation)
{
	CFileReader fileReader(filename);
	std::vector<char> out;
	fileReader.ReadFile(out);

	// Parse the scene hierarchy
	const auto sceneHierarchy = std::string(out.begin(), out.end());
	Document sceneDoc;
	sceneDoc.Parse(sceneHierarchy.data());

	const auto& sceneObj = sceneDoc["Scene"];
	outVecModelInformation.resize(sceneObj.Size());
	for(size_t index = 0; index != sceneObj.Size(); ++index)
	{
		const auto& model = sceneObj[static_cast<rapidjson::SizeType>(index)];
		const auto file = model["Filename"].GetString();
		const auto position = model["Position"].GetObject();
		const auto posX = position["X"].GetFloat();
		const auto posY = position["Y"].GetFloat();
		const auto posZ = position["Z"].GetFloat();

		outVecModelInformation[index].Filename = file;
		outVecModelInformation[index].Position = glm::vec4(posX, posY, posZ, 1.0f);
	}
}

void CModelLoader::LoadModel(const SModelInformation& modelInformation, 
							 std::vector<SVertex>& outVecVertex, 
							 std::vector<uint32_t>& outVecIndex)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> vecShape;
	std::vector<tinyobj::material_t> vecMaterial;
	std::string warn, error;

	
	const auto result = tinyobj::LoadObj(&attrib, &vecShape, &vecMaterial, &warn, &error, modelInformation.Filename.c_str());

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