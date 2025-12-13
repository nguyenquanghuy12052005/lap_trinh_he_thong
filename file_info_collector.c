#define _GNU_SOURCE
#include <sys/sysmacros.h>
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
#include <sys/xattr.h>
#include "file_manager.h"

#ifndef major
#define major(dev) ((int)(((dev) >> 8) & 0xff))
#endif

#ifndef minor
#define minor(dev) ((int)((dev) & 0xff))
#endif


// Thu thập thông tin file từ hệ thống
int collect_file_stats(const char *filename, struct file_info *info) {
    struct stat file_stat;
    
    if (lstat(filename, &file_stat) == -1) {
        snprintf(info->error_msg, sizeof(info->error_msg), 
                 "Error: %s", strerror(errno));
        return -1;
    }
    
    strncpy(info->filename, filename, sizeof(info->filename)-1);
    info->filename[sizeof(info->filename)-1] = '\0';
    info->mode = file_stat.st_mode;
    info->nlink = file_stat.st_nlink;
    info->size = file_stat.st_size;
    info->inode = file_stat.st_ino;
    info->device = file_stat.st_dev;
    info->atime = file_stat.st_atime;
    info->mtime = file_stat.st_mtime;
    info->ctime = file_stat.st_ctime;
    info->uid = file_stat.st_uid;
    info->gid = file_stat.st_gid;
    
    // Thu thập thông tin mở rộng
    info->block_size = file_stat.st_blksize;
    info->blocks = file_stat.st_blocks;
    info->rdev = file_stat.st_rdev;
    
    // Tách major và minor device numbers
    info->major_dev = major(file_stat.st_rdev);
    info->minor_dev = minor(file_stat.st_rdev);
    
    // Kiểm tra file có sparse không
    info->is_sparse = is_sparse_file(info->size, info->blocks);
    
    // Kiểm tra file có thực thi được không
    info->is_executable = (info->mode & S_IXUSR) || 
                          (info->mode & S_IXGRP) || 
                          (info->mode & S_IXOTH);
    
    // Lấy symlink target nếu là symbolic link
    if (S_ISLNK(info->mode)) {
        get_symlink_target(filename, info->symlink_target, 
                          sizeof(info->symlink_target));
    } else {
        info->symlink_target[0] = '\0';
    }
    
    // Detect MIME type
    detect_mime_type(filename, info->mode, info->mime_type, 
                     sizeof(info->mime_type));
    
    // Kiểm tra extended attributes
    info->has_xattr = check_extended_attributes(filename);
    
    // Tính MD5 cho file nhỏ hơn 10MB
    if (S_ISREG(info->mode) && info->size < 10*1024*1024) {
        calculate_md5(filename, info->md5_hash, sizeof(info->md5_hash));
    } else {
        strncpy(info->md5_hash, "N/A", sizeof(info->md5_hash)-1);
        info->md5_hash[sizeof(info->md5_hash)-1] = '\0';
    }
    
    return 0;
}

// Lấy username và groupname từ ID
int get_user_group_info(struct file_info *info) {
    struct passwd *pwd = getpwuid(info->uid);
    if (pwd) {
        strncpy(info->username, pwd->pw_name, sizeof(info->username)-1);
        info->username[sizeof(info->username)-1] = '\0';
    } else {
        snprintf(info->username, sizeof(info->username), "%d", info->uid);
    }
    
    struct group *grp = getgrgid(info->gid);
    if (grp) {
        strncpy(info->groupname, grp->gr_name, sizeof(info->groupname)-1);
        info->groupname[sizeof(info->groupname)-1] = '\0';
    } else {
        snprintf(info->groupname, sizeof(info->groupname), "%d", info->gid);
    }
    
    return 0;
}

// Xác định loại file
void determine_file_type(mode_t mode, char *type_str) {
    if (S_ISREG(mode))
        strcpy(type_str, "Regular File");
    else if (S_ISDIR(mode))
        strcpy(type_str, "Directory");
    else if (S_ISLNK(mode))
        strcpy(type_str, "Symbolic Link");
    else if (S_ISCHR(mode))
        strcpy(type_str, "Character Device");
    else if (S_ISBLK(mode))
        strcpy(type_str, "Block Device");
    else if (S_ISFIFO(mode))
        strcpy(type_str, "FIFO/Pipe");
    else if (S_ISSOCK(mode))
        strcpy(type_str, "Socket");
    else
        strcpy(type_str, "Unknown Type");
}

// Chuyển permissions sang dạng "rwxrwxrwx"
void convert_permissions(mode_t mode, char *perm_str) {
    perm_str[0] = (mode & S_IRUSR) ? 'r' : '-';
    perm_str[1] = (mode & S_IWUSR) ? 'w' : '-';
    perm_str[2] = (mode & S_IXUSR) ? 'x' : '-';
    perm_str[3] = (mode & S_IRGRP) ? 'r' : '-';
    perm_str[4] = (mode & S_IWGRP) ? 'w' : '-';
    perm_str[5] = (mode & S_IXGRP) ? 'x' : '-';
    perm_str[6] = (mode & S_IROTH) ? 'r' : '-';
    perm_str[7] = (mode & S_IWOTH) ? 'w' : '-';
    perm_str[8] = (mode & S_IXOTH) ? 'x' : '-';
    perm_str[9] = '\0';
}

// Lấy đích của symbolic link
int get_symlink_target(const char *filename, char *target, size_t size) {
    ssize_t len = readlink(filename, target, size - 1);
    if (len == -1) {
        target[0] = '\0';
        return -1;
    }
    target[len] = '\0';
    return 0;
}

// Detect MIME type dựa trên extension
int detect_mime_type(const char *filename, mode_t mode, char *mime_type, size_t size) {
    if (S_ISDIR(mode)) {
        strncpy(mime_type, "inode/directory", size-1);
        mime_type[size-1] = '\0';
        return 0;
    }
    
    if (S_ISLNK(mode)) {
        strncpy(mime_type, "inode/symlink", size-1);
        mime_type[size-1] = '\0';
        return 0;
    }
    
    if (S_ISCHR(mode)) {
        strncpy(mime_type, "inode/chardevice", size-1);
        mime_type[size-1] = '\0';
        return 0;
    }
    
    if (S_ISBLK(mode)) {
        strncpy(mime_type, "inode/blockdevice", size-1);
        mime_type[size-1] = '\0';
        return 0;
    }
    
    // Detect bằng extension
    const char *ext = strrchr(filename, '.');
    if (ext) {
        ext++;
        if (strcmp(ext, "txt") == 0) 
            strncpy(mime_type, "text/plain", size-1);
        else if (strcmp(ext, "c") == 0 || strcmp(ext, "h") == 0) 
            strncpy(mime_type, "text/x-c", size-1);
        else if (strcmp(ext, "py") == 0) 
            strncpy(mime_type, "text/x-python", size-1);
        else if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) 
            strncpy(mime_type, "image/jpeg", size-1);
        else if (strcmp(ext, "png") == 0) 
            strncpy(mime_type, "image/png", size-1);
        else if (strcmp(ext, "pdf") == 0) 
            strncpy(mime_type, "application/pdf", size-1);
        else if (strcmp(ext, "zip") == 0) 
            strncpy(mime_type, "application/zip", size-1);
        else if (strcmp(ext, "tar") == 0) 
            strncpy(mime_type, "application/x-tar", size-1);
        else if (strcmp(ext, "gz") == 0) 
            strncpy(mime_type, "application/gzip", size-1);
        else if (strcmp(ext, "sh") == 0) 
            strncpy(mime_type, "application/x-sh", size-1);
        else if (strcmp(ext, "exe") == 0) 
            strncpy(mime_type, "application/x-executable", size-1);
        else 
            strncpy(mime_type, "application/octet-stream", size-1);
    } else {
        strncpy(mime_type, "application/octet-stream", size-1);
    }
    
    mime_type[size-1] = '\0';
    return 0;
}

// Tính MD5 hash (sử dụng md5sum command)
int calculate_md5(const char *filename, char *md5_str, size_t max_size) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "md5sum '%s' 2>/dev/null", filename);
    
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        strncpy(md5_str, "Error", max_size-1);
        md5_str[max_size-1] = '\0';
        return -1;
    }
    
    if (fscanf(fp, "%32s", md5_str) != 1) {
        strncpy(md5_str, "N/A", max_size-1);
        md5_str[max_size-1] = '\0';
        pclose(fp);
        return -1;
    }
    
    pclose(fp);
    md5_str[max_size-1] = '\0';
    return 0;
}

// Kiểm tra extended attributes
int check_extended_attributes(const char *filename) {
    ssize_t size = listxattr(filename, NULL, 0);
    return (size > 0) ? 1 : 0;
}

// Kiểm tra file có sparse không
int is_sparse_file(off_t size, blkcnt_t blocks) {
    off_t expected_blocks = (size + 511) / 512;
    return (blocks < expected_blocks);
}