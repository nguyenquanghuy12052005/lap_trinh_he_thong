#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

// Định nghĩa kích thước
#define MAX_FILENAME 256
#define MAX_USERNAME 32
#define MAX_GROUPNAME 32
#define MAX_ERROR_MSG 128
#define MAX_SYMLINK_TARGET 512
#define MAX_MIMETYPE 64
#define MAX_MD5_HASH 33

// Cấu trúc lưu trữ thông tin file
struct file_info {
    char filename[MAX_FILENAME];
    mode_t mode;
    nlink_t nlink;
    off_t size;
    ino_t inode;
    dev_t device;
    time_t atime;
    time_t mtime;
    time_t ctime;
    uid_t uid;
    gid_t gid;
    char username[MAX_USERNAME];
    char groupname[MAX_GROUPNAME];
    char error_msg[MAX_ERROR_MSG];
    
    blksize_t block_size;
    blkcnt_t blocks;
    dev_t rdev;
    unsigned int major_dev;
    unsigned int minor_dev;
    
    char symlink_target[MAX_SYMLINK_TARGET];
    char mime_type[MAX_MIMETYPE];
    char md5_hash[MAX_MD5_HASH];
    
    int has_xattr;
    int is_sparse;
    int is_executable;
};

// Khai báo các hàm
int collect_file_stats(const char *filename, struct file_info *info);
int get_user_group_info(struct file_info *info);
void determine_file_type(mode_t mode, char *type_str);
void convert_permissions(mode_t mode, char *perm_str);
int get_symlink_target(const char *filename, char *target, size_t size);
int detect_mime_type(const char *filename, mode_t mode, char *mime_type, size_t size);
int calculate_md5(const char *filename, char *md5_str, size_t max_size);
int check_extended_attributes(const char *filename);
int is_sparse_file(off_t size, blkcnt_t blocks);

#endif