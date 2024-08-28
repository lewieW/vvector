/*
    Copyright 2024 I. Laurentiu

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include "vvector.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/// @file vvector.c

#define NR_ELEM_IN_PAGE 32

#define VEC_ENOMEM 0        /**< Returned by functions which return pointers. */
#define VEC_ENOVEC 1        /**< Indicates that the provided vector argument is NULL. */
#define VEC_EBADINDEX 2     /**< Indicates that the provided index argument is not valid. */
#define VEC_ENOVALUE 3      /**< Indicates that the provided data pointer argument is NULL. */

#define VEC_ESEVERE 99      /**< Indicates an error which should typically never occur. If returned, terminate the program immediately. */

/**
 * @internal
 * @struct vvectorMetadata_
 * @brief Represents the basic, minimum metadata of a vector.
 */
struct vvectorMetadata_{
    ptrdiff_t capacity;        /**< The amount of memory allocated to the vector. Includes metadata. */
    ptrdiff_t length;          /**< The number of elements stored within the vector. */ 
    ptrdiff_t element_size;    /**< The size of an individual element in bytes. */
};                                          

/* Default library allocators */

/**
 * @internal
 * @brief Default library implementation of malloc.
 *
 * @param size Number of bytes to allocate.
 * @param ctx  Unused. Context pointer.
 */
void * vvector_lib_malloc(ptrdiff_t size, void * ctx){
    (void) ctx;

    return malloc(size);
}

/**
 * @internal
 * @brief Default library implementation of free.
 *
 * @param ptr  Pointer to be free()'d.
 * @param size Unused. Number of bytes to be free()'d
 * @param ctx  Unused. Context pointer.
 */
void vvector_lib_free(void * ptr, ptrdiff_t size, void * ctx){
    (void) size;
    (void) ctx;

    return free(ptr);
}

/**
 * @internal
 * @brief Default library implementation of realloc.
 *
 * @param ptr      Pointer to be realloc()'d.
 * @param new_size Number of bytes to be allocated to the vector.
 * @param old_size Unused. The number of bytes previously allocated to the vector.
 * @param ctx      Unused. Context pointer.
 */
void * vvector_lib_realloc(void * ptr, ptrdiff_t new_size, ptrdiff_t old_size, void * ctx){
    (void) old_size;
    (void) ctx;

    return realloc(ptr, new_size);
}

/* Helper functions */

/**
 * @internal
 * @brief Get the basic metadata associated with a given vvector. \
 * 'element_size' value should NOT be used with this function. Use vec_get_element_size().
 * 
 * @param   vec     The target vvector.
 * @return  The vvector's basic metadata. 
 */
static struct vvectorMetadata_ get_meta(vvector vec){
    struct vvectorMetadata_ meta;

    meta.capacity = *((ptrdiff_t *) (&((*vec)[sizeof(ptrdiff_t) * 0])));
    meta.length = *((ptrdiff_t *) (&((*vec)[sizeof(ptrdiff_t) * 1])));
    meta.element_size = *((ptrdiff_t *) (&((*vec)[sizeof(ptrdiff_t) * 2])));

    return meta;
}

/**
 * @internal
 * @brief Returns the number of bytes an individual element in the vvector takes up.
 * 
 * The sign of element_size is actually a flag indicating whether custom allocators are used or not.
 * Only use this function to check element_size.
 *
 * @param   vec   The target vvector.
 * @return  Size in bytes of an element within the vvector.
 */
static ptrdiff_t vec_get_element_size(vvector vec){
    ptrdiff_t element_size = get_meta(vec).element_size;
    if (element_size < 0) return (-element_size);

    return element_size;
}

/**
 * @internal
 * @brief Returns the number of bytes allocated to the vvector.
 * 
 * @param   vec   The target vvector.
 * @return  Number of bytes allocated to the vvector.
 */
static ptrdiff_t vec_get_capacity(vvector vec){
    return get_meta(vec).capacity;
}

/**
 * @internal 
 * @brief Find the minimum amount of pages needed to store 'len' elements.
 * 
 * Often used internally for code which deals with memory allocation.
 *
 * @param   len             Number of elements for which to calculate the number of pages needed to store said elements.
 * @param   nr_elem_in_page How many elements are in a page.
 * @return  The number of pages needed to store 'len' elements.
 */
static ptrdiff_t length_to_pages(ptrdiff_t len, ptrdiff_t nr_elem_in_page){
    // No remainder, means we can just return x/y
    if (len % nr_elem_in_page == 0){
        return (len/nr_elem_in_page);
    }

    // We DO have a remainder, so we add one to reach the ceil() value.
    return ((len/nr_elem_in_page) + 1);
}

/**
 * @internal 
 * @brief Returns the number of bytes allocated to a vvector's metadata. 
 * 
 * This function is used internally in code which has to deal with memory allocation.
 * As a vvector can have *additional* metadata representing custom allocators. This complicates code a bit.
 * The vvector library stores a flag indicating this in its vvector object for space-saving reasons.
 *
 * The information returned is found by checking if element_size is negative or not.
 * Negative means there are custom allocators present, positive means that they are not. 
 *
 * @param   vec     The target vvector.
 * @return  The number of bytes.
 */
static ptrdiff_t getLengthOfMetadata(vvector vec){
    // Raw metadata.
    struct vvectorMetadata_ meta = get_meta(vec);

    if (meta.element_size < 0) return (sizeof(struct vvectorMetadata_) + sizeof(struct vvectorAlloc));

    return sizeof(struct vvectorMetadata_);
}

/**
 * @internal 
 * @brief Returns the number of pages currently allocated, but not *necessarily* in use by a vvector.
 * 
 * @param   vec     The target vvector.
 * @return  The number of pages.
 */
static ptrdiff_t nr_pages_available(vvector vec){
    ptrdiff_t vec_capacity = vec_get_capacity(vec);
    ptrdiff_t vec_element_size = vec_get_element_size(vec);
    
    ptrdiff_t element_mem = vec_capacity - getLengthOfMetadata(vec);

    return ((element_mem/vec_element_size)/NR_ELEM_IN_PAGE);
} 

/**
 * @internal 
 * @brief Returns the number of pages currently allocated *and* in use by a vvector.
 * 
 * @param   vec     The target vvector.
 * @return  The number of pages
 */
static ptrdiff_t nr_pages_used(vvector vec){
    ptrdiff_t vec_length = vvectorGetLength(vec);

    return (length_to_pages(vec_length, NR_ELEM_IN_PAGE));
}

/**
 * @internal 
 * @brief Used to check if the vvectorShrinkToFit function should quit early.
 * 
 * @param   vec     The target vvector
 * @return  1 if the vvector does have empty pages, 0 otherwise.
 */
static int has_empty_pages(vvector vec){
    if (nr_pages_available(vec) - nr_pages_used(vec) == 0) return 0;

    return 1;
}

/**
 * @internal
 * @brief Used to check if a vvector has custom allocators attached,
 *
 * @param   vec     The target vvector.
 * @return  1 if the vector  custom allocators, 0 if not.
 */
static int has_custom_alloc(vvector vec){
    struct vvectorMetadata_ meta = get_meta(vec);

    if (meta.element_size < 0) {
        // Custom allocators ARE present.
        return 1;
    } else {
        return 0;
    }
}

/**
 * @internal 
 * @brief Helper function used for convenience, especially when passing the context pointer.
 * 
 * @note All but one use of this function calls 'has_custom_alloc' before running this. I will still run it for future error prevention.
 *
 * @param   vec     The target vvector.
 * @return  Returns a pointer to a vvector's allocators, or NULL if no such allocators exist.
 */
static const struct vvectorAlloc * get_alloc(vvector vec){
    if (has_custom_alloc(vec)) {
        return (struct vvectorAlloc *)(&((*vec)[sizeof(struct vvectorMetadata_)]));
    } else {
        return 0;
    }
}

/**
 * @internal
 * @brief Logic control function which returns 1 if the vvector's capacity is too small to hold another element.
 * 
 * @param   vec     The target vvector.
 * @return  1 if the vvector is full, 0 otherwise.
 */
static int is_full(vvector vec){
    ptrdiff_t vec_capacity = vec_get_capacity(vec);
    ptrdiff_t vec_length = vvectorGetLength(vec);
    ptrdiff_t vec_element_size = vec_get_element_size(vec);    

    ptrdiff_t nr_of_bytes_of_metadata = 0;

    if (has_custom_alloc(vec)) {
        nr_of_bytes_of_metadata = sizeof(struct vvectorAlloc) + sizeof(struct vvectorMetadata_);
    } else {
        nr_of_bytes_of_metadata = sizeof(struct vvectorMetadata_);
    }

    if (vec_capacity == vec_length * vec_element_size + nr_of_bytes_of_metadata) {
        return 1;
    }

    return 0;
}

/**
 * @internal 
 * @brief Check for an index's "validity".
 * 
 * @warning This function does not check its arguments for validity!
 *
 * @note An index is valid if it is in [0, vvectorGetLength(vec)].
 *
 * @param   vec   The target vvector.
 * @param   index Index to check for validity.
 * @return  1 if the index is valid, 0 otherwise.
 */
static int index_is_invalid(vvector vec, ptrdiff_t index){
    // No need to check for bad args as caller already did.

    ptrdiff_t vec_length = vvectorGetLength(vec);

    if (index >= vec_length) {
        return 1;
    }
    return 0;
}

/**
 * @internal
 * @brief Helper function which a pointer to data stored just past the vvector's metadata, aka the space reserved for elements. 
 * 
 * @code
 * To visualize this function:
 * +------------------+----------------------+----------------------+---------
 * | vvectorMetadata_ | vvectorGetAt(vec, 0) | vvectorGetAt(vec, 1) | etc....
 * +------------------+----------------------+----------------------+---------
 *                    ^ Pointer to here is returned.
 *
 * This works the same even with a custom allocator present.
 * +------------------+--------------+----------------------+----------------------+---------
 * | vvectorMetadata_ | vvectorAlloc | vvectorGetAt(vec, 0) | vvectorGetAt(vec, 1) | etc....
 * +------------------+--------------+----------------------+----------------------+---------
 *                                   ^ Pointer to here is returned.
 * @endcode
 *
 * @warning This function does not check its arguments for validity! It is the job of the caller to know what to do.
 * @note if the vector is empty, a pointer is still returned! It is the job of the caller to know what to do.
 *
 * @param   vec     The target vvector.
 * @return  Pointer to the start of section of memory which contains elements.
 */
static void * get_start_of_data(vvector vec){
    if (has_custom_alloc(vec)){
        return &((*vec)[sizeof(struct vvectorMetadata_) + sizeof(struct vvectorAlloc)]);
    } else {
        return &((*vec)[sizeof(struct vvectorMetadata_)]);
    }
}

// << ALLOCATOR RELATED FUNCTIONS >>

/**
 * @brief Returns the malloc functions to use for this vvector.
 * 
 * @param   vec     The target vvector. 
 * @return  The malloc function to use. 
 */
static vvector_malloc_fn get_malloc(vvector vec){
    if (!has_custom_alloc(vec)){
        return vvector_lib_malloc;
    }

    const struct vvectorAlloc * alloc = get_alloc(vec);

    return alloc->malloc_fn;
}

/**
 * @brief Returns the free functions to use for this vvector.
 * 
 * @param   vec     The target vvector. 
 * @return  The free function to use. 
 */
static vvector_free_fn get_free(vvector vec){
    if (!has_custom_alloc(vec)){
        return vvector_lib_free;
    }

    const struct vvectorAlloc * alloc = get_alloc(vec);

    return alloc->free_fn;
}

/**
 * @brief Returns the realloc functions to use for this vvector.
 * 
 * @param   vec     The target vvector. 
 * @return  The realloc function to use. 
 */
static vvector_realloc_fn get_realloc(vvector vec){
    if (!has_custom_alloc(vec)){
        return vvector_lib_realloc;
    }

    const struct vvectorAlloc * alloc = get_alloc(vec);

    return alloc->realloc_fn;
}

// << LOGIC CONTROL >> 

ptrdiff_t vvectorGetLength(vvector vec){
    if (!vec || !*vec) return 0;

    struct vvectorMetadata_ meta = get_meta(vec);

    return meta.length;
}

int vvectorIsEmpty(vvector vec){
    if (!vec || !*vec) {
        // The hope here is somebody does
        // "while (!vvectorIsEmpty(bad_vec))"
        // And it doesn't run...
        return 1;
    }

    ptrdiff_t vec_length = vvectorGetLength(vec);

    if (vec_length == 0){
        return 1;
    }
    return 0;
}

// << MEMORY FUNCTIONS >> 

/**
 * @internal 
 * @brief Allocates another page to the vvector if it is full.
 * 
 * @param   vec     The target vvector.
 * @return  0 on success, 1 on failure.
 */
static int add_page_if_needed(vvector vec){
    if (!is_full(vec)){
        return 0;
    }

    ptrdiff_t vec_capacity = vec_get_capacity(vec);
    ptrdiff_t vec_element_size = vec_get_element_size(vec);

    void * ctx = 0;

    if (has_custom_alloc(vec)){
        const struct vvectorAlloc * alloc = get_alloc(vec);
        ctx = alloc->ctx;
    } else {
        ctx = 0;
    }

    *vec = get_realloc(vec)(*vec, vec_capacity + NR_ELEM_IN_PAGE * vec_element_size, vec_capacity, ctx);
    if (!*vec) return 1;

    // Updating the metadata.
    struct vvectorMetadata_ meta = get_meta(vec);

    meta.capacity = vec_capacity + NR_ELEM_IN_PAGE * vec_element_size;
    memcpy(*vec, &meta, sizeof(struct vvectorMetadata_));

    return 0;
}

int vvectorShrinkToFit(vvector vec){
    if (!vec || !*vec) {
        return VEC_ENOVEC;
    }

    // Get metadata to update later.
    struct vvectorMetadata_ meta = get_meta(vec);

    ptrdiff_t vec_capacity = vec_get_capacity(vec);
    ptrdiff_t vec_element_size = vec_get_element_size(vec);

    // Exit early if we don't have to shrink at all.
    if (!has_empty_pages(vec)) return 0;

    // Remove unused pages.
    ptrdiff_t new_size = vec_capacity - (nr_pages_available(vec) - nr_pages_used(vec)) * vec_element_size * NR_ELEM_IN_PAGE;

    void * ctx = 0;

    // If ctx value is available, use that. Otherwise NULL.
    if (has_custom_alloc(vec)){
        const struct vvectorAlloc * alloc = get_alloc(vec);
        ctx = alloc->ctx;
    } else {
        ctx = 0;
    }

    *vec = get_realloc(vec)(*vec, new_size, vec_capacity, ctx);
    if (!*vec) return 1;

    // Update metadata
    meta.capacity = new_size;
    memcpy(*vec, &meta, sizeof(struct vvectorMetadata_));

    return 0;
}

int vvectorReserve(vvector vec, ptrdiff_t n){
    if (!vec || !*vec) {
        return VEC_ENOVEC;
    }

    // Attempted to allocate negative number of elements
    if (n < 0) {
        return 1;
    }

    // Get metadata now to update it later.
    struct vvectorMetadata_ meta = get_meta(vec);

    ptrdiff_t vec_capacity = vec_get_capacity(vec);
    ptrdiff_t vec_element_size = vec_get_element_size(vec);

    // Get the ctx pointer or lack thereof
    void * ctx = 0;
    if (has_custom_alloc(vec)){
        const struct vvectorAlloc * alloc = get_alloc(vec);
        ctx = alloc->ctx;
    } else {
        ctx = 0;
    }

    // Add the required amount of memory, in addition to the memory already used.
    ptrdiff_t new_capacity = length_to_pages(n, NR_ELEM_IN_PAGE) * vec_element_size * NR_ELEM_IN_PAGE + vec_capacity;

    *vec = get_realloc(vec)(*vec, new_capacity, vec_capacity, ctx);
    if (!*vec) return VEC_ENOVEC;

    // Update metadata
    meta.capacity = new_capacity;
    memcpy(*vec, &meta, sizeof(struct vvectorMetadata_));

    return 0;
}

// << CREATE AND DESTROY >> 

vvector vec_new_(ptrdiff_t sizeof_type, struct vvectorAlloc * allocator){
    // Shouldn't really happen, but if it does handle it.
    if (sizeof_type < 0) return 0;

    struct vvectorMetadata_ meta;
    meta.element_size = sizeof_type;
    meta.length = 0;

    if (!allocator) {
        meta.capacity = sizeof(struct vvectorMetadata_);

        uint8_t * new_vector_data = vvector_lib_malloc(meta.capacity, 0);
        if (!new_vector_data) return VEC_ENOMEM;

        memcpy(new_vector_data, &meta, sizeof(struct vvectorMetadata_));

        uint8_t ** vector_handle = vvector_lib_malloc(sizeof(uint8_t *), 0);
        if (!vector_handle) return VEC_ENOMEM;

        *vector_handle = new_vector_data;

        return vector_handle;
    } else {
        // !!! Hack: A negative element_size value represents that there are custom allocators present.
        meta.element_size = -sizeof_type;

        // If a function pointer is missing, use defaults.
        struct vvectorAlloc a;
        a.malloc_fn = (allocator->malloc_fn) ? allocator->malloc_fn : vvector_lib_malloc;
        a.free_fn = (allocator->free_fn) ? allocator->free_fn : vvector_lib_free;
        a.realloc_fn = (allocator->realloc_fn) ? allocator->realloc_fn : vvector_lib_realloc;
        a.ctx = allocator->ctx;

        meta.capacity = sizeof(struct vvectorMetadata_) + sizeof(struct vvectorAlloc);

        uint8_t * new_vector_data = a.malloc_fn(meta.capacity, a.ctx);
        if (!new_vector_data) return VEC_ENOMEM;


        // Metadata first, then the allocator. This order is very important.
        memcpy(new_vector_data, &meta, sizeof(struct vvectorMetadata_));
        memcpy(&(new_vector_data[sizeof(struct vvectorMetadata_)]), &a, sizeof(struct vvectorAlloc));

        uint8_t ** vector_handle = a.malloc_fn(sizeof(uint8_t *), a.ctx);
        if (!vector_handle) return VEC_ENOMEM;

        *vector_handle = new_vector_data;

        return vector_handle;
    }
}

int vvectorFree(vvector vec){
    if (!vec || !*vec) return VEC_ENOVEC;

    ptrdiff_t vec_capacity = vec_get_capacity(vec);

    const struct vvectorAlloc * alloc = get_alloc(vec);

    // Custom allocators might not be present, so check if alloc is NULL.
    void * ctx = (alloc) ? alloc->ctx : 0;

    // Store the free_fn function pointer and THEN use it. Avoids use-after-free.
    vvector_free_fn free_fn = get_free(vec);

    free_fn(*vec, vec_capacity, ctx);
    *vec = 0;
    free_fn(vec, sizeof(void *), ctx);
    vec = 0;

    return 0;
}

/* Get functions */

void * vvectorGetAt(vvector vec, ptrdiff_t index){
    if (!vec || !*vec) {
        // NULL pointer error.
        return 0;
    }

    ptrdiff_t vec_element_size = vec_get_element_size(vec);

    if (index_is_invalid(vec, index)){
        // Bad index.
        return 0;
    }

    uint8_t * start_of_data = get_start_of_data(vec);

    return &(start_of_data[vec_element_size * index]);
};

void * vvectorGetBack(vvector vec){
    if (!vec || !*vec) {
        return 0;
    }

    // If vvector is empty, there is nothing to return.
    if (vvectorIsEmpty(vec)){
        return 0;
    }

    ptrdiff_t vec_length = vvectorGetLength(vec);

    // Back is always at index = length - 1;
    return vvectorGetAt(vec, vec_length-1);
}

void * vvectorGetFront(vvector vec){
    if (!vec || !*vec) {
        return 0;
    }

    // If the vvector is empty, there is nothing to return.
    if (vvectorIsEmpty(vec)){
        return 0;
    }

    // Front is always at index = 0;
    return vvectorGetAt(vec, 0);
}

/* Add & Write functions */
// Overwrite, no realloc.

int vvectorWriteValueAt(vvector vec, ptrdiff_t index, void * value){
    if (!vec || !*vec) {
        return VEC_ENOVEC;
    }

    ptrdiff_t vec_element_size = vec_get_element_size(vec);

    if (index_is_invalid(vec, index)){
        return VEC_EBADINDEX;
    }

    if (!value) {
        return VEC_ENOVALUE;
    }

    uint8_t * start_of_data = get_start_of_data(vec);

    memcpy(&(start_of_data[index * vec_element_size]), value, vec_element_size);

    return 0;
}

/* Insert, calls realloc. */

int vvectorInsertValueAt(vvector vec, ptrdiff_t index, void * value){
    if (!vec || !*vec) {
        return VEC_ENOVEC;
    }

    add_page_if_needed(vec);

    ptrdiff_t vec_length = vvectorGetLength(vec);
    ptrdiff_t vec_element_size = vec_get_element_size(vec);

    if (index > vvectorGetLength(vec)){
        return VEC_EBADINDEX;
    }

    if (!value) {
        return VEC_ENOVALUE;
    }

    uint8_t * start_of_data = get_start_of_data(vec);

    memmove(&(start_of_data[(index + 1) * vec_element_size]), &(start_of_data[index * vec_element_size]), (vec_length - index) * vec_element_size);
    memmove(&(start_of_data[index * vec_element_size]), value, vec_element_size);
    
    // Update metadata.
    struct vvectorMetadata_ meta = get_meta(vec);

    meta.length++;
    memcpy(*vec, &meta, sizeof(meta));

    return 0;
}

int vvectorPushBack(vvector vec, void * value){
    if (!vec || !*vec) {
        return VEC_ENOVEC;
    }

    if (!value) {
        return VEC_ENOVALUE;
    }

    int err = vvectorInsertValueAt(vec, vvectorGetLength(vec), value);
    if (err) return err;

    return 0;
}

/* Remove/Delete functions */
// Remove, doesn't call realloc.

int vvectorRemoveAt(vvector vec, ptrdiff_t index){
    if (!vec || !*vec) {
        return VEC_ENOVEC;
    }

    ptrdiff_t vec_length = vvectorGetLength(vec);
    ptrdiff_t vec_element_size = vec_get_element_size(vec);

    if (index_is_invalid(vec, index)){
        return VEC_EBADINDEX;
    }

    uint8_t * start_of_data = get_start_of_data(vec);

    memmove(&(start_of_data[index * vec_element_size]), &(start_of_data[(index + 1) * vec_element_size]), (vec_length - index - 1) * vec_element_size);
    
    // Update metadata.
    struct vvectorMetadata_ meta = get_meta(vec);

    meta.length--;
    memcpy(*vec, &meta, sizeof(meta));

    return 0;
}

int vvectorRemoveBack(vvector vec){
    if (!vec || !*vec) {
        return VEC_ENOVEC;
    }

    // Nothing to remove.
    if (vvectorIsEmpty(vec)){
        return 2;
    }

    int err = vvectorRemoveAt(vec, vvectorGetLength(vec) - 1);
    if (err) return err;

    return 0;
}

// << Debug >>

/// This returns the RAW element size, which is either negative or positive. @see vec_get_element_size().
ptrdiff_t vvector_debug_get_element_size(vvector vec){
    if (!vec || !*vec) {
        return VEC_ENOVEC;
    }
    
    struct vvectorMetadata_ meta = get_meta(vec);

    return meta.element_size;
}

ptrdiff_t vvector_debug_get_capacity(vvector vec){
    if (!vec || !*vec) {
        return VEC_ENOVEC;
    }

    struct vvectorMetadata_ meta = get_meta(vec);

    return meta.capacity;
}

void * vvector_debug_get_malloc_fn(vvector vec){
    if (!vec || !*vec) {
        return 0;
    }

    if (has_custom_alloc(vec)){
        const struct vvectorAlloc * alloc = get_alloc(vec);

        return alloc->malloc_fn;
    }

    return 0;
}

void * vvector_debug_get_free_fn(vvector vec){
    if (!vec || !*vec) {
        return 0;
    }

    if (has_custom_alloc(vec)){
        const struct vvectorAlloc * alloc = get_alloc(vec);

        return alloc->free_fn;
    }

    return 0;
}

void * vvector_debug_get_realloc_fn(vvector vec){
    if (!vec || !*vec) {
        return 0;
    }

    if (has_custom_alloc(vec)){
        const struct vvectorAlloc * alloc = get_alloc(vec);

        return alloc->realloc_fn;
    }

    return 0;
}

void * vvector_debug_get_ctx(vvector vec){
    if (!vec || !*vec) {
        return 0;
    }

    if (has_custom_alloc(vec)){
        const struct vvectorAlloc * alloc = get_alloc(vec);

        return alloc->ctx;
    }

    return 0;
}
