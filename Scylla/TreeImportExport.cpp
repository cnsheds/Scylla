#include "TreeImportExport.h"
#include "Architecture.h"
#include "Scylla.h"
#include "StringConversion.h"

TreeImportExport::TreeImportExport(const WCHAR * targetXmlFile)
{
	wcscpy_s(xmlPath, targetXmlFile);
}

bool TreeImportExport::exportTreeList(const std::map<DWORD_PTR, ImportModuleThunk> & moduleList, const Process * process, DWORD_PTR addressOEP, DWORD_PTR addressIAT, DWORD sizeIAT)
{
	xml_document doc;

	xml_node decl = doc.prepend_child(node_declaration);
	decl.append_attribute("version") = "1.0";
	decl.append_attribute("encoding") = "UTF-8";
	decl.append_attribute("standalone") = "no";

	xml_node rootElement = doc.append_child("target");

	setTargetInformation(rootElement, process, addressOEP, addressIAT, sizeIAT);

	addModuleListToRootElement(rootElement, moduleList);

	return saveXmlToFile(doc, xmlPath);
}

bool TreeImportExport::importTreeList(std::map<DWORD_PTR, ImportModuleThunk> & moduleList, DWORD_PTR * addressOEP, DWORD_PTR * addressIAT, DWORD * sizeIAT)
{
	moduleList.clear();
	*addressOEP = *addressIAT = 0;
	*sizeIAT = 0;

	xml_document doc;
	xml_parse_result ret = doc.load_file(xmlPath);
	if (ret.status != xml_parse_status::status_ok)
	{
		Scylla::windowLog.log(L"Load Tree :: Error parsing xml %s: %s\r\n", xmlPath, ret.description());
		return false;
	}

	xml_node targetElement = doc.first_child();
	if (!targetElement)
	{
		Scylla::windowLog.log(L"Load Tree :: Error getting first child element in xml %s\r\n", doc.value());
		return false;
	}

	*addressOEP = ConvertStringToDwordPtr(targetElement.attribute("oep_va").value());
	*addressIAT = ConvertStringToDwordPtr(targetElement.attribute("iat_va").value());
	*sizeIAT = (DWORD)ConvertStringToDwordPtr(targetElement.attribute("iat_size").value());

	parseAllElementModules(targetElement, moduleList);

	return true;
}

void TreeImportExport::setTargetInformation(xml_node &rootElement, const Process * process, DWORD_PTR addressOEP, DWORD_PTR addressIAT, DWORD sizeIAT)
{
	StringConversion::ToASCII(process->filename, xmlStringBuffer, _countof(xmlStringBuffer));
	rootElement.append_attribute("filename").set_value(xmlStringBuffer);

	ConvertDwordPtrToString(addressOEP);
	rootElement.append_attribute("oep_va").set_value(xmlStringBuffer);

	ConvertDwordPtrToString(addressIAT);
	rootElement.append_attribute("iat_va").set_value(xmlStringBuffer);

	ConvertDwordPtrToString(sizeIAT);
	rootElement.append_attribute("iat_size").set_value(xmlStringBuffer);
}

bool TreeImportExport::readXmlFile(xml_document& doc, const WCHAR * xmlFilePath)
{
	bool success = false;

	xml_parse_result ret = doc.load_file(xmlFilePath);
	if (ret == xml_parse_status::status_ok)
		success = true;

	return success;
}

bool TreeImportExport::saveXmlToFile(const xml_document& doc, const WCHAR * xmlFilePath)
{
	return doc.save_file(xmlFilePath);
}

void TreeImportExport::addModuleListToRootElement(xml_node & rootElement, const std::map<DWORD_PTR, ImportModuleThunk> & moduleList)
{
	std::map<DWORD_PTR, ImportModuleThunk>::const_iterator it_mod;
	for(it_mod = moduleList.begin(); it_mod != moduleList.end(); it_mod++)
	{
		const ImportModuleThunk& importModuleThunk = it_mod->second;

		xml_node moduleElement = getModuleXmlElement(rootElement, &importModuleThunk);

		std::map<DWORD_PTR, ImportThunk>::const_iterator it_thunk;
		for(it_thunk = importModuleThunk.thunkList.begin(); it_thunk != importModuleThunk.thunkList.end(); it_thunk++)
		{
			const ImportThunk& importThunk = it_thunk->second;

			getImportXmlElement(moduleElement, &importThunk);
		}
	}
}

xml_node TreeImportExport::getModuleXmlElement(xml_node & rootElement, const ImportModuleThunk * importModuleThunk)
{
	xml_node moduleElement = rootElement.append_child("module");

	StringConversion::ToASCII(importModuleThunk->moduleName, xmlStringBuffer, _countof(xmlStringBuffer));
	moduleElement.append_attribute("filename").set_value(xmlStringBuffer);

	ConvertDwordPtrToString(importModuleThunk->getFirstThunk());
	moduleElement.append_attribute("first_thunk_rva").set_value(xmlStringBuffer);

	return moduleElement;
}

xml_node TreeImportExport::getImportXmlElement(xml_node & moduleElement, const ImportThunk * importThunk)
{
	xml_node importElement = moduleElement.append_child();
	
	if (importThunk->valid)
	{
		importElement.set_name("import_valid");

		if(importThunk->name[0] != '\0')
		{
			importElement.append_attribute("name").set_value(importThunk->name);
		}

		ConvertWordToString(importThunk->ordinal);
		importElement.append_attribute("ordinal").set_value(xmlStringBuffer);

		ConvertWordToString(importThunk->hint);
		importElement.append_attribute("hint").set_value(xmlStringBuffer);
		
		ConvertBoolToString(importThunk->suspect);
		importElement.append_attribute("suspect").set_value(xmlStringBuffer);
	}
	else
	{
		importElement.set_name("import_invalid");
	}

	ConvertDwordPtrToString(importThunk->rva);
	importElement.append_attribute("iat_rva").set_value(xmlStringBuffer);

	ConvertDwordPtrToString(importThunk->apiAddressVA);
	importElement.append_attribute("address_va").set_value(xmlStringBuffer);

	return importElement;
}

void TreeImportExport::ConvertBoolToString(const bool boolValue)
{
	if (boolValue)
	{
		strcpy_s(xmlStringBuffer, "1");
	}
	else
	{
		strcpy_s(xmlStringBuffer, "0");
	}
}

bool TreeImportExport::ConvertStringToBool(const char * strValue)
{
	if (strValue)
	{
		if (strValue[0] == '1')
		{
			return true;
		}
	}
	
	return false;
}

void TreeImportExport::ConvertDwordPtrToString(const DWORD_PTR dwValue)
{
	sprintf_s(xmlStringBuffer, PRINTF_DWORD_PTR_FULL_S, dwValue);
}

DWORD_PTR TreeImportExport::ConvertStringToDwordPtr(const char * strValue)
{
	DWORD_PTR result = 0;

	if (strValue)
	{
#ifdef _WIN64
		result = _strtoi64(strValue, NULL, 16);
#else
		result = strtoul(strValue, NULL, 16);
#endif
	}

	return result;
}

void TreeImportExport::ConvertWordToString(const WORD dwValue)
{
	sprintf_s(xmlStringBuffer, "%04X", dwValue);
}

WORD TreeImportExport::ConvertStringToWord(const char * strValue)
{
	WORD result = 0;

	if (strValue)
	{
		result = (WORD)strtoul(strValue, NULL, 16);
	}

	return result;
}

void TreeImportExport::parseAllElementModules(xml_node & targetElement, std::map<DWORD_PTR, ImportModuleThunk> & moduleList)
{
	ImportModuleThunk importModuleThunk;

	for(xml_node moduleElement = targetElement.first_child(); moduleElement; moduleElement = moduleElement.next_sibling())
	{
		const char * filename = moduleElement.attribute("filename").value();
		if (filename)
		{
			StringConversion::ToUTF16(filename, importModuleThunk.moduleName, _countof(importModuleThunk.moduleName));

			importModuleThunk.firstThunk = ConvertStringToDwordPtr(moduleElement.attribute("first_thunk_rva").value());

			importModuleThunk.thunkList.clear();
			parseAllElementImports(moduleElement, &importModuleThunk);

			moduleList[importModuleThunk.firstThunk] = importModuleThunk;
		}
	}
}

void TreeImportExport::parseAllElementImports(xml_node & moduleElement, ImportModuleThunk * importModuleThunk)
{
	ImportThunk importThunk;

	for(xml_node  importElement = moduleElement.first_child(); importElement; importElement = importElement.next_sibling())
	{
		const char * temp = importElement.name();

		if (!strcmp(temp, "import_valid"))
		{
			temp = importElement.attribute("name").value();
			if (temp)
			{
				strcpy_s(importThunk.name, temp);
			}
			else
			{
				importThunk.name[0] = 0;
			}

			wcscpy_s(importThunk.moduleName, importModuleThunk->moduleName);

			importThunk.suspect = ConvertStringToBool(importElement.attribute("suspect").value());
			importThunk.ordinal = ConvertStringToWord(importElement.attribute("ordinal").value());
			importThunk.hint = ConvertStringToWord(importElement.attribute("hint").value());

			importThunk.valid = true;
		}
		else
		{
			importThunk.valid = false;
			importThunk.suspect = true;
		}

		importThunk.apiAddressVA = ConvertStringToDwordPtr(importElement.attribute("address_va").value());
		importThunk.rva = ConvertStringToDwordPtr(importElement.attribute("iat_rva").value());

		if (importThunk.rva != 0)
		{
			importModuleThunk->thunkList[importThunk.rva] = importThunk;
		}
		
	}
}
