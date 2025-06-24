#include <iostream>
#include <chrono>
#include <string>
#include <algorithm>
#include <filesystem>

std::string fs_helper_gen_filepath(
    const std::string& outdir,
    const std::string& _in
) {
    // Normalize in
    std::filesystem::path in = std::filesystem::path(_in),
        p = in.lexically_normal();
    if (p.is_absolute())
        p = p.relative_path();

    // Absolute outdir to base
    std::filesystem::path base = std::filesystem::path(outdir);
    if (!base.is_absolute())
        base = std::filesystem::absolute(base);
    base = base.lexically_normal();

    // Build path
    std::filesystem::path out = std::filesystem::absolute(base.string() + "/" + p.string());

    // On Windows, convert to backslashes
    out.lexically_normal().make_preferred();

    return out.string();
}

void fs_helper_create_parent_dir_for_path(
    const std::string& _file_path
) {
    // extract just the directory part (everything up to the last slash)
    std::filesystem::path file_path = std::filesystem::path(_file_path),
        dir = file_path.parent_path();

    // if there's something to create, do it (creates intermediates too)
    if (!dir.empty())
        std::filesystem::create_directories(dir);
}

std::string fs_helper_get_output_path_str(const std::string& input_file) {
    std::filesystem::path abs_path = std::filesystem::absolute(input_file);
    std::filesystem::path dir = abs_path.parent_path();
    std::string name = abs_path.stem().string();
    std::string ext = abs_path.extension().string();

    std::string suffix;
    if (!ext.empty()) {
        // if extension exists
        suffix = "";
    }
    else {
        // if no extension, use "_out"
        suffix = "_out";
    }

    std::filesystem::path out_dir = dir / (name + suffix);
    return out_dir.make_preferred().string();
}

void fs_helper_try_set_file_mtime(
    const std::string& path,
    const std::string& date
) {
    if (date.empty()) return;

    int y, m, d, h, i;
    constexpr int UTC_PLUS_9_SECONDS = 9 * 3600;
    constexpr std::chrono::seconds windows_to_unix_epoch_diff{ 11644473600 };

    if (sscanf_s(date.c_str(), "%d/%d/%d %d:%d", &y, &m, &d, &h, &i) == 5) {
        std::tm t = {};
        t.tm_year = y - 1900;
        t.tm_mon = m - 1;
        t.tm_mday = d;
        t.tm_hour = h;
        t.tm_min = i;

#ifdef _WIN32
        std::time_t tt = _mkgmtime(&t) - UTC_PLUS_9_SECONDS;
#else
        std::time_t tt = timegm(&t) - UTC_PLUS_9_SECONDS;
#endif

        if (tt > 0) {
            auto sys_time = std::chrono::system_clock::from_time_t(tt);
            auto duration = sys_time.time_since_epoch();

#ifdef _WIN32
            duration += std::chrono::duration_cast<std::chrono::system_clock::duration>(windows_to_unix_epoch_diff);
#endif
            std::filesystem::last_write_time(
                std::filesystem::path(path),
                std::filesystem::file_time_type{ std::chrono::duration_cast<std::filesystem::file_time_type::duration>(duration) }
            );
        }
    }
}