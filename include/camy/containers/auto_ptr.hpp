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
    // Simple RAII smart pointer.
    // Assignment transfers ownership invalidating the previous AutoPtr<>
    // ALlocation and deallocation are managed together as they require the same set of functions.
    //! With C++14 a templated (variadic too) lambda can be passed as template and make things
    //! slightly cleaner
	//? Currently alloc()s are unaligned (TODO)
    template <typename T,
              template <typename> class Constructor,
              template <typename> class Destructor>
    class CAMY_API IAutoPtr final
    {
    public:
        using TAutoPtr = IAutoPtr<T, Constructor, Destructor>;

        using Alloc = Constructor<T>;

        using TValue = T;
        using TValue = T;
        using TPtr = T*;
        using TConstPtr = const T*;
        using TRef = T&;
        using TConstRef = const T&;

    public:
        ~IAutoPtr();

        IAutoPtr(TAutoPtr& other);
        IAutoPtr(TAutoPtr&& other);
        IAutoPtr(TPtr ptr = nullptr);

        TAutoPtr& operator=(TAutoPtr& other);
        TAutoPtr& operator=(TAutoPtr&& other);
        TAutoPtr& operator=(TPtr new_ptr);

        operator TPtr() { return ptr; }

        TPtr operator->() { return ptr; }
        TConstPtr operator->() const { return ptr; }

        TRef operator*() { return *ptr; }
        TConstRef operator*() const { return *ptr; }

        TRef operator[](rsize idx) { return ptr[idx]; }
        TConstRef operator[](rsize idx) const { return ptr[idx]; }

    private:
        TPtr ptr;
    };

    template <typename T,
              template <typename> class Constructor,
              template <typename> class Destructor>
    CAMY_INLINE IAutoPtr<T, Constructor, Destructor>::~IAutoPtr()
    {
        Destructor<T>()(ptr);
    }

    template <typename T,
              template <typename> class Constructor,
              template <typename> class Destructor>
    CAMY_INLINE IAutoPtr<T, Constructor, Destructor>::IAutoPtr(TAutoPtr& other)
        : ptr(nullptr)
    {
        API::swap(ptr, other.ptr);
    }

    template <typename T,
              template <typename> class Constructor,
              template <typename> class Destructor>
    CAMY_INLINE IAutoPtr<T, Constructor, Destructor>::IAutoPtr(TAutoPtr&& other)
        : ptr(nullptr)
    {
        API::swap(ptr, other.ptr);
    }

    template <typename T,
              template <typename> class Constructor,
              template <typename> class Destructor>
    CAMY_INLINE IAutoPtr<T, Constructor, Destructor>::IAutoPtr(TPtr ptr)
        : ptr(ptr)
    {
    }

    template <typename T,
              template <typename> class Constructor,
              template <typename> class Destructor>
    CAMY_INLINE typename IAutoPtr<T, Constructor, Destructor>::TAutoPtr&
    IAutoPtr<T, Constructor, Destructor>::operator=(TAutoPtr& other)
    {
        Destructor<T>()(ptr);
        API::swap(ptr, other.ptr);
        return *this;
    }

    template <typename T,
              template <typename> class Constructor,
              template <typename> class Destructor>
    CAMY_INLINE typename IAutoPtr<T, Constructor, Destructor>::TAutoPtr&
    IAutoPtr<T, Constructor, Destructor>::operator=(TAutoPtr&& other)
    {
        Destructor<T>()(ptr);
        API::swap(ptr, other.ptr);
        return *this;
    }

    template <typename T,
              template <typename> class Constructor,
              template <typename> class Destructor>
    CAMY_INLINE typename IAutoPtr<T, Constructor, Destructor>::TAutoPtr&
    IAutoPtr<T, Constructor, Destructor>::operator=(TPtr new_ptr)
    {
        Destructor<T>()(ptr);
        ptr = new_ptr;
        return *this;
    }

    template <typename T>
    struct _AutoPtrRawConstructor
    {
        T* operator()(rsize n) { return (T*)API::allocate(CAMY_UALLOC(sizeof(T) * n)); }
    };

	template <typename T>
	struct _AutoPtrRawDestructor
	{
		void operator()(T*& ptr) { API::deallocate(ptr); }
	};

    template <typename T>
    struct _AutoPtrTypedConstructor
    {
        template <typename... Ts>
        T* operator()(Ts&&... args)
        {
            return API::tallocate<T>(CAMY_UALLOC1, std::forward<Ts>(args)...);
        }
    };

    template <typename T>
    struct _AutoPtrTypedDestructor
    {
        void operator()(T*& ptr) { API::tdeallocate(ptr); }
    };

    template <typename T>
    struct _AutoPtrArrayTypedConstructor
    {
        template <typename... Ts>
        T* operator()(rsize n, Ts&&... args)
        {
            return API::tallocate_array<T>(CAMY_UALLOC(n), std::forward<Ts>(args)...);
        }
    };

    template <typename T>
    struct _AutoPtrArrayTypedDestructor
    {
        void operator()(T*& ptr) { API::tdeallocate_array(ptr); }
    };

	template <typename T>
	using AutoPtrRaw = IAutoPtr<T, _AutoPtrRawConstructor, _AutoPtrRawDestructor>;

    template <typename T>
    using AutoPtrT = IAutoPtr<T, _AutoPtrTypedConstructor, _AutoPtrArrayTypedDestructor>;

    template <typename T>
    using AutoPtrTArray = IAutoPtr<T, _AutoPtrArrayTypedConstructor, _AutoPtrArrayTypedDestructor>;
}