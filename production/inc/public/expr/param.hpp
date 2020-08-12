#pragma once

template <typename T, bool>
struct parameter_helper;

template <typename T>
struct parameter_helper<T, true> {
    typedef T type;
};

template <typename T>
struct parameter_helper<T, false> {
    typedef const T& type;
};

template <typename T>
struct parameter {
    typedef typename parameter_helper<T, sizeof(T) <= sizeof(void*)>::type type;
};

template<typename T>
using paramtyp = typename parameter<T>::type;
