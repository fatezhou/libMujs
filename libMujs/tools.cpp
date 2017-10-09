#include <string>
#include <windows.h>

namespace Neko{
	std::string GetProcessDir(){
		char szPath[1024] = { 0 };
		GetModuleFileNameA(NULL, szPath, 1024);
		char* p = strrchr(szPath, '\\');
		*p = 0;
		return szPath;
	}

	std::string GetDataFromFile(const std::string& strPath){
		FILE* pFile = fopen(strPath.c_str(), "r");
		std::string str;
		if (pFile){
			while (feof(pFile) == 0){
				char sz[11] = { 0 };
				fread(sz, 1, 10, pFile);
				str += sz;
			}
			fclose(pFile);
		}
		return str;
	}
}