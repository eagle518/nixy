/*
    The Nixy Library
    Code covered by the MIT License

    Author: mutouyun (http://darkc.at)
*/

#pragma once

#include "finalizer/scopeguard.h"

#include "memory/alloc.h"
#include "thread/threadmodel.h"
#include "bugfix/assert.h"

#include "general/general.h"
#include "typemanip/typemanip.h"
#include "utility/utility.h"
#include "algorithm/algorithm.h"

//////////////////////////////////////////////////////////////////////////
NX_BEG
//////////////////////////////////////////////////////////////////////////

template <class Alloc_ = NX_DEFAULT_ALLOC, class Model_ = NX_DEFAULT_THREAD_MODEL>
class RefBase : nx_operator(typename NX_SHIELD(RefBase<Alloc_, Model_>), Unequal)
{
public:
    typedef typename Model_::template atomic<ulong>::type_t ref_t;

protected:
    ref_t* ref_;
    scope_guard* guard_;

public:
    RefBase(void)
        : ref_(nx::nulptr), guard_(nx::nulptr)
    {}

public:
    friend bool operator==(const RefBase& r1, const RefBase& r2)
    {
        return (r1.ref_ == r2.ref_);
    }

    void init(const nx::functor<void()>& guard)
    {
        dec();
        ref_   = nx::alloc<Alloc_, ref_t>(1);
        nx_assert(ref_);
        guard_ = nx::alloc<Alloc_, scope_guard>(nx::ref(guard));
        nx_assert(guard_);
    }

    bool set(const RefBase& r)
    {
        if ((*this) == r) return true;
        dec();
        return inc(r);
    }

    bool inc(const RefBase& r)
    {
        ref_ = r.ref_;
        if (!ref_) return false;
        ++ (*ref_);
        guard_ = r.guard_;
        return true;
    }

    void dec(void)
    {
        if (!ref_ || (*ref_) == 0) return;
        if (--(*ref_)) return;
        // free the ref counter
        nx::free<Alloc_>(ref_);
        ref_ = nx::nulptr;
        // call and free guard
        nx::free<Alloc_>(guard_);
        guard_ = nx::nulptr;
    }

    ulong ref(void)
    {
        return (ref_ ? (*ref_) : 0);
    }

    void swap(RefBase& rhs)
    {
        nx::swap(ref_  , rhs.ref_);
        nx::swap(guard_, rhs.guard_);
    }
};

/*
    The reference counter
*/

template <typename P>
class RefCounter : public P
{
public:
    RefCounter(void) {}

    template <typename T>
    RefCounter(const T& r)
    {
        P::set(r);
    }

    template <typename T, typename F>
    RefCounter(const T& r, const F& f)
    {
        P::set(r, f);
    }

    ~RefCounter() { P::dec(); }

public:
    template <typename T>
    RefCounter& operator=(const T& r)
    {
        P::set(r);
        return (*this);
    }
};

//////////////////////////////////////////////////////////////////////////
NX_END
//////////////////////////////////////////////////////////////////////////
