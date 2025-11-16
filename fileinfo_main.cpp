#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <glob.h>
#include <sys/stat.h>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "file_manager.h"

using namespace std;

/* ====================== WILDCARD EXPAND ====================== */

vector<string> expand_pattern(const string& pattern) {
    glob_t results;
    vector<string> files;

    if (glob(pattern.c_str(), 0, NULL, &results) == 0) {
        for (size_t i = 0; i < results.gl_pathc; i++)
            files.push_back(results.gl_pathv[i]);
    }
    globfree(&results);
    return files;
}

bool file_exists(const string& path) {
    struct stat s;
    return (lstat(path.c_str(), &s) == 0);
}

vector<string> collect_files(int argc, char* argv[]) {
    vector<string> all;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];

        if (arg.find('*') != string::npos || arg.find('?') != string::npos) {
            auto expanded = expand_pattern(arg);
            all.insert(all.end(), expanded.begin(), expanded.end());
        } else {
            all.push_back(arg);
        }
    }
    return all;
}

/* ====================== FORMAT TIME ====================== */

string format_time(time_t t) {
    struct tm lt;
    localtime_r(&t, &lt);

    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &lt);
    return string(buf);
}

/* ====================== FORMAT SIZE ====================== */

string human_size(off_t bytes) {
    char buf[64];
    if (bytes < 1024)
        snprintf(buf, sizeof(buf), "%ld bytes", bytes);
    else
        snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
    return string(buf);
}

/* ====================== PRINT FILE INFO — PHẦN 3 ====================== */

void print_file_info(struct file_info* info) {
    char type_str[50];
    char perm_str[10];

    determine_file_type(info->mode, type_str);
    convert_permissions(info->mode, perm_str);

    cout << "\n📁 FILE: " << info->filename << "\n";
    cout << "├─ Type: " << type_str << " (" << perm_str << ")\n";
    cout << "├─ Size: " << human_size(info->size)
         << " (" << info->size << " bytes)\n";
    cout << "├─ Owner: " << info->username << ":" << info->groupname << "\n";
    cout << "├─ Links: " << info->nlink << "\n";
    cout << "├─ Inode: " << info->inode << "\n";
    cout << "├─ Times:\n";
    cout << "│  ├─ Accessed: "  << format_time(info->atime) << "\n";
    cout << "│  ├─ Modified: "  << format_time(info->mtime) << "\n";
    cout << "│  └─ Changed: "   << format_time(info->ctime) << "\n";
    cout << "└─ Device: " << info->device << "\n\n";
}

/* ====================== PROCESS FILE — KẾT NỐI 3 PHẦN ====================== */

void process_file(const string& filename) {
    struct file_info info;

    // PHẦN 2: Thu thập dữ liệu
    if (collect_file_stats(filename.c_str(), &info) != 0) {
        cout << "❌ Lỗi: " << filename << " → " << info.error_msg << "\n";
        return;
    }

    // PHẦN 2: Lấy user/group
    get_user_group_info(&info);

    // PHẦN 3: Format đẹp và in ra
    print_file_info(&info);
}

/* ====================== MAIN ====================== */

int main(int argc, char* argv[]) {

    if (argc < 2) {
        cout << "Cách dùng: ./fileinfo <file1> <file2> *.txt ...\n";
        return 1;
    }

    vector<string> list = collect_files(argc, argv);
    vector<string> valid;

    for (auto& f : list) {
        if (file_exists(f)) {
            valid.push_back(f);
        } else {
            cout << "⚠ Warning: File không tồn tại → " << f << "\n";
        }
    }

    if (valid.empty()) {
        cout << "Không có file hợp lệ.\n";
        return 0;
    }

    sort(valid.begin(), valid.end());

    int total = valid.size();

    for (int i = 0; i < total; i++) {
        cout << "🔍 Đang xử lý file " << (i + 1) << "/" << total
             << ": " << valid[i] << "\n";

        process_file(valid[i]);
    }

    cout << "\n✅ Đã xử lý xong " << total << " file.\n";
    return 0;
}
