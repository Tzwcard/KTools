#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdint>
#include <vector>
#include <map>

#include "file_proc.h"
#include "fs_helper.hpp"
#include "csv_parse_simple.hpp"

int FileProcessing::_proc_qar(void) {
	if (!_fs.is_open()) return -1;
	_fs.seekg(0, std::ios::beg);

	HeaderFileTypeQar hdr;
	_fs.read(reinterpret_cast<char*>(&hdr), sizeof(HeaderFileTypeQar));

	printf("QAR: TOTAL %d\n",
		hdr.cnt_files
	);

	std::size_t pos_curr = sizeof(hdr), sz_filename = 0x80;
	std::vector<unsigned char> info(sz_filename + sizeof(FileEntryInfo));

	for (std::size_t index = 0; index < hdr.cnt_files; index++) {
		if (info.size() + pos_curr >= _fs_sz) {
			printf("File #%4zd info out of bound\n", index + 1);
			_info_list.clear();
			return static_cast<int>(FileProcessErrorType::FILEINDEX_OUT_OF_BOUND);
		}

		_fs.read(reinterpret_cast<char*>(info.data()), info.size());

		std::string fn(reinterpret_cast<char*>(info.data()));
		FileEntryInfo* fi = reinterpret_cast<FileEntryInfo*>(info.data() + sz_filename);

		if (pos_curr + fi->size >= _fs_sz) {
			printf("File #%4zd size out of bound\n", index + 1);
			_info_list.clear();
			return static_cast<int>(FileProcessErrorType::FILESIZE_OUT_OF_BOUND);
		}

		std::string fn_formatted = fn;
		std::replace(fn_formatted.begin(), fn_formatted.end(), '\\', '/');

		_info_list.insert_or_assign(
			fn,
			FileInfo{
				fn_formatted,
				pos_curr + info.size(),
				static_cast<std::size_t>(fi->size)
			}
		);

		pos_curr += info.size() + fi->size;
		_fs.seekg(pos_curr, std::ios::beg);
	}

	return hdr.cnt_files;
}

int FileProcessing::_proc_bar(void) {
	if (!_fs.is_open()) return -1;
	_fs.seekg(0, std::ios::beg);

	HeaderFileTypeBar hdr;
	_fs.read(reinterpret_cast<char*>(&hdr), sizeof(HeaderFileTypeBar));

	int year = hdr.year;
	if (!memcmp(hdr.magic, "LDJA", 4)) {
		if (0x700 + year < 1998)
			year += 0x800;
		else
			year += 0x700;
	}
	else if (!memcmp(hdr.magic, "MBRA", 4) || !memcmp(hdr.magic, "M39A", 4)) {
		year += 2000;
	}

	printf("BAR: TYPE '%.4s', DATE %04d-%02d-%02d, TOTAL %d\n",
		hdr.magic,
		year, hdr.month, hdr.day,
		hdr.cnt_files
	);

	std::size_t pos_curr = sizeof(hdr), sz_filename = 0x100;
	if (!memcmp(hdr.magic, "M39A", 4)) {
		sz_filename = 0xFC;
	}

	std::vector<unsigned char> info(sz_filename + sizeof(FileEntryInfo));

	for (std::size_t index = 0; index < hdr.cnt_files; index++) {
		if (info.size() + pos_curr >= _fs_sz) {
			printf("File #%4zd info out of bound\n", index + 1);
			_info_list.clear();
			return static_cast<int>(FileProcessErrorType::FILEINDEX_OUT_OF_BOUND);
		}

		_fs.read(reinterpret_cast<char*>(info.data()), info.size());

		std::string fn(reinterpret_cast<char*>(info.data()));
		FileEntryInfo* fi = reinterpret_cast<FileEntryInfo*>(info.data() + sz_filename);

		if (pos_curr + fi->size >= _fs_sz) {
			printf("File #%4zd size out of bound\n", index + 1);
			_info_list.clear();
			return static_cast<int>(FileProcessErrorType::FILESIZE_OUT_OF_BOUND);
		}

		std::string fn_formatted = fn;
		std::replace(fn_formatted.begin(), fn_formatted.end(), '\\', '/');

		_info_list.insert_or_assign(
			fn,
			FileInfo{
				fn_formatted,
				pos_curr + info.size(),
				static_cast<std::size_t>(fi->size)
			}
		);

		pos_curr += info.size() + fi->size;
		_fs.seekg(pos_curr, std::ios::beg);
	}

	return hdr.cnt_files;
}

int FileProcessing::_write_output_data(
	const FileInfo& fi
) {
	if (!_fs.is_open()) return -1;
	if (fi.size == 0) return -2;

	auto filepath = fs_helper_gen_filepath(
		_out_path,
		fi.name
	);

	fs_helper_create_parent_dir_for_path(filepath);
	std::fstream fout(filepath, std::ios::binary | std::ios::trunc | std::ios::out);
	_fs.seekg(fi.pos, std::ios::beg);

	constexpr std::size_t sz_buf = 4096;
	char _tmp[4096];
	std::size_t size = fi.size;

	while (size && _fs) {
		_fs.read(_tmp, std::min(size, sz_buf));
		std::streamsize rd = _fs.gcount();

		if (rd <= 0) break;

		fout.write(_tmp, rd);
		size -= static_cast<std::size_t>(rd);
	}

	fout.close();
	if (size) {
		printf("Read file failed\n");
		return -3;
	}

	if (fi.date.length()) {
		fs_helper_try_set_file_mtime(
			filepath,
			fi.date
		);
	}

	return 1;
}

int FileProcessing::_load_csv_file_info(const std::string &type) {
	if (!type.length()) return 0;

	std::string fn;
	for (auto const& f : _info_list) {
		if (f.first.find(type) != std::string::npos) {
			fn = f.first;
			break;
		}
	}
	if (!fn.length()) return 0;

	auto csv_info = _info_list.find(fn);
	_fs.seekg(csv_info->second.pos, std::ios::beg);
	std::vector<char> buffer(csv_info->second.size + 1, 0);
	_fs.read(buffer.data(), csv_info->second.size);

	std::string csv_data(buffer.data(), csv_info->second.size);

	std::istringstream ss(csv_data);
	std::string line;

	while (std::getline(ss, line)) {
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		auto fields = csv_parse_simple_row(line);
		std::replace(fields[1].begin(), fields[1].end(), '\\', '/');

		for (auto& fi : _info_list) {
			if (fi.second.name.find(fields[1]) != std::string::npos) {
				fi.second.name = fields[1];
				fi.second.date = fields[4];
				break;
			}
		}
	}
	return 1;
}

int FileProcessing::_proc_data(void) {
	if (!_fs.is_open()) return -1;

	int idx = 0;
	unsigned char magic[8];

	if (_fs_sz < 8) return -2;

	_fs.seekg(0, std::ios::beg);
	_fs.read(reinterpret_cast<char*>(magic), sizeof(magic));

	if (!memcmp(magic, "QAR", 4)) {
		_proc_qar();
		_load_csv_file_info(".qsv");
	}
	else {
		_proc_bar();
		_load_csv_file_info(".bsv");
	}

	for (auto const& f : _info_list) {
		idx++;

		printf("File #%4d: POS [%016zx:%016zx], NAME '%s', DATE '%s'\n",
			idx,
			f.second.pos,
			f.second.size,
			f.first.c_str(),
			f.second.date.c_str()
		);
		if (_write_output_data(f.second) < 0) {
			printf("File write error!\n");
			break;
		}
	}

	return idx;
}

int FileProcessing::proc(
	const std::string& fp,
	const std::string& out
) {
	_out_path = out;
	if (_out_path == "") {
		_out_path = fs_helper_get_output_path_str(fp);
	}

	if (_fs.is_open()) _fs.close();

	_fs_sz = 0;
	_fs = std::fstream(fp, std::ios::in | std::ios::ate | std::ios::binary);
	if (!_fs.is_open()) return -1;

	_fs_sz = static_cast<std::size_t>(_fs.tellg());
	_fs.seekg(0, std::ios::beg);

	int ret = _proc_data();

	_fs.close();
	return ret;
}