#include <not_implemented.h>
#include <mutex>

#include "../include/allocator_boundary_tags.h"

allocator_boundary_tags::~allocator_boundary_tags()
{
    deallocate_with_guard(_trusted_memory);

    //dealocate with guardant
}

allocator_boundary_tags::allocator_boundary_tags(
    allocator_boundary_tags &&other) noexcept
{
    _trusted_memory = other._trusted_memory;
    other._trusted_memory = nullptr;
}

allocator_boundary_tags &allocator_boundary_tags::operator=(
    allocator_boundary_tags &&other) noexcept
{
    if (this != &other)
    {
        deallocate_with_guard(_trusted_memory);
        _trusted_memory = other._trusted_memory;
        other._trusted_memory = nullptr;
        
    }
    return *this;

}

allocator_boundary_tags::allocator_boundary_tags(
    size_t space_size,
    allocator *parent_allocator,
    logger *logger,
    allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (space_size < get_occupied_block_metadata_size())
    {
        throw std::logic_error("Too low memory");
    }

    space_size += get_allocator_metadata_size();

    try
    {
        _trusted_memory = parent_allocator == nullptr
            ? ::operator new(space_size)
            : parent_allocator->allocate(space_size, 1);
    }
    catch (std::bad_alloc const &ex)
    {
        if (logger != nullptr)
        {
            logger->log("", logger::severity::error);
            logger->error("");
        }
        throw;
    }

    allocator **parent_allocator_space = reinterpret_cast<allocator **>(_trusted_memory);
    *parent_allocator_space = parent_allocator;

    class logger **logger_space = reinterpret_cast<class logger **>(parent_allocator_space + 1);
    *logger_space = logger;

    size_t *space_size_space = reinterpret_cast<size_t *>(logger_space + 1);
    *space_size_space = space_size;

    std::mutex *sync_object_space = reinterpret_cast<std::mutex *>(space_size_space + 1);
    //new (sync_object_space) std::mutex();
    allocator::construct(sync_object_space);

    allocator_with_fit_mode::fit_mode *fit_mode_space = reinterpret_cast<allocator_with_fit_mode::fit_mode *>(sync_object_space + 1);
    *fit_mode_space = allocate_fit_mode;

    void **first_block_address_space = reinterpret_cast<void **>(fit_mode_space + 1);
    *first_block_address_space = nullptr;

    // TODO: logs...
}

[[nodiscard]] void *allocator_boundary_tags::allocate(
    size_t value_size,
    size_t values_count)
{

    // TODO: std::unique_lock

    std::lock_guard<std::mutex> x(get_sync_object());

    void* next = get_first_occupied_block_address();
    void* previous = reinterpret_cast<void*>( reinterpret_cast<unsigned char*>(_trusted_memory) + get_allocator_metadata_size());
    auto mode = reinterpret_cast< allocator_with_fit_mode::fit_mode *>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(std::mutex));
    size_t size = reinterpret_cast<size_t>(reinterpret_cast<unsigned char*>(value_size * values_count) + get_occupied_block_metadata_size());
    void* block_to_occupate = nullptr;
    void* empty_block = nullptr;
    void* best_block = nullptr;

    if (next == nullptr)
    {
        void** block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + reinterpret_cast<unsigned char>(get_allocator_metadata_size));
        *reinterpret_cast<size_t*>(block) = size;

        //*block = size; // значение size должно лежать в метаданных
        block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(size_t));
        //block = nullptr; // left = null
        block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*));
        //block = nullptr; // right = null
        block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*));
            // указатель на аллокатор

        block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*));
        return *block;
    }

    do
    { 
     
        empty_block = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(next) - reinterpret_cast<unsigned char*>(previous));
     

         if (*mode == allocator_with_fit_mode::fit_mode::first_fit && (reinterpret_cast<unsigned char*>(empty_block) > reinterpret_cast<unsigned char*>( size + get_occupied_block_metadata_size() ))) 
         {
             //можем выделить память 
             return reinterpret_cast<void*>( reinterpret_cast<unsigned char*>(empty_block) + reinterpret_cast<unsigned char>(get_occupied_block_metadata_size));
         }

         if (*mode == allocator_with_fit_mode::fit_mode::the_best_fit && ( (best_block == nullptr && reinterpret_cast<unsigned char*>(empty_block) > reinterpret_cast<unsigned char*>(size + get_occupied_block_metadata_size())) || 
             ((best_block!= nullptr && reinterpret_cast<unsigned char*>(empty_block) > reinterpret_cast<unsigned char*>(size + get_occupied_block_metadata_size())) && reinterpret_cast<unsigned char*>(empty_block) < reinterpret_cast<unsigned char*>(best_block))))
         {

             best_block = empty_block;
             
         }
         if (*mode == allocator_with_fit_mode::fit_mode::the_worst_fit && ((best_block == nullptr && reinterpret_cast<unsigned char*>(empty_block) > reinterpret_cast<unsigned char*>(size + get_occupied_block_metadata_size())) ||
             ((best_block != nullptr && reinterpret_cast<unsigned char*>(empty_block) > reinterpret_cast<unsigned char*>(size + get_occupied_block_metadata_size())) && reinterpret_cast<unsigned char*>(empty_block) > reinterpret_cast<unsigned char*>(best_block)))) 
         {
             best_block = empty_block;
         }
     
         previous = next ; //добавить метаданные и размер занятого блока чтоб переместиться в конец?
         next = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(previous) + sizeof(size_t) + sizeof(void*));
    
    } while (next != nullptr );

    if ((*mode == allocator_with_fit_mode::fit_mode::the_best_fit || *mode == allocator_with_fit_mode::fit_mode::the_worst_fit) && best_block != nullptr) 
    {
        //можем выделить память 

        return best_block;
    }
    else if ((*mode == allocator_with_fit_mode::fit_mode::the_best_fit || *mode == allocator_with_fit_mode::fit_mode::the_worst_fit) && best_block == nullptr)
    {
        //ошибка bad alloc
    }
    





}

void allocator_boundary_tags::deallocate(
    void *at)
{
    
    std::lock_guard<std::mutex> x(get_sync_object());

    void* previous = reinterpret_cast<void**>(reinterpret_cast<char*>(at) + sizeof(size_t)); 
    void* next = reinterpret_cast<void**>(reinterpret_cast<char*>(at) + sizeof(size_t) + sizeof(void*));


    if (previous != nullptr && next!= nullptr) { // есть и справа и слева

        *reinterpret_cast<void**>(reinterpret_cast<char*>(previous) + sizeof(size_t) + sizeof(void*)) = next;
        *reinterpret_cast<void**>(reinterpret_cast<char*>(next) + sizeof(size_t)) = previous;
        
    }
    else if(previous == nullptr && next != nullptr) { //есть только справа( теперь первый зарнятый блок )
  
        *reinterpret_cast<void**>(reinterpret_cast<char*>(next) + sizeof(size_t)) = nullptr; 
        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(std::mutex) + sizeof(allocator_with_fit_mode::fit_mode)) = next ; 
        //изменяем указатель на первый блок в метаданных алокатора

    }
    else if (previous != nullptr && next == nullptr) { // есть только слева (теперь последний занятый блок)
  
        *reinterpret_cast<void**>(reinterpret_cast<char*>(previous) + sizeof(size_t) + sizeof(void*)) = nullptr;
    }
    else if (previous == nullptr && next == nullptr) { // справа и слева свободно 

        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(std::mutex) + sizeof(allocator_with_fit_mode::fit_mode)) = nullptr;

    }
}

inline void allocator_boundary_tags::set_fit_mode(allocator_with_fit_mode::fit_mode mode)
{
    *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(std::mutex)) = mode;
}

inline allocator *allocator_boundary_tags::get_allocator() const
{
    return *reinterpret_cast<allocator**>(_trusted_memory);
}

std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info() const noexcept
{
    throw not_implemented("std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info() const noexcept", "your code should be here...");
}

inline logger *allocator_boundary_tags::get_logger() const
{
    return *reinterpret_cast<class logger **>(reinterpret_cast<allocator **>(_trusted_memory) + 1);

    // return *reinterpret_cast<class logger **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *));
}

inline std::string allocator_boundary_tags::get_typename() const noexcept
{
    return "allocator_boundary_tags";
}

size_t allocator_boundary_tags::get_occupied_block_metadata_size()
{
    return sizeof(size_t) + sizeof(void *) * 3;
}

size_t allocator_boundary_tags::get_allocator_metadata_size()
{
    return sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(void *) + sizeof(std::mutex) + sizeof(allocator_with_fit_mode::fit_mode);
}

inline std::mutex& allocator_boundary_tags::get_sync_object()
{
    return *reinterpret_cast<std::mutex *>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t));
}

inline void *allocator_boundary_tags::get_first_occupied_block_address()
{
    return *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(std::mutex) + sizeof(allocator_with_fit_mode::fit_mode));

    // return *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + get_allocator_metadata_size() - sizeof(void *));
}
