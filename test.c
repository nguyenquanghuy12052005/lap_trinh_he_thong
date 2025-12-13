
#include <stdio.h>
#include <stdlib.h>
#include "file_manager.h"

void test_file(const char* filename, const char* description) {
    struct file_info info;
    char type_str[50];  // Buffer cho loại file
    char perm_str[10];   // Buffer cho permissions
    
    printf("%s:\n", description);
    // THU THẬP THÔNG TIN
    if (collect_file_stats(filename, &info) == 0) {

        // XỬ LÝ VÀ PHÂN TÍCH
        determine_file_type(info.mode, type_str);  //xác định loại file
        convert_permissions(info.mode, perm_str); ////Chuyển permissions sang dạng "rwxrwxrwx"
        get_user_group_info(&info); ////Lấy username và groupname từ ID
        
        printf("    Type: %s\n", type_str);
        printf("    Permissions: %s\n", perm_str);
        printf("    Size: %ld bytes\n", info.size);
        printf("    Owner: %s:%s\n", info.username, info.groupname);
        printf("    Links: %lu\n", info.nlink);
        printf("    Inode: %lu\n", info.inode);
    } else {
        printf("    Cannot access file\n");
    }
    printf("\n");
}

int main() {
    printf(" TEST FILE\n");
    printf("\n\n");
    
    // TEST VỚI FILE THỰC TẾ TRONG MÁY
    test_file("/bin/ls", "1.  SYSTEM EXECUTABLE (/bin/ls)");
    test_file("/etc/passwd", "2.  SYSTEM CONFIG (/etc/passwd)");
    test_file("/home", "3.  HOME DIRECTORY");
    test_file("/dev/null", "4.  DEVICE FILE (/dev/null)");
    
    // TEST VỚI FILE TRONG THƯ MỤC HIỆN TẠI
    test_file("file_info_collector.c", "5.  SOURCE CODE FILE");
    test_file("file_manager.h", "6.  HEADER FILE");
    test_file(".", "7.  CURRENT DIRECTORY");
    test_file("..", "8.  PARENT DIRECTORY");
    
    printf(" ACTUAL FILES TEST COMPLETED!\n");
    return 0;
}
