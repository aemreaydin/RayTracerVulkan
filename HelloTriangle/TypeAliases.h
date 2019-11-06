#pragma once

#include <vector>
#include <memory>

class IGameObject;

using GameObjectUPtr = std::unique_ptr<IGameObject>;
using GameObjectVecPtrs = std::vector<GameObjectUPtr>;