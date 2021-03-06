/*
    The Nixy Library
    Code covered by the MIT License

    Author: mutouyun (http://darkc.at)
*/

#pragma once

#include "nixycore/memory/construct.h"

#include "nixycore/general/general.h"
#include "nixycore/preprocessor/preprocessor.h"
#include "nixycore/typemanip/typemanip.h"

//////////////////////////////////////////////////////////////////////////
NX_BEG
//////////////////////////////////////////////////////////////////////////

/*
    construct alloc
*/

namespace private_alloc
{
#if defined(NX_CC_MSVC)
#   pragma warning(push)
    /*
        behavior change: an object of POD type constructed 
        with an initializer of the form () will be default-initialized
    */
#   pragma warning(disable: 4345)
#endif

    template <typename AllocT, typename T>
    struct detail
    {
        static T* alloc(void)
        {
            return nx_construct(AllocT::alloc(sizeof(T)), T);
        }

#   define NX_ALLOC_(n) \
        template <NX_PP_TYPE_1(n, typename P)> \
        static T* alloc(NX_PP_TYPE_2(n, P, par)) \
        { \
            return nx_construct(AllocT::alloc(sizeof(T)), T, NX_PP_TYPE_1(n, par)); \
        }
        NX_PP_MULT_MAX(NX_ALLOC_)
#   undef NX_ALLOC_
    };

    /* Make array to array pointer */
    template <typename AllocT, typename T_, size_t N>
    struct detail<AllocT, T_[N]>
    {
        typedef T_ T[N];
        static T* alloc(void)
        {
            T* p = (T*)AllocT::alloc(sizeof(T));
            nx_construct_arr(*p, T_, N);
            return p;
        }

#   define NX_ALLOC_(n) \
        template <NX_PP_TYPE_1(n, typename P)> \
        static T* alloc(NX_PP_TYPE_2(n, P, par)) \
        { \
            T* p = (T*)AllocT::alloc(sizeof(T)); \
            nx_construct_arr(*p, T_, N, NX_PP_TYPE_1(n, par)); \
            return p; \
        }
        NX_PP_MULT_MAX(NX_ALLOC_)
#   undef NX_ALLOC_
    };

    template <typename AllocT>
    struct detail<AllocT, void>
    {
        static void* alloc(size_t size)
        {
            return AllocT::alloc(size);
        }
    };

#if defined(NX_CC_MSVC)
#   pragma warning(pop)
#endif
}

template <typename AllocT>
inline pvoid alloc(size_t size)
{
    return AllocT::alloc(size);
}

template <typename AllocT, typename T>
inline T* alloc(void)
{
    return private_alloc::detail<AllocT, T>::alloc();
}

#define NX_ALLOC_(n) \
template <typename AllocT, typename T, NX_PP_TYPE_1(n, typename P)> \
inline T* alloc(NX_PP_TYPE_2(n, P, par)) \
{ \
    return private_alloc::detail<AllocT, T>::alloc(NX_PP_TYPE_1(n, par)); \
}
NX_PP_MULT_MAX(NX_ALLOC_)
#undef NX_ALLOC_

/*
    destruct free
*/

template <typename AllocT>
inline void free(pvoid p, size_t size)
{
    AllocT::free(p, size);
}

// make a concept to check size_of function

NX_CONCEPT(size_of_normal, size_t, size_of, )
NX_CONCEPT(size_of_const , size_t, size_of, const)

// has no virtual-destructor or size_of function

template <typename AllocT, typename T>
inline typename nx::enable_if<!nx::has_virtual_destructor<T>::value ||
                             (!nx::has_size_of_normal<T>::value &&
                              !nx::has_size_of_const<T>::value)
>::type_t free(T* p)
{
    if (!p) return;
    nx_destruct(p, T);
    AllocT::free(p, sizeof(T));
}

// has virtual-destructor and size_of function

template <typename AllocT, typename T>
inline typename nx::enable_if<nx::has_virtual_destructor<T>::value &&
                             (nx::has_size_of_normal<T>::value ||
                              nx::has_size_of_const<T>::value)
>::type_t free(T* p)
{
    if (!p) return;
    size_t s = p->size_of();
    nx_destruct(p, T);
    AllocT::free(p, s);
}

// array

template <typename AllocT, typename T, size_t N>
inline void free(T(* p)[N])
{
    if (!p) return;
    nx_destruct_arr(*p, T, N);
    AllocT::free(p, sizeof(T[N]));
}

/*
    realloc
*/

template <typename AllocT>
inline pvoid realloc(pvoid p, size_t old_size, size_t new_size)
{
    return AllocT::realloc(p, old_size, new_size);
}

// has no virtual-destructor or size_of function

template <typename AllocT, typename T>
inline typename nx::enable_if<!nx::has_virtual_destructor<T>::value ||
                             (!nx::has_size_of_normal<T>::value &&
                              !nx::has_size_of_const<T>::value),
pvoid>::type_t realloc(T* p, size_t size)
{
    if (p) nx_destruct(p, T);
    return AllocT::realloc(p, sizeof(T), size);
}

// has virtual-destructor and size_of function

template <typename AllocT, typename T>
inline typename nx::enable_if<nx::has_virtual_destructor<T>::value &&
                             (nx::has_size_of_normal<T>::value ||
                              nx::has_size_of_const<T>::value),
pvoid>::type_t realloc(T* p, size_t size)
{
    size_t s = 0;
    if (p)
    {
        s = p->size_of();
        nx_destruct(p, T);
    }
    return AllocT::realloc(p, s, size);
}

//////////////////////////////////////////////////////////////////////////
NX_END
//////////////////////////////////////////////////////////////////////////
