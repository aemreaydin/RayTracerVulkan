#pragma once
#include "Common.h"
#include "FileReader.h"

class CShaderLoader
{
public:
	static void ReadShader(const char* filename, std::vector<char>& outShaderCode)
	{
		CFileReader fileReader(filename, std::ios::binary);
		fileReader.ReadFile(outShaderCode);
	}
};
