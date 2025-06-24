#pragma once
#include <fstream>
#include <string>
#include <cstdint>
#include <map>

class FileProcessing {
private:
	struct FileInfo {
		std::string name;
		std::size_t pos;
		std::size_t size;
		std::string date;
	};

	struct HeaderFileTypeQar {
		char          magic[4];
		std::uint32_t cnt_files;
	};

	struct HeaderFileTypeBar {
		char          magic[4];

		unsigned char year;
		unsigned char month;
		unsigned char day;
		unsigned char padding_07;

		unsigned char ver_major;
		unsigned char ver_minor;
		std::uint16_t cnt_files;
	};

	struct FileEntryInfo {
		std::int32_t unk_00;
		std::int32_t unk_04;
		std::int32_t size;
		std::int32_t unk_0C;
	};

	enum class FileProcessErrorType : int {
		FILEINDEX_OUT_OF_BOUND = -100,
		FILESIZE_OUT_OF_BOUND  = -101,
	};

	std::map<std::string, FileInfo> _info_list;
	std::string                     _out_path;

	std::fstream                    _fs;
	std::size_t                     _fs_sz;

	int _proc_qar(void);
	int _proc_bar(void);

	int _write_output_data(const FileInfo& fi);
	int _load_csv_file_info(const std::string& type);

	int _proc_data(void);

public:
	FileProcessing() : _fs_sz{} {}

	~FileProcessing() {
		if (_fs.is_open()) _fs.close();
	}

	int proc(const std::string& fp, const std::string& out = "");
};