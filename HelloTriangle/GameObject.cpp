#include "GameObject.h"
#include "ModelLoader.h"


IGameObject::IGameObject(SObjectInformation objectInfo)
{
	mObjectInformation = std::move(objectInfo);
}

//SVertex* IGameObject::GetVertexData()
//{
//	return mObjectInformation.ModelInformation.VecVertex.data();
//}
//
//uint32_t* IGameObject::GetIndexData()
//{
//	return mObjectInformation.ModelInformation.VecIndex.data();
//}

size_t IGameObject::GetVertexBufferSize()
{
	return sizeof(SVertex) * mObjectInformation.ModelInformation.VecVertex.size();
}

size_t IGameObject::GetIndexBufferSize()
{
	return sizeof(uint32_t) * mObjectInformation.ModelInformation.VecVertex.size();
}

CStaticGameObject::CStaticGameObject(SObjectInformation objectInfo)
	: IGameObject(std::move(objectInfo))
{
	CModelLoader::LoadModel(mObjectInformation);
}

void CStaticGameObject::Update()
{
}

void CStaticGameObject::Draw()
{
	
}
