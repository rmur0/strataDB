#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
typedef struct {
    int key;
    int value;
}map_item;
typedef struct{
    map_item * hash_map;
    int current_occupation;
    int total_size;
} static_hash_map;
static_hash_map *map = NULL;
void construct_hash_map();
int hash_function(int key,int total_size);
static_hash_map* resize(static_hash_map *map);
int add_key(int key, int value);
int get_value(int key);
int get_size();
int delete_key(int key);
void print_hash_map();
void free_memory_hash_map();
void construct_hash_map(){
    //printf("Construction Called \n");
    map = (static_hash_map*)malloc(sizeof(static_hash_map));
    //printf("Address: %p\n", map);
    map_item mapping;
    mapping.key = -1;
    mapping.value = -1;
    int size = 2;
    map->hash_map = (map_item*)malloc(sizeof(map_item) * size);
    map->current_occupation = 0;
    map->total_size = size;
    for (int i = 0; i < map->total_size; i++){
        map->hash_map[i] = mapping; 
        printf("mapping %d : %d\n", i, map->hash_map[i].key);
    }
    printf("Pointer to map %p \n", map);
}
int hash_function(int key,int total_size){
    printf("total size: %d\n", total_size);
    return key% total_size;
}
int add_key_resize(static_hash_map* new_map, int key, int value){
    printf("Pointer to map add_key %p \n", new_map);
    printf("total size : %d\n", new_map->total_size);
    int hash_value = hash_function(key, new_map->total_size);
    printf("Hash Function %d \n", hash_value);
    printf("actual key %d \n", key);
    printf("Hash key %d \n", new_map->hash_map[hash_value].key);
    while (new_map->hash_map[hash_value].key != -1 && new_map->hash_map[hash_value].value != -1){
        printf("KEY %d\n",new_map->hash_map[hash_value].key);
        printf("resize \n");
        printf("Current occupation %d \n",new_map->current_occupation);
        printf("Total Capacity %d \n",new_map->total_size);
        printf("Hash Function %d \n", hash_value);
        if (new_map->hash_map[hash_value].key == key){
            new_map->hash_map[hash_value].value = value;
            return 0;
        }
        if (hash_value + 1 == map->total_size){
            hash_value = 0;
        }
        else{
            hash_value ++;
        }   
        }
    map_item mapping;
    mapping.key = key;
    mapping.value = value;

    new_map->hash_map[hash_value] = mapping;
    new_map->current_occupation ++;
    return 0;
}
static_hash_map * resize(static_hash_map *map){
    printf("Resize called \n");
    printf("Pointer to map inside resize %p \n", map);
    int new_size = map->total_size * 2;
    static_hash_map *new_map = (static_hash_map*)malloc(sizeof(static_hash_map));
    new_map->hash_map = (map_item*)malloc(sizeof(map_item) * new_size);
    new_map->current_occupation = 0;
    new_map->total_size = new_size;
    map_item mapping;
    mapping.key = -1;
    mapping.value = -1;
    for (int i = 0; i < new_map->total_size; i++){
        new_map->hash_map[i] = mapping; 
    }
    printf("Map current ocu %d\n", map->current_occupation);
    for (int i=0; i < map->total_size; i++){
        printf("i%d: %d\n", i, map->hash_map[i].key);
        if (map->hash_map[i].key != -1 && map->hash_map[i].value != -1){
            printf("resize key %d\n", map->hash_map[i].key);
            add_key_resize(new_map, map->hash_map[i].key, map->hash_map[i].value);
        }
    }
    free(map->hash_map);
    map = new_map;
    return map;
}
int add_key(int key, int value){
    printf("Pointer to map add_key %p \n", map);
    if (map->current_occupation == (map->total_size >> 1)){
        printf("Current occupation %d \n",map->current_occupation);
        map = resize(map);
        printf("Pointer to map post resize %p \n", map);
    }
    int hash_value = hash_function(key, map->total_size);
    printf("Hash Function %d \n", hash_value);
    printf("actual key %d \n", key);
    printf("Hash key %d \n", map->hash_map[hash_value].key);
    while (map->hash_map[hash_value].key != -1 && map->hash_map[hash_value].value != -1){
        printf("KEY %d\n",map->hash_map[hash_value].key);
        printf("Map total size %d \n", map-> total_size);
        printf("Current occupation %d \n",map->current_occupation);
        printf("Total Capacity %d \n",map->total_size);
        printf("Hash Function %d \n", hash_value);
        if (map->hash_map[hash_value].key == key){
            map->hash_map[hash_value].value = value;
            return 0;
        }
        if (hash_value + 1 == map->total_size){
            hash_value = 0;
        }
        else{
            hash_value ++;
        }   
        }
    map_item mapping;
    mapping.key = key;
    mapping.value = value;

    map->hash_map[hash_value] = mapping;
    map->current_occupation ++;
    return 0;
}

int get_value(int key){
    int hash_value = hash_function(key, map->total_size);
    int original_hash = hash_value;
    while (map->hash_map[hash_value].key != -1){
        if (hash_value == original_hash -1){
            break;
        }
        if (map->hash_map[hash_value].key == key && map->hash_map[hash_value].value != -1){
            return map->hash_map[hash_value].value;
        }
        if (map->hash_map[hash_value].key == key && map->hash_map[hash_value].value == -1){
            //ran into a tombstone meaning value does not exist
            break;
        }
        if (hash_value + 1 == map->total_size){
            hash_value = 0;
        }
        else{
            hash_value ++;
        }   
        }
    printf("No value Found for key %d \n", key);
    return -1;
}
int get_size(){
    return map->current_occupation;
}
int delete_key(int key){
    int hash_value = hash_function(key, map->total_size);
    int original_hash = hash_value;
    while (map->hash_map[hash_value].key != -1){
        if (hash_value == original_hash -1){
            break;
        }
        if (map->hash_map[hash_value].key == key){
            map->current_occupation -= 1;
            map->hash_map[hash_value].value = -1;
            return 1;
        }
        if (hash_value + 1 == map->total_size){
            hash_value = 0;
        }
        else{
            hash_value ++;
        }
    }
    printf("Deleted key: %d not found \n", key);
    return -1;
}
void print_hash_map(){
    printf("current occupation %d\n", map->current_occupation);
    printf("{");
    int printed_keys = 0;
    for (int i=0; i < map->total_size; i++){
        if (map->hash_map[i].key != -1){
            printed_keys ++;
            printf("%d:%d", map->hash_map[i].key, map->hash_map[i].value);
            if (printed_keys != map->current_occupation){
                printf(",");
            }
        }
    }
    printf("}\n");
}

void free_memory_hash_map() {
    if (map != NULL) {
        if (map->hash_map != NULL) {
            printf("Not null \n");
            free(map->hash_map);
            map->hash_map = NULL;
        }
        free(map);
        map = NULL;
    }
}