#pragma once
#include "Common.h"
#include <fstream>

class CShaderLoader
{
public:
	static std::vector<char> ReadShader(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open the shader file.");
		}
		// Starting to read from the end gives us the ability to determine
		// the size of the file and allocate a buffer
		const auto fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();
		return buffer;
	}
};
