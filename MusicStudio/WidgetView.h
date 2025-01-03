#pragma once
#include <Ty/ArenaAllocator.h>


template <typename>
struct Task;

template <typename Out, typename... In>
struct Task<Out(In...)> {
    Task() = default;

    template<typename CallableType>
    Task(ArenaAllocator& arena, CallableType&& callable)
    requires((IsCallableWithArguments<CallableType, Out, In...> && !IsSame<RemoveCVReference<CallableType>, Task>) && is_trivially_destructible<CallableType>)
    {
        using WrapperType = CallableWrapper<CallableType>;
        m_arena = &arena;
        m_wrapper = MUST(arena.alloc<WrapperType>());
        new (m_wrapper) WrapperType(forward<CallableType>(callable));
    }

    Out operator()(In... in) const
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

    ArenaAllocator* m_arena { nullptr };
    CallableWrapperBase* m_wrapper { nullptr };
};

struct RefBase {
    RefBase(ArenaAllocator& allocator)
        : m_allocator(allocator)
    {
    }

    ArenaAllocator& allocator()
    {
        return m_allocator;
    }

private:
    ArenaAllocator& m_allocator;
};

template <typename T>
struct Ref : RefBase {
    Ref(ArenaAllocator& allocator)
        : RefBase(allocator)
    {
    }

    static T* make(ArenaAllocator& allocator)
    {
        auto slot = MUST(allocator.alloc<T>());
        return new (slot)T(allocator);
    }
};

struct NodeBase : RefBase {
    NodeBase(ArenaAllocator& allocator)
        : RefBase(allocator)
    {
    }

    virtual StringView type_name() const = 0;

    template <typename T>
    T* cast()
    {
        auto test = new (MUST(allocator().alloc<T>()))T(allocator());
        VERIFY(test->type_name() == type_name());
        allocator().free(test);
        return (T*)this;
    }

    template <typename F>
        requires (IsCallableWithArguments<F, void, NodeBase*>)
    NodeBase* with_on_click(F&& callback) {
        m_on_click = Task<void(NodeBase*)>(allocator(), move(callback));
        return this;
    }

    template <typename F>
        requires (IsCallableWithArguments<F, void>)
    NodeBase* with_on_click(F&& callback) {
        return with_on_click([=](NodeBase*){
            callback();
        });
    }

    void click()
    {
        if (m_on_click) {
            m_on_click(this);
        }
    }

protected:
    Task<void(NodeBase*)> m_on_click {};
};

using NodeRef = NodeBase*;
using Nodes = View<NodeRef>;

template <typename T>
struct Node : NodeBase {
    Node(ArenaAllocator& allocator)
        : NodeBase(allocator)
    {
    }

    static T* make(ArenaAllocator& allocator)
    {
        auto slot = MUST(allocator.alloc<T>());
        return new (slot)T(allocator);
    }
};

struct Label : Node<Label> {
    Label(ArenaAllocator& allocator)
        : Node(allocator)
    {
    }

    StringView type_name() const override { return "Label"sv; }

    Label* with_text(StringView text)
    {
        auto chars = MUST(allocator().alloc<char>(text.size()));
        for (usize i = 0; i < text.size(); i++) {
            chars[i] = text[i];
        }
        m_text = chars;
        return this;
    }

    StringView text() const { return m_text; }

private:
    StringView m_text {};
};

struct List : Node<List> {
    List(ArenaAllocator& allocator)
        : Node(allocator)
    {
    }

    StringView type_name() const override { return "List"sv; }

    template <usize Size>
    List* with_nodes(NodeRef (&&items)[Size])
    {
        return with_nodes({items, Size});
    }

    Nodes nodes() { return m_nodes; }

private:
    List* with_nodes(Nodes items)
    {
        m_nodes = MUST(allocator().alloc<NodeRef>(items.size()));
        m_nodes.assign_from(items);
        return this;
    }

    Nodes m_nodes {};
};

struct Pool {
    Pool(ArenaAllocator& allocator)
        : m_allocator(allocator)
    {
    }

    ArenaAllocator& allocator() { return m_allocator; }

    template <typename T>
    T* custom() { return T::make(allocator()); }

    List* list() { return custom<List>(); }
    Label* label() { return custom<Label>(); }

private:
    ArenaAllocator& m_allocator;
};

