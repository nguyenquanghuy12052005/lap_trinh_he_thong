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

/* ====================== WILDCARD EXPAND ====================== */

char **expand_pattern(const char *pattern, int *count) {
    glob_t results;
    *count = 0;

    if (glob(pattern, 0, NULL, &results) != 0)
        return NULL;

    *count = results.gl_pathc;
    if (*count == 0)
        return NULL;

    char **files = malloc(sizeof(char*) * (*count));
    for (size_t i = 0; i < results.gl_pathc; i++) {
        files[i] = strdup(results.gl_pathv[i]);
    }

    globfree(&results);
    return files;
}

int file_exists(const char *path) {
    struct stat s;
    return (lstat(path, &s) == 0);
}

/* Thu thập các file từ argv */
char **collect_files(int argc, char *argv[], int *count) {
    char **result = NULL;
    int total = 0;

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];

        if (strchr(arg, '*') || strchr(arg, '?')) {
            int n = 0;
            char **expanded = expand_pattern(arg, &n);
            if (expanded) {
                result = realloc(result, sizeof(char*) * (total + n));
                for (int j = 0; j < n; j++)
                    result[total + j] = expanded[j];
                total += n;
                free(expanded);
            }
        } else {
            result = realloc(result, sizeof(char*) * (total + 1));
            result[total++] = strdup(arg);
        }
    }

    *count = total;
    return result;
}

/* ====================== FORMAT TIME ====================== */

char *format_time(time_t t) {
    static char buf[64];
    struct tm lt;

    localtime_r(&t, &lt);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &lt);
    return buf;
}

/* ====================== FORMAT SIZE ====================== */

char *human_size(off_t bytes) {
    static char buf[64];
    if (bytes < 1024)
        snprintf(buf, sizeof(buf), "%ld bytes", (long)bytes);
    else
        snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
    return buf;
}

/* ====================== PRINT FILE INFO ====================== */

void print_file_info(struct file_info *info) {
    char type_str[50];
    char perm_str[10];

    determine_file_type(info->mode, type_str);
    convert_permissions(info->mode, perm_str);

    printf("\n📁 FILE: %s\n", info->filename);
    printf("├─ Type: %s (%s)\n", type_str, perm_str);
    printf("├─ Size: %s (%ld bytes)\n", human_size(info->size), (long)info->size);
    printf("├─ Owner: %s:%s\n", info->username, info->groupname);
    printf("├─ Links: %ld\n", (long)info->nlink);
    printf("├─ Inode: %ld\n", (long)info->inode);

    printf("├─ Times:\n");
    printf("│  ├─ Accessed:  %s\n", format_time(info->atime));
    printf("│  ├─ Modified:  %s\n", format_time(info->mtime));
    printf("│  └─ Changed:   %s\n", format_time(info->ctime));

    printf("└─ Device: %ld\n\n", (long)info->device);
}

/* ====================== PROCESS FILE ====================== */

void process_file(const char *filename) {
    struct file_info info;

    if (collect_file_stats(filename, &info) != 0) {
        printf("❌ Lỗi: %s → %s\n", filename, info.error_msg);
        return;
    }

    get_user_group_info(&info);
    print_file_info(&info);
}

/* ====================== SORT STRINGS ====================== */
int cmp_str(const void *a, const void *b) {
    char * const *sa = a;
    char * const *sb = b;
    return strcmp(*sa, *sb);
}

/* ====================== MAIN ====================== */

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Cách dùng: ./fileinfo <file1> <file2> *.txt ...\n");
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
            valid = realloc(valid, sizeof(char*) * (valid_count + 1));
            valid[valid_count++] = list[i];
        } else {
            printf("⚠ Warning: File không tồn tại → %s\n", list[i]);
            free(list[i]);
        }
    }
    free(list);

    if (valid_count == 0) {
        printf("Không có file hợp lệ.\n");
        return 0;
    }

    qsort(valid, valid_count, sizeof(char*), cmp_str);

    for (int i = 0; i < valid_count; i++) {
        printf("🔍 Đang xử lý file %d/%d: %s\n", i + 1, valid_count, valid[i]);
        process_file(valid[i]);
        free(valid[i]);
    }

    free(valid);

    printf("\n✅ Đã xử lý xong %d file.\n", valid_count);
    return 0;
}
