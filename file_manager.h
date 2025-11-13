//khai báo

#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <time.h>

#define MAX_FILENAME 256
#define MAX_USERNAME 32
#define MAX_GROUPNAME 32
#define MAX_ERROR_MSG 128

// Cấu trúc lưu trữ thông tin file
struct file_info {
    char filename[MAX_FILENAME];  // Tên file
    mode_t mode;                  // Loại file và permissions
    nlink_t nlink;                // Số lượng links
    off_t size;                   // Kích thước file
    ino_t inode;                  // Số inode
    dev_t device;                 // ID thiết bị
    time_t atime;                 // Thời gian truy cập cuối
    time_t mtime;                 // Thời gian sửa đổi cuối
    time_t ctime;                 // Thời gian thay đổi metadata
    uid_t uid;                    // User ID
    gid_t gid;                    // Group ID
    char username[MAX_USERNAME];  // Tên user
    char groupname[MAX_GROUPNAME]; // Tên group
    char error_msg[MAX_ERROR_MSG]; // Thông báo lỗi
};

// KHAI BÁO CÁC HÀM CỦA THÀNH VIÊN 2
int collect_file_stats(const char *filename, struct file_info *info);
int get_user_group_info(struct file_info *info);
void determine_file_type(mode_t mode, char *type_str);
void convert_permissions(mode_t mode, char *perm_str);

#endif