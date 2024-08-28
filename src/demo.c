// This is used to enable functions which users should typically never access.
// These functions reveal parts of the metadata.
// Only used for the sake of example.
#define LIBVVECTOR_ENABLE_DEBUG_FN
#include "vvector.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* You can use your own custom allocators! This is just an example. */

void my_free(void * ptr, ptrdiff_t size, void * ctx){
    (void) size;
    (void) ctx;
    free(ptr);
}

void * my_realloc(void * ptr, ptrdiff_t new_size, ptrdiff_t old_size, void * ctx){
    (void) old_size;
    (void) ctx;
    return realloc(ptr, new_size);
}

void * my_malloc(ptrdiff_t size, void * ctx){
    (void) ctx;
    return malloc(size);
}

// A vvector can easily be returned from a function.
vvector create_vector_of_100_numbers(void){
    vvector vec = vvectorNew(int, 0);
    if (!vec) return 0;

    for (int i = 0 ; i < 100 ; i++){
        int error = vvectorPushBack(vec, &i);
        if (error) return 0;
    }

    return vec;
};

int main(){
    struct vvectorAlloc my_allocator = 
    {
        .malloc_fn = my_malloc,
        .free_fn = my_free,
        .realloc_fn = my_realloc,
        // The context pointer offers greater flexibility when using custom allocators. 
        // In this example, I'll just let it be null.
        .ctx = 0
    };
    // Create a new vector with you own custom allocator.
    // The data from my_allocator is *copied* into the vector.
    // The context pointer is *your* business.
    vvector my_float_vector = vvectorNew(float, &my_allocator);
    // Simple error checking.
    if (!my_float_vector) exit(1);

    // Add some values.

    // These functions could fail, so let's declare an error variable to check.
    // This is optional, but it is recommended.
    int error = 0;
    
    // To insert data into the vector, we need a pointer to said data. This value is then *copied*.
    float my_value = 1.12f;
    error = vvectorPushBack(my_float_vector, &my_value);
    if (error) exit (error);

    // Since the data within my_value is copied, we can reuse this variable.
    my_value = 3.14159f;
    // This adds a new value to the *front* of our vector, aka index 0, pushing everything else back.
    // Works with other indexes, even when index = length (thus adding a new element to the back)
    error = vvectorInsertValueAt(my_float_vector, 0, &my_value);
    if (error) exit(error);

    // Adding some more values.
    my_value = 2.71f;
    error = vvectorPushBack(my_float_vector, &my_value);
    if (error) exit(error);
    my_value = 6.25f;
    error = vvectorPushBack(my_float_vector, &my_value);
    if (error) exit(error);
    my_value = 6.50f;
    error = vvectorPushBack(my_float_vector, &my_value);
    if (error) exit(error);
    my_value = 6.75f;
    error = vvectorPushBack(my_float_vector, &my_value);
    if (error) exit(error);

    // By now, the vector looks like this: [3.14159, 1.12, 2.71, 6.25, 6.50, 6.75]
    // Let's say we REALLY hate pi. Let's write something else in its place!
    my_value = 0.1f;
    error = vvectorWriteValueAt(my_float_vector, 0, &my_value);
    if (error) exit(error); 
    // Now the vector looks like this: [0.1, 1.12, 2.71, 6.25, 6.50, 6.75]

    // Let's remove some values.

    // We want to remove the element at index 1, which is 1.12
    error = vvectorRemoveAt(my_float_vector, 1);
    if (error) exit(error); 

    // Let's also remove what's at the back of the vector, which is: 6.75
    error = vvectorRemoveBack(my_float_vector);
    if (error) exit(error); 

    // Finally, let's look at our vector's contents. It should be:
    // [0.1, 2.71, 6.25, 6.50]
    // For this, we need a return pointer of appropriate type.
    float * ret = 0;

    // Get the front value
    ret = vvectorGetFront(my_float_vector);
    if (!ret) exit (1);
    printf("The value at the front is: %f\n", *ret);
    // Get the values in the middle:
    ret = vvectorGetAt(my_float_vector, 1);
    if (!ret) exit (1);
    printf("The value at index 1 is: %f\n", *ret);
    ret = vvectorGetAt(my_float_vector, 2);
    if (!ret) exit (1);
    printf("The value at index 2 is: %f\n", *ret);
    // Get the back value
    ret = vvectorGetBack(my_float_vector);
    if (!ret) exit (1);
    printf("The value at the back is: %f\n", *ret);

    // Or, we can access these values as such:
    // First, get the length of the vector.
    ptrdiff_t length = vvectorGetLength(my_float_vector);
    for (ptrdiff_t i = 0; i < length; i++) {
        ret = vvectorGetAt(my_float_vector, i);
        if (!ret) exit(1);
        printf("The value at index %ld is: %f\n", i, *ret);
    }

    // Now, let's get rid of the entire contents.
    // While there are still elements in the vector...
    while (!vvectorIsEmpty(my_float_vector)) {
        // remove the last one.
        error = vvectorRemoveBack(my_float_vector);
        if (error) exit(error);
    }

    // Our vector is now empty. Let's free it.
    error = vvectorFree(my_float_vector);
    if (error) exit(error);





    // Vectors can be returned from functions!
    vvector my_int_vector = create_vector_of_100_numbers();
    // We're not the biggest fans of the number 2. Let's remove it. ( It is at index 2 )
    error = vvectorRemoveAt(my_int_vector, 2);
    if (error) exit(error);

    // We also plan on adding 100 more elements in this vector.
    // Let's reserve some space.
    printf("Capacity before reserving: %ld\n", vvector_debug_get_capacity(my_int_vector));
    error = vvectorReserve(my_int_vector, 100);
    if (error) exit(error);
    printf("Capacity after reserving: %ld\n", vvector_debug_get_capacity(my_int_vector));

    for (int i = 100 ; i < 200 ; i++){
        error = vvectorPushBack(my_int_vector, &i);
        if (error) exit(error);
    }

    // Printing the values within the vector.
    printf("my_int_vector before removing last 50 values: \n[ ");
    for (ptrdiff_t i = 0 ; i < vvectorGetLength(my_int_vector) ; i++){
        int * int_ret = vvectorGetAt(my_int_vector, i);
        printf("%d ", *int_ret);
    }
    printf("]\n");

    // Let's remove some values...
    for (int i = 0; i < 50; i++) {
        error = vvectorRemoveBack(my_int_vector);
        if (error) exit(error);
    }

    printf("my_int_vector after removing last 50 values: \n[ ");
    for (ptrdiff_t i = 0 ; i < vvectorGetLength(my_int_vector) ; i++){
        int * int_ret = vvectorGetAt(my_int_vector, i);
        printf("%d ", *int_ret);
    }
    printf("]\n");

    printf("Capacity before shrinking: %ld\n", vvector_debug_get_capacity(my_int_vector));

    // And since we removed the values, we can shrink the vector a little bit to save space.
    error = vvectorShrinkToFit(my_int_vector);
    if (error) exit(error);

    printf("Capacity after shrinking: %ld\n", vvector_debug_get_capacity(my_int_vector));

    // Finally, free the vector.
    error = vvectorFree(my_int_vector);
    if (error) exit(error);
}
