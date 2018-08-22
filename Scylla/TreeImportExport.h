#pragma once

#include <windows.h>
#include "ProcessLister.h"
#include "Thunks.h"
#include "pugixml.hpp"

using namespace pugi;
class TreeImportExport
{
public:

	TreeImportExport(const WCHAR * targetXmlFile);

	bool exportTreeList(const std::map<DWORD_PTR, ImportModuleThunk> & moduleList, const Process * process, DWORD_PTR addressOEP, DWORD_PTR addressIAT, DWORD sizeIAT) ;
	bool importTreeList(std::map<DWORD_PTR, ImportModuleThunk> & moduleList, DWORD_PTR * addressOEP, DWORD_PTR * addressIAT, DWORD * sizeIAT);

private:

	WCHAR xmlPath[MAX_PATH];

	char xmlStringBuffer[MAX_PATH];

	void setTargetInformation(xml_node & rootElement, const Process * process, DWORD_PTR addressOEP, DWORD_PTR addressIAT, DWORD sizeIAT);
	void addModuleListToRootElement(xml_node & rootElement, const std::map<DWORD_PTR, ImportModuleThunk> & moduleList);

	void parseAllElementModules(xml_node & targetElement, std::map<DWORD_PTR, ImportModuleThunk> & moduleList);
	void parseAllElementImports(xml_node & moduleElement, ImportModuleThunk * importModuleThunk);

	xml_node getModuleXmlElement(xml_node & rootElement, const ImportModuleThunk * importModuleThunk);
	xml_node getImportXmlElement(xml_node & moduleElement, const ImportThunk * importThunk);

	bool saveXmlToFile(const xml_document& doc, const WCHAR * xmlFilePath);
	bool readXmlFile(xml_document& doc, const WCHAR * xmlFilePath);

	void ConvertBoolToString(const bool boolValue);
	void ConvertWordToString(const WORD dwValue);
	void ConvertDwordPtrToString(const DWORD_PTR dwValue);

	DWORD_PTR ConvertStringToDwordPtr(const char * strValue);
	WORD ConvertStringToWord(const char * strValue);
	bool ConvertStringToBool(const char * strValue);
};
