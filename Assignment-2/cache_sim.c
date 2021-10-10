#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>   // So i can use log()
#include <limits.h> // So I can use CHAR_BIT to convert from bytes to bit

typedef enum
{
    dm,
    fa
} cache_map_t;
typedef enum
{
    uc,
    sc
} cache_org_t;
typedef enum
{
    instruction,
    data
} access_t;

typedef struct
{
    uint32_t address;
    access_t accesstype;
} mem_access_t;

typedef struct
{
    uint64_t accesses;
    uint64_t hits;
    // You can declare additional statistics if
    // you like, however you are now allowed to
    // remove the accesses or hits
} cache_stat_t;

typedef struct
{
    uint32_t *cache;
    uint32_t count;
} FA_cache;
// DECLARE CACHES AND COUNTERS FOR THE STATS HERE

uint32_t cache_size;
uint32_t block_size = 64;
cache_map_t cache_mapping;
cache_org_t cache_org;
uint32_t *cache_instructions;
uint32_t *cache_data;
uint32_t *cache_data_and_instructions;
uint32_t count = 0;
// Cache info

// USE THIS FOR YOUR CACHE STATISTICS
cache_stat_t cache_statistics;

/* Reads a memory access from the trace file and returns
 * 1) access type (instruction or data access
 * 2) memory address
 */
mem_access_t read_transaction(FILE *ptr_file)
{
    char buf[1000];
    char *token;
    char *string = buf;
    mem_access_t access;

    if (fgets(buf, 1000, ptr_file) != NULL)
    {

        /* Get the access type */
        token = strsep(&string, " \n");
        if (strcmp(token, "I") == 0)
        {
            access.accesstype = instruction;
        }
        else if (strcmp(token, "D") == 0)
        {
            access.accesstype = data;
        }
        else
        {
            printf("%s\n", token);
            printf("Unknown access type\n");
            exit(0);
        }

        /* Get the access type */
        token = strsep(&string, " \n");
        access.address = (uint32_t)strtol(token, NULL, 16);

        return access;
    }

    /* If there are no more entries in the file,
     * return an address 0 that will terminate the infinite loop in main
     */
    access.address = 0;
    return access;
}

uint32_t get_num_of_blocks()
{
    return cache_size / block_size;
}
uint32_t get_offset()
{
    return log(block_size) / log(2);
}
uint32_t get_index_size()
{
    return log(get_num_of_blocks()) / log(2);
}
uint32_t get_tag_size()
{
    return ((CHAR_BIT * sizeof(uint32_t)) - get_index_size() - get_offset());
}
uint32_t get_index(uint32_t address)
{
    return (((1 << get_index_size()) - 1) & (address >> get_offset()));
}
uint32_t get_tag_DM(uint32_t address)
{
    return (((1 << get_tag_size()) - 1) & (address >> ((get_offset() + get_index_size()) - 1)));
}
uint32_t get_tag_FA(uint32_t address)
{
    return (((1 << get_tag_size() + get_index_size()) - 1) & (address >> get_offset()));
}

void add(uint32_t element, uint32_t *cache)
{
    if (count == get_num_of_blocks())
    {
        delet(cache);
        add(element, cache);
    }
    else
    {
        cache[count] = element;
        count++;
    }
}
void delet(uint32_t *cache)
{
    for (int i = 0; i < get_num_of_blocks() - 1; i++)
    {
        cache[i] = cache[i + 1];
    }
    count--;
}
void check_cache_DM(uint32_t address, uint32_t *cache)
{
    cache_statistics.accesses++;
    uint32_t index = get_index(address);
    uint32_t tag = get_tag_DM(address);
    if (cache[index] != 0)
    {
        if (cache[index] == tag)
        {
            cache_statistics.hits++;
        }
        else
        {
            cache[index] = tag;
        }
    }
    else
    {
        cache[index] = tag;
    }
}
void check_cache_FA(uint32_t address, uint32_t *cache)
{
    cache_statistics.accesses++;
    uint32_t tag = get_tag_FA(address);
    for (int i = 0; i < get_num_of_blocks(); i++)
    {
        if (cache[i] == tag)
        {
            cache_statistics.hits++;
            return;
        }
    }
    add(tag, cache);
}

void main(int argc, char **argv)
{

    // Reset statistics:
    memset(&cache_statistics, 0, sizeof(cache_stat_t));

    /* Read command-line parameters and initialize:
     * cache_size, cache_mapping and cache_org variables
     */

    if (argc != 4)
    { /* argc should be 2 for correct execution */
        printf("Usage: ./cache_sim [cache size: 128-4096] [cache mapping: dm|fa] [cache organization: uc|sc]\n");
        exit(0);
    }
    else
    {
        /* argv[0] is program name, parameters start with argv[1] */

        /* Set cache size */
        cache_size = atoi(argv[1]);

        /* Set Cache Mapping */
        if (strcmp(argv[2], "dm") == 0)
        {
            cache_mapping = dm;
        }
        else if (strcmp(argv[2], "fa") == 0)
        {
            cache_mapping = fa;
        }
        else
        {
            printf("Unknown cache mapping\n");
            exit(0);
        }

        /* Set Cache Organization */
        if (strcmp(argv[3], "uc") == 0)
        {
            cache_org = uc;
        }
        else if (strcmp(argv[3], "sc") == 0)
        {
            cache_org = sc;
        }
        else
        {
            printf("Unknown cache organization\n");
            exit(0);
        }
    }
    /* Open the file mem_trace.txt to read memory accesses */
    FILE *
        ptr_file;
    ptr_file = fopen("mem_trace_2.txt", "r");
    if (!ptr_file)
    {
        printf("Unable to open the trace file\n");
        exit(1);
    }
    mem_access_t access;
    switch (cache_mapping)
    {

    case dm:
        if (cache_org == sc)
        {
            uint32_t size_of_cache = get_num_of_blocks() / 2;
            cache_instructions = (uint32_t *)malloc(sizeof(uint32_t) * size_of_cache);
            cache_data = (uint32_t *)malloc(sizeof(uint32_t) * size_of_cache);

            if (cache_instructions == NULL || cache_data == NULL)
            {
                exit(1); //failed to allocate memory for our cache
            }
            while (1)
            {
                access = read_transaction(ptr_file);
                //If no transactions left, break out of loop
                if (access.address == 0)
                    break;
                if (access.accesstype == instruction)
                {
                    check_cache_DM(access.address, cache_instructions);
                }
                else
                {
                    check_cache_DM(access.address, cache_data);
                }
            }
            free(cache_instructions);
            free(cache_data);
        }
        else
        {
            cache_data_and_instructions = (uint32_t *)malloc(sizeof(uint32_t) * get_num_of_blocks());
            if (cache_data_and_instructions == NULL)
            {
                exit(1); //failed to allocate memory for our cache
            }
            while (1)
            {
                access = read_transaction(ptr_file);
                //If no transactions left, break out of loop
                if (access.address == 0)
                    break;
                check_cache_DM(access.address, cache_data_and_instructions);
            }
            free(cache_data_and_instructions);
        }

        break;
    case fa:
        if (cache_org == sc)
        {

            uint32_t size_of_cache = get_num_of_blocks() / 2;
            cache_instructions = (uint32_t *)malloc(sizeof(uint32_t) * size_of_cache);
            cache_data = (uint32_t *)malloc(sizeof(uint32_t) * size_of_cache);

            if (cache_instructions == NULL || cache_data == NULL)
            {
                exit(1); //failed to allocate memory for our cache
            }
            while (1)
            {
                access = read_transaction(ptr_file);
                //If no transactions left, break out of loop
                if (access.address == 0)
                    break;
                if (access.accesstype == instruction)
                {
                    check_cache_FA(access.address, cache_instructions);
                }
                else
                {
                    check_cache_FA(access.address, cache_data);
                }
            }
            free(cache_instructions);
            free(cache_data);
        }
        else
        {
            cache_data_and_instructions = (uint32_t *)malloc(sizeof(uint32_t) * get_num_of_blocks());
            if (cache_data_and_instructions == NULL)
            {
                exit(1); //failed to allocate memory for our cache
            }
            while (1)
            {
                access = read_transaction(ptr_file);
                //If no transactions left, break out of loop
                if (access.address == 0)
                    break;
                check_cache_FA(access.address, cache_data_and_instructions);
            }
            free(cache_data_and_instructions);
        }

        break;

    default:
        printf("Invalid input");
        break;
    }

    /* Print the statistics */
    printf("Number of blocks: %d\n", get_num_of_blocks());
    printf("Size of offset: %d\n", get_offset());
    printf("Size of index: %d\n", get_index_size());
    printf("Size of tag: %d\n", get_tag_size());

    // DO NOT CHANGE THE FOLLOWING LINES!
    printf("\nCache Statistics\n");
    printf("-----------------\n\n");
    printf("Accesses: %ld\n", cache_statistics.accesses);
    printf("Hits:     %ld\n", cache_statistics.hits);
    printf("Hit Rate: %.4f\n", (double)cache_statistics.hits / cache_statistics.accesses);
    // You can extend the memory statistic printing if you like!

    /* Close the trace file */
    fclose(ptr_file);
}