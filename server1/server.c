#include "common.h"
#include <errno.h>
#include <libgen.h> 

char current_username[BUFFER_SIZE];
char current_directory[BUFFER_SIZE];

void register_user(int client_sock);
int login_user(int client_sock);
void list_files(int client_sock);
void create_directory(int client_sock);
void change_directory(int client_sock);
void receive_file(int client_sock);

void send_file(int client_sock);
void search_files(int client_sock);
void search_in_directory(const char *dir_path, const char *query, char *results);
void send_directory(int client_sock, const char *dir_path, int is_top_level);
void receive_directory(int client_sock);

void handle_client(int client_sock);
void send_response(int client_sock, const char *message);
int create_directories(const char *path);

void rename_file_server(int client_sock);
void delete_file_server(int client_sock);
void move_file_server(int client_sock);
void rename_directory_server(int client_sock);
void delete_directory_server(int client_sock);
void move_directory_server(int client_sock);
int remove_directory(const char *path);

int main()
{
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    // Tạo socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1)
        handle_error("Không thể tạo socket.");

    // Add this code to enable address reuse
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        exit(1);
    }

    // Thiết lập địa chỉ máy chủ
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Gắn socket với địa chỉ máy chủ
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        handle_error("Gắn socket thất bại.");

    // Lắng nghe kết nối
    if (listen(server_sock, 5) < 0)
        handle_error("Lắng nghe thất bại.");

    printf("Máy chủ đang lắng nghe trên cổng %d...\n", PORT);

    while (1)
    {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
        if (client_sock < 0)
            handle_error("Chấp nhận kết nối thất bại.");
        printf("Kết nối từ máy khách.\n");

        // Xử lý máy khách trong một tiến trình riêng
        if (fork() == 0)
        {
            close(server_sock);
            handle_client(client_sock);
            exit(0);
        }
        close(client_sock);
    }
    close(server_sock);
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

void handle_client(int client_sock)
{
    int authenticated = 0;
    char command[BUFFER_SIZE];
    while (1)
    {
        memset(command, 0, BUFFER_SIZE);
        if (recv(client_sock, command, BUFFER_SIZE, 0) <= 0)
            break;

        if (strcmp(command, "Register") == 0)
        {
            register_user(client_sock);
        }
        else if (strcmp(command, "Login") == 0)
        {
            authenticated = login_user(client_sock);
        }
        else if (authenticated)
        {

            if (strcmp(command, "LIST") == 0)
            {
                list_files(client_sock);
            }
            else if (strcmp(command, "MKDIR") == 0)
            {
                create_directory(client_sock);
            }
            else if (strcmp(command, "CD") == 0)
            {
                change_directory(client_sock);
            }
            else if (strcmp(command, "UPLOAD") == 0)
            {
                receive_file(client_sock);
            }
            else if (strcmp(command, "DOWNLOAD") == 0)
            {
                send_file(client_sock);
            }
            else if (strcmp(command, "SEARCH") == 0)
            {
                search_files(client_sock);
            }
            else if (strcmp(command, "DOWNLOAD_DIR") == 0)
            {
                char dir_name[BUFFER_SIZE];
                recv(client_sock, dir_name, BUFFER_SIZE, 0);

                // Kiểm tra sự tồn tại của thư mục
                if (access(dir_name, F_OK) == -1)
                {
                    send_response(client_sock, "DirectoryNotFound");
                }
                else
                {
                    // Bắt đầu gửi thư mục
                    send_directory(client_sock, dir_name, 1);
                }
            }
            else if (strcmp(command, "UPLOAD_DIR") == 0)
            {
                receive_directory(client_sock);
            }
            else if (strcmp(command, "RENAME_FILE") == 0)
            {
                rename_file_server(client_sock);
            }
            else if (strcmp(command, "DELETE_FILE") == 0)
            {
                delete_file_server(client_sock);
            }
            else if (strcmp(command, "MOVE_FILE") == 0)
            {
                move_file_server(client_sock);
            }
            else if (strcmp(command, "RENAME_DIR") == 0)
            {
                rename_directory_server(client_sock);
            }
            else if (strcmp(command, "DELETE_DIR") == 0)
            {
                delete_directory_server(client_sock);
            }
            else if (strcmp(command, "MOVE_DIR") == 0)
            {
                move_directory_server(client_sock);
            }
            else
            {
                send_response(client_sock, "Lệnh không hợp lệ.");
            }
        }
        else
        {
            send_response(client_sock, "Vui lòng đăng nhập trước.");
        }
    }
    close(client_sock);
    printf("Đã ngắt kết nối với máy khách.\n");
}

void send_response(int client_sock, const char *message)
{
    send(client_sock, message, BUFFER_SIZE, 0);
}

void receive_directory(int client_sock)
{
    char command[BUFFER_SIZE];
    while (1)
    {
        memset(command, 0, BUFFER_SIZE);
        if (recv(client_sock, command, BUFFER_SIZE, 0) <= 0)
        {
            printf("Lỗi khi nhận dữ liệu từ máy khách.\n");
            break;
        }
        if (strcmp(command, "END_OF_DIR") == 0)
        {
            send_response(client_sock, "UploadDirectorySuccess");
            break;
        }
        else if (strcmp(command, "MKDIR") == 0)
        {
            char dirname[BUFFER_SIZE];
            recv(client_sock, dirname, BUFFER_SIZE, 0);

            // char full_path[BUFFER_SIZE];
            // snprintf(full_path, BUFFER_SIZE, "%s/%s", current_username, dirname);

            if (create_directories(dirname) == 0)
            {
                printf("Đã tạo thư mục %s.\n", dirname);
                send_response(client_sock, "DirectoryCreated");
            }
            else
            {
                printf("Không thể tạo thư mục %s.\n", dirname);
                send_response(client_sock, "DirectoryCreationFailed");
            }
        }
        else if (strcmp(command, "UPLOAD") == 0)
        {
            receive_file(client_sock);
        }
    }
}

// Hàm gửi thư mục tới máy khách
void send_directory(int client_sock, const char *dir_path, int is_top_level)
{
    DIR *d = opendir(dir_path);
    if (!d)
    {
        if (is_top_level)
        {
            send_response(client_sock, "DirectoryNotFound");
        }
        return;
    }

    if (is_top_level)
    {
        send_response(client_sock, "DirectoryFound");
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
            send_response(client_sock, "MKDIR");
            send_response(client_sock, path);

            // Đệ quy gửi thư mục con
            send_directory(client_sock, path, 0);
        }
        else if (S_ISREG(st.st_mode))
        {
            // Gửi lệnh gửi file
            send_response(client_sock, "FILE");
            send_response(client_sock, path);

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
            if (send_all(client_sock, &filesize_net, sizeof(filesize_net)) != 0)
            {
                printf("Lỗi khi gửi kích thước file.\n");
                fclose(fp);
                continue;
            }

            // Gửi nội dung file
            char buffer[BUFFER_SIZE];
            size_t bytesRead;
            while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, fp)) > 0)
            {
                if (send_all(client_sock, buffer, bytesRead) != 0)
                {
                    printf("Lỗi khi gửi dữ liệu.\n");
                    fclose(fp);
                    return;
                }
            }
            fclose(fp);
            printf("Đã gửi file %s tới máy khách.\n", path);
        }
    }
    closedir(d);

    // Chỉ gửi "END_OF_DIR" ở cấp trên cùng
    if (is_top_level)
    {
        send_response(client_sock, "END_OF_DIR");
    }
}

// Hàm đăng ký người dùng
void register_user(int client_sock)
{
    char username[BUFFER_SIZE], password[BUFFER_SIZE];
    recv(client_sock, username, BUFFER_SIZE, 0);
    recv(client_sock, password, BUFFER_SIZE, 0);

    // Kiểm tra xem người dùng đã tồn tại chưa
    FILE *fp = fopen("users.txt", "a+");
    if (!fp)
    {
        send_response(client_sock, "Không thể mở file người dùng.");
        return;
    }

    char line[BUFFER_SIZE], stored_user[BUFFER_SIZE];
    int exists = 0;
    while (fgets(line, sizeof(line), fp))
    {
        sscanf(line, "%s", stored_user);
        if (strcmp(username, stored_user) == 0)
        {
            exists = 1;
            break;
        }
    }

    if (exists)
    {
        send_response(client_sock, "UserExists");
    }
    else
    {
        fprintf(fp, "%s %s\n", username, password);
        send_response(client_sock, "RegisterSuccess");
    }
    fclose(fp);
}

// Hàm đăng nhập người dùng
int login_user(int client_sock)
{
    char username[BUFFER_SIZE], password[BUFFER_SIZE];
    recv(client_sock, username, BUFFER_SIZE, 0);
    recv(client_sock, password, BUFFER_SIZE, 0);

    FILE *fp = fopen("users.txt", "r");
    if (!fp)
    {
        send_response(client_sock, "Không thể mở file người dùng.");
        return 0;
    }

    char line[BUFFER_SIZE], stored_user[BUFFER_SIZE], stored_pass[BUFFER_SIZE];
    int authenticated = 0;
    while (fgets(line, sizeof(line), fp))
    {
        sscanf(line, "%s %s", stored_user, stored_pass);
        if (strcmp(username, stored_user) == 0 && strcmp(password, stored_pass) == 0)
        {
            authenticated = 1;
            break;
        }
    }
    fclose(fp);

    if (authenticated)
    {
        strcpy(current_username, username);

        // Tạo và chuyển đến thư mục của user
        char base_path[BUFFER_SIZE];
        char *pwd = getenv("PWD");
        snprintf(base_path, BUFFER_SIZE, "%s/data/%s", pwd, username);

        if (create_directories(base_path) != 0)
        {
            printf("Không thể tạo thư mục: %s\n", base_path);
            return 0;
        }

        if (chdir(base_path) != 0)
        {
            printf("Không thể chuyển đến thư mục: %s\n", base_path);
            return 0;
        }

        strcpy(current_directory, base_path);
        printf("Đã đăng nhập user: %s\nThư mục hiện tại: %s\n", username, base_path);
        send_response(client_sock, "LoginSuccess");
        return 1;
    }

    send_response(client_sock, "LoginFailed");
    return 0;
}

// Hàm liệt kê file
void list_files(int client_sock)
{
    DIR *d;
    struct dirent *dir;
    char buffer[BUFFER_SIZE] = "";

    d = opendir(".");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            strcat(buffer, dir->d_name);
            strcat(buffer, "\n");
        }
        closedir(d);
    }
    send_response(client_sock, buffer);
}

// Hàm tạo thư mục
void create_directory(int client_sock)
{
    char dirname[BUFFER_SIZE];
    recv(client_sock, dirname, BUFFER_SIZE, 0);
    if (mkdir(dirname, 0777) == 0)
    {
        send_response(client_sock, "DirectoryCreated");
    }
    else
    {
        send_response(client_sock, "DirectoryCreationFailed");
    }
}

// Hàm thay đổi thư mục
void change_directory(int client_sock)
{
    char dirname[BUFFER_SIZE];
    recv(client_sock, dirname, BUFFER_SIZE, 0);

    // Lấy đường dẫn hiện tại trước khi thay đổi
    char current_path[BUFFER_SIZE];
    getcwd(current_path, BUFFER_SIZE);

    // Thử chuyển đến thư mục mới
    if (chdir(dirname) == 0)
    {
        // Lấy đường dẫn mới sau khi chuyển
        char new_path[BUFFER_SIZE];
        getcwd(new_path, BUFFER_SIZE);

        // Lấy đường dẫn gốc của user
        char base_path[BUFFER_SIZE];
        snprintf(base_path, BUFFER_SIZE, "%s/data/%s", getenv("PWD"), current_username);

        // Kiểm tra xem đường dẫn mới có bắt đầu bằng thư mục gốc của user không
        if (strstr(new_path, base_path) != NULL)
        {
            send_response(client_sock, "DirectoryChanged");
            printf("Chuyển đến thư mục: %s\n", new_path);
        }
        else
        {
            // Nếu ra khỏi thư mục của user, quay lại thư mục cũ
            chdir(current_path);
            send_response(client_sock, "AccessDenied");
            printf("Từ chối truy cập: %s\n", new_path);
        }
    }
    else
    {
        send_response(client_sock, "DirectoryChangeFailed");
        printf("Không thể chuyển đến thư mục: %s\n", dirname);
    }
}
void receive_file(int client_sock)
{
    char filename[BUFFER_SIZE];
    recv(client_sock, filename, BUFFER_SIZE, 0);

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
            send_response(client_sock, "UploadFailed");
            return;
        }
    }

    // Nhận kích thước file
    uint64_t filesize_net;
    if (recv_all(client_sock, &filesize_net, sizeof(filesize_net)) <= 0)
    {
        printf("Lỗi khi nhận kích thước file.\n");
        send_response(client_sock, "UploadFailed");
        return;
    }
    uint64_t filesize = ntohll(filesize_net);

    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        send_response(client_sock, "FileOpenError");
        return;
    }

    // Nhận nội dung file
    char buffer[BUFFER_SIZE];
    uint64_t totalReceived = 0;
    while (totalReceived < filesize)
    {
        size_t bytesToReceive = (filesize - totalReceived) < BUFFER_SIZE ? (filesize - totalReceived) : BUFFER_SIZE;
        ssize_t bytesReceived = recv_all(client_sock, buffer, bytesToReceive);
        if (bytesReceived <= 0)
        {
            printf("Lỗi khi nhận dữ liệu.\n");
            fclose(fp);
            send_response(client_sock, "UploadFailed");
            return;
        }
        fwrite(buffer, 1, bytesReceived, fp);
        totalReceived += bytesReceived;
    }
    fclose(fp);
    printf("Đã nhận file %s từ máy khách.\n", filename);
    send_response(client_sock, "UploadSuccess");
}

// Hàm gửi file tới máy khách (DOWNLOAD)
void send_file(int client_sock)
{
    char filename[BUFFER_SIZE];
    recv(client_sock, filename, BUFFER_SIZE, 0);

    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        send_response(client_sock, "FileNotFound");
        return;
    }
    else
    {
        send_response(client_sock, "FileFound");
    }

    // Lấy kích thước file
    fseek(fp, 0, SEEK_END);
    uint64_t filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Chuyển đổi kích thước file sang network byte order
    uint64_t filesize_net = htonll(filesize);

    // Gửi kích thước file
    send_all(client_sock, &filesize_net, sizeof(filesize_net));

    // Gửi nội dung file
    char buffer[BUFFER_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, fp)) > 0)
    {
        if (send_all(client_sock, buffer, bytesRead) != 0)
        {
            printf("Lỗi khi gửi dữ liệu.\n");
            fclose(fp);
            return;
        }
    }
    fclose(fp);
    printf("Đã gửi file %s tới máy khách.\n", filename);
}

// Hàm tìm kiếm file
void search_files(int client_sock)
{
    char query[BUFFER_SIZE];
    recv(client_sock, query, BUFFER_SIZE, 0);

    char results[BUFFER_SIZE] = "";
    search_in_directory(".", query, results);

    send_response(client_sock, results);
}

void search_in_directory(const char *dir_path, const char *query, char *results)
{
    DIR *d = opendir(dir_path);
    if (!d)
        return;

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL)
    {
        char path[BUFFER_SIZE];
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
            continue;
        snprintf(path, BUFFER_SIZE, "%s/%s", dir_path, dir->d_name);

        if (dir->d_type == DT_DIR)
        {
            search_in_directory(path, query, results);
        }
        else if (dir->d_type == DT_REG)
        {
            if (strstr(dir->d_name, query) != NULL)
            {
                strcat(results, path);
                strcat(results, "\n");
            }
        }
    }
    closedir(d);
}

void rename_file_server(int client_sock)
{
    char old_name[BUFFER_SIZE], new_name[BUFFER_SIZE];
    recv(client_sock, old_name, BUFFER_SIZE, 0);
    recv(client_sock, new_name, BUFFER_SIZE, 0);

    if (rename(old_name, new_name) == 0)
    {
        send_response(client_sock, "RenameSuccess");
        printf("Đã đổi tên file %s thành %s.\n", old_name, new_name);
    }
    else
    {
        send_response(client_sock, "RenameFailed");
        printf("Không thể đổi tên file %s.\n", old_name);
    }
}


void delete_file_server(int client_sock)
{
    char filename[BUFFER_SIZE];
    recv(client_sock, filename, BUFFER_SIZE, 0);

    if (remove(filename) == 0)
    {
        send_response(client_sock, "DeleteSuccess");
        printf("Đã xóa file %s.\n", filename);
    }
    else
    {
        send_response(client_sock, "DeleteFailed");
        printf("Không thể xóa file %s.\n", filename);
    }
}


void move_file_server(int client_sock)
{
    char filename[BUFFER_SIZE], destination[BUFFER_SIZE];
    
    if (recv_all(client_sock, filename, BUFFER_SIZE) < 0)
    {
        send_response(client_sock, "MoveFailed");
        printf("Lỗi khi nhận tên file.\n");
        return;
    }

    if (recv_all(client_sock, destination, BUFFER_SIZE) < 0)
    {
        send_response(client_sock, "MoveFailed");
        printf("Lỗi khi nhận đường dẫn đích.\n");
        return;
    }

    // Kiểm tra sự tồn tại của thư mục đích
    struct stat st;
    if (stat(destination, &st) == -1 || !S_ISDIR(st.st_mode))
    {
        send_response(client_sock, "MoveFailed");
        printf("Thư mục đích %s không tồn tại hoặc không phải là thư mục.\n", destination);
        return;
    }

    // Tạo đường dẫn đích đầy đủ
    char dest_path[BUFFER_SIZE];
    snprintf(dest_path, BUFFER_SIZE, "%s/%s", destination, basename(filename));

    // Thực hiện di chuyển file
    if (rename(filename, dest_path) == 0)
    {
        send_response(client_sock, "MoveSuccess");
        printf("Đã di chuyển file %s đến %s.\n", filename, dest_path);
    }
    else
    {
        send_response(client_sock, "MoveFailed");
        perror("rename");
        printf("Không thể di chuyển file %s đến %s.\n", filename, dest_path);
    }
}


void rename_directory_server(int client_sock)
{
    char old_dir[BUFFER_SIZE], new_dir[BUFFER_SIZE];
    recv(client_sock, old_dir, BUFFER_SIZE, 0);
    recv(client_sock, new_dir, BUFFER_SIZE, 0);

    if (rename(old_dir, new_dir) == 0)
    {
        send_response(client_sock, "RenameSuccess");
        printf("Đã đổi tên thư mục %s thành %s.\n", old_dir, new_dir);
    }
    else
    {
        send_response(client_sock, "RenameFailed");
        printf("Không thể đổi tên thư mục %s.\n", old_dir);
    }
}

int remove_directory(const char *path)
{
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = 0;

    if (d)
    {
        struct dirent *p;
        while (!r && (p = readdir(d)))
        {
            // Bỏ qua các thư mục hiện tại và thư mục cha
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
                continue;

            char *buf;
            size_t len = path_len + strlen(p->d_name) + 2;
            buf = malloc(len);
            if (buf)
            {
                snprintf(buf, len, "%s/%s", path, p->d_name);

                struct stat statbuf;
                if (!stat(buf, &statbuf))
                {
                    if (S_ISDIR(statbuf.st_mode))
                        r = remove_directory(buf); // Đệ quy xóa thư mục con
                    else
                        r = unlink(buf); // Xóa file
                }
                free(buf);
            }
        }
        closedir(d);
    }

    if (!r)
        r = rmdir(path); // Xóa thư mục sau khi đã xóa hết nội dung bên trong

    return r;
}
void delete_directory_server(int client_sock)
{
    char dirname[BUFFER_SIZE];
    if (recv_all(client_sock, dirname, BUFFER_SIZE) < 0)
    {
        send_response(client_sock, "DeleteFailed");
        printf("Lỗi khi nhận tên thư mục.\n");
        return;
    }

    // Kiểm tra sự tồn tại của thư mục
    struct stat st;
    if (stat(dirname, &st) == -1 || !S_ISDIR(st.st_mode))
    {
        send_response(client_sock, "DeleteFailed");
        printf("Thư mục %s không tồn tại hoặc không phải là thư mục.\n", dirname);
        return;
    }

    // Thực hiện xóa thư mục đệ quy
    if (remove_directory(dirname) == 0)
    {
        send_response(client_sock, "DeleteSuccess");
        printf("Đã xóa thư mục %s và tất cả nội dung bên trong.\n", dirname);
    }
    else
    {
        send_response(client_sock, "DeleteFailed");
        perror("remove_directory");
        printf("Không thể xóa thư mục %s.\n", dirname);
    }
}

void move_directory_server(int client_sock)
{
    char dirname[BUFFER_SIZE], destination[BUFFER_SIZE];
    recv(client_sock, dirname, BUFFER_SIZE, 0);
    recv(client_sock, destination, BUFFER_SIZE, 0);

    // Tạo đường dẫn đích đầy đủ
    char dest_path[BUFFER_SIZE];
    snprintf(dest_path, BUFFER_SIZE, "%s/%s", destination, basename(dirname));

    if (rename(dirname, dest_path) == 0)
    {
        send_response(client_sock, "MoveSuccess");
        printf("Đã di chuyển thư mục %s đến %s.\n", dirname, dest_path);
    }
    else
    {
        send_response(client_sock, "MoveFailed");
        printf("Không thể di chuyển thư mục %s.\n", dirname);
    }
}
