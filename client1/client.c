#include "common.h"
#include <errno.h>
#include "sha256_utils.h"

void register_user(int sock);
int login_user(int sock);
void list_files(int sock);
void create_directory(int sock);
void change_directory(int sock);
void upload_file(int sock);
void download_file(int sock);
void search_files(int sock);
void upload_directory(int sock, const char *dir_path);
void download_directory(int sock, const char *dir_name);
void send_command(int sock, const char *command);
int create_directories(const char *path);
void rename_file(int sock);
void delete_file(int sock);
void move_file(int sock);
void rename_directory(int sock);
void delete_directory(int sock);
void move_directory(int sock);



int main()
{
    int sock;
    struct sockaddr_in server_addr;

    // Tạo socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        handle_error("Không thể tạo socket.");

    // Thiết lập địa chỉ máy chủ
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Kết nối tới máy chủ
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        handle_error("Kết nối tới máy chủ thất bại.");

    printf("Đã kết nối tới máy chủ.\n");

    int choice;
    int authenticated = 0;

     while (1)
    {
        if (!authenticated)
        {
            printf("\n1. Đăng ký\n2. Đăng nhập\nLựa chọn: ");
            scanf("%d", &choice);
            getchar(); // Xóa ký tự newline

            if (choice == 1)
            {
                register_user(sock);
            }
            else if (choice == 2)
            {
                authenticated = login_user(sock);
            }
            else
            {
                printf("Lựa chọn không hợp lệ.\n");
            }
        }
        else
        {
            printf("\n1. Xem danh sách file\n2. Tạo thư mục\n3. Thay đổi thư mục\n4. Tải lên file\n5. Tải xuống file\n6. Tìm kiếm file\n7. Tải lên thư mục\n8. Tải xuống thư mục\n9. Sửa tên file\n10. Xóa file\n11. Di chuyển file\n12. Sửa tên thư mục\n13. Xóa thư mục\n14. Di chuyển thư mục\n15. Đăng xuất\nLựa chọn: ");
            scanf("%d", &choice);
            getchar(); // Xóa ký tự newline

            switch (choice)
            {
            case 1:
                list_files(sock);
                break;
            case 2:
                create_directory(sock);
                break;
            case 3:
                change_directory(sock);
                break;
            case 4:
                upload_file(sock);
                break;
            case 5:
                download_file(sock);
                break;
            case 6:
                search_files(sock);
                break;
            case 7:
            {
                char dir_name[BUFFER_SIZE];
                printf("Nhập tên thư mục cần tải lên: ");
                scanf("%s", dir_name);
                upload_directory(sock, dir_name);
                break;
            }
            case 8:
            {
                char dir_name[BUFFER_SIZE];
                printf("Nhập tên thư mục cần tải xuống: ");
                scanf("%s", dir_name);
                download_directory(sock, dir_name);
                break;
            }
            case 9:
                rename_file(sock);
                break;
            case 10:
                delete_file(sock);
                break;
            case 11:
                move_file(sock);
                break;
            case 12:
                rename_directory(sock);
                break;
            case 13:
                delete_directory(sock);
                break;
            case 14:
                move_directory(sock);
                break;
            case 15:
                {
                    // Gửi lệnh "Logout" tới server
                    send_command(sock, "Logout");

                    // Nhận phản hồi từ server
                    char response[BUFFER_SIZE];
                    if (recv_all(sock, response, BUFFER_SIZE) <= 0)
                    {
                        printf("Lỗi khi nhận phản hồi từ server.\n");
                        break;
                    }

                    if (strcmp(response, "LogoutSuccess") == 0)
                    {
                        printf("Đăng xuất thành công.\n");
                        authenticated = 0; // Đặt trạng thái không xác thực
                    }
                    else
                    {
                        printf("Đăng xuất thất bại.\n");
                    }
                    break;
                }
                
            default:
                printf("Lựa chọn không hợp lệ.\n");
            }
        }
    }
    close(sock);
    return 0;
}

int create_directories(const char *path)
{
    char tmp[BUFFER_SIZE];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = '\0';

    for (p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = '\0';
            if (mkdir(tmp, 0777) != 0 && errno != EEXIST)
            {
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, 0777) != 0 && errno != EEXIST)
    {
        return -1;
    }
    return 0;
}

// Sửa đổi hàm upload_directory
void upload_directory(int sock, const char *dir_path)
{
    // Thêm dòng gửi lệnh "UPLOAD_DIR" trước khi bắt đầu
    send_command(sock, "UPLOAD_DIR");

    DIR *d = opendir(dir_path);
    if (!d)
    {
        printf("Không thể mở thư mục %s\n", dir_path);
        return;
    }

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL)
    {
        char path[BUFFER_SIZE];
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
            continue;
        snprintf(path, BUFFER_SIZE, "%s/%s", dir_path, dir->d_name);

        struct stat st;
        stat(path, &st);
        if (S_ISDIR(st.st_mode))
        {
            // Gửi lệnh tạo thư mục
            send_command(sock, "MKDIR");
            send(sock, path, BUFFER_SIZE, 0);
            // Nhận phản hồi từ máy chủ
            char response[BUFFER_SIZE];
            recv(sock, response, BUFFER_SIZE, 0);
            if (strcmp(response, "DirectoryCreated") == 0)
            {
                printf("Đã tạo thư mục %s trên máy chủ.\n", path);
            }
            else
            {
                printf("Không thể tạo thư mục %s trên máy chủ.\n", path);
            }
            // Đệ quy tải lên thư mục con
            upload_directory(sock, path);
        }
        else if (S_ISREG(st.st_mode))
        {
            // Tải lên file
            send_command(sock, "UPLOAD");
            send(sock, path, BUFFER_SIZE, 0);

            FILE *fp = fopen(path, "rb");
            if (!fp)
            {
                printf("Không thể mở file %s\n", path);
                continue;
            }

            // Lấy kích thước file
            fseek(fp, 0, SEEK_END);
            uint64_t filesize = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            // Chuyển đổi kích thước file sang network byte order
            uint64_t filesize_net = htonll(filesize);

            // Gửi kích thước file
            send_all(sock, &filesize_net, sizeof(filesize_net));

            // Gửi nội dung file
            char buffer[BUFFER_SIZE];
            size_t bytesRead;
            while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, fp)) > 0)
            {
                if (send_all(sock, buffer, bytesRead) != 0)
                {
                    printf("Lỗi khi gửi dữ liệu.\n");
                    fclose(fp);
                    return;
                }
            }
            fclose(fp);

            // Tính toán hash SHA-256 của tệp
            char file_hash[65];
            if (compute_file_sha256(path, file_hash) != 0)
            {
                printf("Lỗi khi tính toán hash.\n");
                return;
            }

            // Gửi hash đến server
            send(sock, file_hash, 64, 0);

            // Nhận phản hồi từ máy chủ
            char response[BUFFER_SIZE];
            recv(sock, response, BUFFER_SIZE, 0);
            if (strcmp(response, "UploadSuccess") == 0)
            {
                printf("Đã tải lên file %s.\n", path);
            }
            else
            {
                printf("Tải lên file %s thất bại.\n", path);
            }
        }
    }
    closedir(d);
    // Gửi lệnh kết thúc thư mục
    send_command(sock, "END_OF_DIR");

    // Nhận phản hồi từ máy chủ
    char response[BUFFER_SIZE];
    recv(sock, response, BUFFER_SIZE, 0);
    if (strcmp(response, "UploadDirectorySuccess") == 0)
    {
        printf("Đã tải lên thư mục %s thành công.\n", dir_path);
    }
    else
    {
        printf("Tải lên thư mục %s thất bại.\n", dir_path);
    }
}

// Sửa đổi hàm receive_directory
void download_directory(int sock, const char *dir_name)
{
    // Gửi lệnh tải xuống thư mục
    send_command(sock, "DOWNLOAD_DIR");
    send(sock, dir_name, BUFFER_SIZE, 0);

    char response[BUFFER_SIZE];
    recv(sock, response, BUFFER_SIZE, 0);

    if (strcmp(response, "DirectoryNotFound") == 0)
    {
        printf("Thư mục không tồn tại trên máy chủ.\n");
        return;
    }

    while (1)
    {
        memset(response, 0, BUFFER_SIZE);
        if (recv(sock, response, BUFFER_SIZE, 0) <= 0)
        {
            printf("Lỗi khi nhận dữ liệu từ máy chủ.\n");
            break;
        }

        if (strcmp(response, "END_OF_DIR") == 0)
        {
            printf("Hoàn tất tải xuống thư mục %s.\n", dir_name);
            break;
        }
        else if (strcmp(response, "MKDIR") == 0)
        {
            // Tạo thư mục
            char dirname[BUFFER_SIZE];
            recv(sock, dirname, BUFFER_SIZE, 0);
            if (create_directories(dirname) == 0)
            {
                printf("Đã tạo thư mục %s.\n", dirname);
            }
            else
            {
                printf("Không thể tạo thư mục %s.\n", dirname);
            }
        }
        else if (strcmp(response, "FILE") == 0)
        {
            // Nhận file
            char filename[BUFFER_SIZE];
            recv(sock, filename, BUFFER_SIZE, 0);

            // Nhận kích thước file
            uint64_t filesize_net;
            if (recv_all(sock, &filesize_net, sizeof(filesize_net)) <= 0)
            {
                printf("Lỗi khi nhận kích thước file.\n");
                continue;
            }
            uint64_t filesize = ntohll(filesize_net);

            // Tạo các thư mục cha nếu chưa tồn tại
            char file_dir[BUFFER_SIZE];
            strcpy(file_dir, filename);
            char *last_slash = strrchr(file_dir, '/');
            if (last_slash != NULL)
            {
                *last_slash = '\0';
                if (create_directories(file_dir) != 0)
                {
                    printf("Không thể tạo thư mục %s.\n", file_dir);
                    continue;
                }
            }

            // Nhận nội dung file
            FILE *fp = fopen(filename, "wb");
            if (!fp)
            {
                printf("Không thể tạo file %s\n", filename);
                continue;
            }

            char buffer[BUFFER_SIZE];
            uint64_t totalReceived = 0;
            while (totalReceived < filesize)
            {
                size_t bytesToReceive = (filesize - totalReceived) < BUFFER_SIZE ? (filesize - totalReceived) : BUFFER_SIZE;
                ssize_t bytesReceived = recv_all(sock, buffer, bytesToReceive);
                if (bytesReceived <= 0)
                {
                    printf("Lỗi khi nhận dữ liệu.\n");
                    fclose(fp);
                    return;
                }
                fwrite(buffer, 1, bytesReceived, fp);
                totalReceived += bytesReceived;
            }
            fclose(fp);
            printf("Đã tải xuống file %s.\n", filename);

            // Nhận hash từ server
            char received_hash[65];
            memset(received_hash, 0, sizeof(received_hash));
            if (recv_all(sock, received_hash, 64) <= 0)
            {
                printf("Lỗi khi nhận hash từ máy chủ.\n");
                continue;
            }
            received_hash[64] = '\0'; // Kết thúc chuỗi

            // Tính toán hash SHA-256 của tệp đã nhận
            char computed_hash[65];
            if (compute_file_sha256(filename, computed_hash) != 0)
            {
                printf("Lỗi khi tính toán hash.\n");
                continue;
            }

            // So sánh các hash
            if (strcmp(received_hash, computed_hash) == 0)
            {
                printf("Hash xác minh thành công cho file %s.\n", filename);
            }
            else
            {
                printf("Hash xác minh thất bại cho file %s.\n", filename);
            }
        }
        else
        {
            printf("Nhận thông báo không xác định: %s\n", response);
        }
    }
}

void send_command(int sock, const char *command)
{
    send(sock, command, BUFFER_SIZE, 0);
}

// Hàm đăng ký người dùng
void register_user(int sock)
{
    char username[BUFFER_SIZE], password[BUFFER_SIZE];
    printf("Nhập tên người dùng: ");
    scanf("%s", username);
    printf("Nhập mật khẩu: ");
    scanf("%s", password);

    send_command(sock, "Register");
    send(sock, username, BUFFER_SIZE, 0);
    send(sock, password, BUFFER_SIZE, 0);

    char response[BUFFER_SIZE];
    recv(sock, response, BUFFER_SIZE, 0);
    if (strcmp(response, "RegisterSuccess") == 0)
    {
        printf("Đăng ký thành công!\n");
    }
    else if (strcmp(response, "UserExists") == 0)
    {
        printf("Người dùng đã tồn tại.\n");
    }
    else
    {
        printf("Đăng ký thất bại.\n");
    }
}

// Hàm đăng nhập người dùng
int login_user(int sock)
{
    char username[BUFFER_SIZE], password[BUFFER_SIZE];
    printf("Nhập tên người dùng: ");
    scanf("%s", username);
    printf("Nhập mật khẩu: ");
    scanf("%s", password);

    send_command(sock, "Login");
    send(sock, username, BUFFER_SIZE, 0);
    send(sock, password, BUFFER_SIZE, 0);

    char response[BUFFER_SIZE];
    recv(sock, response, BUFFER_SIZE, 0);
    if (strcmp(response, "LoginSuccess") == 0)
    {
        printf("Đăng nhập thành công!\n");
        // strcpy(curr_usr, username);
        return 1;
    }
    else
    {
        printf("Đăng nhập thất bại. Kiểm tra lại thông tin đăng nhập.\n");
        return 0;
    }
}

// Hàm xem danh sách file
void list_files(int sock)
{
    send_command(sock, "LIST");
    char buffer[BUFFER_SIZE];
    recv(sock, buffer, BUFFER_SIZE, 0);
    printf("Danh sách file và thư mục:\n%s", buffer);
}

// Hàm tạo thư mục
void create_directory(int sock)
{
    char dirname[BUFFER_SIZE];
    printf("Nhập tên thư mục cần tạo: ");
    scanf("%s", dirname);

    send_command(sock, "MKDIR");
    send(sock, dirname, BUFFER_SIZE, 0);

    char response[BUFFER_SIZE];
    recv(sock, response, BUFFER_SIZE, 0);
    if (strcmp(response, "DirectoryCreated") == 0)
    {
        printf("Tạo thư mục thành công.\n");
    }
    else
    {
        printf("Tạo thư mục thất bại.\n");
    }
}

// Hàm thay đổi thư mục
void change_directory(int sock)
{
    char dirname[BUFFER_SIZE];
    printf("Nhập tên thư mục cần chuyển đến: ");
    scanf("%s", dirname);

    send_command(sock, "CD");
    send(sock, dirname, BUFFER_SIZE, 0);

    char response[BUFFER_SIZE];
    recv(sock, response, BUFFER_SIZE, 0);
    if (strcmp(response, "DirectoryChanged") == 0)
    {
        printf("Chuyển thư mục thành công.\n");
    }
    else
    {
        printf("Chuyển thư mục thất bại.\n");
    }
}
//fix1
void upload_file(int sock)
{
    char filename[BUFFER_SIZE];
    printf("Nhập tên file cần tải lên: ");
    scanf("%s", filename);

    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        printf("Không thể mở file.\n");
        return;
    }

    // Lấy kích thước file
    fseek(fp, 0, SEEK_END);
    uint64_t filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    send_command(sock, "UPLOAD");
    send(sock, filename, BUFFER_SIZE, 0);

    // Chuyển đổi kích thước file sang network byte order
    uint64_t filesize_net = htonll(filesize);

    // Gửi kích thước file
    send_all(sock, &filesize_net, sizeof(filesize_net));

    // Gửi nội dung file
    char buffer[BUFFER_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, fp)) > 0)
    {
        if (send_all(sock, buffer, bytesRead) != 0)
        {
            printf("Lỗi khi gửi dữ liệu.\n");
            fclose(fp);
            return;
        }
    }
    fclose(fp);

    // Tính toán hash SHA-256 của tệp
    char file_hash[65];
    if (compute_file_sha256(filename, file_hash) != 0)
    {
        printf("Lỗi khi tính toán hash.\n");
        return;
    }

    // Gửi hash đến server
    send(sock, file_hash, 64, 0);

    printf("Đã tải lên file %s.\n", filename);

    // Nhận phản hồi
    char response[BUFFER_SIZE];
    recv(sock, response, BUFFER_SIZE, 0);
    if (strcmp(response, "UploadSuccess") == 0)
    {
        printf("Tải lên thành công.\n");
    }
    else
    {
        printf("Tải lên thất bại.\n");
    }
}

// Sửa đổi hàm download_file
void download_file(int sock)
{
    char filename[BUFFER_SIZE];
    printf("Nhập tên file cần tải xuống: ");
    scanf("%s", filename);

    send_command(sock, "DOWNLOAD");
    send(sock, filename, BUFFER_SIZE, 0);

    char response[BUFFER_SIZE];
    recv(sock, response, BUFFER_SIZE, 0);
    if (strcmp(response, "FileNotFound") == 0)
    {
        printf("File không tồn tại trên máy chủ.\n");
        return;
    }

    // Nhận kích thước file
    uint64_t filesize_net;
    if (recv_all(sock, &filesize_net, sizeof(filesize_net)) <= 0)
    {
        printf("Lỗi khi nhận kích thước file.\n");
        return;
    }
    uint64_t filesize = ntohll(filesize_net);

    // Tạo các thư mục cha nếu cần thiết
    char file_dir[BUFFER_SIZE];
    strcpy(file_dir, filename);
    char *last_slash = strrchr(file_dir, '/');
    if (last_slash != NULL)
    {
        *last_slash = '\0';
        if (create_directories(file_dir) != 0)
        {
            printf("Không thể tạo thư mục %s.\n", file_dir);
            return;
        }
    }

    // Nhận nội dung file
    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        printf("Không thể tạo file.\n");
        return;
    }

    char buffer[BUFFER_SIZE];
    uint64_t totalReceived = 0;
    while (totalReceived < filesize)
    {
        size_t bytesToReceive = (filesize - totalReceived) < BUFFER_SIZE ? (filesize - totalReceived) : BUFFER_SIZE;
        ssize_t bytesReceived = recv_all(sock, buffer, bytesToReceive);
        if (bytesReceived <= 0)
        {
            printf("Lỗi khi nhận dữ liệu.\n");
            fclose(fp);
            return;
        }
        fwrite(buffer, 1, bytesReceived, fp);
        totalReceived += bytesReceived;
    }
    fclose(fp);
    printf("Đã tải xuống file %s.\n", filename);

    // Nhận hash từ server
    char received_hash[65];
    memset(received_hash, 0, sizeof(received_hash));
    if (recv_all(sock, received_hash, 64) <= 0)
    {
        printf("Lỗi khi nhận hash từ máy chủ.\n");
        return;
    }
    received_hash[64] = '\0'; // Kết thúc chuỗi

    // Tính toán hash SHA-256 của tệp đã nhận
    char computed_hash[65];
    if (compute_file_sha256(filename, computed_hash) != 0)
    {
        printf("Lỗi khi tính toán hash.\n");
        return;
    }

    // So sánh các hash
    if (strcmp(received_hash, computed_hash) == 0)
    {
        printf("Hash xác minh thành công cho file %s.\n", filename);
    }
    else
    {
        printf("Hash xác minh thất bại cho file %s.\n", filename);
    }
}

// Hàm tìm kiếm file
void search_files(int sock)
{
    char query[BUFFER_SIZE];
    printf("Nhập từ khóa tìm kiếm: ");
    scanf("%s", query);

    send_command(sock, "SEARCH");
    send(sock, query, BUFFER_SIZE, 0);

    char results[BUFFER_SIZE];
    recv(sock, results, BUFFER_SIZE, 0);
    printf("Kết quả tìm kiếm:\n%s", results);

    printf("Nhập đường dẫn file chính xác để tải xuống hoặc 'exit' để hủy: ");
    char choice[BUFFER_SIZE];
    scanf("%s", choice);

    if (strcmp(choice, "exit") != 0)
    {
        send_command(sock, "DOWNLOAD");
        send(sock, choice, BUFFER_SIZE, 0);

        char response[BUFFER_SIZE];
        recv(sock, response, BUFFER_SIZE, 0);
        if (strcmp(response, "FileNotFound") == 0)
        {
            printf("File không tồn tại trên máy chủ.\n");
            return;
        }

        // Nhận kích thước file
        uint64_t filesize_net;
        if (recv_all(sock, &filesize_net, sizeof(filesize_net)) <= 0)
        {
            printf("Lỗi khi nhận kích thước file.\n");
            return;
        }
        uint64_t filesize = ntohll(filesize_net);

        // Tạo các thư mục cha nếu cần thiết
        char file_dir[BUFFER_SIZE];
        strcpy(file_dir, choice);
        char *last_slash = strrchr(file_dir, '/');
        if (last_slash != NULL)
        {
            *last_slash = '\0';
            if (create_directories(file_dir) != 0)
            {
                printf("Không thể tạo thư mục %s.\n", file_dir);
                return;
            }
        }

        FILE *fp = fopen(choice, "wb");
        if (!fp)
        {
            printf("Không thể tạo file.\n");
            return;
        }

        // Nhận nội dung file
        char buffer[BUFFER_SIZE];
        uint64_t totalReceived = 0;
        while (totalReceived < filesize)
        {
            size_t bytesToReceive = (filesize - totalReceived) < BUFFER_SIZE ? (filesize - totalReceived) : BUFFER_SIZE;
            ssize_t bytesReceived = recv_all(sock, buffer, bytesToReceive);
            if (bytesReceived <= 0)
            {
                printf("Lỗi khi nhận dữ liệu.\n");
                fclose(fp);
                return;
            }
            fwrite(buffer, 1, bytesReceived, fp);
            totalReceived += bytesReceived;
        }
        fclose(fp);
        printf("Đã tải xuống file %s.\n", choice);
    }
}

void rename_file(int sock)
{
    char old_name[BUFFER_SIZE], new_name[BUFFER_SIZE];
    printf("Nhập tên file hiện tại: ");
    scanf("%s", old_name);
    printf("Nhập tên file mới: ");
    scanf("%s", new_name);

    send_command(sock, "RENAME_FILE");
    send(sock, old_name, BUFFER_SIZE, 0);
    send(sock, new_name, BUFFER_SIZE, 0);

    char response[BUFFER_SIZE];
    recv(sock, response, BUFFER_SIZE, 0);
    if (strcmp(response, "RenameSuccess") == 0)
    {
        printf("Đổi tên file thành công.\n");
    }
    else
    {
        printf("Đổi tên file thất bại.\n");
    }
}
void delete_file(int sock)
{
    char filename[BUFFER_SIZE];
    printf("Nhập tên file cần xóa: ");
    scanf("%s", filename);

    send_command(sock, "DELETE_FILE");
    send(sock, filename, BUFFER_SIZE, 0);

    char response[BUFFER_SIZE];
    recv(sock, response, BUFFER_SIZE, 0);
    if (strcmp(response, "DeleteSuccess") == 0)
    {
        printf("Xóa file thành công.\n");
    }
    else
    {
        printf("Xóa file thất bại.\n");
    }
}
void move_file(int sock)
{
    char filename[BUFFER_SIZE], destination[BUFFER_SIZE];
    printf("Nhập tên file cần di chuyển: ");
    scanf("%s", filename);
    printf("Nhập đường dẫn đích: ");
    scanf("%s", destination);

    send_command(sock, "MOVE_FILE");
    
    if (send_all(sock, filename, BUFFER_SIZE) != 0)
    {
        printf("Lỗi khi gửi tên file.\n");
        return;
    }
    if (send_all(sock, destination, BUFFER_SIZE) != 0)
    {
        printf("Lỗi khi gửi đường dẫn đích.\n");
        return;
    }

    char response[BUFFER_SIZE];
    if (recv_all(sock, response, BUFFER_SIZE) <= 0)
    {
        printf("Lỗi khi nhận phản hồi từ server.\n");
        return;
    }
    if (strcmp(response, "MoveSuccess") == 0)
    {
        printf("Di chuyển file thành công.\n");
    }
    else
    {
        printf("Di chuyển file thất bại.\n");
    }
}

void rename_directory(int sock)
{
    char old_dir[BUFFER_SIZE], new_dir[BUFFER_SIZE];
    printf("Nhập tên thư mục hiện tại: ");
    scanf("%s", old_dir);
    printf("Nhập tên thư mục mới: ");
    scanf("%s", new_dir);

    send_command(sock, "RENAME_DIR");
    send(sock, old_dir, BUFFER_SIZE, 0);
    send(sock, new_dir, BUFFER_SIZE, 0);

    char response[BUFFER_SIZE];
    recv(sock, response, BUFFER_SIZE, 0);
    if (strcmp(response, "RenameSuccess") == 0)
    {
        printf("Đổi tên thư mục thành công.\n");
    }
    else
    {
        printf("Đổi tên thư mục thất bại.\n");
    }
}
void delete_directory(int sock)
{
    char dirname[BUFFER_SIZE];
    printf("Nhập tên thư mục cần xóa: ");
    scanf("%s", dirname);

    send_command(sock, "DELETE_DIR");
    send(sock, dirname, BUFFER_SIZE, 0);

    char response[BUFFER_SIZE];
    recv(sock, response, BUFFER_SIZE, 0);
    if (strcmp(response, "DeleteSuccess") == 0)
    {
        printf("Xóa thư mục thành công.\n");
    }
    else
    {
        printf("Xóa thư mục thất bại.\n");
    }
}
void move_directory(int sock)
{
    char dirname[BUFFER_SIZE], destination[BUFFER_SIZE];
    printf("Nhập tên thư mục cần di chuyển: ");
    scanf("%s", dirname);
    printf("Nhập đường dẫn đích: ");
    scanf("%s", destination);

    send_command(sock, "MOVE_DIR");
    send(sock, dirname, BUFFER_SIZE, 0);
    send(sock, destination, BUFFER_SIZE, 0);

    char response[BUFFER_SIZE];
    recv(sock, response, BUFFER_SIZE, 0);
    if (strcmp(response, "MoveSuccess") == 0)
    {
        printf("Di chuyển thư mục thành công.\n");
    }
    else
    {
        printf("Di chuyển thư mục thất bại.\n");
    }
}


