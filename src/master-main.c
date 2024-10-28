#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "dynamic_hash_map_string.h"
//#include "data_structures/dynamic_hash_map_string_array.h"
#include "a_master_map.h"
#include <pthread.h>
#include "lru_hash_map.h"
#include "uthash.h"
int construct_hash_map_from_directory();
char *get(char *key);
int set(char * key, char * value);
void *compaction(void *arg);
int construct_hash_map_from_directory();
int command_line_interface();
void flush_db();
int PAGE_FAULT = 40;
char tombstone[] = "_TOMBSTONE_";
int FILE_LIMIT = 3;
int directory_buffer [1024];
int current_fd_buffer_index = -1;
int writer_thread_offset = 0;
size_t dir_byte_count;
int dir_fd;
int bit_flag = 0;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
master_hash_map_array *master_map;
typedef struct {
    char *path;              
    char *block_ptr;           
    UT_hash_handle hh;         
} HashEntry;
typedef struct{
    link_node* head;
    link_node* tail;
}doubly_linked_list;
typedef struct{
    int capacity; 
    int frame_size;
    HashEntry *page_table;
    char *block; 
    int * free_frame;
    lru_static_hash_map* lru_hash_map;
} buffer_pool;
char * read_from_buffer_pool(char *path);
link_node* eviction();
buffer_pool*manager;
doubly_linked_list *lru_cache;
void print_linked_list();
int table_add_key(HashEntry **page_table, char *path, char *block_ptr);
char* table_get_value(HashEntry *page_table, char *path);
void boot_up_buffer_pool();
void free_buffer_pool();
void change_buffer();

char *get(char *key){
    
    int file_index = -1;
    pthread_mutex_lock(&mtx);
    for (int i = current_fd_buffer_index; i > -1; i --){
        int *temp_array = get_value_array(master_get_value_array(master_map, directory_buffer[i]), key);
        if (temp_array[0] != nan_value){
            file_index = directory_buffer[i];
            break;
        }
    }
    if (file_index == -1){
        pthread_mutex_unlock(&mtx);
        return strdup("Key Does Not Exist");
        
    }
    char path[256];
    snprintf(path, sizeof(path), "db/%d", file_index);
    char *location = read_from_buffer_pool(path);
    int *arr = get_value_array(master_get_value_array(master_map, file_index), key);
    int offset = arr[0];
    //int length_of_record = arr[1];
    int size_of_key;
    int size_of_value;
    memcpy(&size_of_key, location+ offset, sizeof(int));
    memcpy(&size_of_value, location + offset+ sizeof(int), sizeof(int));
    char *found_key;
    found_key = (char*) malloc ((size_of_key + 1) * sizeof(char));
    memcpy(found_key, location+ offset + sizeof(int) + sizeof(int), size_of_key);
    found_key[size_of_key] = '\0';
    if (strcmp(found_key, key) != 0){
        pthread_mutex_unlock(&mtx);
        return strdup("Keys are not equal in DB");
    }
    char *found_value;
    found_value = (char*) malloc((size_of_value + 1) * sizeof(char));
    memcpy(found_value, location + offset + sizeof(int) + sizeof(int) + size_of_key, size_of_value);
    found_value[size_of_value] = '\0';
    if (strcmp(tombstone, found_value) == 0){
        pthread_mutex_unlock(&mtx);
        return strdup("Key Does Not Exist");
    }
    pthread_mutex_unlock(&mtx);
    free(found_key);
    link_node *node = lru_get_value(manager->lru_hash_map, path);
    node-> pin = 0;
    memset(path, 0, 256);
    return found_value;
}

void boot_up_buffer_pool(){
    manager = (buffer_pool*)malloc(sizeof(buffer_pool));
    manager->block = (char*)malloc(sizeof(char) * 40000);
    manager->capacity = 10;
    manager -> lru_hash_map = lru_construct_hash_map();
    manager->free_frame = (int*)malloc(sizeof(int) * manager->capacity);
    memset(manager->free_frame, 0, manager->capacity);
    manager->frame_size = 4000;
    manager-> page_table = NULL;
    lru_cache = (doubly_linked_list*)malloc(sizeof(doubly_linked_list));
    lru_cache->head = (link_node*)malloc(sizeof(link_node));
    lru_cache->tail = (link_node*)malloc(sizeof(link_node));
    char head[] = "0";
    char tail[] = "0";
    lru_cache->head->value = head;
    lru_cache->tail-> value = tail;
    lru_cache->head->next = lru_cache->tail;
    lru_cache->tail->prev = lru_cache->head;
    lru_cache->head->prev = NULL;
    lru_cache->tail->next = NULL;
}
char * read_from_buffer_pool(char *path){
    if (table_get_value(manager->page_table, path) == NULL){
        for (int i = 0; i < manager->capacity; i++){
            if (manager->free_frame[i] == 0){
                manager->free_frame[i] = 1;
                table_add_key(&manager -> page_table, path, &manager->block[i*manager->frame_size]);
                int fd = open(path, O_RDONLY, 0);
                link_node *node = (link_node*)malloc(sizeof(link_node));
                node->value = strdup(path);
                node->pin = 1;
                lru_add_key(manager->lru_hash_map, path, node);
                link_node *temp = lru_cache->head->next;
                lru_cache->head->next = node;
                node->prev = lru_cache->head;
                node->next = temp;
                temp->prev = node;
                char *location = table_get_value(manager-> page_table, path);
                read(fd, location, manager->frame_size);
                return location;
            }
        }
        link_node *node = eviction();
        char *location = table_get_value(manager->page_table, node->value);
        memset(location, 0, manager->frame_size);
        node->value = strdup(path);
        node->pin = 1;
        table_add_key(&manager -> page_table, path, location);
        int fd = open(path, O_RDONLY, 0);
        lru_add_key(manager->lru_hash_map, path, node);
        link_node *temp = lru_cache->head->next;
        lru_cache->head->next = node;
        node->prev = lru_cache->head;
        node->next = temp;
        temp->prev = node;
        read(fd, location, manager->frame_size);
        return location;
    }
    else{
        char * location = table_get_value(manager-> page_table, path);
        link_node *node = lru_get_value(manager->lru_hash_map, path);
        node->pin = 1;
        link_node *next = node->next;
        link_node *prev = node->prev;
        prev->next = next;
        next->prev = prev;
        link_node * temp = lru_cache->head->next;
        lru_cache->head->next = node;
        node->prev = lru_cache->head;
        node->next = temp;
        temp->prev = node;
        return location;
    }
}
link_node* eviction(){
    link_node *node = lru_cache->tail->prev;
    while (node->pin != 0){
        node = node->prev;
    }
    link_node *prev = node-> prev;
    link_node *next = node-> next;
    node->next = NULL;
    node->prev = NULL;
    prev->next = next;
    next->prev = prev;
    return node;
}
int table_add_key(HashEntry **page_table, char *path, char *block_ptr) {
    HashEntry *entry = (HashEntry *)malloc(sizeof(HashEntry));

    entry->path = strdup(path);
    entry->block_ptr = block_ptr;
    HASH_ADD_STR(*page_table, path, entry);
    return 0;
}
char* table_get_value(HashEntry *page_table, char *path) {
    HashEntry *entry;
    HASH_FIND_STR(page_table, path, entry);
    if (entry) {
        return entry->block_ptr;
    } else {
        return NULL;
    }
}
void print_linked_list(){
    link_node * node = lru_cache->head;
    node = node->next;
    while (node != lru_cache->tail){
        printf("string(path) value: %s \n", node->value);
        node = node->next;
    }
}
void free_buffer_pool(){
    memset(manager->block, 0, manager->capacity * manager->frame_size);
    free(manager->block);
    free(manager->free_frame);
    free(manager);
}
char * write_to_buffer_pool(char * path, int total_length){
    if (writer_thread_offset + total_length >= manager->frame_size){
        change_buffer();
    } 
    char * location = table_get_value(manager-> page_table, path);
    link_node *node = lru_get_value(manager->lru_hash_map, path);
    link_node *next = node->next;
    link_node *prev = node->prev;
    prev->next = next;
    next->prev = prev;
    link_node * temp = lru_cache->head->next;
    lru_cache->head->next = node;
    node->prev = lru_cache->head;
    node->next = temp;
    temp->prev = node;
    return location;
}
void change_buffer(){
    int *copy = malloc(sizeof(int));
    *copy = current_fd_buffer_index;
    directory_buffer[current_fd_buffer_index + 1] = directory_buffer[current_fd_buffer_index]+1;
    current_fd_buffer_index++;
    //int file_number = directory_buffer[current_fd_buffer_index];
    dir_byte_count = sizeof(directory_buffer);
    lseek(dir_fd, 0L, 2);
    write(dir_fd, &directory_buffer[current_fd_buffer_index],(sizeof(int)));
    char path[256];
    snprintf(path, sizeof(path), "db/%d", directory_buffer[current_fd_buffer_index]);
    int fd = open(path, O_RDWR|O_CREAT, 0666);
    close(fd);
    memset(path, 0, 256);
    snprintf(path, sizeof(path), "db/%d", directory_buffer[*copy]);
    link_node *node = lru_get_value(manager->lru_hash_map, path);
    node -> pin = 0;
    memset(path, 0, 256);
    snprintf(path, sizeof(path), "db/%d", directory_buffer[current_fd_buffer_index]);
    fd = open(path, O_RDWR|O_CREAT, 0666);
    close(fd);
    read_from_buffer_pool(path);
    writer_thread_offset = 0;
    static_hash_map_array *map = construct_hash_map_array();
    master_add_key_array(master_map, directory_buffer[current_fd_buffer_index], map);
    if (current_fd_buffer_index > FILE_LIMIT && bit_flag == 0){
        bit_flag = 1;
        pthread_t t1;
        pthread_create(&t1, NULL, compaction, copy);
        pthread_detach(t1);
    }
    
}
int set(char * key, char * value){
    int file_number;
    pthread_mutex_lock(&mtx);
    file_number = directory_buffer[current_fd_buffer_index]; 
    //pthread_mutex_unlock(&mtx);
    //thread onlock
    char path[256];
    snprintf(path, sizeof(path), "db/%d", file_number); 
    int key_length = strlen(key);
    int val_length = strlen(value);
    int total_length = (2 * sizeof(int)) + key_length + val_length;
    char *buf = write_to_buffer_pool(path, total_length);
    memset(path, 0, 256);
    int *arr = (int *)malloc(2 * sizeof(int));
    memcpy(buf + writer_thread_offset, &key_length, sizeof(int));
    memcpy(buf + writer_thread_offset+sizeof(int), &val_length, sizeof(int));
    memcpy(buf + writer_thread_offset + (2 * sizeof(int)), key, key_length);
    memcpy(buf + writer_thread_offset + (2* sizeof(int)) + key_length, value, val_length);
    total_length = (2 * sizeof(int)) + key_length + val_length;
    //buf[total_length] = '\0';
    snprintf(path, sizeof(path), "db/%d", directory_buffer[current_fd_buffer_index]);
    int fd = open(path, O_RDWR, 0666);
    long pos = lseek(fd, 0L, 2);
    int bytes_written;
    bytes_written = write(fd, buf+writer_thread_offset, total_length);
    writer_thread_offset += total_length;
    close(fd);
    if (bytes_written == total_length){
        arr[0] = pos;
        arr[1] = total_length;
        add_key_array(master_get_value_array(master_map, directory_buffer[current_fd_buffer_index]), key, arr);
        pthread_mutex_unlock(&mtx);
        return 0;
    }
    pthread_mutex_unlock(&mtx);
    return -1;
}
void *compaction(void *arg){
    int copy = *(int *)arg;
    static_hash_map *temp_map = construct_hash_map();
    char path [256];
    int current_fd_buffer_index_copy = copy;
    int original_buffer_index = copy;
    int fd;
    char file_buffer [4096];
    int deleted_group [current_fd_buffer_index_copy];
    for (int i = 0; i < current_fd_buffer_index_copy+1; i++){
        snprintf(path, sizeof(path), "db/%d", directory_buffer[i]);
        fd = open(path, O_RDONLY, 0666);
        memset(path, 0, 256);
        lseek(fd, 0L, 0);
        deleted_group[i] = directory_buffer[i];
        int bytes_read = read(fd, file_buffer, 1024);
        int key_size;
        int value_size;
        int j = 0;
        while (j < bytes_read){
            memcpy(&key_size, file_buffer + j, sizeof(int));
            memcpy(&value_size, file_buffer+ j + sizeof(int), sizeof(int));
            char * key = (char*) malloc(key_size+1);
            char * value = (char *) malloc(value_size+1);
            memcpy(key, file_buffer + j + sizeof(int) + sizeof(int), key_size);
            memcpy(value, file_buffer + j + sizeof(int) + sizeof(int) + key_size, value_size);
            key[key_size] = '\0';
            value[value_size] = '\0';
            if (strcmp(value, tombstone) != 0){
                add_key(temp_map, key, value);
            }
            j = j + sizeof(int) + sizeof(int) + key_size + value_size;
        }
    }
    for (int i=0; i < temp_map->total_size; i++) {
        if (strcmp(temp_map->hash_map[i].key, null_value_array) == 0){
            continue;
        }
        set(temp_map->hash_map[i].key, temp_map->hash_map[i].value);
    }
    int new_directory_buffer [1024];
    int j = 0;
    for (int i = original_buffer_index+1; i < current_fd_buffer_index+1;i++){
        new_directory_buffer[j] = directory_buffer[i];
        j ++;
    }
    pthread_mutex_lock(&mtx);
    for (int i = 0; i < original_buffer_index+1; i++){
        char path[256];
        snprintf(path, sizeof(path), "db/%d", directory_buffer[i]);
        master_delete_key_array(master_map, directory_buffer[i]);
        remove(path);
        memset(path, 0, 256);
    }
    current_fd_buffer_index = current_fd_buffer_index - (original_buffer_index+1);
    memcpy(directory_buffer, new_directory_buffer, sizeof(int) * (current_fd_buffer_index+1));
    write(dir_fd, new_directory_buffer, sizeof(int) * (current_fd_buffer_index+1));
    free_memory_hash_map(temp_map);
    pthread_mutex_unlock(&mtx);
    bit_flag = 0;
    return NULL;
    
}
int construct_hash_map_from_directory(){
    writer_thread_offset = 0;
    master_map = master_construct_hash_map_array();
    boot_up_buffer_pool();
    dir_fd = open("db/directory", O_RDWR|O_CREAT, 0666);
    if (dir_fd == -1){
        //printf("File Could not be opened\n");
        return -1;
    }
    int bytes_read_dir = read(dir_fd, directory_buffer, 1024);
    int fd;
    if (bytes_read_dir == 0){
       static_hash_map_array *map = construct_hash_map_array();
       master_add_key_array(master_map, 0, map);
       directory_buffer[0] = 0;
       dir_byte_count = sizeof(int);
       write(dir_fd, directory_buffer,dir_byte_count);
       char path[256];
       snprintf(path, sizeof(path), "db/%d", directory_buffer[0]);
       fd = open(path, O_RDWR|O_CREAT, 0666);
       current_fd_buffer_index = 0;
       read_from_buffer_pool(path);
       memset(path, 0, 256);
    }
    else{
        int file_descriptors_written = bytes_read_dir / sizeof(int);
        char file_buffer[1024];
        for (int i = 0; i < file_descriptors_written; i++){
            static_hash_map_array *map = construct_hash_map_array();
            master_add_key_array(master_map, i, map);
            char path[256];
            snprintf(path, sizeof(path), "db/%d", directory_buffer[i]);
            fd = open(path, O_RDONLY, 0);
            memset(path, 0, 256);
            int bytes_read = read(fd, file_buffer, 1024);
            int key_size;
            int value_size;
            int j = 0;
            while (j < bytes_read){
                memcpy(&key_size, file_buffer + j, sizeof(int));
                memcpy(&value_size, file_buffer+ j + sizeof(int), sizeof(int));
                char * key = (char*) malloc(key_size+1);
                char * value = (char *) malloc(value_size+1);
                memcpy(key, file_buffer + j + sizeof(int) + sizeof(int), key_size);
                memcpy(value, file_buffer + j + sizeof(int) + sizeof(int) + key_size, value_size);
                key[key_size] = '\0';
                value[value_size] = '\0';
                int *arr = (int*)malloc(2 * sizeof(int));
                arr[0] = j;
                arr[1] = sizeof(int) + sizeof(int) + key_size + value_size;
                add_key_array(map, key, arr);
                j = j + sizeof(int) + sizeof(int) + key_size + value_size;
                free(key);
                free(value);
            }
            //writer_thread_offset = j;
            close(fd);
            current_fd_buffer_index = i;
            
        }
    }
    return 0;
}
    int command_line_interface(){
        int return_value = construct_hash_map_from_directory();
        if (return_value == -1){
            return -1;
        }
        char path[256];
        snprintf(path, sizeof(path), "db/%d", directory_buffer[current_fd_buffer_index]);
        int fd = open(path, O_RDWR, 0);
        memset(path, 0, 256);
        char command[3];
        char temp_buffer [1000];
        char* user_input_key;
        char * user_input_value;
        size_t length;
        while(1){
            printf("Get OR Set: ");
            scanf("%3s", command);
            if (strcmp(command, "Set") == 0){
                printf("Enter Key: ");
                scanf("%999s", temp_buffer);
                length = strlen(temp_buffer);
                user_input_key = (char *) malloc((length+1)* sizeof(char));
                strcpy(user_input_key, temp_buffer);
                memset(temp_buffer, 0, 1000);
                printf("Enter Value: ");
                scanf("%999s", temp_buffer);
                length = strlen(temp_buffer);
                user_input_value = (char*)malloc((length +1) * sizeof(char));
                strcpy(user_input_value, temp_buffer);
                memset(temp_buffer, 0, 1000);
                printf("directory buffer of current file index %d\n", directory_buffer[current_fd_buffer_index]);
                int return_error = set(user_input_key, user_input_value);
                if (return_error == -1){
                    printf("Error occured inserting Key & Value\n");
                }
                else{
                    printf("Succesfully Inserted Key: %s \n", user_input_key);

                }
                memset(command, 0, sizeof(command));
                free(user_input_key);
                free(user_input_value);
                memset(temp_buffer, 0, 1000);
            }
            else if (strcmp(command, "Get") == 0){
                printf("Enter Key: ");
                scanf("%999s", temp_buffer);
                length = strlen(temp_buffer);
                user_input_key = (char *) malloc((length+1)* sizeof(char));
                strcpy(user_input_key, temp_buffer);
                printf("Directory buffer of current file index, %d\n", directory_buffer[current_fd_buffer_index]);
                printf("Current fd index %d \n", current_fd_buffer_index);
                printf("Current size of directory buffer %lu \n", sizeof(directory_buffer) / sizeof(int));
                char * return_value = get(user_input_key);
                printf("Key: %s, -> Value: %s\n", user_input_key, return_value);
                memset(command, 0, sizeof(command));
                free(user_input_key);
                memset(temp_buffer, 0, 1000);
            }
            else if (strcmp(command, "DEL") == 0){
                printf("Enter Key: ");
                scanf("%999s", temp_buffer);
                length = strlen(temp_buffer);
                user_input_key = (char *) malloc((length+1)* sizeof(char));
                strcpy(user_input_key, temp_buffer);
                int return_error = set(user_input_key, tombstone);
                if (return_error == -1){
                    printf("Error occured Deleting Key & Value\n");
                }
                else{
                    printf("Succesfully Deleted Key: %s \n", user_input_key);
                }
                free(user_input_key);
                memset(command, 0, sizeof(command));
                memset(temp_buffer, 0, 1000);
            }
            else{
                memset(command, 0, sizeof(command));
                continue;
            }
            close(fd);
        }
        return 0;
    }
    void flush_db(){
        for (int i = 0; i < current_fd_buffer_index + 1; i++){
            char path[256];
            snprintf(path, sizeof(path), "db/%d", directory_buffer[i]);
            master_delete_key_array(master_map, directory_buffer[i]);
            remove(path);
        }
        // flush the directory too
        current_fd_buffer_index = 0;
        char path[] = "db/directory";
        remove(path);
        free_buffer_pool();
    }


