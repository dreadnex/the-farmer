#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <openssl/evp.h>
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

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        perror("Failed to create EVP_MD_CTX");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    if (!EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL)) {
        perror("Failed to initialize digest");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    unsigned char buffer[1024];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) != 0) {
        if (!EVP_DigestUpdate(mdctx, buffer, bytes_read)) {
            perror("Failed to update digest");
            EVP_MD_CTX_free(mdctx);
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    if (!EVP_DigestFinal_ex(mdctx, hash, NULL)) {
        perror("Failed to finalize digest");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    EVP_MD_CTX_free(mdctx);
    fclose(file);
}

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "hf:")) != -1) {
        switch (opt) {
            case 'h':
                printf(" The Farmer, An Adrian Gaithuma Tool ; Usage: %s [-f folder1 folder2 ...]\n", argv[0]);
                printf("  -f: Specify folders to monitor\n");
                return 0;
            case 'f':
                const int max_folders = 10;
                char **folders = (char **)malloc(max_folders * sizeof(char *));
                if (folders == NULL) {
                    perror("Memory allocation failed");
                    return 1;
                }
                int num_folders = 0;

                for (int i = optind; i < argc; i++) {
                    if (num_folders >= max_folders) {
                        fprintf(stderr, "Too many folders specified\n");
                        return 1;
                    }
                    folders[num_folders++] = argv[i];
                }

                unsigned char **current_hashes = (unsigned char **)malloc(num_folders * sizeof(unsigned char *));
                unsigned char **previous_hashes = (unsigned char **)malloc(num_folders * sizeof(unsigned char *));
                if (current_hashes == NULL || previous_hashes == NULL) {
                    perror("Memory allocation failed");
                    return 1;
                }

                for (int i = 0; i < num_folders; i++) {
                    current_hashes[i] = (unsigned char *)malloc(HASH_SIZE);
                    previous_hashes[i] = (unsigned char *)malloc(HASH_SIZE);
                    if (current_hashes[i] == NULL || previous_hashes[i] == NULL) {
                        perror("Memory allocation failed");
                        return 1;
                    }
                }

                unsigned char hash[HASH_SIZE];

                while (1) {
                    for (int i = 0; i < num_folders; i++) {
                        calculate_hash(folders[i], hash);
                        if (memcmp(hash, previous_hashes[i], HASH_SIZE) != 0) {
                            printf("Hash mismatch for folder %s!\n", folders[i]);
                            show_notification("Hash mismatch detected!");
                            memcpy(previous_hashes[i], hash, HASH_SIZE);
                        }
                    }

                    if (sleep(60) != 0) {
                        perror("Failed to sleep");
                        // Clean up allocated memory
                        for (int i = 0; i < num_folders; i++) {
                            free(current_hashes[i]);
                            free(previous_hashes[i]);
                        }
                        free(current_hashes);
                        free(previous_hashes);
                        free(folders);
                        exit(EXIT_FAILURE);
                    }
                }

                // Clean up allocated memory
                for (int i = 0; i < num_folders; i++) {
                    free(current_hashes[i]);
                    free(previous_hashes[i]);
                }
                free(current_hashes);
                free(previous_hashes);
                free(folders);
                
                break;

            default:
                fprintf(stderr, "Usage: %s [-f folder1 folder2 ...]\n", argv[0]);
                return 1;
        }
    }

    fprintf(stderr, "Usage: %s [-f folder1 folder2 ...]\n", argv[0]);
    return 1;
}
