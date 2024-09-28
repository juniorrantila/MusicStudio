/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
 * Copyright (c) 2021, Gunnar Beutner <gbeutner@serenityos.org>
 * Copyright (c) 2018-2023, Andreas Kling <kling@serenityos.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "./Forward.h"
#include "./Base.h"
#include "./Traits.h"
#include "./Defer.h"
#include "./Verify.h"
#include "./Move.h"
#include "./BitCast.h"

namespace Ty {

template<typename>
struct SmallCapture;

template<typename Out, typename... In>
struct SmallCapture<Out(In...)> {
    SmallCapture(SmallCapture const&) = delete;
    SmallCapture& operator=(SmallCapture const&) = delete;

public:
    using FunctionType = Out(In...);
    using ReturnType = Out;

    constexpr static auto AccommodateExcessiveAlignmentRequirements = true;
    constexpr static usize ExcessiveAlignmentThreshold = 16;

    constexpr SmallCapture() = default;
    constexpr SmallCapture(nullptr_t)
    {
    }

    constexpr ~SmallCapture()
    {
        clear(false);
    }

    template<typename CallableType>
    constexpr SmallCapture(CallableType&& callable)
    requires((IsFunctionObject<CallableType> && IsCallableWithArguments<CallableType, Out, In...> && !IsSame<RemoveCVReference<CallableType>, SmallCapture>))
    {
        init_with_callable(forward<CallableType>(callable), CallableKind::FunctionObject);
    }

    template<typename FunctionType>
    SmallCapture(FunctionType f)
    requires((IsFunctionPointer<FunctionType> && IsCallableWithArguments<RemovePointer<FunctionType>, Out, In...> && !IsSame<RemoveCVReference<FunctionType>, SmallCapture>))
    {
        init_with_callable(move(f), CallableKind::FunctionPointer);
    }

    SmallCapture(SmallCapture&& other)
    {
        move_from(move(other));
    }

    // Note: Despite this method being const, a mutable lambda _may_ modify its own captures.
    Out operator()(In... in) const
    {
        auto* wrapper = callable_wrapper();
        VERIFY(wrapper);
        ++m_call_nesting_level;
        Defer guard([this] {
            if (--m_call_nesting_level == 0 && m_deferred_clear)
                const_cast<SmallCapture*>(this)->clear(false);
        });
        return wrapper->call(forward<In>(in)...);
    }

    explicit operator bool() const { return !!callable_wrapper(); }

    template<typename CallableType>
    SmallCapture& operator=(CallableType&& callable)
    requires((IsFunctionObject<CallableType> && IsCallableWithArguments<CallableType, Out, In...>))
    {
        clear();
        init_with_callable(forward<CallableType>(callable), CallableKind::FunctionObject);
        return *this;
    }

    template<typename FunctionType>
    SmallCapture& operator=(FunctionType f)
    requires((IsFunctionPointer<FunctionType> && IsCallableWithArguments<RemovePointer<FunctionType>, Out, In...>))
    {
        clear();
        if (f)
            init_with_callable(move(f), CallableKind::FunctionPointer);
        return *this;
    }

    SmallCapture& operator=(nullptr_t)
    {
        clear();
        return *this;
    }

    SmallCapture& operator=(SmallCapture&& other)
    {
        if (this != &other) {
            clear();
            move_from(move(other));
        }
        return *this;
    }

private:
    enum class CallableKind {
        FunctionPointer,
        FunctionObject,
    };

    class CallableWrapperBase {
    public:
        virtual ~CallableWrapperBase() = default;
        // Note: This is not const to allow storing mutable lambdas.
        virtual Out call(In...) = 0;
        virtual void destroy() = 0;
        virtual void init_and_swap(u8*, usize) = 0;
    };

    template<typename CallableType>
    class CallableWrapper final : public CallableWrapperBase {
        CallableWrapper(CallableWrapper&&) = delete;
        CallableWrapper& operator=(CallableWrapper&&) = delete;

        CallableWrapper(CallableWrapper const&) = delete;
        CallableWrapper& operator=(CallableWrapper const&) = delete;
    public:
        explicit CallableWrapper(CallableType&& callable)
            : m_callable(move(callable))
        {
        }

        Out call(In... in) final override
        {
            return m_callable(forward<In>(in)...);
        }

        void destroy() final override
        {
            delete this;
        }

        // NOLINTNEXTLINE(readability-non-const-parameter) False positive; destination is used in a placement new expression
        void init_and_swap(u8* destination, usize size) final override
        {
            VERIFY(size >= sizeof(CallableWrapper));
            new (destination) CallableWrapper { move(m_callable) };
        }

    private:
        CallableType m_callable;
    };

    enum class FunctionKind {
        NullPointer,
        Inline,
        Outline,
    };

    CallableWrapperBase* callable_wrapper() const
    {
        switch (m_kind) {
        case FunctionKind::NullPointer:
            return nullptr;
        case FunctionKind::Inline:
            return bit_cast<CallableWrapperBase*>(&m_storage);
        case FunctionKind::Outline:
            return *bit_cast<CallableWrapperBase**>(&m_storage);
        default:
            VERIFY(false);
        }
    }

    void clear(bool may_defer = true)
    {
        bool called_from_inside_function = m_call_nesting_level > 0;
        // NOTE: This VERIFY could fail because a Function is destroyed from within itself.
        VERIFY(may_defer || !called_from_inside_function);
        if (called_from_inside_function && may_defer) {
            m_deferred_clear = true;
            return;
        }
        m_deferred_clear = false;
        auto* wrapper = callable_wrapper();
        if (m_kind == FunctionKind::Inline) {
            VERIFY(wrapper);
            wrapper->~CallableWrapperBase();
        } else if (m_kind == FunctionKind::Outline) {
            VERIFY(wrapper);
            wrapper->destroy();
        }
        m_kind = FunctionKind::NullPointer;
    }

    template<typename Callable>
    void init_with_callable(Callable&& callable, CallableKind callable_kind)
    {
        if constexpr (alignof(Callable) > ExcessiveAlignmentThreshold && !AccommodateExcessiveAlignmentRequirements) {
            static_assert(
                alignof(Callable) <= ExcessiveAlignmentThreshold,
                "This callable object has a very large alignment requirement, "
                "check your capture list if it is a lambda expression, "
                "and make sure your callable object is not excessively aligned.");
        }
        VERIFY(m_call_nesting_level == 0);
        using WrapperType = CallableWrapper<Callable>;
        static_assert(sizeof(WrapperType) <= inline_capacity);
        new (m_storage) WrapperType(forward<Callable>(callable));
        m_kind = FunctionKind::Inline;
        if (callable_kind == CallableKind::FunctionObject)
            m_size = sizeof(WrapperType);
        else
            m_size = 0;
    }

    void move_from(SmallCapture&& other)
    {
        VERIFY(m_call_nesting_level == 0 && other.m_call_nesting_level == 0);
        auto* other_wrapper = other.callable_wrapper();
        m_size = other.m_size;
        switch (other.m_kind) {
        case FunctionKind::NullPointer:
            break;
        case FunctionKind::Inline:
            other_wrapper->init_and_swap(m_storage, inline_capacity);
            m_kind = FunctionKind::Inline;
            break;
        case FunctionKind::Outline:
            *bit_cast<CallableWrapperBase**>(&m_storage) = other_wrapper;
            m_kind = FunctionKind::Outline;
            break;
        default:
            VERIFY(false);
        }
        other.m_kind = FunctionKind::NullPointer;
    }

    usize m_size { 0 };
    FunctionKind m_kind { FunctionKind::NullPointer };
    bool m_deferred_clear { false };
    mutable u16 m_call_nesting_level { 0 };

    static constexpr usize inline_alignment = alignof(CallableWrapperBase) > alignof(CallableWrapperBase*) ? alignof(CallableWrapperBase) : alignof(CallableWrapperBase*);
    static constexpr usize inline_capacity = 6 * sizeof(void*);

    alignas(inline_alignment) u8 m_storage[inline_capacity];
};

}
