#ifndef VVECTOR_H
#define VVECTOR_H

#ifdef __cplusplus
    extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/// @file vvector.h

/**
 * @typedef vvector
 *
 * @brief   A vvector is an object. It is a pointer to a pointer of bytes.
 *
 * @note    vvector can be returned from a function because it is allocated dynamically on the heap.
 */
typedef uint8_t ** vvector;

/**
 * @typedef vvector_malloc_fn.
 * 
 * @brief   A type representing a custom 'malloc' function.
 *
 * The vvector library supports custom memory allocators.
 * The context pointer is used to support more freedom when creating these custom allocators. 
 * Use of the context pointer is optional.  
 * To be passed to the 'new_vec()' function by use of a 'vvectorAlloc' struct. 
 *
 * @param   size Number of bytes to allocate.
 * @param   ctx  Optional: Context pointer.
 * @return  The allocated pointer or NULL.
 */
typedef void * (*vvector_malloc_fn)(ptrdiff_t size, void * ctx);

/**
 * @typedef vvector_free_fn
 *
 * @brief   A type representing a custom 'free' function.
 * 
 * The vvector library supports custom memory allocators.
 * The size value and context pointer are used to support more freedom when creating these custom allocators. 
 * Use of the size value and context pointer is optional.
 * To be passed to the 'new_vec()' function by use of a 'vvectorAlloc' struct. 
 *
 * @param ptr  The pointer to free.
 * @param size Optional: The number of bytes allocated to the pointer to be free()'d.
 * @param ctx  Optional: Context pointer.
 */
typedef void (*vvector_free_fn)(void * ptr, ptrdiff_t size, void * ctx);

/**
 * @typedef vvector_realloc_fn
 *
 * @brief   A type representing a custom 'realloc' function.
 * 
 * The vvector library supports custom memory allocators.
 * The size value and context pointer are used to support more freedom when creating these custom allocators. 
 * Use of the size value and context pointer is optional.
 * To be passed to the 'new_vec()' function by use of a 'vvectorAlloc' struct. 
 *
 * @param   ptr      The pointer to free.
 * @param   new_size The number of bytes to be allocated to the new pointer.
 * @param   old_size Optional: The number of bytes previously allocated to the pointer.
 * @param   ctx      Optional: Context pointer.
 * @return  The new pointer or NULL.
 */
typedef void * (*vvector_realloc_fn)(void * ptr, ptrdiff_t new_size, ptrdiff_t old_size, void * ctx);

/**
 * @brief   A struct represeting a user defined allocator.
 * @note    Leave a function pointer NULL to use the default implementation.
 * 
 * The vvector library supports custom allocators. 
 * Pass a reference to the vec_new function and it will *copy* this data into the vvector for later use.
 */
struct vvectorAlloc {
    vvector_malloc_fn malloc_fn;
    vvector_free_fn free_fn;
    vvector_realloc_fn realloc_fn;
    void * ctx;
};

// Create and destroy

/**
 * @brief   Internal function to create a new vector. Use 'vvectorNew'.
 * 
 * @code
 * // Create a new vector which stores 'int's and uses default allocators.
 * vvector vector = vec_new_(sizeof(int), 0); 
 * @endcode
 * 
 * @param   sizeof_type The returned value of the sizeof() operator applied on the datatype you wish the vvector to store.
 * @param   allocator   Pass a 'vvectorAlloc' struct pointer to use your own allocator, or NULL for defaults. @see vvectorAlloc.
 * @return  The returned vvector or NULL.
 *
 * @see vvectorNew
 */
vvector vec_new_(ptrdiff_t sizeof_type, struct vvectorAlloc * allocator);

/**
 * @brief Create a new vvector
 * 
 * @code
 * // Example: Create a new vector to store floats using custom allocators;
 * struct vvectorAlloc alloc = {my_malloc, my_free, my_realloc, my_ctx};
 * vvector float_vector = vec_new(float, &alloc);
 * @endcode
 * 
 * A prettier, easier to understand version of 'vec_new_'.
 *
 * @param TYPE                  The type to be stored in the vvector.
 * @param vvectorAlloc_pointer  Pass a 'vvectorAlloc' struct pointer to use your own allocator, or NULL for defaults. @see vvectorAlloc.
 */
#define vvectorNew(TYPE, vvectorAlloc_pointer) vec_new_(sizeof(TYPE), vvectorAlloc_pointer)

/**
 * @brief Free a previously created vvector.
 * 
 * @param   vec   Vector to be free()'d
 * @return  By default, returns 0 on success or 1 on failure.
 */
int vvectorFree(vvector vec);

// Shrink and Grow

/**
 * @brief Reallocate space for at least 'count' *more* elements within the vvector.
 * 
 * @param   vec       The vvector for which to reserve memory for.
 * @param   count     The number of elements to be reserved space for.  
 * @return  Returns 0 on success or 1 on failure. 
 */
int vvectorReserve(vvector vec, ptrdiff_t count);

/**
 * @brief Reallocates the memory associated to the vector to have just enough pages to hold its current elements.
 * 
 * @param   vec     The target vvector.
 * @return  0 on success, 1 on failure.
 */
int vvectorShrinkToFit(vvector vec);

// Get values

/**
 * @brief Get a pointer to the data at the specified index.
 * 
 * @param   vec   Target vvector.
 * @param   index Target index.
 * @return  Returns a pointer to the desired value or NULL.
 */
void * vvectorGetAt(vvector vec, ptrdiff_t index);

/**
 * @brief Get a pointer to the last element in the vvector.
 * 
 * @param   vec   Target vvector.
 * @return  Returns a pointer to the desired value or NULL.
 */
void * vvectorGetBack(vvector vec);

/**
 * @brief Get a pointer to the first element in the vvector.
 * 
 * @param   vec    Target vvector 
 * @return  Returns a pointer to the desired value or NULL. 
 */
void * vvectorGetFront(vvector vec);

// Add values

/**
 * @brief Replace the element stored at 'index' with the data pointed to by 'value'.
 * 
 * @param   vec     Target vvector.
 * @param   index   Target index.
 * @param   value   Pointer to the value which is to replace previously stored data.
 * @return  Returns 0 on success or a positive, non-zero value on error.
 */
int vvectorWriteValueAt(vvector vec, ptrdiff_t index, void * value);

/**
 * @brief Add an element at the back of the vvector.
 * 
 * This function calls 'vec_insert_value_at' with the 'index' value set to 'vec_length(vec)'.
 * 
 * @param   vec     Target vvector.
 * @param   value   Pointer to the value which is to be added to the vector.
 * @return  Returns 0 on success or a positive, non-zero value on error.
 */
int vvectorPushBack(vvector vec, void * value);

/**
 * @brief Add the data pointed to by 'value' to the vvector, placing it at position 'index', pushing any elements forward.
 * 
 * This function takes 'index' values: [0, vec_length(vec)], so either an already occupied position or adding it at the back.
 *
 * @param   vec     Target vvector.
 * @param   index   Target index.
 * @param   value   Pointer to the value which is to be added to the vector.
 * @return  Returns 0 on success or a positive, non-zero value on error.
 */
int vvectorInsertValueAt(vvector vec, ptrdiff_t index, void * value);

// Remove values

/**
 * @brief Remove the element stored at 'index'.
 * 
 * Other elements in the vvector get pushed forwards. No gaps left.
 *
 * @param   vec     Target vvector.
 * @param   index   Target index.
 * @return  Returns 0 on success or a positive, non-zero value on error.
 */
int vvectorRemoveAt(vvector vec, ptrdiff_t index);

/**
 * @brief Remove the element at the back of the vvector.
 * 
 * @param   vec     Target vvector.
 * @return  Returns 0 on success or a positive, non-zero value on error.
 */
int vvectorRemoveBack(vvector vec);

// Control logic

/**
 * @brief Get the length of the vvector, aka the number of elements stored within the vvector.
 *
 * @code 
 * // Example of loop control.
 * ptrdiff_t len = vec_length(vec);
 * for (ptrdiff_t i = 0 ; i < len ; i++ ){
 *      // Do something
 * }
 * @endcode
 *
 * @param   vec     Target vvector.
 * @return  Returns the length of the vvector or 0 on error.
 */
ptrdiff_t vvectorGetLength(vvector vec);

/**
 * @brief Check if a vector is empty
 * 
 * @code
 * // Example of loop control.
 * while(!vec_is_empty(vec)){
 *      // Do something
 *      vec_remove_back(vec);
 * }
 * @endcode
 * 
 * @param   vec     Target vvector.
 * @return  Returns 1 if the vector is empty or 0 if it is not.
 */
int vvectorIsEmpty(vvector vec);

// Debug functions
#ifdef LIBVVECTOR_ENABLE_DEBUG_FN
ptrdiff_t vvector_debug_get_capacity(vvector vec);
ptrdiff_t vvector_debug_get_element_size(vvector vec);
void * vvector_debug_get_malloc_fn(vvector vec);
void * vvector_debug_get_free_fn(vvector vec);
void * vvector_debug_get_realloc_fn(vvector vec);
void * vvector_debug_get_ctx(vvector vec);
#endif // LIBVVECTOR_ENABLE_DEBUG_FN

#ifdef __cplusplus
}
#endif

#endif // VVECTOR_H
