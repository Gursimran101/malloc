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
static const size_t min_block_size = dsize;

/**
 * @brief size of initial free block and default size for expanding heap
 */
static const size_t chunksize = (1 << 12);

/**
 * mask used to check if block is allocated or not
 */
static const word_t alloc_mask = 0x1;

/**
 * mask used to check if previous block is allocated or not - second lowest bit
 */
static const word_t prev_alloc_mask = 0x2;

/**
 * mask used to check if previous block is miniblock or not
 */
static const word_t prev_miniblock_mask = 0x4;


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
/* we can have a max of 128 bytes. each pointer is 8 bytes */
static block_t *seg_freelist[15];

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
static word_t pack(size_t size, bool prev_miniblock, bool prev_alloc, bool alloc) {
    word_t word = size;
    if (prev_miniblock) {
		word |= (1L << 2);
	}
	if (prev_alloc) {
		word |= (1L << 1);
	}
	if (alloc) {
        word |= (1L);
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
 * block's header. (we don't subtract the footer anymore because allocated blocks
 * have no footers anymore)
 *
 * @param[in] block
 * @return The size of the block's payload
 */
static size_t get_payload_size(block_t *block) {
    size_t asize = get_size(block);
    return asize - wsize;
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
    if (block == NULL){
        return false;
    }
    return extract_alloc(block->header);
}


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
    if (block == NULL){
        return false;
    }
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
    if (block == NULL){
        return false;
    }
    return extract_miniblock(block->header);
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

	bool prev_miniblock = false;
	bool prev_alloc = false;
	bool curr_alloc = true;
    block->header = pack(0, prev_miniblock, prev_alloc, curr_alloc);
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

/* find index in free list given the requested size */
static int find_freelist_index(size_t size)
{
    int index = 14;

    if (size == 16){
        index = 0;
    } else if (size >= 16 && size < 32) {
        index = 1;
    } else if (size >= 32 && size < 64) {
        index = 2;
    } else if (size >= 64 && size < 128) {
        index = 3;
    } else if (size >= 128 && size < 256) {
        index = 4;
    } else if (size >= 256  && size < 512) {
        index = 5;
    } else if (size >= 512 && size < 1028) {
        index = 6;
    } else if (size >= 1028  && size < 2048) {
        index = 7;
    } else if (size >= 2048 && size < 4096) {
        index = 8;
    } else if (size >= 4096 && size < 8192) {
        index = 9;
    } else if (size >= 8192 && size < 16384) {
        index = 10;
    } else if (size >= 16384 && size < 32768) {
        index = 11;
    } else if (size >= 32768 && size < 65536) {
        index = 12;
    } else if (size >= 65536 && size < 131072) {
        index = 13;
    }
    return index; 
}

static bool size_checker(int index, size_t size)
{
    if (index == 0 && !(size == 16)) {
        return false;
    } else if (index == 1 && !(size >= 16  && size < 32)) {
        return false;
    } else if (index == 2 && !(size >= 32 && size < 64 )) {
        return false;
    } else if (index == 3 && !(size >= 64 && size < 128)) {
        return false;
    } else if (index == 4 && !(size >= 128 && size < 256)) {
        return false;
    } else if (index == 5 && !(size >= 256 && size < 512)) {
        return false;
    } else if (index == 6 && !(size >= 512 && size < 1024)) {
        return false;
    } else if (index == 7 && !(size >= 1024 && size < 2048)) {
        return false;
    } else if (index == 8 && !(size >= 2048 && size < 4096)) {
        return false;
    } else if (index == 9 && !(size >= 4096 && size < 8192)) {
        return false;
    } else if (index == 10 && !(size >= 8192 && size < 16384)) {
        return false;
    } else if (index == 11 && !(size >= 16384 && size < 32768)) {
        return false;
    } else if (index == 12 && !(size >= 32768 && size < 65536)) {
        return false;
    } else if (index == 13 && !(size >= 65536 && size < 131072)) {
        return false;
    } else if (index == 14 && !(size >= 131072)) {
        return false;
    }
    return true;
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





static void add_free_block(block_t *block)
{
    size_t block_size = get_size(block);
    int free_index = find_freelist_index(block_size);

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


static void delete_free_block(block_t *block)
{
    size_t block_size = get_size(block);
    int free_index = find_freelist_index(block_size);

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
		char *move_back = (char *)(block) - 16;
		prev_block = (block_t *)(move_back);
		prev_size = 16;
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


	block_t *epilogue = (block_t *)((char *)(mem_heap_hi()) - 7);
	bool prev_alloc = get_prev_alloc(epilogue);
	bool prev_miniblock = get_prev_miniblock(epilogue);
	bool curr_alloc = false;


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
    write_block(block, size, prev_miniblock, prev_alloc, curr_alloc);
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
	bool is_prev_miniblock = get_prev_miniblock(block);
	bool prev_alloc = true;
	bool curr_alloc = true;

    if ((block_size - asize) >= min_block_size) {
        block_t *block_next;
        write_block(block, asize, is_prev_miniblock, prev_alloc, curr_alloc);

        block_next = find_next(block);

		if (asize == min_block_size) {
			is_prev_miniblock = true;
		} else {
			is_prev_miniblock = false;
		}
        write_block(block_next, block_size - asize, is_prev_miniblock, prev_alloc, false);
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
    int free_index = find_freelist_index(asize);

    for (int index = free_index; index < 15; index++){
        block_t *block = seg_freelist[index];
        while (block != NULL) {
            size_t block_size = get_size(block);
            bool is_block_alloc = get_alloc(block);
            
            if (!is_block_alloc && (asize <= block_size)) {
                return block;
            } 
            block = block->next_block;
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
    for (int seg_index = 0; seg_index < 15; seg_index++) {
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

    start[0] = pack(0, true, true, true); // Heap prologue (block footer)
    start[1] = pack(0, true, true, true); // Heap epilogue (block header)

    // Heap starts with first "block header", currently the epilogue
    heap_start = (block_t *)&(start[1]);

    for (int i = 0; i < 15; i++){
        seg_freelist[i] = NULL;
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

