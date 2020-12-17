/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-12-16
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

template<typename T, size_t N>
struct Ringbuffer
{
    size_t Head{}, Tail{}, Size{};
    std::array<T, N> Storage{};

    Ringbuffer() = default;
    ~Ringbuffer() { clear(); }
    Ringbuffer(const Ringbuffer &Right) noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        Head = Right.Head; Tail = Right.Tail; Size = Right.Size;

        if constexpr (std::is_trivially_copyable_v<T>)
        {
            std::memcpy(Storage, Right.Storage, Right.Size * sizeof(T));
        }
        else
        {
            try
            {
                for (auto i = 0; i < Size; ++i)
                    new(Storage + ((Tail + i) % N)) T(Right[Tail + ((Tail + i) % N)]);
            }
            catch (...)
            {
                while (!empty())
                {
                    erase(Tail);
                    Tail = ++Tail % N;
                    --Size;
                }
                throw;
            }
        }
    }
    Ringbuffer &operator=(const Ringbuffer &Right) noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        // Sanity checking.
        if (this == &Right) return *this;

        clear();
        this = Ringbuffer(Right);
        return *this;
    }

    [[nodiscard]] bool empty() const noexcept { return Size == 0; }
    [[nodiscard]] bool full() const noexcept { return Size == N; }
    [[nodiscard]] size_t capacity() const noexcept { return N; }

    void clear() noexcept
    {
        if constexpr (std::is_trivially_destructible_v<T>)
        {
            while (!empty())
            {
                erase(Tail);
                Tail = ++Tail % N;
                --Size;
            }
        }
    }
    void erase(size_t Index) noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            reinterpret_cast<T *>(&Storage[Index])->~T();
    }

    void pop_front() noexcept
    {
        if (empty()) return;
        erase(Tail);

        Tail = ++Tail % N;
        --Size;
    }
    template<typename U> void push_back(U &&Value)
    {
        if (full()) erase(Head);

        Storage[Head] = std::move(Value);
        if (full()) Tail = ++Tail % N;
        if (!full()) ++Size;
        Head = ++Head % N;
    }

    [[nodiscard]] T &back() noexcept { return reinterpret_cast<T &>(Storage[std::clamp(Head, 0UL, N - 1)]); }
    [[nodiscard]] const T &front() const noexcept { return const_cast<Ringbuffer<T, N> *>(this)->front(); }
    [[nodiscard]] const T &back() const noexcept { return const_cast<Ringbuffer<T, N> *>(back)->back(); }
    [[nodiscard]] T &front() noexcept { return reinterpret_cast<T &>(Storage[Tail]); }

    [[nodiscard]] T &operator[](size_t Tndex) noexcept { return reinterpret_cast<T &>(Storage[Tndex]); }
    [[nodiscard]] const T &operator[](size_t Tndex) const noexcept { return const_cast<Ringbuffer<T, N> *>(this)->operator[](Tndex); }

    template<typename T, size_t N, bool C>
    struct Iterator
    {
        using Buffer_t = std::conditional_t<!C, Ringbuffer<T, N> *, Ringbuffer<T, N> const *>;
        using iterator_category = std::bidirectional_iterator_tag;
        using self_type = Iterator<T, N, C>;
        using difference_type = ptrdiff_t;
        using const_reference = T const &;
        using const_pointer = T const *;
        using size_type = size_t;
        using value_type = T;
        using reference = T &;
        using pointer = T *;

        Buffer_t Storage{};
        size_t Index, Count{};

        Iterator() noexcept = default;
        Iterator(Iterator const &) noexcept = default;
        Iterator &operator=(Iterator const &) noexcept = default;
        Iterator(Buffer_t source, size_t index, size_t count) noexcept : Storage{ source }, Index{ index }, Count{ count } { }

        template<bool Z = C, typename = std::enable_if<(!Z), int>>
        [[nodiscard]] T &operator*() noexcept { return (*Storage)[Index]; }
        template<bool Z = C, typename = std::enable_if<(Z), int>>
        [[nodiscard]] const T &operator*() const noexcept { return (*Storage)[Index]; }
        template<bool Z = C, typename = std::enable_if<(!Z), int>>
        [[nodiscard]] T &operator->() noexcept { return &((*Storage)[Index]); }
        template<bool Z = C, typename = std::enable_if<(Z), int>>
        [[nodiscard]] const T &operator->() const noexcept { return &((*Storage)[Index]); }

        bool operator!=(const Iterator &Right) const { return Index != Right.Index; };

        Iterator<T, N, C> &operator++() noexcept
        {
            Index = ++Index % N;
            ++Count;
            return *this;
        }
        Iterator<T, N, C> operator++(int) noexcept
        {
            auto result = *this;
            this->operator*();
            return result;
        }
        Iterator<T, N, C> &operator--() noexcept
        {
            Index = (Index + N - 1) % N;
            --Count;
            return *this;
        }
        Iterator<T, N, C> operator--(int) noexcept
        {
            auto result = *this;
            this->operator*();
            return result;
        }

        [[nodiscard]] size_t index() const noexcept { return Index; }
        [[nodiscard]] size_t count() const noexcept { return Count; }
        ~Iterator() = default;
    };

    using iterator = Iterator<T, N, false>;
    using const_iterator = Iterator<T, N, true>;
    using reverse_iterator = std::reverse_iterator<Iterator<T, N, false>>;
    using const_reverse_iterator = std::reverse_iterator<Iterator<T, N, true>>;
    using const_reference = T const &;
    using const_pointer = T const *;
    using size_type = size_t;
    using value_type = T;
    using reference = T &;
    using pointer = T *;

    [[nodiscard]] iterator begin() noexcept { return iterator{ this, Tail, 0 }; }
    [[nodiscard]] iterator end() noexcept { return iterator{ this, Head, Size }; }
    [[nodiscard]] const_iterator begin() const noexcept { return const_iterator{ this, Tail, 0 }; }
    [[nodiscard]] const_iterator end() const noexcept { return const_iterator{ this, Head, Size }; }
    [[nodiscard]] const_iterator cbegin() const noexcept { return const_iterator{ this, Tail, 0 }; }
    [[nodiscard]] const_iterator cend() const noexcept { return const_iterator{ this, Head, Size }; }

    [[nodiscard]] reverse_iterator rend() noexcept { return reverse_iterator{ iterator{this, Tail, 0} }; }
    [[nodiscard]] reverse_iterator rbegin() noexcept { return reverse_iterator{ iterator{this, Head, Size} }; }
    [[nodiscard]] const_reverse_iterator rend() const noexcept { return const_reverse_iterator{ const_iterator{this, Tail, 0} }; }
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator{ const_iterator{this, Head, Size} }; }
    [[nodiscard]] const_reverse_iterator crend() const noexcept { return const_reverse_iterator{ const_iterator{this, Tail, 0} }; }
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator{ const_iterator{this, Head, Size} }; }
};
