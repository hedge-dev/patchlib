#pragma once
#include <string>
#include <vector>

namespace gnu
{
	struct ar_file
	{
		char name[16];
		char date[12];
		char uid[6];
		char gid[6];
		char mode[8];
		char size[10];
		char fmag[2];

		void* data() const
		{
			return const_cast<void*>(reinterpret_cast<const void*>(this + 1));
		}
	};
}

class ArArchive
{
public:
	struct iterator;
	void* data{};
	size_t size{};

	ArArchive(void* data, size_t size) : data(data), size(size)
	{
		
	}

	static bool CheckHeader(const void* data, size_t size)
	{
		return size >= sizeof(gnu::ar_file) && !memcmp(data, "!<arch>\xA", 8);
	}

	bool CheckHeader() const
	{
		return CheckHeader(data, size);
	}

	iterator begin() const
	{
		return {this, reinterpret_cast<gnu::ar_file*>(static_cast<char*>(data) + 8), 8};
	}

	iterator end() const
	{
		return { this, reinterpret_cast<gnu::ar_file*>(static_cast<char*>(data) + size), size };
	}

	struct iterator
	{
		const ArArchive* archive{};
		gnu::ar_file* file{};
		size_t offset{};

		iterator(const ArArchive* archive, gnu::ar_file* file, size_t offset) : archive(archive), file(file), offset(offset)
		{
		}

		iterator& operator++()
		{
			offset += sizeof(gnu::ar_file) + atoi(file->size);
			if (offset % 2 == 1)
			{
				offset += 1; // padding (if needed)
			}

			file = reinterpret_cast<gnu::ar_file*>(static_cast<char*>(archive->data) + offset);
			return *this;
		}

		bool operator!=(const iterator& other) const
		{
			return file != other.file;
		}

		gnu::ar_file& operator*() const
		{
			return *file;
		}
	};
};