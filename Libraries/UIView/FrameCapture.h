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

#include "./View.h"
#include <Ty/Base.h>
#include <Ty/Traits.h>
#include <Ty/Verify.h>
#include <Ty/Move.h>

namespace UIView {

template<typename>
struct FrameCapture;

template<typename Out, typename... In>
struct FrameCapture<Out(In...)> {
    constexpr FrameCapture() = default;
    constexpr FrameCapture(nullptr_t)
        : m_wrapper(nullptr)
    {
    }

    template<typename CallableType>
    constexpr FrameCapture(CallableType&& callable)
    requires((IsCallableWithArguments<CallableType, Out, In...> && !IsSame<RemoveCVReference<CallableType>, FrameCapture>) && is_trivially_destructible<CallableType>)
    {
        using WrapperType = CallableWrapper<CallableType>;
        m_wrapper = MUST(ViewBase::alloc<WrapperType>());
        new (m_wrapper) WrapperType(forward<CallableType>(callable));
    }

    template<typename CallableType>
    constexpr FrameCapture& operator=(CallableType&& callable)
    requires(IsCallableWithArguments<CallableType, Out, In...> && is_trivially_destructible<CallableType>)
    {
        using WrapperType = CallableWrapper<CallableType>;
        m_wrapper = MUST(ViewBase::alloc<WrapperType>());
        new (m_wrapper) WrapperType(forward<CallableType>(callable));
        return *this;
    }

    constexpr FrameCapture& operator=(nullptr_t)
    {
        m_wrapper = nullptr;
        return *this;
    }

    constexpr FrameCapture(FrameCapture&& other)
        : m_wrapper(other.m_wrapper)
    {
        other.m_wrapper = nullptr;
    }

    constexpr FrameCapture& operator=(FrameCapture&& other)
    {
        if (this != &other) {
            m_wrapper = other.m_wrapper;
            other.m_wrapper = nullptr;
        }
        return *this;
    }

    constexpr FrameCapture(FrameCapture const& other)
        : m_wrapper(other.m_wrapper)
    {
    }

    constexpr FrameCapture& operator=(FrameCapture const& other)
    {
        if (this != &other) {
            m_wrapper = other.m_wrapper;
        }
        return *this;
    }

    constexpr Out operator()(In... in) const
    {
        return m_wrapper->call(forward<In>(in)...);
    }

    explicit operator bool() const { return m_wrapper != nullptr; }

private:
    struct CallableWrapperBase {
        virtual Out call(In...) = 0;
    };

    template<typename CallableType>
    struct CallableWrapper : public CallableWrapperBase {
        explicit CallableWrapper(CallableType&& callable)
            : m_callable(move(callable))
        {
        }

        Out call(In... in) final override
        {
            return m_callable(forward<In>(in)...);
        }

    private:
        CallableType m_callable;
    };

    CallableWrapperBase* m_wrapper { nullptr };
};

}
