#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <openssl/sha.h>
#include <libnotify/notify.h>

#define MAX_PATH_LEN 256
#define HASH_SIZE SHA256_DIGEST_LENGTH

void show_notification(const char *message) {
    if (!notify_init("Hash Checker")) {
        fprintf(stderr, "Failed to initialize libnotify\n");
        exit(EXIT_FAILURE);
    }

    NotifyNotification *notification = notify_notification_new("Hash Mismatch", message, NULL);
    if (notification == NULL) {
        fprintf(stderr, "Failed to create notification\n");
        notify_uninit(); // Clean up libnotify
        exit(EXIT_FAILURE);
    }

    if (!notify_notification_show(notification, NULL)) {
        fprintf(stderr, "Failed to show notification\n");
        g_object_unref(G_OBJECT(notification));
        notify_uninit(); // Clean up libnotify
        exit(EXIT_FAILURE);
    }

    g_object_unref(G_OBJECT(notification));
    notify_uninit(); // Clean up libnotify
}

void calculate_hash(const char *path, unsigned char *hash) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        perror("Unable to open file");
        exit(EXIT_FAILURE);
    }

    SHA256_CTX sha256_context;
    SHA256_Init(&sha256_context);

    unsigned char buffer[1024];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) != 0) {
        SHA256_Update(&sha256_context, buffer, bytes_read);
    }

    SHA256_Final(hash, &sha256_context);

    fclose(file);
}

int main(int argc, char *argv[]) {
    // Parse command-line arguments
    int opt;
    while ((opt = getopt(argc, argv, "hf:")) != -1) {
        switch (opt) {
            case 'h':
                printf("Usage: %s [-f folder1 folder2 ...]\n", argv[0]);
                printf("  -f: Specify folders to monitor\n");
                return 0;
            case 'f':
                // Parse specified folders
                const int max_folders = 10;
                const char *folders[max_folders];
                int num_folders = 0;

                for (int i = optind - 1; i < argc; i++) {
                    if (num_folders >= max_folders) {
                        fprintf(stderr, "Too many folders specified\n");
                        return 1;
                    }
                    folders[num_folders++] = argv[i];
                }

                unsigned char current_hashes[num_folders][HASH_SIZE];
                unsigned char previous_hashes[num_folders][HASH_SIZE];

                while (1) {
                    for (int i = 0; i < num_folders; i++) {
                        calculate_hash(folders[i], current_hashes[i]);
                        if (memcmp(current_hashes[i], previous_hashes[i], HASH_SIZE) != 0) {
                            printf("Hash mismatch for folder %s!\n", folders[i]);
                            show_notification("Hash mismatch detected!");
                            memcpy(previous_hashes[i], current_hashes[i], HASH_SIZE);
                        }
                    }

                    if (sleep(60) != 0) {
                        perror("Failed to sleep");
                        exit(EXIT_FAILURE);
                    }
                }
                break;

            default:
                fprintf(stderr, "Usage: %s [-f folder1 folder2 ...]\n", argv[0]);
                return 1;
        }
    }

    fprintf(stderr, "Usage: %s [-f folder1 folder2 ...]\n", argv[0]);
    return 1;
}