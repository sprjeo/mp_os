#include <not_implemented.h>

#include "../include/allocator_global_heap.h"

allocator_global_heap::allocator_global_heap(
    logger *logger):
        _logger(logger)
{

}

[[nodiscard]] void *allocator_global_heap::allocate(
    size_t value_size,
    size_t values_count)
{
    debug_with_guard("Called method *allocator_global_heap::allocate(size_t, siae_t)");
    void* result;
    
    try {
        auto size = values_count * value_size;
        result = ::operator new(sizeof(allocator *) + sizeof(size_t) + values_count * value_size); //глобальный new

       allocator** a = reinterpret_cast<allocator**>(result);
       *a = reinterpret_cast<allocator*>(this);
       size_t* s = reinterpret_cast<size_t*>(a + 1);
       *s = size;
       return reinterpret_cast<void*>(s + 1);

    }
    catch (std::bad_alloc const &ex) {

        error_with_guard(std::string("Failed to perform void *allocator_global_heap::allocate(size_t, size_t): an exception of type std::bad_alloc occured: \"") + ex.what() + "\"")
            ->debug_with_guard("Method void* allocator_global_heap::allocate(size_t, size_t) executed with");
    }
    throw;

    debug_with_guard("Method void *allocator_global_heap::allocate(size_t, size_t) executed succesfully");

    return result;

    //return new unsigned char[values_count * value_size]; локальный new этот опертаор может быть прегружен
   
}

void allocator_global_heap::deallocate(
    void *at)
{
    debug_with_guard("Called method allocator_global_heap::deallocate(void *)");

    auto *block_start = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(at) - sizeof(size_t) - sizeof(allocator*));

    if (*reinterpret_cast<allocator**>(block_start) != this) {

        error_with_guard("Failed to perform void *allocator_global_heap::deallocate(void*): an exception of type std::bad_alloc occured")
            ->debug_with_guard("Method void* allocator_global_heap::deallocate(void *) executed reised an error");
        throw std::logic_error("block can't be deallocated ");
    }

    size_t block_size = reinterpret_cast<size_t>(reinterpret_cast<allocator**>(block_start) + 1);

    std::string bytes;

    unsigned char* p = reinterpret_cast<unsigned char*>(at);

    for (int i = 0; i < block_size; ++i)
    {
        bytes += std::to_string(*p);
        if (i != block_size - 1) {
            bytes += ' ';
        }

        ++p;
    }

    debug_with_guard(std::string("Block state before deallocation: ") + bytes);////????????????
    
    ::operator delete(block_start);

    debug_with_guard("Method void *allocator_global_heap::deallocate(void*)");

}

inline logger *allocator_global_heap::get_logger() const
{
    return _logger;
    
}

inline std::string allocator_global_heap::get_typename() const noexcept
{
    return "allocator_global_heap";
    
}