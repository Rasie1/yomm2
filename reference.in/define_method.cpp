// target: YOMM2_DEFINE
// md<
// <sub>/ ->home / ->reference </sub>
// ## define_method
// <sub>defined in <yorel/yomm2/cute.hpp>, also provided by <yorel/yomm2/keywords.hpp></sub>
// ```
// #define_method(/*unspecified*/) /*unspecified*/
// ```
// ### usage
// ```
// define_method(return-type, name, (function-parameter-list)) {
//     ...
// }

// define_method(container, return-type, name, (function-parameter-list)) {
//     ...
// }
// ```
// ---
// Add a definition to a method.

// Locate a method with the same name, with a signature compatible with
// `function-parameter-list`, and add the definition to the method's list of
// definitions. The method must exist and must be unique. `return-type` must be
// covariant with the method's return type. `return-type` may be `auto`.

// The types of the arguments are _not_ marked with `virtual_`.

// Inside the block, a function pointer named `next` points to the next most
// specific definition, if one exists, and it is unique. Otherwise, `next` points
// to an error handler, which writes a message to `std::cerr`, and terminates the
// program via `abort`. The handler can be customized. See
// ->set_method_call_error_handler.


// See the documentation of ->declare_method for information on handling types that
// contain commas.

// If `container` is specified, the method definition is placed inside the
// container, which must have been declared with `method_container`. See the
// documentation of ->`method_container` for more information on method containers.

// ## example
// >

#define BOOST_TEST_MODULE yomm2
#include <boost/test/included/unit_test.hpp>

// code<
#include <string>
#include <yorel/yomm2/keywords.hpp>

struct Animal { virtual ~Animal() {} };
struct Dog : Animal {};
struct Bulldog : Dog {};

register_classes(Animal, Dog, Bulldog);

declare_method(std::string, kick, (virtual_<Animal*>));

define_method(std::string, kick, (Dog* dog)) {
    return "bark";
}

define_method(std::string, kick, (Bulldog* dog)) {
    return next(dog) + " and bite";
}

BOOST_AUTO_TEST_CASE(example) {
    yorel::yomm2::update_methods();

    Dog snoopy;
    Bulldog hector;
    Animal* animal;

    animal = &snoopy;
    BOOST_TEST(kick(animal) == "bark");

    animal = &hector;
    BOOST_TEST(kick(animal) == "bark and bite");
}
// >
