//xử lý hàm 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include "file_manager.h"

// Thu thập thông tin file từ hệ thống
int collect_file_stats(const char *filename, struct file_info *info) {
    struct stat file_stat;  // lưu info từ hệ thống
    
    // Gọi hàm lstat() để lấy thông tin file  0 true | -1 false
    if (lstat(filename, &file_stat) == -1) {
        //ghi thông báo lỗi vào struct
        snprintf(info->error_msg, sizeof(info->error_msg), 
                 "Error: %s", strerror(errno));
        return -1;  // Trả về -1 để báo lỗi
    }
    
    // Copy thông tin từ struct stat vào struct file_info
    strncpy(info->filename, filename, sizeof(info->filename)-1);
    info->mode = file_stat.st_mode;     // Loại file và permissions
    info->nlink = file_stat.st_nlink;   // Số lượng links
    info->size = file_stat.st_size;     // Kích thước file (bytes)
    info->inode = file_stat.st_ino;     // Số inode
    info->device = file_stat.st_dev;    // ID thiết bị
    info->atime = file_stat.st_atime;   // Thời gian truy cập cuối
    info->mtime = file_stat.st_mtime;   // Thời gian sửa đổi cuối  
    info->ctime = file_stat.st_ctime;   // Thời gian thay đổi metadata
    info->uid = file_stat.st_uid;       // User ID
    info->gid = file_stat.st_gid;       // Group ID
    
    return 0;  
}

//Lấy username và groupname từ ID
int get_user_group_info(struct file_info *info) {
    struct passwd *pwd = getpwuid(info->uid);    //tìm tên uid
    if (pwd) {
        // Chuyển uid thành username
        strncpy(info->username, pwd->pw_name, sizeof(info->username)-1);
    } else {
        //  không tìm thấy user, dùng số ID
        snprintf(info->username, sizeof(info->username), "%d", info->uid);
    }
    
    struct group *grp = getgrgid(info->gid);     //tìm tên gid
    if (grp) {
        // Chuyển gid thành groupname
        strncpy(info->groupname, grp->gr_name, sizeof(info->groupname)-1);
    } else {
        snprintf(info->groupname, sizeof(info->groupname), "%d", info->gid);
    }
    
    return 0;
}

// Xác định loại file
void determine_file_type(mode_t mode, char *type_str) {
   
    //các hàm kiểm tra
    if (S_ISREG(mode))  //.txt, .docx, .jpg,.png,.c, .py,.exe
        strcpy(type_str, "Regular File");
    
    else if (S_ISDIR(mode)) //folder
        strcpy(type_str, "Directory");
   
    else if (S_ISLNK(mode)) //chứa link
        strcpy(type_str, "Symbolic Link");
   
    else if (S_ISCHR(mode))
        strcpy(type_str, "Character Device");
   
    else if (S_ISBLK(mode))//các thiết bị đọc ghi vd  Ổ đĩa cứng (Hard drive), USB drive, CD-ROM.
        strcpy(type_str, "Block Device");

    else if (S_ISFIFO(mode))  //(First-In, First-Out / Ống dẫn)
        strcpy(type_str, "FIFO/Pipe");
   
    else if (S_ISSOCK(mode))
        strcpy(type_str, "Socket");
    else
        strcpy(type_str, "Unknown Type");
}

//Chuyển permissions sang dạng "rwxrwxrwx"
void convert_permissions(mode_t mode, char *perm_str) {
    // User permissions (chủ sở hữu)
    perm_str[0] = (mode & S_IRUSR) ? 'r' : '-';  // Read permission
    perm_str[1] = (mode & S_IWUSR) ? 'w' : '-';  // Write permission  
    perm_str[2] = (mode & S_IXUSR) ? 'x' : '-';  // Execute permission
    
    // Group permissions (nhóm)
    perm_str[3] = (mode & S_IRGRP) ? 'r' : '-';  // Read permission
    perm_str[4] = (mode & S_IWGRP) ? 'w' : '-';  // Write permission
    perm_str[5] = (mode & S_IXGRP) ? 'x' : '-';  // Execute permission
    
    // Others permissions (người khác)
    perm_str[6] = (mode & S_IROTH) ? 'r' : '-';  // Read permission
    perm_str[7] = (mode & S_IWOTH) ? 'w' : '-';  // Write permission
    perm_str[8] = (mode & S_IXOTH) ? 'x' : '-';  // Execute permission
    
    perm_str[9] = '\0'; 
}