#define _CRT_SECURE_NO_WARNINGS
#define VERSION "0.1"

#include <iostream>
#include <vector>
#include <windows.h>
#include "ar.h"

std::vector<std::string> to_remove_directives{};
std::unique_ptr<std::vector<uint8_t>> ReadFile(const wchar_t* path)
{
	auto* file = _wfopen(path, L"rb");
	if (file == nullptr)
	{
		return {};
	}

	fseek(file, 0, SEEK_END);
	const auto size = ftell(file);
	fseek(file, 0, SEEK_SET);
	std::vector<uint8_t> buffer{};
	buffer.resize(size);
	fread(buffer.data(), 1, size, file);
	fclose(file);

	return std::move(std::make_unique<std::vector<uint8_t>>(std::move(buffer)));
}

void WriteFile(const wchar_t* path, const std::vector<uint8_t>& data)
{
	auto* file = _wfopen(path, L"wb");
	if (file == nullptr)
	{
		return;
	}

	fwrite(data.data(), 1, data.size(), file);
	fclose(file);
}

std::string MultiByteFromWideString(const wchar_t* str)
{
	// Use WideCharToMultiByte
	const auto len = WideCharToMultiByte(CP_UTF8, 0, str, -1, nullptr, 0, nullptr, nullptr);
	std::string result{};
	result.resize(len);
	WideCharToMultiByte(CP_UTF8, 0, str, -1, const_cast<LPSTR>(result.data()), len, nullptr, nullptr);
	return result;
}

void PrintHelp()
{
	printf("Usage: patchlib.exe [options] <lib_path>\n");
	printf("Options:\n");
	printf("  -r <directive>  Directive to remove.\n");
	printf("  -i <lib_path>   Input library path.\n");
	printf("  -o <lib_path>   Output library path. If not specified, input library will be overwritten.\n");
	printf("  -j              Input library is an object file.\n");
	printf("  -h              Print this help.\n");
	printf("  -v              Print version.\n");
}

void PatchObject(void* data)
{
	auto* header = static_cast<IMAGE_FILE_HEADER*>(data);
	const auto* sections = reinterpret_cast<IMAGE_SECTION_HEADER*>(reinterpret_cast<char*>(header + 1) + header->SizeOfOptionalHeader);

	for (int i = 0; i < header->NumberOfSections; i++)
	{
		const auto& section = sections[i];
		if (!memcmp(section.Name, ".drectve", 8))
		{
			void* src_section_data = static_cast<char*>(data) + section.PointerToRawData;

			std::vector<uint8_t> section_data = {};
			section_data.resize(section.SizeOfRawData + 1);
			section_data[section_data.size() - 1] = 0;

			memcpy(section_data.data(), src_section_data, section.SizeOfRawData);

			char* directives = reinterpret_cast<char*>(section_data.data());

			char* token = strtok(directives, " ");
			while (token)
			{
				for (const auto& redir : to_remove_directives)
				{
					if (strstr(token, redir.c_str()))
					{
						const auto len = strlen(token);
						const size_t start = static_cast<size_t>(token - directives);

						memset(static_cast<char*>(src_section_data) + start, ' ', len);
					}
				}

				token = strtok(nullptr, " ");
			}
		}
	}
}

int wmain(int argc, wchar_t** argv)
{
	//const wchar_t* lib_path = LR"(E:\Git\libgens-sonicglvl\depends\hk2012_2_0_r1\Lib\win32_vs2010_noSimd\release\hkBase.lib)";
	const wchar_t* lib_path = nullptr;
	const wchar_t* out_path = nullptr;
	bool is_obj{};

	if (argc == 1)
	{
		PrintHelp();
		return 0;
	}

	for (int i = 1; i < argc; i++)
	{
		if (_wcsnicmp(argv[i], L"-h", 2) == 0)
		{
			PrintHelp();
			return 0;
		}

		if (_wcsnicmp(argv[i], L"-v", 2) == 0)
		{
			printf(VERSION "\n");
			return 0;
		}

		if (_wcsnicmp(argv[i], L"-j", 2) == 0)
		{
			is_obj = true;
			continue;
		}

		if (_wcsnicmp(argv[i], L"-r", 2) == 0 && argc > i + 1)
		{
			i++;
			to_remove_directives.emplace_back(MultiByteFromWideString(argv[i]));
			continue;
		}

		if (_wcsnicmp(argv[i], L"-i", 2) == 0 && argc > i + 1)
		{
			i++;
			lib_path = argv[i];
			continue;
		}

		if (_wcsnicmp(argv[i], L"-o", 2) == 0 && argc > i + 1)
		{
			i++;
			out_path = argv[i];
			continue;
		}

		lib_path = argv[i];
	}

	if (out_path == nullptr)
	{
		out_path = lib_path;
	}

	const auto file = ReadFile(lib_path);
	if (file == nullptr)
	{
		printf("Failed to read file: %ls\n", lib_path);
		return 1;
	}

	if (!is_obj && ArArchive::CheckHeader(file->data(), file->size()))
	{
		const ArArchive archive{ file->data(), file->size() };
		for (const auto& entry : archive)
		{
			// Skip root entries ("/" and "//")
			if ((entry.name[0] == '/' && entry.name[1] == ' ') || (entry.name[0] == '/' && entry.name[1] == '/' && entry.name[2] == ' '))
			{
				continue;
			}

			PatchObject(entry.data());
		}

		WriteFile(out_path, *file);
	}
	else if(is_obj)
	{
		PatchObject(file->data());
		WriteFile(out_path, *file);
	}
	return 0;
}

