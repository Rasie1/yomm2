// Copyright (c) 2021 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <yorel/yomm2/cute.hpp>

#include <iostream>
#include <memory>
#include <string>

using yorel::yomm2::virtual_;
using std::cout;

// register_class(classes);

// declare_method(return, name, (params));

// define_method(return, name, (params)) {
// }

int main() {
    yorel::yomm2::update_methods();
    return 0;
}
