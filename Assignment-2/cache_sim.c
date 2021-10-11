#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>   // So i can use log()
#include <limits.h> // So I can use CHAR_BIT to convert from bytes to bit

typedef enum
{
    DM,
    FA
} cache_map_t;

typedef enum
{
    UC,
    SC
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

} cache_stat_t;

uint32_t total_cache_size;
uint32_t cache_partition_size;
uint32_t block_size = 64;
cache_map_t cache_mapping;
cache_org_t cache_org;

// If it's a split cache we have two different caches for instructions and data
uint32_t *cache_instructions;
uint32_t *cache_data;

// If it's a unified cache we only have one,
// I could have used one of the caches above but for code readability I made a separat cache for it
uint32_t *cache_data_and_instructions;

uint32_t count = 0; // used in FIFO to keep track of how full the cache is
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

// Method to extract a given number of bits from a set of bits, with a certian starting point in that bit set
/***************************************************************************************
*    Title: Extract ‘k’ bits from a given position in a number
*    Author: GeeksforGeeks
*    Date: 26 May, 2021
*    Availability: https://www.geeksforgeeks.org/extract-k-bits-given-position-number/
***************************************************************************************/
uint32_t extract_bits(uint32_t bits, uint32_t nr_of_bits_to_extract, uint32_t start_position)
{
    return (((1 << nr_of_bits_to_extract) - 1) & (bits >> (start_position)));
}

// Getting the different sizes
uint32_t num_of_blocks()
{
    return (total_cache_size / block_size);
}

uint32_t offset()
{
    return log(block_size) / log(2);
}

uint32_t index_size()
{
    return log(cache_partition_size) / log(2);
}

uint32_t tag_size()
{
    return ((CHAR_BIT * sizeof(uint32_t)) - index_size() - offset());
}

// Methods for getting different parts of the address we need to do the simulations
uint32_t get_index(uint32_t address)
{
    return extract_bits(address, index_size(), offset());
}

uint32_t get_tag_DM(uint32_t address)
{
    return extract_bits(address, tag_size(), (offset() + index_size()));
}

uint32_t get_tag_FA(uint32_t address)
{
    return extract_bits(address, (tag_size() + index_size()), offset());
}

/***************************************************************************************
 *                                      FIFO                                           *
***************************************************************************************/
// Method for removing the first item in our cache.
void remove_from_cache(uint32_t *cache)
{
    for (int i = 0; i < cache_partition_size - 1; i++)
    {
        cache[i] = cache[i + 1];
    }
    count--;
}
// Method for adding a new item
void add_to_cache(uint32_t element, uint32_t *cache)
{
    if (count == cache_partition_size)
    {
        remove_from_cache(cache);
        add_to_cache(element, cache);
    }
    else
    {
        cache[count] = element;
        count++;
    }
}
/***************************************************************************************/

// If the cache is directly mapped we only need to check if the tag of the address is at the index given
void check_cache_DM(uint32_t address, uint32_t *cache)
{
    cache_statistics.accesses++;
    uint32_t index = get_index(address);
    uint32_t tag = get_tag_DM(address);

    if (cache[index] == tag)
    {
        cache_statistics.hits++;
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
    for (int i = 0; i < cache_partition_size; i++)
    {
        if (cache[i] == tag)
        {
            cache_statistics.hits++;
            return;
        }
    }
    add_to_cache(tag, cache);
}
// Printing stats for the cache organization depending on what cache we are dealing with
void print_cache_organization(cache_map_t map_type, cache_org_t org_type)
{

    printf("\nCache Organization\n");
    printf("-----------------\n");
    if (org_type == UC)
    {
        printf("Size: %d bytes\n", cache_partition_size * block_size);
        printf("Organization: Unified Cache\n");
    }
    else
    {
        printf("Size: %d/%d bytes\n", cache_partition_size * block_size, cache_partition_size * block_size);
        printf("Organization: Split Cache\n");
    }
    if (map_type == DM)
    {
        printf("Mapping: Direct Mapped\n");
        printf("Size of index: %d\n", index_size());
        printf("Size of tag: %d\n", tag_size());
    }
    else
    {
        printf("Mapping: Fully Associative\n");
        // If we have a FA cache the index is absorbed by the tag
        // This means that the tag_size will be equal to the size of tag plus the size of the index
        printf("Size of index: 0\n");
        printf("Size of tag: %d\n", tag_size() + index_size());
    }
    printf("Block Offset: %d\n", offset());
    printf("-----------------\n\n");
}

void check_caches()
{
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
        total_cache_size = atoi(argv[1]);

        /* Set Cache Mapping */
        if (strcmp(argv[2], "dm") == 0)
        {
            cache_mapping = DM;
        }
        else if (strcmp(argv[2], "fa") == 0)
        {
            cache_mapping = FA;
        }
        else
        {
            printf("Unknown cache mapping\n");
            exit(0);
        }

        /* Set Cache Organization */
        if (strcmp(argv[3], "uc") == 0)
        {
            cache_org = UC;
        }
        else if (strcmp(argv[3], "sc") == 0)
        {
            cache_org = SC;
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

    // Sets what function we need to use when checking the cache
    void (*check_cache)();
    if (cache_mapping == FA)
    {
        check_cache = check_cache_FA;
    }
    else
    {
        check_cache = check_cache_DM;
    }

    // Do the simulation on a split cache or a unified one
    if (cache_org == SC)
    {
        cache_partition_size = num_of_blocks() / 2;
        cache_instructions = (uint32_t *)malloc(sizeof(uint32_t) * cache_partition_size);
        cache_data = (uint32_t *)malloc(sizeof(uint32_t) * cache_partition_size);

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
                // Depending on if we have a FA or a DM cache this function will vary
                check_cache(access.address, cache_instructions);
            }
            else
            {
                check_cache(access.address, cache_data);
            }
        }
        free(cache_instructions);
        free(cache_data);
    }
    else
    {
        cache_partition_size = num_of_blocks();
        cache_data_and_instructions = (uint32_t *)malloc(sizeof(uint32_t) * cache_partition_size);
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
            check_cache(access.address, cache_data_and_instructions);
        }
        free(cache_data_and_instructions);
    }

    print_cache_organization(cache_mapping, cache_org);

    // DO NOT CHANGE THE FOLLOWING LINES!
    printf("\nCache Statistics\n");
    printf("-----------------\n");
    printf("Accesses: %ld\n", cache_statistics.accesses);
    printf("Hits:     %ld\n", cache_statistics.hits);
    printf("Hit Rate: %.4f\n", (double)cache_statistics.hits / cache_statistics.accesses);

    /* Close the trace file */
    fclose(ptr_file);
}