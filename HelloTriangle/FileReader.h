#pragma once

#include <iostream>
#include <fstream>
#include <vector>

class CFileReader
{
public:
	explicit CFileReader(const char* filename, const int flags = 0)
	{
		readStream.open(filename, flags);
		if (!readStream.is_open())
			throw std::exception("Failed to open the file.");
	}
	void ReadFile(std::vector<char>& vecOutput)
	{
		readStream.clear();
		readStream.seekg(0, std::ifstream::end);
		const auto fileSize = static_cast<size_t>(readStream.tellg());
		readStream.seekg(0, std::ifstream::beg);

		vecOutput.resize(fileSize);
		readStream.read(vecOutput.data(), fileSize);
	}
	~CFileReader() = default;
	CFileReader(const CFileReader&) = delete;
	CFileReader& operator=(const CFileReader&) = delete;
	CFileReader(CFileReader&&) = delete;
	CFileReader& operator=(CFileReader&&) = delete;
private:
	std::ifstream readStream;
};
