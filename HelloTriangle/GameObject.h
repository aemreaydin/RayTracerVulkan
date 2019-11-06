#pragma once
#include "CommonStructs.h"

#include <glm/glm.hpp>
#include <memory>

class IGameObject
{
public:
	explicit IGameObject(SObjectInformation);

	virtual ~IGameObject() = default;	
	IGameObject(const IGameObject&) = delete;
	IGameObject(IGameObject&&) = delete;
	IGameObject& operator=(const IGameObject&) = delete;
	IGameObject& operator=(IGameObject&&) = delete;

	virtual void Update() = 0;
	virtual void Draw() = 0;

	//[[nodiscard]] SVertex* GetVertexData();
	//[[nodiscard]] uint32_t* GetIndexData();
	virtual size_t GetVertexBufferSize();
	virtual size_t GetIndexBufferSize();
protected:
	SObjectInformation mObjectInformation;	
};

class CStaticGameObject final : public IGameObject
{
public:
	explicit CStaticGameObject(SObjectInformation objectInfo);

	void Update() override;
	void Draw() override;
};

