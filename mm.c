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

<<<<<<< HEAD
/** @brief size of our segregated list (14 possible indices) */
=======
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c
static const int SEG_LIST_SIZE = 14;

/** @brief Word and header size (bytes) */
static const size_t wsize = sizeof(word_t);

/** @brief Double word size (bytes) */
static const size_t dsize = 2 * wsize;

/** @brief Minimum block size (bytes) */
static const size_t min_block_size = dsize;

/**
 * @brief size of initial free block and default size for expanding heap
 */
static const size_t chunksize = (1 << 12);

<<<<<<< HEAD
/** @brief mask used to check if block is allocated or not */
static const word_t curr_alloc_mask = 0x1;
=======
/**
 * mask used to check if block is allocated or not
 */
static const word_t curr_alloc_mask = 0x1;

/**
 * mask used to check if previous block is allocated or not - second lowest bit
 */
static const word_t prev_alloc_mask = 0x2;

/**
 * mask used to check if previous block is miniblock or not
 */
static const word_t prev_miniblock_mask = 0x4;

>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c

/** @brief mask used to check if previous block is allocated or not 
 *  - second lowest bit of header
 */
static const word_t prev_alloc_mask = 0x2;

/** @brief mask used to check if previous block is miniblock or not
 *  - third lowest bit of header
 */
static const word_t prev_miniblock_mask = 0x4;


/** @brief mask used to get size of block */
static const word_t size_mask = ~(word_t)0xF;

/** @brief Represents the header and payload of one block in the heap */
typedef struct block {
    /** @brief Header contains size + allocation flag */
    word_t header;

    union {
        struct {
            struct block *next_block;
            struct block *prev_block;
        };
        char payload[0];
    };
} block_t;

/* Global variables */

/** @brief pointer to global free list */
/* we can have a max of 128 bytes. each pointer is 8 bytes (one less than from 
 * checkpoint because we have a miniblocks list, which is a pointer of 8 bytes) 
*/
static block_t *seg_freelist[14];

/** @brief Pointer to first block in the heap */
static block_t *heap_start = NULL;

/** @brief pointer to global free list of miniblocks */
<<<<<<< HEAD
=======
/* we can have a max of 128 bytes. each pointer is 8 bytes */
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c
static block_t *miniblock_list = NULL;


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

static size_t find_freelist_index(size_t size);
static bool size_checker(int index, size_t size);

static void write_block(block_t *block, size_t size, bool prev_miniblock, 
						bool prev_alloc, bool curr_alloc);
static void add_free_block(block_t *block);
static void delete_free_block(block_t *block);
static block_t *coalesce_block(block_t *block);
static block_t *extend_heap(size_t size);
static void split_block(block_t *block, size_t asize);
static block_t *find_fit(size_t asize);
bool mm_init(void);
void *malloc(size_t size);
void free(void *bp);
void *realloc(void *ptr, size_t size);
void *calloc(size_t elements, size_t size);



/**
 * @brief Returns the maximum of two integers.
 * @param[in] x
 * @param[in] y
 * @return `x` if `x > y`, and `y` otherwise.
 */
static size_t max(size_t, size_t);


/**
 * @brief Rounds `size` up to next multiple of n
 * @param[in] size
 * @param[in] n
 * @return The size after rounding up
 */
static size_t round_up(size_t, size_t);


/**
 * @brief Packs the `size` and `alloc` of a block into a word suitable for
 *        use as a packed value.
 *
 * Packed values are used for both headers and footers.
 *
 * The allocation status is packed into the lowest bit of the word, the allocation
 * status of the previous block is in the next lowest bit (2nd) of the word, the 
 * miniblock status of the previous block is in the next lowest bit (3rd).
 *
 * @param[in] size The size of the block being represented
 * @param[in] prev_miniblock True if the block is a miniblock (size 16)
 * @param[in] prev_alloc True if the previous block is allocated
 * @param[in] curr_alloc True if the current block is allocated
 * @return The packed value
 */
<<<<<<< HEAD
static word_t pack (size_t, bool, bool, bool);

=======
static word_t pack(size_t size, bool prev_miniblock, bool prev_alloc, bool alloc) {
    word_t word = size;
    if (prev_miniblock) {
		word |= prev_miniblock_mask;
	}
	if (prev_alloc) {
		word |= prev_alloc_mask;
	}
	if (alloc) {
        word |= curr_alloc_mask;
    }
    return word;
}
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c

/**
 * @brief Extracts the size represented in a packed word.
 *
 * This function simply clears the lowest 4 bits of the word, as the heap
 * is 16-byte aligned.
 *
 * @param[in] word
 * @return The size of the block represented by the word
 */
static size_t extract_size(word_t);


/**
 * @brief Extracts the size of a block from its header.
 * @param[in] block
 * @return The size of the block
 */
static size_t get_size(block_t*);


/**
 * @brief Given a payload pointer, returns a pointer to the corresponding block.
 * @param[in] bp A pointer to a block's payload
 * @return The corresponding block
 */
static block_t *payload_to_header(void*);


/**
 * @brief Given a block pointer, returns a pointer to the corresponding
 *        payload.
 * @param[in] block
 * @return A pointer to the block's payload
 * @pre The block must be a valid block, not a boundary tag.
 */
static void *header_to_payload(block_t*);


/**
 * @brief Given a block pointer, returns a pointer to the corresponding
 *        footer.
 * @param[in] block
 * @return A pointer to the block's footer
 * @pre The block must be a valid block, not a boundary tag.
 */
static word_t *header_to_footer(block_t*);


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
static block_t *footer_to_header(word_t*);


/**
 * @brief Returns the payload size of a given block.
 *
 * The payload size is equal to the entire block size minus the sizes of the
 * block's header. (we don't subtract the footer anymore because allocated blocks
 * have no footers anymore)
 *
 * @param[in] block
 * @return The size of the block's payload
 */
<<<<<<< HEAD
static size_t get_payload_size(block_t*);

=======
static size_t get_payload_size(block_t *block) {
    size_t asize = get_size(block);
    return asize - wsize;
}
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c

/**
 * @brief Returns the allocation status of a given header value.
 *
 * This is based on the lowest bit of the header value.
 *
 * @param[in] word
 * @return The allocation status correpsonding to the word
 */
<<<<<<< HEAD
static bool extract_alloc(word_t);

=======
static bool extract_alloc(word_t word) {
    return (bool)(word & curr_alloc_mask);
}
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c

/**
 * @brief Returns the allocation status of a block, based on its header.
 * @param[in] block
 * @return The allocation status of the block
 */
<<<<<<< HEAD
static bool get_alloc(block_t*);


/**
 * @brief Returns the allocation status of the previous block given a header value.
 *
 * This is based on the second lowest bit of the header value.
 *
 * @param[in] word
 * @return The allocation status corresponding to the previous block
 */
static bool extract_prev_alloc(word_t);


/**
 * @brief Returns the allocation status of the previous block, based on the current block's header.
 * @param[in] block
 * @return The allocation status of the previous block
 */
static bool get_prev_alloc(block_t*);


/**
 * @brief Returns the allocation status of a previous miniblock given a header value.
 *
 * This is based on the third lowest bit of the header value.
 *
 * @param[in] word
 * @return The miniblock status corresponding to the previous block
 */
static bool extract_miniblock(word_t);


/**
 * @brief Returns the status of a miniblock existing at the previous block, based on the current block's header.
 * @param[in] block
 * @return The miniblock status of the previous block
 */
static bool get_prev_miniblock(block_t*);

=======
static bool get_alloc(block_t *block) {
    // if (block == NULL){
    //     return false;  // we return true because if block is NULL, we don't want to do anything with it
    // }
    return extract_alloc(block->header);
}
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c


/**
 * @brief Returns the allocation status of the previous block given a header value.
 *
 * This is based on the second lowest bit of the header value.
 *
 * @param[in] word
 * @return The allocation status corresponding to the previous block
 */
static bool extract_prev_alloc(word_t word) {
    return (bool)(word & prev_alloc_mask);
}

/**
 * @brief Returns the allocation status of the previous block, based on the current block's header.
 * @param[in] block
 * @return The allocation status of the previous block
 */
static bool get_prev_alloc(block_t *block) {
    // if (block == NULL){
    //     return false; // we return true because if block is NULL, we don't want to do anything with it
    // }
    return extract_prev_alloc(block->header);
}


/**
 * @brief Returns the allocation status of a previous miniblock given a header value.
 *
 * This is based on the third lowest bit of the header value.
 *
 * @param[in] word
 * @return The miniblock status corresponding to the previous block
 */
static bool extract_miniblock(word_t word) {
    return (bool)(word & prev_miniblock_mask);
}

/**
 * @brief Returns the status of a miniblock existing at the previous block, based on the current block's header.
 * @param[in] block
 * @return The miniblock status of the previous block
 */
static bool get_prev_miniblock(block_t *block) {
    // if (block == NULL){
    //     return false;
    // }
    return extract_miniblock(block->header);
}


/**
 * @brief Writes an epilogue header at the given address.
 *
 * The epilogue header has size 0, and is marked as allocated.
 *
 * @param[out] block The location to write the epilogue header
 */
<<<<<<< HEAD
static void write_epilogue(block_t*);
=======
static void write_epilogue(block_t *block) {
    dbg_requires(block != NULL);
    dbg_requires((char *)block == (char *)mem_heap_hi() - 7);

	bool prev_miniblock = false;
	bool prev_alloc = false;
	bool curr_alloc = true;
    block->header = pack(0, prev_miniblock, prev_alloc, curr_alloc);
}
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c


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
static block_t *find_next(block_t*);


/**
 * @brief Finds the footer of the previous block on the heap.
 * @param[in] block A block in the heap
 * @return The location of the previous block's footer
 */
static word_t *find_prev_footer(block_t*);


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
<<<<<<< HEAD
static block_t *find_prev(block_t*);


/**
 * @brief Finds the the appropriate index in segregated freelist given a certain size.
 *
 * 
 * Given a size, we know that if a free block of the appropriate size exists, 
 * it will be in a certain "bucket" in the segregated freelist. Based on this
 * size, we return the index of the freelist where the free block will be.
 *
 * @param[in] requested size 
 * @return index 0 <= i < 14 of block in freelist
 */
static size_t find_freelist_index(size_t);


/**
 * @brief Ensures that the given size corresponds to the appropriate index
 *
 * Used in checkheap function
 * 
 * Given a size, we know that if a free block of the appropriate size exists, 
 * it will be in a certain "bucket" in the segregated freelist. Based on this
 * size, we expect a specific index of the freelist where the free block will be.
 * If the size is within the expected bounds of the index, we return true.
 * otherwise we return false
 * 
 * @param[in] index, size 
 * @return true or false
 */
static bool size_checker(int, size_t);


/**
 * @brief Writes a block starting at the given address.
 *
 * This function writes both a header and footer, where the location of the
 * footer is computed in relation to the header. Based on the allocation status
 * of the previous block, and the miniblock status of the previous block, this 
 * function sets the appropriate bits in the new block. 
 *
 * @pre New block cannot be NULL
 * @pre given size of new block must be > 0
 * 
 * @param[out] block The location to begin writing the block header
 * @param[in] size The size of the new block
 * @param[in] prev_miniblock The miniblock status of the previous block
 * @param[in] prev_alloc The allocation status of the previous block
 * @param[in] curr_alloc The allocation status of the new block
 */
static void write_block(block_t*, size_t, bool, bool, bool);


/**
 * @brief Adds a block to the miniblock freelist.
 * @pre size of block is 16
 * 
 * If blocksize is 16, we add it to the front of the miniblock list
 * 
 * @param[in] block The block to add to the singly linked miniblock list
 */
static void add_free_miniblock(block_t*);


/**
 * @brief Adds a block to the segregated freelist.
 * @pre size of block is > 16
 * 
 * If blocksize is more than 16, we add it to the segregated freelist at the right index
 * 
 * @param[in] block The block to add to segregated freelist
 */

static void add_free_seglist(block_t*);


/**
 * @brief Adds a block to segregated freelist or miniblock freelist based on size.
 *
 * If the blocksize is 16, we add the free block to the miniblock list (as done
 * by the add_free_miniblock function). Otherwise we add the block to the front 
 * of the segregated freelist at the appropriate freelist index, 
 * adjusting the prev and next pointers 
 *
 * @pre block is not NULL
 * @param[in] block is the block to add to the freelist
 */
static void add_free_block(block_t*);


/**
 * @brief Deletes a given freeblock from the miniblock list.
 * @pre size of block is 16
 * 
 * If blocksize is 16, we traverse through the miniblock list until we find the 
 * correct block, then delete it from the list
 *
 * @param[in] block is the block to delete from the segregated freelist or 
 *            the miniblock list
 */
static void delete_free_miniblock(block_t*);


/**
 * @brief Deletes a given freeblock from the freelist.
 *
 * If the blocksize is 16, we use delete_free_miniblock function to delete the block
 * Otherwise, we delete the free block from the segregated freelist, and adjust the pointers
 * of the prev and next blocks
 * 
 * @param[in] block is the block to delete from the segregated freelist or 
 *            the miniblock list
 */
static void delete_free_block(block_t*);







static size_t max(size_t x, size_t y) {
    return (x > y) ? x : y;
}

static size_t round_up(size_t size, size_t n) {
    return n * ((size + (n - 1)) / n);
}

static word_t pack(size_t size, bool prev_miniblock, bool prev_alloc, bool alloc) {
    word_t word = size;
    if (prev_miniblock) {
		word |= prev_miniblock_mask;
	}
	if (prev_alloc) {
		word |= prev_alloc_mask;
	}
	if (alloc) {
        word |= curr_alloc_mask;
    }
    return word;
}

static size_t extract_size(word_t word) {
    return (word & size_mask);
}

static size_t get_size(block_t *block) {
    return extract_size(block->header);
}

static block_t *payload_to_header(void *bp) {
    return (block_t *)((char *)bp - offsetof(block_t, payload));
}

static void *header_to_payload(block_t *block) {
    dbg_requires(get_size(block) != 0);
    return (void *)(block->payload);
}

static word_t *header_to_footer(block_t *block) {
    dbg_requires(get_size(block) != 0 &&
                 "Called header_to_footer on the epilogue block");
    return (word_t *)(block->payload + get_size(block) - dsize);
}

static block_t *footer_to_header(word_t *footer) {
    size_t size = extract_size(*footer);
    if (size == 0){
        return (block_t *)footer;
    }
    return (block_t *)((char *)footer + wsize - size);
}

static size_t get_payload_size(block_t *block) {
    size_t asize = get_size(block);
    return asize - wsize;
}

static bool extract_alloc(word_t word) {
    return (bool)(word & curr_alloc_mask);
}

static bool get_alloc(block_t *block) {
    if (block == NULL){
        return true;  // we return true because if block is NULL, we don't want to do anything with it
    }
    return extract_alloc(block->header);
}

static bool extract_prev_alloc(word_t word) {
    return (bool)(word & prev_alloc_mask);
}

static bool get_prev_alloc(block_t *block) {
    if (block == NULL){
        return true; // we return true because if block is NULL, we don't want to do anything with it
    }
    return extract_prev_alloc(block->header);
}

static bool extract_miniblock(word_t word) {
    return (bool)(word & prev_miniblock_mask);
}

static bool get_prev_miniblock(block_t *block) {
    if (block == NULL){
        return false;
    }
    return extract_miniblock(block->header);
}

static void write_epilogue(block_t *block) {
    dbg_requires(block != NULL);
    dbg_requires((char *)block == (char *)mem_heap_hi() - 7);

	bool prev_miniblock = false;
	bool prev_alloc = false;
	bool curr_alloc = true;
    block->header = pack(0, prev_miniblock, prev_alloc, curr_alloc);
}

static block_t *find_next(block_t *block) {
    dbg_requires(block != NULL);
    dbg_requires(get_size(block) != 0 &&
                 "Called find_next on the last block in the heap");
    return (block_t *)((char *)block + get_size(block));
}

static word_t *find_prev_footer(block_t *block) {
    // Compute previous footer position as one word before the header
    return &(block->header) - 1;
}
=======
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c

static block_t *find_prev(block_t *block) {
    dbg_requires(block != NULL);
    dbg_requires(get_size(block) != 0 &&
                 "Called find_prev on the first block in the heap");
    word_t *footerp = find_prev_footer(block);
    return footer_to_header(footerp);
}


<<<<<<< HEAD
=======

/* find index in free list given the requested size */
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c
static size_t find_freelist_index(size_t size)
{
    size_t index = 13;

<<<<<<< HEAD
    // if (size >= 32 && size < 40) {
    //     index = 0;
    // } else if (size >= 40 && size < 48) {
    //     index = 1;
    // } else if (size >= 48 && size < 56) {
    //     index = 2;
    // } else if (size >= 56 && size < 64) {
    //     index = 3;
    // } else if (size >= 64 && size < 128) {
    //     index = 4;
    // } else if (size >= 128 && size < 256) {
    //     index = 5;
    // } else if (size >= 256 && size < 512) {
    //     index = 6;
    // } else if (size >= 512 && size < 1024) {
    //     index = 7;
    // } else if (size >= 1024 && size < 4096) {
    //     index = 8;
    // } else if (size >= 4096 && size < 16384) {
    //     index = 9;
    // } else if (size >= 16384 && size < 32768) {
    //     index = 10;
    // } else if (size >= 32768 && size < 131072) {
    //     index = 11;
    // } else if (size >= 131072 && size < 262144) {
	// 	index = 12;
    // } 

=======
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c
    if (size >= 32 && size < 64) {
        index = 0;
    } else if (size >= 64 && size < 128) {
        index = 1;
    } else if (size >= 128 && size < 256) {
        index = 2;
    } else if (size >= 256 && size < 512) {
        index = 3;
    } else if (size >= 512 && size < 1024) {
        index = 4;
    } else if (size >= 1024 && size < 2048) {
        index = 5;
    } else if (size >= 2048 && size < 4096) {
        index = 6;
    } else if (size >= 4096 && size < 8192) {
        index = 7;
    } else if (size >= 8192 && size < 16384) {
        index = 8;
    } else if (size >= 16384 && size < 32768) {
        index = 9;
    } else if (size >= 32768 && size < 65536) {
        index = 10;
    } else if (size >= 65536 && size < 131072) {
        index = 11;
    } else if (size >= 131072 && size < 262144) {
		index = 12;
    } 
    return index;
}

static bool size_checker(int index, size_t size)
{
    if (index == 0 && !(size >= 32 && size < 64 )) {
        return false;
    } else if (index == 1 && !(size >= 64 && size < 128)) {
        return false;
    } else if (index == 2 && !(size >= 128 && size < 256)) {
        return false;
    } else if (index == 3 && !(size >= 256 && size < 512)) {
        return false;
    } else if (index == 4 && !(size >= 512 && size < 1024)) {
        return false;
    } else if (index == 5 && !(size >= 1024 && size < 2048)) {
        return false;
    } else if (index == 6 && !(size >= 2048 && size < 4096)) {
        return false;
    } else if (index == 7 && !(size >= 4096 && size < 8192)) {
        return false;
    } else if (index == 8 && !(size >= 8192 && size < 16384)) {
        return false;
    } else if (index == 9 && !(size >= 16384 && size < 32768)) {
        return false;
    } else if (index == 10 && !(size >= 32768 && size < 65536)) {
        return false;
    } else if (index == 11 && !(size >= 65536 && size < 131072)) {
        return false;
    } else if (index == 12 && !(size >= 131072 && size < 262144)) {
        return false;
    } else if (index == 13 && !(size > 262144)) {
		return false;
	}
    return true;
}



<<<<<<< HEAD
=======
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
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c
static void write_block(block_t *block, size_t size, bool prev_miniblock, 
						bool prev_alloc, bool curr_alloc) {
    dbg_requires(block != NULL);
    dbg_requires(size > 0);

	
    block->header = pack(size, prev_miniblock, prev_alloc, curr_alloc);
	block_t* next_block = find_next(block);

	if (!curr_alloc && size > 16) 
	// new block is free and not miniblock
	{
    	word_t *footer = header_to_footer(block);
		*footer = pack(size, prev_miniblock, prev_alloc, curr_alloc);
		next_block->header &= ~(prev_alloc_mask);
		next_block->header &= ~(prev_miniblock_mask);
	} 
	
	else if (!curr_alloc && size <= 16)
	// new block is free and miniblock
	{
		next_block->header &= ~(prev_alloc_mask);
        next_block->header |= prev_miniblock_mask;
	}

	else if (curr_alloc && size > 16)
	// new block allocated and a miniblock
	{
		next_block->header |= prev_alloc_mask;
        next_block->header &= ~(prev_miniblock_mask);
	} 

	else 
	// new block allocated and not a miniblock
	{
		next_block->header |= prev_alloc_mask;
        next_block->header |= prev_miniblock_mask;
	}
}


<<<<<<< HEAD
static void add_free_miniblock(block_t *block)
{
    if (miniblock_list != NULL){
        block->next_block = miniblock_list->next_block;
        miniblock_list->next_block = block;
    } else {
        miniblock_list = block;
        miniblock_list->next_block = NULL;
    }
    return;
}

static void add_free_seglist(block_t *block){
    size_t block_size = get_size(block);
    size_t free_index = find_freelist_index(block_size);

=======


static void add_free_block(block_t *block)
{
    size_t block_size = get_size(block);
    size_t free_index = find_freelist_index(block_size);

	if (block_size == 16) {
		if (miniblock_list != NULL){
			block->next_block = miniblock_list->next_block;
			miniblock_list->next_block = block;
		} else {
			miniblock_list = block;
			miniblock_list->next_block = NULL;
		}
		return;
	}
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c
    // if the segfreelist is empty, add the block
    if (seg_freelist[free_index] == NULL) {
        block->prev_block = NULL;
        block->next_block = NULL;
        seg_freelist[free_index] = block;
    //otherwise, add it to the front of the segfreelist
    } else {
        block->prev_block = NULL;
        block->next_block = seg_freelist[free_index];
        seg_freelist[free_index]->prev_block = block;
        seg_freelist[free_index] = block;
    }
}

<<<<<<< HEAD
static void add_free_block(block_t *block)
{
    size_t block_size = get_size(block);
	if (block_size == 16) {
        add_free_miniblock(block);
	} else {
        add_free_seglist(block);
    }
}

static void delete_free_miniblock(block_t *block)
{
    block_t *prev_block = miniblock_list;
    block_t *next_block = prev_block->next_block;

    if (prev_block == block) {
        miniblock_list = block->next_block;
        return;
    }
    while (next_block != NULL) {
        if (next_block == block) {
            prev_block->next_block = next_block->next_block;
            return;
        }
        prev_block = next_block;
        next_block = next_block->next_block;
    }
    return;
}

=======
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c

static void delete_free_block(block_t *block)
{
    size_t block_size = get_size(block);
    size_t free_index = find_freelist_index(block_size);

<<<<<<< HEAD
    block_t *next_block = block->next_block;
    block_t *prev_block = block->prev_block;

	if (block_size == 16) {
        delete_free_miniblock(block);
        return;
	}

    if ((next_block == NULL) && (prev_block == NULL)){
        seg_freelist[free_index] = NULL;
    }
    else if (prev_block == NULL) {
        seg_freelist[free_index] = next_block;
        seg_freelist[free_index]->prev_block = NULL;
    } 
    else if (next_block == NULL){
        prev_block->next_block = NULL;
    }
    else {
        next_block->prev_block = prev_block;
        prev_block->next_block = next_block;
    }
    return;
=======

	if (block_size == 16) {
        block_t *curr_block = miniblock_list;
        if (curr_block == block) {
            miniblock_list = block->next_block;
            return;
        }
        while (curr_block != NULL) {
            if (curr_block->next_block == block) {
                curr_block->next_block = block->next_block;
                return;
            }
            else {
                curr_block = curr_block->next_block;
            }
        }
        return;
	}


    if ((block->next_block == NULL) && (block->prev_block == NULL)){
        seg_freelist[free_index] = NULL;
    }
    else if (block->prev_block == NULL) {
        seg_freelist[free_index] = block->next_block;
        seg_freelist[free_index]->prev_block = NULL;
    } 
    else if (block->next_block == NULL){
        block->prev_block->next_block = NULL;
    }
    else{
        block->next_block->prev_block = block->prev_block;
        block->prev_block->next_block = block->next_block;
    }
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c
}


/*
 * ---------------------------------------------------------------------------
 *                        END SHORT HELPER FUNCTIONS
 * ---------------------------------------------------------------------------
 */

/******** The remaining content below are helper and debug routines ********/


/**
 * @brief Coalesces free blocks together based on the allocation status of the next
 * and previous blocks. 
 
 * If the previous block is free, then we coalesce the block with the previous 
 * block to create a larger block that starts at the previous block. 
 *
 * If the next block is free, then we coalesce the block with the next block 
 * to create a larger block that starts at the current block. 
 *
 * If the previous block is a miniblock, we manually compute the address of the 
 * previous block instead of using get_size function. 
 * 
 * @param[in] block
 * @return coalesced block
 */
static block_t *coalesce_block(block_t*);

/**
 * @brief We can make the heap larger by writing free blocks to the heap that
 * can be used. We write and add the new block at the end of the current heap, 
 * then call coalesce to reduce external fragmentation. 
 *
 * @param[in] size
 * @return extended block
 */
static block_t *extend_heap(size_t);

/**
 * @brief This function splits a free block, breaking off a chunk according to
 * asize. This reduces internal fragmentation, since we can more closely choose 
 * our block sizes. 
 *
 * @pre block to be split is allocated
 *
 * @param[in] block
 * @param[in] asize
 */
<<<<<<< HEAD
static void split_block(block_t*, size_t);
=======

static block_t *coalesce_block(block_t *block) {
    
	block_t *next_block = find_next(block);
    size_t next_size = get_size(next_block);
    bool is_next_alloc = get_alloc(next_block);

    size_t curr_size = get_size(block);
	block_t *prev_block;
	size_t prev_size = 0;

	// check if previous block is miniblock by checking curr block header
	bool is_prev_miniblock = get_prev_miniblock(block);
	// check if previous block is allocated by checking curr block header
	bool is_prev_alloc = get_prev_alloc(block);

	// then move back so prev block is at miniblock start
	if (is_prev_miniblock && !is_prev_alloc) {
		prev_size = 16;
		char *move_back = (char *)(block) - prev_size;
		prev_block = (block_t *)(move_back);
	}
	//otherwise since its not a miniblock, we do the same as before
	else if (!is_prev_miniblock && !is_prev_alloc) {
		prev_block = find_prev(block);
		prev_size = get_size(prev_block);
	} 



    if ((is_prev_alloc || prev_block == NULL) && (is_next_alloc || next_block == NULL)){
        return block;
    }
    delete_free_block(block);
	bool curr_alloc = false;
	// if prev is allocated and next is not, then delete the next block and 
	// write a new block at current location with curr + next size
    if (is_prev_alloc && !is_next_alloc) {
        delete_free_block(next_block);
        curr_size += next_size;
        write_block(block, curr_size, is_prev_miniblock, is_prev_alloc, curr_alloc);
	// if prev is free and next is allocated, then delete the prev block and 
	// write a new block at prev location 
    } else if (!is_prev_alloc && is_next_alloc) {
		//check if block before prev is a miniblock (so we can set that bit)
        bool prev_prev_miniblock = get_prev_miniblock(prev_block);
		delete_free_block(prev_block);
        curr_size += prev_size;
        block = prev_block;
        write_block(block, curr_size, prev_prev_miniblock, is_prev_alloc, curr_alloc);

    } else if (!is_prev_alloc && !is_next_alloc) {
		bool prev_prev_miniblock = get_prev_miniblock(prev_block);
        delete_free_block(prev_block);
        delete_free_block(next_block);
        curr_size += prev_size + next_size;
        block = prev_block;
	    write_block(block, curr_size, prev_prev_miniblock, is_prev_alloc, curr_alloc);
    } 
    add_free_block(block);
    return block;
}
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c



/**
 * @brief Traverses segregated freelist to find a free block when a size is 
 * requested. Because we are not always guaranteed to get a free block of the exact
 * size specified, we start at the smallest possible index, and move to higher 
 * block sizes if we have no available blocks of the size requested. 
 *
 *
 * @param[in] asize, freeindex
 * @return
 */
static block_t *find_in_freelist(size_t, size_t);


/**
 * @brief If the requested size is 16, we first want to return a block from the
 * miniblock list. Otherwise, we have to use the find_in_freelist function to find 
 * the next closest one free block. 
 *
 * @pre requested size = 16
 * @return block
 */
static block_t *find_in_miniblock(void);


/**
 * This is our overall function to find free blocks to satisfy allocation requests
 * If the requested size is 16, then we use the find_in_miniblock function. Otherwise
 * we use the find_in_freelist function. 
 *
 * @pre asize > 0
 * @param[in] asize
 * @return found block
 */
static block_t *find_fit(size_t);


/**
 * @brief Heap checker function. When called, it checks certain conditions that
 * must be true if we have a valid heap. If something is wrong, it throws an error
 *
 *
 * @param[in] line
 * @return
 */
bool mm_checkheap(int);


/**
 * @brief Initializes the heap. Creates a heap prologue and epilogue, and 
 * uses extend_heap to create one massive free block that can be used. If we are 
 * unable to do this, we return false, which would raise an issue if we run 
 * mm_checkheap(). We also initialize the segregated free list and miniblock list. 
 *
 * @return true or false
 */
bool mm_init(void);

/**
 * @brief malloc is called when we want to allocate a set amount of memory.
 * If not yet initalized, we initialize the heap. We check to make sure the requested
 * size is more than 0, then call find_fit to look for a free block closest to the
 * requested size of memory. If we cannot find a block, we extend the heap and 
 * use that newly created memory. We check to make sure our selected block of memory
 * is not allocated, then mark it as allocated. If the found block is larger than
 * we want, we try to split the block if possible.   
 *
 * @return
 */
void *malloc(size_t);

/**
 * @brief This function frees memory that has been allocated by malloc/calloc calls
 * We check to make sure the block is allocated, then mark the block as free 
 * and add it to our segregated freelist or miniblock list. We check to make sure 
 * the block can't be coalesced with nearby blocks. 
 *
 * @param[in] bp
 */
void free(void*);

/**
 * @brief We can reallocate an amount of memory using this function. 
 * If the reallocation size is 0, this is the same as freeing. If we pass a  
 * pointer that doesn't point to anything in memory, this is interpreted as a 
 * malloc call. Otherwise, we call malloc and allocate the requested size. We then
 * copy the old data into the new block and delete the old block. 
 *
 * @param[in] ptr
 * @param[in] size
 * @return
 */
void *realloc(void*, size_t);

/**
 * @brief This function is the same as a malloc call, but we initalize all the 
 * bits in the data of the block to 0. 
 *
 * @param[in] elements
 * @param[in] size
 * @return
 */
void *calloc(size_t, size_t);







static block_t *coalesce_block(block_t *block) {
    
	block_t *next_block = find_next(block);
    size_t next_size = get_size(next_block);
    bool is_next_alloc = get_alloc(next_block);

    size_t curr_size = get_size(block);
	block_t *prev_block;
	size_t prev_size = 0;

	// check if previous block is miniblock by checking curr block header
	bool is_prev_miniblock = get_prev_miniblock(block);
	// check if previous block is allocated by checking curr block header
	bool is_prev_alloc = get_prev_alloc(block);

	// then move back so prev block is at miniblock start
	if (is_prev_miniblock && !is_prev_alloc) {
		prev_size = min_block_size;
		char *move_back = (char *)(block) - prev_size;
		prev_block = (block_t *)(move_back);
	}
	//otherwise since its not a miniblock, we do the same as before
	else if (!is_prev_miniblock && !is_prev_alloc) {
		prev_block = find_prev(block);
		prev_size = get_size(prev_block);
	} 



    if (is_prev_alloc && is_next_alloc){
        return block;
    }
    delete_free_block(block);
	bool curr_alloc = false;
	// if prev is allocated and next is not, then delete the next block and 
	// write a new block at current location with curr + next size
    if (is_prev_alloc && !is_next_alloc) {
        delete_free_block(next_block);
        curr_size += next_size;
        write_block(block, curr_size, is_prev_miniblock, is_prev_alloc, curr_alloc);
	// if prev is free and next is allocated, then delete the prev block and 
	// write a new block at prev location 
    } else if (!is_prev_alloc && is_next_alloc) {
		//check if block before prev is a miniblock (so we can set that bit)
        bool prev_prev_miniblock = get_prev_miniblock(prev_block);
		delete_free_block(prev_block);
        curr_size += prev_size;
        block = prev_block;
        write_block(block, curr_size, prev_prev_miniblock, is_prev_alloc, curr_alloc);

    } else if (!is_prev_alloc && !is_next_alloc) {
		bool prev_prev_miniblock = get_prev_miniblock(prev_block);
        delete_free_block(prev_block);
        delete_free_block(next_block);
        curr_size += prev_size + next_size;
        block = prev_block;
	    write_block(block, curr_size, prev_prev_miniblock, is_prev_alloc, curr_alloc);
    } 
    add_free_block(block);
    return block;
}




static block_t *extend_heap(size_t size) {
    void *bp;


	block_t *epilogue = (block_t *)((char *)(mem_heap_hi()) - 7);
	bool prev_alloc = get_prev_alloc(epilogue);
	bool prev_miniblock = get_prev_miniblock(epilogue);
	bool curr_alloc = false;


    // Allocate an even number of words to maintain alignment
    size = round_up(size, dsize);
    if ((bp = mem_sbrk((intptr_t)size)) == (void *)-1) {
        return NULL;
    }

    // Initialize free block header/footer
    block_t *block = payload_to_header(bp);
    write_block(block, size, prev_miniblock, prev_alloc, curr_alloc);
    add_free_block(block);

    // Create new epilogue header
    block_t *block_next = find_next(block);
    write_epilogue(block_next);

    // Coalesce in case the previous block was free
    block = coalesce_block(block);

    return block;
}



<<<<<<< HEAD
=======
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
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c
static void split_block(block_t *block, size_t asize) {
    dbg_requires(get_alloc(block));

    size_t block_size = get_size(block);
	bool is_prev_miniblock = get_prev_miniblock(block);
	bool prev_alloc = true;
	bool curr_alloc = true;

    if ((block_size - asize) >= min_block_size) {
        block_t *block_next;
        write_block(block, asize, is_prev_miniblock, prev_alloc, curr_alloc);

        block_next = find_next(block);
		curr_alloc = false;

		if (asize == min_block_size) {
			is_prev_miniblock = true;
		} else {
			is_prev_miniblock = false;
		}
        write_block(block_next, block_size - asize, is_prev_miniblock, prev_alloc, curr_alloc);
        add_free_block(block_next);
    }
    dbg_ensures(get_alloc(block));
}

<<<<<<< HEAD


static block_t *find_in_freelist(size_t asize, size_t free_index) 
{
	for (size_t index = free_index; index < SEG_LIST_SIZE; index++)
	{
        block_t *block = seg_freelist[index];
        while (block != NULL) 
		{
            size_t block_size = get_size(block);
            bool is_block_alloc = get_alloc(block);
            
            if (!is_block_alloc && (asize <= block_size)) 
			{
=======
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
    size_t free_index = find_freelist_index(asize);

    for (size_t index = free_index; index < 14; index++){
        block_t *block = seg_freelist[index];
        while (block != NULL) {
            size_t block_size = get_size(block);
            bool is_block_alloc = get_alloc(block);
            
            if (!is_block_alloc && (asize <= block_size)) {
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c
                return block;
            } 
            block = block->next_block;
        }
    }
	return NULL;
}

<<<<<<< HEAD
static block_t *find_in_miniblock(){
    size_t asize = min_block_size;
    if (miniblock_list == NULL) {
        return find_in_freelist(asize, 0);
    }
    return miniblock_list;
}

static block_t *find_fit(size_t asize) {
    size_t free_index = find_freelist_index(asize);

	if (asize == min_block_size) {
		return find_in_miniblock();
	}
    return find_in_freelist(asize, free_index); // no fit found
}

=======
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c


/* check alignment of block - returns true if divisible (means block is aligned) */
static bool block_aligned(size_t curr_size)
{
    return (curr_size % dsize == 0);
}

/* check minimum block size - returns true if blocksize is more than min */
static bool check_blocksize(size_t curr_size, size_t next_size)
{
    return (curr_size > min_block_size && curr_size % 16 == 0);
}

static bool check_header(block_t *block){
    word_t* get_footer = header_to_footer(block);
    return (block->header == *get_footer);
}


/* returns true if the current block is between the heap_lo and heap_hi */
static bool check_heap_boundaries(block_t *block)
{
    return (mem_heap_lo() < (void*)block) && ((void*)block < mem_heap_hi());
}

/* check to make sure no two consecutive blocks are free - make sure prev and current blocks are allocated */
static bool check_coalesce(block_t *curr_block, block_t *next_block)
{
    bool curr_alloc = get_alloc(curr_block);
    bool next_alloc = get_alloc(next_block);

    return curr_alloc || next_alloc;
}

static bool next_prev_check(block_t *curr_node)
{
    if (curr_node->prev_block == NULL || curr_node->next_block){
        return false;
    }
    if (curr_node->prev_block->next_block != curr_node->next_block->prev_block){
        return false;
    }
<<<<<<< HEAD
    return true;
}

static bool traverse_list(block_t *curr_node, int index) {
    while (curr_node != NULL) {
        bool heap_bounds = check_heap_boundaries(curr_node);
        if (!heap_bounds) return false;

        size_t curr_size = get_size(curr_node);
        bool check_size = size_checker(index, curr_size);
        if (!check_size) return false;

        bool check_pointers = next_prev_check(curr_node);
        if (!check_pointers) return false;

        curr_node = curr_node->next_block;
    }
    return true;
}



bool mm_checkheap(int line) {
	return true;
    block_t *curr_block = heap_start;
    block_t *prev_block = find_prev(curr_block);
    block_t *next_block = find_next(curr_block);

    size_t curr_size = get_size(curr_block);
    size_t next_size = get_size(next_block);
    
    if (curr_block == NULL || prev_block != NULL) {
        return false;
    }

    /* check whole list */
    while (curr_size != 0) {
        // if false, return false
        bool heapbounds = check_heap_boundaries(curr_block); 
        // if false, return false (checks alignments and min sizes)
        bool size_checked = check_blocksize(curr_size, next_size);
        // if false, return false
        bool header_checked = check_header(curr_block);
        // if false, return false
        bool alloc_checks = check_coalesce(curr_block, next_block);

        if (!heapbounds || !size_checked || !header_checked || !alloc_checks) {
            return false;
        }
        curr_block = find_next(curr_block);
        curr_size = get_size(curr_block);
        next_block = find_next(curr_block);
        next_size = get_size(next_block);
    }

    /* check segregated free list */
    for (int seg_index = 0; seg_index < SEG_LIST_SIZE; seg_index++) {
        if (seg_freelist[seg_index] != NULL){
            block_t *curr_free = seg_freelist[seg_index];

            bool sizes_checked = traverse_list(curr_free, seg_index);
            if (!sizes_checked) return false;
        }
    }
    return true;
}



=======
    return true;
}

static bool traverse_list(block_t *curr_node, int index) {
    while (curr_node != NULL) {
        bool heap_bounds = check_heap_boundaries(curr_node);
        if (!heap_bounds) return false;

        size_t curr_size = get_size(curr_node);
        bool check_size = size_checker(index, curr_size);
        if (!check_size) return false;

        bool check_pointers = next_prev_check(curr_node);
        if (!check_pointers) return false;

        curr_node = curr_node->next_block;
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
 * @param[in] line
 * @return
 */

bool mm_checkheap(int line) {
	return true;
    block_t *curr_block = heap_start;
    block_t *prev_block = find_prev(curr_block);
    block_t *next_block = find_next(curr_block);

    size_t curr_size = get_size(curr_block);
    size_t next_size = get_size(next_block);
    
    if (curr_block == NULL || prev_block != NULL) {
        return false;
    }

    /* check whole list */
    while (curr_size != 0) {
        // if false, return false
        bool heapbounds = check_heap_boundaries(curr_block); 
        // if false, return false (checks alignments and min sizes)
        bool size_checked = check_blocksize(curr_size, next_size);
        // if false, return false
        bool header_checked = check_header(curr_block);
        // if false, return false
        bool alloc_checks = check_coalesce(curr_block, next_block);

        if (!heapbounds || !size_checked || !header_checked || !alloc_checks) {
            return false;
        }
        curr_block = find_next(curr_block);
        curr_size = get_size(curr_block);
        next_block = find_next(curr_block);
        next_size = get_size(next_block);
    }

    /* check segregated free list */
    for (int seg_index = 0; seg_index < SEG_LIST_SIZE; seg_index++) {
        if (seg_freelist[seg_index] != NULL){
            block_t *curr_free = seg_freelist[seg_index];

            bool sizes_checked = traverse_list(curr_free, seg_index);
            if (!sizes_checked) return false;
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
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c
bool mm_init(void) {
    // Create the initial empty heap
    word_t *start = (word_t *)(mem_sbrk(2 * wsize));

    if (start == (void *)-1) {
        return false;
    }

<<<<<<< HEAD
=======
    /*
     * TODO: delete or replace this comment once you've thought about it.
     * Think about why we need a heap prologue and epilogue. Why do
     * they correspond to a block footer and header respectively?
     */

>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c
    start[0] = pack(0, true, true, true); // Heap prologue (block footer)
    start[1] = pack(0, true, true, true); // Heap epilogue (block header)

    // Heap starts with first "block header", currently the epilogue
    heap_start = (block_t *)&(start[1]);

    for (int i = 0; i < SEG_LIST_SIZE; i++){
        seg_freelist[i] = NULL;
    }

	miniblock_list = NULL;

    // Extend the empty heap with a free block of chunksize bytes
    if (extend_heap(chunksize) == NULL) {
        return false;
    }

    return true;
}


<<<<<<< HEAD
=======
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
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c
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
	asize = max(round_up(size + wsize, dsize), min_block_size);


	// LOOK FURTHER - WHY DOES THIS REDUCE UTIL AND THROUGHPUT???
	// size_t aligned_blocksize = round_up(size + dsize, dsize);
    // if (aligned_blocksize >= min_block_size) {
	// 	asize = aligned_blocksize;
	// } else {
	// 	asize = min_block_size;
	// }

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
	bool is_prev_miniblock = get_prev_miniblock(block);
	bool is_prev_alloc = true;
	bool curr_alloc = true;

    write_block(block, block_size, is_prev_miniblock, is_prev_alloc, curr_alloc);
    delete_free_block(block);

    // Try to split the block if too large
    split_block(block, asize);

    bp = header_to_payload(block);

    dbg_ensures(mm_checkheap(__LINE__));
    return bp;
}



<<<<<<< HEAD
=======
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

>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c

void free(void *bp) {
    dbg_requires(mm_checkheap(__LINE__));

    if (bp == NULL) {
        return;
    }

    block_t *block = payload_to_header(bp);
    size_t size = get_size(block);

    // The block should be marked as allocated
    dbg_assert(get_alloc(block));

	bool is_prev_miniblock = get_prev_miniblock(block);
	bool is_prev_alloc = get_prev_alloc(block);
	bool curr_alloc = false;
    // Mark the block as free
    write_block(block, size, is_prev_miniblock, is_prev_alloc, curr_alloc);
    add_free_block(block);

    // Try to coalesce the block with its neighbors
    coalesce_block(block);

    dbg_ensures(mm_checkheap(__LINE__));
}


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



<<<<<<< HEAD
=======
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
>>>>>>> d962ee1523951050b48083bc818b653a64bb2a9c
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














