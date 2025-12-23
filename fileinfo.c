#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include "file_manager.h"

//COLORS & ICONS
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"
#define DIM     "\033[2m"

// Hàm lấy Icon dựa trên tên file
const char* get_file_icon(const char *filename, mode_t mode) {
    if (S_ISDIR(mode)) return "📂";
    if (S_ISLNK(mode)) return "🔗";
    if (S_ISCHR(mode) || S_ISBLK(mode)) return "⚙️";
    
    const char *ext = strrchr(filename, '.');
    if (!ext) return "📄";

    if (strcmp(ext, ".c") == 0) return "🇨 ";
    if (strcmp(ext, ".h") == 0) return "🏷️";
    if (strcmp(ext, ".txt") == 0) return "📝";
    if (strcmp(ext, ".pdf") == 0) return "📕";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".png") == 0) return "🖼️";
    if (strcmp(ext, ".exe") == 0 || strcmp(ext, ".sh") == 0) return "🚀";
    if (strcmp(ext, ".zip") == 0 || strcmp(ext, ".tar") == 0) return "📦";
    
    return "📄";
}

//Xử lý ký tự đại diện (Wildcard)
char **expand_pattern(const char *pattern, int *count) {
    glob_t results;
    *count = 0;

    if (glob(pattern, 0, NULL, &results) != 0)
        return NULL;

    *count = results.gl_pathc;
    if (*count == 0) {
        globfree(&results);
        return NULL;
    }

    char **files = malloc(sizeof(char*) * (*count));
    if (!files) {
        globfree(&results);
        return NULL;
    }

    for (size_t i = 0; i < results.gl_pathc; i++) {
        files[i] = strdup(results.gl_pathv[i]);
        if (!files[i]) {
            for (size_t j = 0; j < i; j++) free(files[j]);
            free(files);
            globfree(&results);
            return NULL;
        }
    }

    globfree(&results);
    return files;
}

int file_exists(const char *path) {
    struct stat s;
    return (lstat(path, &s) == 0);
}

char **collect_files(int argc, char *argv[], int *count) {
    char **result = NULL;
    int total = 0;

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];

        if (strchr(arg, '*') || strchr(arg, '?')) {
            int n = 0;
            char **expanded = expand_pattern(arg, &n);
            if (expanded) {
                char **temp = realloc(result, sizeof(char*) * (total + n));
                if (!temp) {
                    for (int j = 0; j < total; j++) free(result[j]);
                    free(result);
                    for (int j = 0; j < n; j++) free(expanded[j]);
                    free(expanded);
                    *count = 0;
                    return NULL;
                }
                result = temp;
                for (int j = 0; j < n; j++)
                    result[total + j] = expanded[j];
                total += n;
                free(expanded);
            }
        } else {
            char **temp = realloc(result, sizeof(char*) * (total + 1));
            if (!temp) {
                for (int j = 0; j < total; j++) free(result[j]);
                free(result);
                *count = 0;
                return NULL;
            }
            result = temp;
            result[total] = strdup(arg);
            if (!result[total]) {
                for (int j = 0; j < total; j++) free(result[j]);
                free(result);
                *count = 0;
                return NULL;
            }
            total++;
        }
    }

    *count = total;
    return result;
}

//FORMAT TIME 
char *format_time(time_t t) {
    static char buf[64];
    struct tm lt;

    localtime_r(&t, &lt);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &lt);
    return buf;
}

//Chuyển đổi byte sang MB
char *human_size(off_t bytes) {
    static char buf[64];
    double size = (double)bytes;

    if (size < 1024) {
        snprintf(buf, sizeof(buf), "%ld bytes", (long)bytes);
    } 
    else if (size < 1024 * 1024) {
        snprintf(buf, sizeof(buf), "%.1f KB", size / 1024.0);
    } 
    else if (size < 1024 * 1024 * 1024) {
        snprintf(buf, sizeof(buf), "%.1f MB", size / (1024.0 * 1024.0));
    } 
    else {
        snprintf(buf, sizeof(buf), "%.2f GB", size / (1024.0 * 1024.0 * 1024.0));
    }
    return buf;
}

void print_colored_perms(const char *perm_str) {
    for (int i = 0; i < 9; i++) {
        if (perm_str[i] == 'r') printf("%s%c", YELLOW, perm_str[i]);
        else if (perm_str[i] == 'w') printf("%s%c", RED, perm_str[i]);
        else if (perm_str[i] == 'x') printf("%s%c", GREEN, perm_str[i]);
        else printf("%s%c", RESET, perm_str[i]);
    }
    printf("%s", RESET);
}

//vẽ giao diện ra màn hình
void print_file_info(struct file_info *info) {
    char type_str[50];
    char perm_str[10];

    determine_file_type(info->mode, type_str);
    convert_permissions(info->mode, perm_str);

    const char *title_color = GREEN;
    if (S_ISDIR(info->mode)) title_color = BLUE;
    if (S_ISLNK(info->mode)) title_color = CYAN;
    if (info->is_executable) title_color = MAGENTA;

    const char *icon = get_file_icon(info->filename, info->mode);

    printf("\n%s%s %s%s%s\n", title_color, icon, BOLD, info->filename, RESET);
    
    printf("  ├─ %sType:%s   %s", BOLD, RESET, type_str);
    printf(" ("); print_colored_perms(perm_str); printf(")\n");
    
    const char *size_color = (info->size > 1024*1024) ? RED : GREEN;
    printf("  ├─ %sSize:%s   %s%s%s (%ld bytes)", 
           BOLD, RESET, size_color, human_size(info->size), RESET, (long)info->size);
    
    if (info->is_sparse) {
        printf(" %s[SPARSE]%s", CYAN, RESET);
    }
    printf("\n");
    
    printf("  ├─ %sOwner:%s  %s%s:%s%s", 
           BOLD, RESET, CYAN, info->username, info->groupname, RESET);
    printf(" (UID:%d GID:%d)\n", info->uid, info->gid);
    
    printf("  ├─ %sStorage:%s\n", BOLD, RESET);
    printf("  │  ├─ Blocks: %s%ld%s × 512 bytes\n", 
           YELLOW, (long)info->blocks, RESET);
    printf("  │  ├─ Block Size: %s%ld%s bytes (optimal I/O)\n", 
           YELLOW, (long)info->block_size, RESET);
    printf("  │  └─ Actual Disk Usage: %s%s%s\n", 
           GREEN, human_size(info->blocks * 512), RESET);
    
    printf("  ├─ %sLinks:%s  %ld   %sInode:%s %ld\n", 
           BOLD, RESET, (long)info->nlink, BOLD, RESET, (long)info->inode);

    if (S_ISCHR(info->mode) || S_ISBLK(info->mode)) {
        printf("  ├─ %sDevice Numbers:%s Major=%u, Minor=%u\n",
               BOLD, RESET, info->major_dev, info->minor_dev);
    }
    
    printf("  ├─ %sDevice ID:%s %ld\n", BOLD, RESET, (long)info->device);

    printf("  ├─ %sTimes:%s\n", BOLD, RESET);
    printf("  │  ├─ Access: %s%s%s\n", YELLOW, format_time(info->atime), RESET);
    printf("  │  ├─ Modify: %s%s%s\n", BLUE, format_time(info->mtime), RESET);
    printf("  │  └─ Change: %s\n", format_time(info->ctime));

    printf("  ├─ %sMIME Type:%s %s%s%s\n", 
           BOLD, RESET, CYAN, info->mime_type, RESET);
    
    if (info->symlink_target[0] != '\0') {
        printf("  ├─ %sSymlink Target:%s %s→ %s%s\n", 
               BOLD, RESET, YELLOW, info->symlink_target, RESET);
    }
    
    if (strcmp(info->md5_hash, "N/A") != 0 && strcmp(info->md5_hash, "Error") != 0) {
        size_t hash_len = strlen(info->md5_hash);
        const char *hash_type = (hash_len > 40) ? "SHA256" : "MD5";
        
        printf("  ├─ %s%s Hash:%s %s%s%s\n", 
               BOLD, hash_type, RESET, DIM, info->md5_hash, RESET);
    }
    
    printf("  └─ %sFlags:%s", BOLD, RESET);
    if (info->is_executable) printf(" %s[EXEC]%s", MAGENTA, RESET);
    if (info->has_xattr) printf(" %s[XATTR]%s", YELLOW, RESET);
    if (info->is_sparse) printf(" %s[SPARSE]%s", CYAN, RESET);
    printf("\n");
    
    printf("─────────────────────────────────────────────────────────\n");
}

//PROCESS FILE
void process_file(const char *filename) {
    struct file_info info;
    memset(&info, 0, sizeof(info));

    if (collect_file_stats(filename, &info) != 0) {
        printf("❌ Lỗi: %s → %s\n", filename, info.error_msg);
        return;
    }

    get_user_group_info(&info);
    print_file_info(&info);
}

//SORT STRINGS 
int cmp_str(const void *a, const void *b) {
    char * const *sa = a;
    char * const *sb = b;
    return strcmp(*sa, *sb);
}

//MAIN: Kiểm tra tham số đầu vào, Chạy vòng lặp qua từng file và gọi process_file để in thông tin.
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Cách dùng: ./fileinfo <file1> <file2> *.txt ...\n");
        printf("\n%sVí dụ với files thông thường:%s\n", BOLD, RESET);
        printf("  ./fileinfo file.txt\n");
        printf("  ./fileinfo *.c *.h\n");
        printf("  ./fileinfo test_file/*\n");
        
        printf("\n%sVí dụ với Device Files:%s\n", BOLD, RESET);
        printf("  ./fileinfo /dev/null /dev/zero /dev/random\n");
        printf("  ./fileinfo /dev/sda /dev/sda1\n");
        printf("  ./fileinfo /dev/tty /dev/pts/0\n");
        
        printf("\n%sVí dụ với System Files:%s\n", BOLD, RESET);
        printf("  ./fileinfo /proc/cpuinfo /proc/meminfo\n");
        printf("  ./fileinfo /sys/class/net/eth0/address\n");
        printf("  ./fileinfo /etc/passwd /etc/group\n");
        
        printf("\n%sVí dụ với Symbolic Links:%s\n", BOLD, RESET);
        printf("  ./fileinfo /bin/sh /usr/bin/python\n");
        printf("  ./fileinfo /lib64/ld-linux-x86-64.so.2\n");
        
        printf("\n%sTip:%s Dùng sudo để xem device files đặc biệt:\n", BOLD, RESET);
        printf("  sudo ./fileinfo /dev/sda /dev/nvme0n1\n");
        return 1;
    }

    int count = 0;
    char **list = collect_files(argc, argv, &count);

    if (!list || count == 0) {
        printf("Không có file nào để xử lý.\n");
        return 0;
    }

    char **valid = NULL;
    int valid_count = 0;

    for (int i = 0; i < count; i++) {
        if (file_exists(list[i])) {
            char **temp = realloc(valid, sizeof(char*) * (valid_count + 1));
            if (!temp) {
                for (int j = 0; j < valid_count; j++) free(valid[j]);
                free(valid);
                for (int j = 0; j < count; j++) free(list[j]);
                free(list);
                return 1;
            }
            valid = temp;
            valid[valid_count++] = list[i];
        } else {
            printf("⚠ Warning: File không tồn tại → %s\n", list[i]);
            free(list[i]);
        }
    }
    free(list);

    if (valid_count == 0) {
        printf("Không có file hợp lệ.\n");
        free(valid);
        return 0;
    }

    qsort(valid, valid_count, sizeof(char*), cmp_str);

    for (int i = 0; i < valid_count; i++) {
        printf("\n🔍 Đang xử lý file %d/%d: %s%s%s\n", 
               i + 1, valid_count, CYAN, valid[i], RESET);
        process_file(valid[i]);
        free(valid[i]);
    }

    free(valid);

    printf("\n Đã xử lý xong %s%d%s file.\n", GREEN, valid_count, RESET);
    return 0;
}
