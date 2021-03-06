// ----------------------------------------------------------------------------
//  AOgmaNeo
//  Copyright(c) 2020 Ogma Intelligent Systems Corp. All rights reserved.
//
//  This copy of AOgmaNeo is licensed to you under the terms described
//  in the AOGMANEO_LICENSE.md file included in this distribution.
// ----------------------------------------------------------------------------

#pragma once

namespace ogmaneo {
template<class T>
class Array {
private:
    T* p;
    int s;

public:
    Array()
    :
    p(nullptr),
    s(0)
    {}

    Array(
        const Array<T> &other
    )
    :
    p(nullptr),
    s(0)
    {
        *this = other;
    }

    Array(
        int size
    )
    :
    p(nullptr),
    s(0)
    {
        resize(size);
    }

    Array(
        int size,
        T value
    )
    :
    p(nullptr),
    s(0)
    {
        resize(size, value);
    }

    ~Array() {
        if (p != nullptr)
            delete[] p;
    }

    Array<T> &operator=(
        const Array<T> &other
    ) {
        if (s != other.s) {
            if (p != nullptr)
                delete[] p;

            s = other.s;

            p = new T[s];
        }

        for (int i = 0; i < s; i++)
            p[i] = other.p[i];
        
        return *this;
    }

    void resize(
        int size
    ) {
        if (s == size)
            return;

        T* temp = p;
        int oldSize = s;
        
        s = size;

        if (s != 0) {
            p = new T[s];

            int ms = oldSize < s ? oldSize : s;

            for (int i = 0; i < ms; i++)
                p[i] = temp[i];
        }
        else
            p = nullptr;

        if (temp != nullptr)
            delete[] temp;
    }

    void resize(
        int size,
        T value
    ) {
        int oldS = s;

        resize(size);

        for (int i = oldS; i < s; i++)
            p[i] = value;
    }

    T &operator[](
        int index
    ) {
        return p[index];
    }

    const T &operator[](
        int index
    ) const {
        return p[index];
    }

    int size() const {
        return s;
    }
};
} // namespace ogmaneo