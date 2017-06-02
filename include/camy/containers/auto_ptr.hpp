/* auto_ptr.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/system.hpp>

namespace camy
{
    template <typename PtrType>
    using DestructorFunc = void (*)(PtrType* ptr);

    /*
            Simple RAII smart pointer. assignment operator transfers ownership
            invalidating the previous owner.
            TODO: Does it really make sense to define move ctor/assignment ?
            && should be casted to & and normal operator should apply. Defining
            them just for completeness.
    */
    template <typename T, template <typename> class Destructor>
    class CAMY_API IAutoPtr final
    {
      public:
        ~IAutoPtr();

        IAutoPtr(IAutoPtr<T, Destructor>&& other);
        IAutoPtr(IAutoPtr<T, Destructor>& other);
        IAutoPtr(T* ptr = nullptr);

        IAutoPtr<T, Destructor>& operator=(IAutoPtr<T, Destructor>&& other);
        IAutoPtr<T, Destructor>& operator=(IAutoPtr<T, Destructor>& other);
        IAutoPtr<T, Destructor>& operator=(T* new_ptr);

        operator T*() { return ptr; }

        T* operator->() { return ptr; }
        const T* operator->() const { return ptr; }

        T& operator*() { return *ptr; }
        const T& operator*() const { return *ptr; }

        T& operator[](rsize idx) { return ptr[idx]; }
        const T& operator[](rsize idx) const { return ptr[idx]; }

      private:
        T* ptr;
    };

    template <typename T, template <typename> class Destructor>
    inline IAutoPtr<T, Destructor>::~IAutoPtr()
    {
        if (ptr != nullptr)
        {
            Destructor<T>()(ptr);
            ptr = nullptr;
        }
    }

    template <typename T, template <typename> class Destructor>
    inline IAutoPtr<T, Destructor>::IAutoPtr(IAutoPtr<T, Destructor>&& other)
    {
        ptr = other.ptr;
        other.ptr = nullptr;
    }

    template <typename T, template <typename> class Destructor>
    inline IAutoPtr<T, Destructor>::IAutoPtr(IAutoPtr<T, Destructor>& other)
    {
        ptr = other.ptr;
        other.ptr = nullptr;
    }

    template <typename T, template <typename> class Destructor>
    inline IAutoPtr<T, Destructor>::IAutoPtr(T* ptr) : ptr(ptr)
    {
    }

    template <typename T, template <typename> class Destructor>
    inline IAutoPtr<T, Destructor>& IAutoPtr<T, Destructor>::
    operator=(IAutoPtr<T, Destructor>&& other)
    {
        if (ptr != nullptr)
        {
            Destructor<T>()(ptr);
            ptr = nullptr;
        }

        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }

    template <typename T, template <typename> class Destructor>
    inline IAutoPtr<T, Destructor>& IAutoPtr<T, Destructor>::
    operator=(IAutoPtr<T, Destructor>& other)
    {
        if (ptr != nullptr)
        {
            Destructor<T>()(ptr);
            ptr = nullptr;
        }

        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }

    template <typename T, template <typename> class Destructor>
    inline IAutoPtr<T, Destructor>& IAutoPtr<T, Destructor>::operator=(T* new_ptr)
    {
        if (ptr != nullptr)
        {
            Destructor<T>()(ptr);
            ptr = nullptr;
        }

        ptr = new_ptr;

        return *this;
    }

    template <typename T>
    struct _c_tdeallocate_array
    {
        void operator()(T* ptr) { API::tdeallocate_array(ptr); }
    };

    template <typename T>
    struct _c_deallocate
    {
        void operator()(void* ptr) { API::deallocate(ptr); }
    };

    template <typename T>
    using ArrayAutoPtr = IAutoPtr<T, _c_tdeallocate_array>;

    template <typename T>
    using RawAutoPtr = IAutoPtr<T, _c_deallocate>;
}