// ----------------------------------------------------------------------------
//  AOgmaNeo
//  Copyright(c) 2020 Ogma Intelligent Systems Corp. All rights reserved.
//
//  This copy of AOgmaNeo is licensed to you under the terms described
//  in the AOGMANEO_LICENSE.md file included in this distribution.
// ----------------------------------------------------------------------------

#pragma once

#include "SparseMatrix.h"
#include "Ptr.h"

namespace ogmaneo {
const int expIters = 10;
const float expFactorials[] = { 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800 };

float expf(float x);

template <typename T>
T min(
    T left,
    T right
) {
    if (left < right)
        return left;
    
    return right;
}

template <typename T>
T max(
    T left,
    T right
) {
    if (left > right)
        return left;
    
    return right;
}

// Vector types
template <typename T> 
struct Vec2 {
    T x, y;

    Vec2() {}

    Vec2(
        T x,
        T y
    )
    : x(x), y(y)
    {}
};

template <typename T> 
struct Vec3 {
    T x, y, z;
    T pad;

    Vec3()
    {}

    Vec3(
        T x,
        T y,
        T z
    )
    : x(x), y(y), z(z)
    {}
};

template <typename T> 
struct Vec4 {
    T x, y, z, w;

    Vec4()
    {}

    Vec4(
        T x,
        T y,
        T z,
        T w
    )
    : x(x), y(y), z(z), w(w)
    {}
};

// Some basic definitions
typedef Vec2<int> Int2;
typedef Vec3<int> Int3;
typedef Vec4<int> Int4;
typedef Vec2<float> Float2;
typedef Vec3<float> Float3;
typedef Vec4<float> Float4;

typedef Array<int> IntBuffer;
typedef Array<float> FloatBuffer;

// --- Circular Buffer ---

template <typename T> 
struct CircleBuffer {
    Array<T> data;
    int start;

    CircleBuffer()
    :
    start(0)
    {}

    void resize(
        int size
    ) {
        data.resize(size);
    }

    void pushFront() {
        start--;

        if (start < 0)
            start += data.size();
    }

    T &front() {
        return data[start];
    }

    const T &front() const {
        return data[start];
    }

    T &back() {
        return data[(start + data.size() - 1) % data.size()];
    }

    const T &back() const {
        return data[(start + data.size() - 1) % data.size()];
    }

    T &operator[](int index) {
        return data[(start + index) % data.size()];
    }

    const T &operator[](int index) const {
        return data[(start + index) % data.size()];
    }

    int size() const {
        return data.size();
    }
};

// --- Basic Kernels ---

void copyInt(
    const IntBuffer* src, // Source buffer
    IntBuffer* dst // Destination buffer
);

void copyFloat(
    const FloatBuffer* src, // Source buffer
    FloatBuffer* dst // Destination buffer
);

// --- Bounds ---

// Bounds check from (0, 0) to upperBound
inline bool inBounds0(
    const Int2 &pos, // Position
    const Int2 &upperBound // Bottom-right corner
) {
    return pos.x >= 0 && pos.x < upperBound.x && pos.y >= 0 && pos.y < upperBound.y;
}

// Bounds check in range
inline bool inBounds(
    const Int2 &pos, // Position
    const Int2 &lowerBound, // Top-left corner
    const Int2 &upperBound // Bottom-right corner
) {
    return pos.x >= lowerBound.x && pos.x < upperBound.x && pos.y >= lowerBound.y && pos.y < upperBound.y;
}

// --- Projections ---

inline Int2 project(
    const Int2 &pos, // Position
    const Float2 &toScalars // Ratio of sizes
) {
    return Int2(pos.x * toScalars.x + 0.5f, pos.y * toScalars.y + 0.5f);
}

inline Int2 projectf(
    const Float2 &pos, // Position
    const Float2 &toScalars // Ratio of sizes
) {
    return Int2(pos.x * toScalars.x + 0.5f, pos.y * toScalars.y + 0.5f);
}

// --- Addressing ---

// Row-major
inline int address2(
    const Int2 &pos, // Position
    const Int2 &dims // Dimensions to ravel with
) {
    return pos.y + pos.x * dims.y;
}

inline int address3(
    const Int3 &pos, // Position
    const Int3 &dims // Dimensions to ravel with
) {
    return pos.z + pos.y * dims.z + pos.x * dims.z * dims.y;
}

inline int address4(
    const Int4 &pos, // Position
    const Int4 &dims // Dimensions to ravel with
) {
    return pos.w + pos.z * dims.w + pos.y * dims.w * dims.z + pos.x * dims.w * dims.z * dims.y;
}

// --- Getters ---

Array<IntBuffer*> get(
    Array<IntBuffer> &v
);

Array<FloatBuffer*> get(
    Array<FloatBuffer> &v
);

Array<const IntBuffer*> constGet(
    const Array<IntBuffer> &v
);

Array<const FloatBuffer*> constGet(
    const Array<FloatBuffer> &v
);

Array<IntBuffer*> get(
    CircleBuffer<IntBuffer> &v
);

Array<FloatBuffer*> get(
    CircleBuffer<FloatBuffer> &v
);

Array<const IntBuffer*> constGet(
    const CircleBuffer<IntBuffer> &v
);

Array<const FloatBuffer*> constGet(
    const CircleBuffer<FloatBuffer> &v
);

// --- Noninearities ---

inline float sigmoid(
    float x
) {
    if (x < 0.0f) {
        float z = expf(x);

        return z / (1.0f + z);
    }
    
    return 1.0f / (1.0f + expf(-x));
}

// --- RNG ---

// From http://cas.ee.ic.ac.uk/people/dt10/research/rngs-gpu-mwc64x.html

extern unsigned long seed;

inline unsigned int MWC64X(unsigned long* state) {
    unsigned int c = (*state) >> 32, x= (*state) & 0xFFFFFFFF;

    *state = x*((unsigned long)4294883355U) + c;

    return x^c;
}

int rand();
float randf();
float randf(float low, float high);

// --- Sparse Matrix Generation ---

// Sparse matrix init
void initSMLocalRF(
    const Int3 &inSize, // Size of input field
    const Int3 &outSize, // Size of output field
    int radius, // Radius of output onto input
    SparseMatrix &mat // Matrix to fill
);
} // namespace ogmaneo