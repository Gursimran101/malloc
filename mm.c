/**
 * @file mm.c
 * @brief A 64-bit struct-based implicit free list memory allocator
 *
 * 15-213: Introduction to Computer Systems
 *
 * TODO: insert your documentation here. :)
 *
 *************************************************************************
 *
 * ADVICE FOR STUDENTS.
 * - Step 0: Please read the writeup!
 * - Step 1: Write your heap checker.
 * - Step 2: Write contracts / debugging assert statements.
 * - Good luck, and have fun!
 *
 *************************************************************************
 *
 * @author Gursimran Panesar <gpanesar@andrew.cmu.edu>
 */

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/* Do not change the following! */

#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#define memset mem_memset
#define memcpy mem_memcpy
#endif /* def DRIVER */

/* You can change anything from here onward */

/*
 *****************************************************************************
 * If DEBUG is defined (such as when running mdriver-dbg), these macros      *
 * are enabled. You can use them to print debugging output and to check      *
 * contracts only in debug mode.                                             *
 *                                                                           *
 * Only debugging macros with names beginning "dbg_" are allowed.            *
 * You may not define any other macros having arguments.                     *
 *****************************************************************************
 */
#ifdef DEBUG
/* When DEBUG is defined, these form aliases to useful functions */
#define dbg_requires(expr) assert(expr)
#define dbg_assert(expr) assert(expr)
#define dbg_ensures(expr) assert(expr)
#define dbg_printf(...) ((void)printf(__VA_ARGS__))
#define dbg_printheap(...) print_heap(__VA_ARGS__)
#else
/* When DEBUG is not defined, these should emit no code whatsoever,
 * not even from evaluation of argument expressions.  However,
 * argument expressions should still be syntax-checked and should
 * count as uses of any variables involved.  This used to use a
 * straightforward hack involving sizeof(), but that can sometimes
 * provoke warnings about misuse of sizeof().  I _hope_ that this
 * newer, less straightforward hack will be more robust.
 * Hat tip to Stack Overflow poster chqrlie (see
 * https://stackoverflow.com/questions/72647780).
 */
#define dbg_discard_expr_(...) ((void)((0) && printf(__VA_ARGS__)))
#define dbg_requires(expr) dbg_discard_expr_("%d", !(expr))
#define dbg_assert(expr) dbg_discard_expr_("%d", !(expr))
#define dbg_ensures(expr) dbg_discard_expr_("%d", !(expr))
#define dbg_printf(...) dbg_discard_expr_(__VA_ARGS__)
#define dbg_printheap(...) ((void)((0) && print_heap(__VA_ARGS__)))
#endif

/* Basic constants */

typedef uint64_t word_t;

/** @brief Word and header size (bytes) */
static const size_t wsize = sizeof(word_t);

/** @brief Double word size (bytes) */
static const size_t dsize = 2 * wsize;

/** @brief Minimum block size (bytes) */
static const size_t min_block_size = 2 * dsize;

/**
 * @brief size of initial free block and default size for expanding heap
 */
static const size_t chunksize = (1 << 12);

/**
 * mask used to check if block is allocated or not
 */
static const word_t alloc_mask = 0x1;

/**
 * mask used to get size of block
 */
static const word_t size_mask = ~(word_t)0xF;

/** @brief Represents the header and payload of one block in the heap */
typedef struct block {
    /** @brief Header contains size + allocation flag */
    word_t header;

    /**
     * @brief A pointer to the block payload.
     *
     * TODO: feel free to delete this comment once you've read it carefully.
     * We don't know what the size of the payload will be, so we will declare
     * it as a zero-length array, which is a GNU compiler extension. This will
     * allow us to obtain a pointer to the start of the payload. (The similar
     * standard-C feature of "flexible array members" won't work here because
     * those are not allowed to be members of a union.)
     *
     * WARNING: A zero-length array must be the last element in a struct, so
     * there should not be any struct fields after it. For this lab, we will
     * allow you to include a zero-length array in a union, as long as the
     * union is the last field in its containing struct. However, this is
     * compiler-specific behavior and should be avoided in general.
     *
     * WARNING: DO NOT cast this pointer to/from other types! Instead, you
     * should use a union to alias this zero-length array with another struct,
     * in order to store additional types of data in the payload memory.
     */
    union {
        struct {
            struct block *next_block;
            struct block *prev_block;
        };
        char payload[0];
    };

    /*
     * TODO: delete or replace this comment once you've thought about it.
     * Why can't we declare the block footer here as part of the struct?
     * Why do we even have footers -- will the code work fine without them?
     * which functions actually use the data contained in footers?
     */
} block_t;

/* Global variables */

/** @brief pointer to global free list */
static block_t *seg_list[15];

/** @brief Pointer to first block in the heap */
static block_t *heap_start = NULL;

/*
 *****************************************************************************
 * The functions below are short wrapper functions to perform                *
 * bit manipulation, pointer arithmetic, and other helper operations.        *
 *                                                                           *
 * We've given you the function header comments for the functions below      *
 * to help you understand how this baseline code works.                      *
 *                                                                           *
 * Note that these function header comments are short since the functions    *
 * they are describing are short as well; you will need to provide           *
 * adequate details for the functions that you write yourself!               *
 *****************************************************************************
 */

/*
 * ---------------------------------------------------------------------------
 *                        BEGIN SHORT HELPER FUNCTIONS
 * ---------------------------------------------------------------------------
 */

/**
 * @brief Returns the maximum of two integers.
 * @param[in] x
 * @param[in] y
 * @return `x` if `x > y`, and `y` otherwise.
 */
static size_t max(size_t x, size_t y) {
    return (x > y) ? x : y;
}

/**
 * @brief Rounds `size` up to next multiple of n
 * @param[in] size
 * @param[in] n
 * @return The size after rounding up
 */
static size_t round_up(size_t size, size_t n) {
    return n * ((size + (n - 1)) / n);
}

/**
 * @brief Packs the `size` and `alloc` of a block into a word suitable for
 *        use as a packed value.
 *
 * Packed values are used for both headers and footers.
 *
 * The allocation status is packed into the lowest bit of the word.
 *
 * @param[in] size The size of the block being represented
 * @param[in] alloc True if the block is allocated
 * @return The packed value
 */
static word_t pack(size_t size, bool alloc) {
    word_t word = size;
    if (alloc) {
        word |= alloc_mask;
    }
    return word;
}

/**
 * @brief Extracts the size represented in a packed word.
 *
 * This function simply clears the lowest 4 bits of the word, as the heap
 * is 16-byte aligned.
 *
 * @param[in] word
 * @return The size of the block represented by the word
 */
static size_t extract_size(word_t word) {
    return (word & size_mask);
}

/**
 * @brief Extracts the size of a block from its header.
 * @param[in] block
 * @return The size of the block
 */
static size_t get_size(block_t *block) {
    return extract_size(block->header);
}

/**
 * @brief Given a payload pointer, returns a pointer to the corresponding
 *        block.
 * @param[in] bp A pointer to a block's payload
 * @return The corresponding block
 */
static block_t *payload_to_header(void *bp) {
    return (block_t *)((char *)bp - offsetof(block_t, payload));
}

/**
 * @brief Given a block pointer, returns a pointer to the corresponding
 *        payload.
 * @param[in] block
 * @return A pointer to the block's payload
 * @pre The block must be a valid block, not a boundary tag.
 */
static void *header_to_payload(block_t *block) {
    dbg_requires(get_size(block) != 0);
    return (void *)(block->payload);
}

/**
 * @brief Given a block pointer, returns a pointer to the corresponding
 *        footer.
 * @param[in] block
 * @return A pointer to the block's footer
 * @pre The block must be a valid block, not a boundary tag.
 */
static word_t *header_to_footer(block_t *block) {
    dbg_requires(get_size(block) != 0 &&
                 "Called header_to_footer on the epilogue block");
    return (word_t *)(block->payload + get_size(block) - dsize);
}

/**
 * @brief Given a block footer, returns a pointer to the corresponding
 *        header.
 *
 * The header is found by subtracting the block size from
 * the footer and adding back wsize.
 *
 * If the prologue is given, then the footer is return as the block.
 *
 * @param[in] footer A pointer to the block's footer
 * @return A pointer to the start of the block
 */
static block_t *footer_to_header(word_t *footer) {
    size_t size = extract_size(*footer);
    if (size == 0){
        return (block_t *)footer;
    }
    return (block_t *)((char *)footer + wsize - size);
}

/**
 * @brief Returns the payload size of a given block.
 *
 * The payload size is equal to the entire block size minus the sizes of the
 * block's header and footer.
 *
 * @param[in] block
 * @return The size of the block's payload
 */
static size_t get_payload_size(block_t *block) {
    size_t asize = get_size(block);
    return asize - dsize;
}

/**
 * @brief Returns the allocation status of a given header value.
 *
 * This is based on the lowest bit of the header value.
 *
 * @param[in] word
 * @return The allocation status correpsonding to the word
 */
static bool extract_alloc(word_t word) {
    return (bool)(word & alloc_mask);
}

/**
 * @brief Returns the allocation status of a block, based on its header.
 * @param[in] block
 * @return The allocation status of the block
 */
static bool get_alloc(block_t *block) {
    return extract_alloc(block->header);
}

/**
 * @brief Writes an epilogue header at the given address.
 *
 * The epilogue header has size 0, and is marked as allocated.
 *
 * @param[out] block The location to write the epilogue header
 */
static void write_epilogue(block_t *block) {
    dbg_requires(block != NULL);
    dbg_requires((char *)block == (char *)mem_heap_hi() - 7);
    block->header = pack(0, true);
}

/**
 * @brief Writes a block starting at the given address.
 *
 * This function writes both a header and footer, where the location of the
 * footer is computed in relation to the header.
 *
 * TODO: Are there any preconditions or postconditions?
 *
 * @param[out] block The location to begin writing the block header
 * @param[in] size The size of the new block
 * @param[in] alloc The allocation status of the new block
 */
static void write_block(block_t *block, size_t size, bool alloc) {
    dbg_requires(block != NULL);
    dbg_requires(size > 0);
    block->header = pack(size, alloc);
    word_t *footerp = header_to_footer(block);
    *footerp = pack(size, alloc);
}

/**
 * @brief Finds the next consecutive block on the heap.
 *
 * This function accesses the next block in the "implicit list" of the heap
 * by adding the size of the block.
 *
 * @param[in] block A block in the heap
 * @return The next consecutive block on the heap
 * @pre The block is not the epilogue
 */
static block_t *find_next(block_t *block) {
    dbg_requires(block != NULL);
    dbg_requires(get_size(block) != 0 &&
                 "Called find_next on the last block in the heap");
    return (block_t *)((char *)block + get_size(block));
}

/**
 * @brief Finds the footer of the previous block on the heap.
 * @param[in] block A block in the heap
 * @return The location of the previous block's footer
 */
static word_t *find_prev_footer(block_t *block) {
    // Compute previous footer position as one word before the header
    return &(block->header) - 1;
}

/**
 * @brief Finds the previous consecutive block on the heap.
 *
 * This is the previous block in the "implicit list" of the heap.
 *
 * The position of the previous block is found by reading the previous
 * block's footer to determine its size, then calculating the start of the
 * previous block based on its size.
 *
 * @param[in] block A block in the heap
 * @return The previous consecutive block in the heap.
 * @pre The block is not the prologue
 */
static block_t *find_prev(block_t *block) {
    dbg_requires(block != NULL);
    dbg_requires(get_size(block) != 0 &&
                 "Called find_prev on the first block in the heap");
    word_t *footerp = find_prev_footer(block);
    return footer_to_header(footerp);
}



static void add_free_block(block_t *block)
{
    size_t block_size = get_size(block);
    int i;

    if (block_size >= 32 && block_size <= 64){
        i = 0;
    } else if (block_size > 64 && block_size <= 128) {
        i = 1;
    } else if (block_size > 128 && block_size <= 256) {
        i = 2;
    } else if (block_size > 256 && block_size <= 512) {
        i = 3;
    } else if (block_size > 512 && block_size <= 1024) {
        i = 4;
    } else if (block_size > 1024 && block_size <= 2048) {
        i = 5;
    } else if (block_size > 2048 && block_size <= 4096) {
        i = 6;
    } else if (block_size > 4096 && block_size <= 8192) {
        i = 7;
    } else if (block_size > 8192 && block_size <= 16384) {
        i = 8;
    } else if (block_size > 16384 && block_size <= 32768) {
        i = 9;
    } else if (block_size > 32768 && block_size <= 65536) {
        i = 10;
    } else if (block_size > 65536 && block_size <= 131072) {
        i = 11;
    } else if (block_size > 131072 && block_size <= 262144) {
        i = 12;
    } else if (block_size > 262144 && block_size <= 524288) {
        i = 13;
    } else {
        i = 14;
    }

    if (seg_list[i] == NULL) {
        seg_list[i] = block;
        block->next_block = NULL;
        block->prev_block = NULL;
    } else {
        block->next_block = seg_list[i]->next_block;
        seg_list[i]->next_block = block;
        block->prev_block = seg_list[i];
        if (block->next_block != NULL) {
            (block->next_block)->prev_block = block;
        }
    }
}


static void delete_free_block(block_t *block)
{
    size_t block_size = get_size(block);
    int i;

    if (block_size >= 32 && block_size <= 64){
        i = 0;
    } else if (block_size > 64 && block_size <= 128) {
        i = 1;
    } else if (block_size > 128 && block_size <= 256) {
        i = 2;
    } else if (block_size > 256 && block_size <= 512) {
        i = 3;
    } else if (block_size > 512 && block_size <= 1024) {
        i = 4;
    } else if (block_size > 1024 && block_size <= 2048) {
        i = 5;
    } else if (block_size > 2048 && block_size <= 4096) {
        i = 6;
    } else if (block_size > 4096 && block_size <= 8192) {
        i = 7;
    } else if (block_size > 8192 && block_size <= 16384) {
        i = 8;
    } else if (block_size > 16384 && block_size <= 32768) {
        i = 9;
    } else if (block_size > 32768 && block_size <= 65536) {
        i = 10;
    } else if (block_size > 65536 && block_size <= 131072) {
        i = 11;
    } else if (block_size > 131072 && block_size <= 262144) {
        i = 12;
    } else if (block_size > 262144 && block_size <= 524288) {
        i = 13;
    } else {
        i = 14;
    }

    if (block->next_block == NULL && block->prev_block == NULL) {
        seg_list[i] = NULL;
    } else if (block->next_block == NULL) {
        (block->prev_block)->next_block = NULL;
    } else if (block->prev_block == NULL) {
        seg_list[i] = block->next_block;
        seg_list[i]->prev_block = NULL;
    } else {
        ((block->next_block)->prev_block) = block->prev_block;
        ((block->prev_block)->next_block) = block->next_block;
    }
}



/*
 * ---------------------------------------------------------------------------
 *                        END SHORT HELPER FUNCTIONS
 * ---------------------------------------------------------------------------
 */

/******** The remaining content below are helper and debug routines ********/

/**
 * @brief
 *
 * <What does this function do?>
 * <What are the function's arguments?>
 * <What is the function's return value?>
 * <Are there any preconditions or postconditions?>
 *
 * @param[in] block
 * @return
 */
// static block_t *coalesce_block(block_t *block) {
//     block_t *prev_block = find_prev(block);
//     block_t *next_block = find_next(block);

//     bool is_prev_alloc = get_alloc(prev_block);
//     bool is_next_alloc = get_alloc(next_block);
//     size_t curr_size = get_size(block);

//     /* if the next block is free, coalesce the current block and next */
//     if(is_prev_alloc && !is_next_alloc)
//     {
//         size_t next_size = get_size(next_block);
//         delete_free_block(block);
//         delete_free_block(next_block);
//         curr_size += next_size;
//         write_block(block, curr_size, 0);
//         add_free_block(block);
//     }

//     /* if the previous block is free, coalesce the current block and previous */
//     else if(is_next_alloc && !is_prev_alloc)
//     {
//         size_t prev_size = get_size(prev_block);
//         delete_free_block(block);
//         delete_free_block(block);
//         curr_size += prev_size;
//         block = prev_block;
//         write_block(block, curr_size, 0);
//         add_free_block(block);
//     }

//     /* if both previous and next block are free, coalesce current, next, previous */
//     else if(!is_prev_alloc && !is_next_alloc)
//     {
//         size_t next_size = get_size(next_block);
//         size_t prev_size = get_size(prev_block);
//         delete_free_block(block);
//         delete_free_block(prev_block);
//         delete_free_block(next_block);
//         curr_size += next_size + prev_size;
//         block = prev_block;
//         write_block(block, curr_size, 0);
//         add_free_block(block);
//     }

//     /* add coalesced block at the front of the free list */
//     return block;
// }

static block_t *coalesce_block(block_t *block) {
    block_t *prev_block = find_prev(block);
    block_t *next_block = find_next(block);

    word_t curr_size = get_size(block);

    if (prev_block == NULL || get_alloc(prev_block)) {
        if (next_block == NULL || get_alloc(next_block)) {
            return block;
        } else {
            word_t next_size = get_size(next_block);
            delete_free_block(block);
            delete_free_block(next_block);
            write_block(block, (curr_size + next_size), false);
            add_free_block(block);
            return (block);
        }
    } else {
        if (next_block == NULL || get_alloc(next_block)) {
            word_t prev_size = get_size(prev_block);
            delete_free_block(block);
            delete_free_block(prev_block);
            write_block(prev_block, (curr_size + prev_size), false);
            add_free_block(prev_block);
            return (prev_block);
        } else {
            word_t prev_size = get_size(prev_block);
            word_t next_size = get_size(next_block);
            delete_free_block(block);
            delete_free_block(prev_block);
            delete_free_block(next_block);
            write_block(prev_block, (prev_size + curr_size + next_size), false);
            add_free_block(prev_block);
            return (prev_block);
        }
    }
    return (block);
}


/**
 * @brief
 *
 * <What does this function do?>
 * <What are the function's arguments?>
 * <What is the function's return value?>
 * <Are there any preconditions or postconditions?>
 *
 * @param[in] size
 * @return
 */
static block_t *extend_heap(size_t size) {
    void *bp;

    // Allocate an even number of words to maintain alignment
    size = round_up(size, dsize);
    if ((bp = mem_sbrk((intptr_t)size)) == (void *)-1) {
        return NULL;
    }

    /*
     * TODO: delete or replace this comment once you've thought about it.
     * Think about what bp represents. Why do we write the new block
     * starting one word BEFORE bp, but with the same size that we
     * originally requested?
     */

    // Initialize free block header/footer
    block_t *block = payload_to_header(bp);
    write_block(block, size, false);
    add_free_block(block);

    // Create new epilogue header
    block_t *block_next = find_next(block);
    write_epilogue(block_next);

    // Coalesce in case the previous block was free
    block = coalesce_block(block);

    return block;
}

/**
 * @brief
 *
 * <What does this function do?>
 * <What are the function's arguments?>
 * <What is the function's return value?>
 * <Are there any preconditions or postconditions?>
 *
 * @param[in] block
 * @param[in] asize
 */
static void split_block(block_t *block, size_t asize) {
    dbg_requires(get_alloc(block));
    /* TODO: Can you write a precondition about the value of asize? */

    size_t block_size = get_size(block);

    if ((block_size - asize) >= min_block_size) {
        block_t *block_next;
        write_block(block, asize, true);

        block_next = find_next(block);
        write_block(block_next, block_size - asize, false);
        add_free_block(block_next);
    }

    dbg_ensures(get_alloc(block));
}

/**
 * @brief
 *
 * <What does this function do?>
 * <What are the function's arguments?>
 * <What is the function's return value?>
 * <Are there any preconditions or postconditions?>
 *
 * @param[in] asize
 * @return
 */
static block_t *find_fit(size_t asize) {
    block_t *block;
    int i;

    if (asize >= 32 && asize <= 64){
        i = 0;
    } else if (asize > 64 && asize <= 128) {
        i = 1;
    } else if (asize > 128 && asize <= 256) {
        i = 2;
    } else if (asize > 256 && asize <= 512) {
        i = 3;
    } else if (asize > 512 && asize <= 1024) {
        i = 4;
    } else if (asize > 1024 && asize <= 2048) {
        i = 5;
    } else if (asize > 2048 && asize <= 4096) {
        i = 6;
    } else if (asize > 4096 && asize <= 8192) {
        i = 7;
    } else if (asize > 8192 && asize <= 16384) {
        i = 8;
    } else if (asize > 16384 && asize <= 32768) {
        i = 9;
    } else if (asize > 32768 && asize <= 65536) {
        i = 10;
    } else if (asize > 65536 && asize <= 131072) {
        i = 11;
    } else if (asize > 131072 && asize <= 262144) {
        i = 12;
    } else if (asize > 262144 && asize <= 524288) {
        i = 13;
    } else {
        i = 14;
    }


    for (int remake = i; remake < 15; remake++){
        if (seg_list[remake] == NULL) {
            continue;
        }

        block = seg_list[remake];
        while (block != NULL) {
            if (!(get_alloc(block)) && (asize <= get_size(block))) {
                return block;
            } else {
                block = block->next_block;
            }
        }
    }

    return NULL; // no fit found
}



/* check alignment of block - returns true if divisible (means block is aligned) */
static bool block_aligned(size_t curr_size)
{
    return (curr_size % dsize == 0);
}

/* check minimum block size - returns true if blocksize is more than min */
static bool check_blocksize(size_t size)
{
    return size > min_block_size;
}


/* returns true if the current block is between the heap_lo and heap_hi */
static bool check_heap_boundaries(block_t *curr_block)
{
    return (mem_heap_lo() < (void*)curr_block) && ((void*)curr_block < mem_heap_hi());
}

/* check to make sure no two consecutive blocks are free - make sure prev and current blocks are allocated */
static bool check_coalesce(bool prev_alloc, bool curr_alloc)
{
    return prev_alloc || curr_alloc;
}


/**
 * @brief
 *
 * <What does this function do?>
 * <What are the function's arguments?>
 * <What is the function's return value?>
 * <Are there any preconditions or postconditions?>
 *
 * @param[in] line
 * @return
 */
// bool mm_checkheap(int line) {
//     //start at prologue block
//     block_t *curr_block = heap_start;
//     //prologue is always allocated, so we start at true
//     bool prev_alloc = true;

//     size_t curr_size = get_size(curr_block);
//     bool curr_alloc = get_alloc(curr_block);

//     while(curr_size > 0){
//         if (!block_aligned(curr_size) || !check_blocksize(curr_size) ||
//             !check_heap_boundaries(curr_block) || !check_coalesce(prev_alloc, curr_alloc))
//         {
//             return false;
//         }

//         prev_alloc = curr_alloc;
//         curr_block = find_next(curr_block);
//         curr_alloc = get_alloc(curr_block);
//         curr_size = get_size(curr_block);
//     }

//     //check epilogue block
//     if (!curr_alloc || curr_size != 0)
//     {
//         return false;
//     }
//     dbg_printf("All checks good on %d \n", line);
//     return true;
// }

bool mm_checkheap(int line) {
    /*
     * TODO: Delete this comment!
     *
     * You will need to write the heap checker yourself.
     * Please keep modularity in mind when you're writing the heap checker!
     *
     * As a filler: one guacamole is equal to 6.02214086 x 10**23 guacas.
     * One might even call it...  the avocado's number.
     *
     * Internal use only: If you mix guacamole on your bibimbap,
     * do you eat it with a pair of chopsticks, or with a spoon?
     */
    // dbg_printf("I did not write a heap checker (called at line %d)\n", line);

    block_t *curr_block = heap_start;
    if (heap_start == NULL) {
        return false;
    }
    if (find_prev(heap_start) != NULL) {
        return false;
    }
    while (get_size(curr_block) != 0) {
        if ((void *)curr_block < mem_heap_lo() ||
            (void *)curr_block > mem_heap_hi()) {
            return false;
        }
        if (get_size(curr_block) < 32) {
            return false;
        }
        if (curr_block->header != *(header_to_footer(curr_block))) {
            return false;
        }
        block_t *next_block = find_next(curr_block);
        if (get_size(next_block) != 0) {
            if (!get_alloc(curr_block) && !get_alloc(next_block)) {
                return false;
            }
        } else if (get_size(curr_block) % 16 != 0 ||
                   get_size(curr_block) < 16) {
            return false;
        }
        curr_block = find_next(curr_block);
    }
    for (int i = 0; i < 15; i++) {
        if (seg_list[i] == NULL) {
            continue;
        } else {
            block_t *curr_free_block = seg_list[i];
            while (curr_free_block != NULL) {
                if ((void *)curr_free_block < mem_heap_lo() ||
                    (void *)curr_free_block > mem_heap_hi()) {
                    return false;
                }
                block_t *start = heap_start;
                while (start != NULL) {
                    if (find_next(start) == NULL &&
                        (start != curr_free_block)) {
                        return false;
                    }
                    if (start == curr_free_block) {
                        break;
                    }
                    start = find_next(start);
                }
                size_t size = get_size(curr_free_block);
                if (i == 0) {
                    if (!(size >= 32 && size <= 64)) {
                        return false;
                    };
                } else if (i == 1) {
                    if (!(size > 64 && size <= 128)) {
                        return false;
                    };
                } else if (i == 2) {
                    if (!(size > 128 && size <= 256)) {
                        return false;
                    };
                } else if (i == 3) {
                    if (!(size > 256 && size <= 512)) {
                        return false;
                    };
                } else if (i == 4) {
                    if (!(size > 512 && size <= 1024)) {
                        return false;
                    };
                } else if (i == 5) {
                    if (!(size > 1024 && size <= 2048)) {
                        return false;
                    };
                } else if (i == 6) {
                    if (!(size > 2048 && size <= 4096)) {
                        return false;
                    };
                } else if (i == 7) {
                    if (!(size > 4096 && size <= 8192)) {
                        return false;
                    };
                } else if (i == 8) {
                    if (!(size > 8192 && size <= 16384)) {
                        return false;
                    };
                } else if (i == 9) {
                    if (!(size > 16384 && size <= 32768)) {
                        return false;
                    };
                } else if (i == 10) {
                    if (!(size > 32768 && size <= 65536)) {
                        return false;
                    };
                } else if (i == 11) {
                    if (!(size > 65536 && size <= 65536 * 2)) {
                        return false;
                    };
                } else if (i == 12) {
                    if (!(size > 65536 * 2 && size <= 65536 * 4)) {
                        return false;
                    };
                } else if (i == 13) {
                    if (!(size > 65536 * 4 && size <= 65536 * 8)) {
                        return false;
                    };
                } else {
                    if (!(size > 65536 * 8)) {
                        return false;
                    };
                }
                if (curr_free_block->prev_block != NULL &&
                    curr_free_block->next_block != NULL) {
                    if ((curr_free_block->prev_block)->next_block !=
                        (curr_free_block->next_block)->prev_block) {
                        return false;
                    }
                }
                curr_free_block = curr_free_block->next_block;
            }
        }
    }
    return true;
}



/**
 * @brief
 *
 * <What does this function do?>
 * <What are the function's arguments?>
 * <What is the function's return value?>
 * <Are there any preconditions or postconditions?>
 *
 * @return
 */
bool mm_init(void) {
    // Create the initial empty heap
    word_t *start = (word_t *)(mem_sbrk(2 * wsize));

    if (start == (void *)-1) {
        return false;
    }

    /*
     * TODO: delete or replace this comment once you've thought about it.
     * Think about why we need a heap prologue and epilogue. Why do
     * they correspond to a block footer and header respectively?
     */

    start[0] = pack(0, true); // Heap prologue (block footer)
    start[1] = pack(0, true); // Heap epilogue (block header)

    // Heap starts with first "block header", currently the epilogue
    heap_start = (block_t *)&(start[1]);

    for (int i = 0; i < 15; i++){
        seg_list[i] = NULL;
    }


    // Extend the empty heap with a free block of chunksize bytes
    if (extend_heap(chunksize) == NULL) {
        return false;
    }

    return true;
}

/**
 * @brief
 *
 * <What does this function do?>
 * <What are the function's arguments?>
 * <What is the function's return value?>
 * <Are there any preconditions or postconditions?>
 *
 * @param[in] size
 * @return
 */
void *malloc(size_t size) {
    dbg_requires(mm_checkheap(__LINE__));

    size_t asize;      // Adjusted block size
    size_t extendsize; // Amount to extend heap if no fit is found
    block_t *block;
    void *bp = NULL;

    // Initialize heap if it isn't initialized
    if (heap_start == NULL) {
        if (!(mm_init())) {
            dbg_printf("Problem initializing heap. Likely due to sbrk");
            return NULL;
        }
    }

    // Ignore spurious request
    if (size == 0) {
        dbg_ensures(mm_checkheap(__LINE__));
        return bp;
    }

    // Adjust block size to include overhead and to meet alignment requirements
    asize = round_up(size + dsize, dsize);

    // Search the free list for a fit
    block = find_fit(asize);

    // If no fit is found, request more memory, and then and place the block
    if (block == NULL) {
        // Always request at least chunksize
        extendsize = max(asize, chunksize);
        block = extend_heap(extendsize);
        // extend_heap returns an error
        if (block == NULL) {
            return bp;
        }
    }

    // The block should be marked as free
    dbg_assert(!get_alloc(block));

    // Mark block as allocated
    size_t block_size = get_size(block);
    write_block(block, block_size, true);
    delete_free_block(block);

    // Try to split the block if too large
    split_block(block, asize);

    bp = header_to_payload(block);

    dbg_ensures(mm_checkheap(__LINE__));
    return bp;
}

/**
 * @brief
 *
 * <What does this function do?>
 * <What are the function's arguments?>
 * <What is the function's return value?>
 * <Are there any preconditions or postconditions?>
 *
 * @param[in] bp
 */
void free(void *bp) {
    dbg_requires(mm_checkheap(__LINE__));

    if (bp == NULL) {
        return;
    }

    block_t *block = payload_to_header(bp);
    size_t size = get_size(block);

    // The block should be marked as allocated
    dbg_assert(get_alloc(block));

    // Mark the block as free
    write_block(block, size, false);
    add_free_block(block);

    // Try to coalesce the block with its neighbors
    coalesce_block(block);

    dbg_ensures(mm_checkheap(__LINE__));
}

/**
 * @brief
 *
 * <What does this function do?>
 * <What are the function's arguments?>
 * <What is the function's return value?>
 * <Are there any preconditions or postconditions?>
 *
 * @param[in] ptr
 * @param[in] size
 * @return
 */
void *realloc(void *ptr, size_t size) {
    block_t *block = payload_to_header(ptr);
    size_t copysize;
    void *newptr;

    // If size == 0, then free block and return NULL
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    // If ptr is NULL, then equivalent to malloc
    if (ptr == NULL) {
        return malloc(size);
    }

    // Otherwise, proceed with reallocation
    newptr = malloc(size);

    // If malloc fails, the original block is left untouched
    if (newptr == NULL) {
        return NULL;
    }

    // Copy the old data
    copysize = get_payload_size(block); // gets size of old payload
    if (size < copysize) {
        copysize = size;
    }
    memcpy(newptr, ptr, copysize);

    // Free the old block
    free(ptr);

    return newptr;
}

/**
 * @brief
 *
 * <What does this function do?>
 * <What are the function's arguments?>
 * <What is the function's return value?>
 * <Are there any preconditions or postconditions?>
 *
 * @param[in] elements
 * @param[in] size
 * @return
 */
void *calloc(size_t elements, size_t size) {
    void *bp;
    size_t asize = elements * size;

    if (elements == 0) {
        return NULL;
    }
    if (asize / elements != size) {
        // Multiplication overflowed
        return NULL;
    }

    bp = malloc(asize);
    if (bp == NULL) {
        return NULL;
    }

    // Initialize all bits to 0
    memset(bp, 0, asize);

    return bp;
}

/*
 *****************************************************************************
 * Do not delete the following super-secret(tm) lines!                       *
 *                                                                           *
 * 53 6f 20 79 6f 75 27 72 65 20 74 72 79 69 6e 67 20 74 6f 20               *
 *                                                                           *
 * 66 69 67 75 72 65 20 6f 75 74 20 77 68 61 74 20 74 68 65 20               *
 * 68 65 78 61 64 65 63 69 6d 61 6c 20 64 69 67 69 74 73 20 64               *
 * 6f 2e 2e 2e 20 68 61 68 61 68 61 21 20 41 53 43 49 49 20 69               *
 *                                                                           *
 * 73 6e 27 74 20 74 68 65 20 72 69 67 68 74 20 65 6e 63 6f 64               *
 * 69 6e 67 21 20 4e 69 63 65 20 74 72 79 2c 20 74 68 6f 75 67               *
 * 68 21 20 2d 44 72 2e 20 45 76 69 6c 0a c5 7c fc 80 6e 57 0a               *
 *                                                                           *
 *****************************************************************************
 */
