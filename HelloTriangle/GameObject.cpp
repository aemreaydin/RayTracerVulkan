#include "GameObject.h"
#include "ModelLoader.h"


IGameObject::IGameObject(SObjectInformation objectInfo, STransform transform)
{
	mObjectInformation = std::move(objectInfo);
	mTransform = transform;
}

VkBuffer& IGameObject::GetVertexBuffer() 
{
	return mModelInformation.VertexBuffer.Buffer;
}

VkBuffer& IGameObject::GetIndexBuffer()
{
	return mModelInformation.IndexBuffer.Buffer;
}

VkDeviceMemory& IGameObject::GetVertexBufferMemory()
{
	return mModelInformation.VertexBuffer.BufferMemory;
}

VkDeviceMemory& IGameObject::GetIndexBufferMemory()
{
	return mModelInformation.IndexBuffer.BufferMemory;
}

const SVertex* IGameObject::GetVertexData() const
{
	return mModelInformation.VecVertex.data();
}

const uint32_t* IGameObject::GetIndexData() const
{
	return mModelInformation.VecIndex.data();
}

size_t IGameObject::GetVertexBufferSize() const
{
	return sizeof(SVertex) * mModelInformation.VecVertex.size();
}

size_t IGameObject::GetIndexBufferSize() const
{
	return sizeof(uint32_t) * mModelInformation.VecVertex.size();
}

uint32_t IGameObject::GetIndexArraySize() const
{
	return static_cast<uint32_t>(mModelInformation.VecIndex.size());
}

CStaticGameObject::CStaticGameObject(SObjectInformation objectInfo, STransform transform)
	: IGameObject(std::move(objectInfo), transform)
{
	CModelLoader::LoadModel(mObjectInformation, mModelInformation);
}

void CStaticGameObject::Update()
{
}

void CStaticGameObject::Draw()
{
	
}
