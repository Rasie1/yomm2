#include <iostream>
#include <type_traits>

#include <yorel/yomm2.hpp>
#include <yorel/yomm2/runtime.hpp>

#define BOOST_TEST_MODULE runtime
#include <boost/test/included/unit_test.hpp>

using namespace yorel::yomm2;
using namespace yorel::yomm2::detail;

std::ostream& operator<<(std::ostream& os, const rt_class* cls) {
    return os << cls->name();
}

std::string empty = "{}";

template<template<typename...> typename Container, typename T>
auto str(const Container<T>& container) {
    std::ostringstream os;
    os << "{";
    const char* sep = "";

    for (const auto& item : container) {
        os << sep << item;
        sep = ", ";
    }

    os << "}";

    return os.str();
}

template<typename... T>
auto str(T... args) {
    std::ostringstream os;
    os << "{";
    const char* sep = "";
    ((os << sep, os << args, sep = ", "), ...);
    return os.str();
}

template<typename... Ts>
auto sstr(Ts... args) {
    std::vector<rt_class*> vec{args...};
    std::sort(vec.begin(), vec.end());
    return str(vec);
}

template<typename T>
auto sstr(const std::unordered_set<T>& container) {
    return sstr(std::vector<T>(container.begin(), container.end()));
}

template<typename C, typename T>
bool contains(const C& coll, const T& value) {
    return std::find(coll.begin(), coll.end(), value) != coll.end();
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
    os << "{ ";
    const char* sep = "";
    for (auto& val : vec) {
        os << sep << val;
        sep = ", ";
    }
    os << " }";
    return os;
}

template<typename Key>
struct test_policy_ : policy::hash_factors_in_method {
    static struct catalog catalog;
    static struct context context;
};

template<typename Key>
catalog test_policy_<Key>::catalog;

template<typename Key>
context test_policy_<Key>::context;

template<typename T>
auto get(const runtime& rt) {
    return rt.class_map.at(typeid(T));
}

namespace ns_use_classes {

struct Animal {
    virtual ~Animal() {
    }
};

struct Herbivore : virtual Animal {};
struct Carnivore : virtual Animal {};
struct Cow : Herbivore {};
struct Wolf : Carnivore {};
struct Human : Carnivore, Herbivore {};

struct whole_hierarchy {};
use_classes<
    test_policy_<whole_hierarchy>, Animal, Herbivore, Carnivore, Cow, Wolf,
    Human>
    YOMM2_GENSYM;

struct incremental {};
use_classes<test_policy_<incremental>, Animal, Herbivore, Cow> YOMM2_GENSYM;
use_classes<test_policy_<incremental>, Animal, Carnivore, Wolf> YOMM2_GENSYM;
use_classes<test_policy_<incremental>, Herbivore, Carnivore, Human>
    YOMM2_GENSYM;

template<typename T>
auto get(const runtime& rt) {
    return rt.class_map.at(typeid(T));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    test_use_classes, Key, std::tuple<whole_hierarchy>) {
    runtime rt(test_policy_<Key>::catalog, test_policy_<Key>::context);
    rt.update();

    auto animal = get<Animal>(rt);
    auto herbivore = get<Herbivore>(rt);
    auto carnivore = get<Carnivore>(rt);
    auto cow = get<Cow>(rt);
    auto wolf = get<Wolf>(rt);
    auto human = get<Human>(rt);

    BOOST_TEST(animal->direct_bases.empty());
    BOOST_TEST(sstr(animal->direct_derived) == sstr(herbivore, carnivore));
    BOOST_TEST(sstr(herbivore->direct_bases) == sstr(animal));
    BOOST_TEST(sstr(herbivore->direct_derived) == sstr(cow, human));
    BOOST_TEST(sstr(carnivore->direct_bases) == sstr(animal));
    BOOST_TEST(sstr(carnivore->direct_derived) == sstr(wolf, human));
    BOOST_TEST(sstr(cow->direct_bases) == sstr(herbivore));
    BOOST_TEST(cow->direct_derived.empty());
    BOOST_TEST(sstr(wolf->direct_bases) == sstr(carnivore));
    BOOST_TEST(wolf->direct_derived.empty());
    BOOST_TEST(sstr(human->direct_bases) == sstr(carnivore, herbivore));
    BOOST_TEST(human->direct_derived.empty());
}
} // namespace ns_use_classes

namespace rolex {

struct Role {
    virtual ~Role() {
    }
};

struct Employee : Role {};
struct Manager : Employee {};
struct Founder : Role {};

struct Expense {
    virtual ~Expense() {
    }
};

struct Public : Expense {};
struct Bus : Public {};
struct Metro : Public {};
struct Taxi : Expense {};
struct Jet : Expense {};

using test_policy = test_policy_<Role>;
// any type from this namespace would work.

use_classes<
    test_policy, Role, Employee, Manager, Founder, Expense, Public, Bus, Metro,
    Taxi, Jet>
    YOMM2_GENSYM;

YOMM2_DECLARE(double, pay, (virtual_<const Employee&>), test_policy);

YOMM2_DECLARE(
    bool, approve, (virtual_<const Role&>, virtual_<const Expense&>, double),
    test_policy);

YOMM2_DEFINE(double, pay, (const Employee&)) {
    return 3000;
}

YOMM2_DEFINE(double, pay, (const Manager& exec)) {
    return next(exec) + 2000;
}

YOMM2_DEFINE(bool, approve, (const Role& r, const Expense& e, double amount)) {
    return false;
}

YOMM2_DEFINE(
    bool, approve, (const Employee& r, const Public& e, double amount)) {
    return true;
}

YOMM2_DEFINE(bool, approve, (const Manager& r, const Taxi& e, double amount)) {
    return true;
}

YOMM2_DEFINE(
    bool, approve, (const Founder& r, const Expense& e, double amount)) {
    return true;
}

inline const word* mptr(const context& t, const std::type_info* ti) {
    return t.hash_table[t.hash(ti)].pw;
}

BOOST_AUTO_TEST_CASE(runtime_test) {

    runtime rt(test_policy::catalog, test_policy::context);

    rt.augment_classes();

    BOOST_TEST_REQUIRE(
        rt.classes.size() == test_policy::catalog.classes.size());

    {
        auto c_iter = test_policy::catalog.classes.begin();
        auto r_iter = rt.classes.rbegin();

        while (r_iter != rt.classes.rend()) {
            BOOST_TEST(r_iter->info == &*c_iter);
            ++c_iter;
            ++r_iter;
        }
    }

    auto role = get<Role>(rt);
    auto employee = get<Employee>(rt);
    auto manager = get<Manager>(rt);
    auto founder = get<Founder>(rt);
    auto expense = get<Expense>(rt);
    auto public_ = get<Public>(rt);
    auto bus = get<Bus>(rt);
    auto metro = get<Metro>(rt);
    auto taxi = get<Taxi>(rt);
    auto jet = get<Jet>(rt);

    BOOST_TEST(sstr(role->direct_bases) == empty);
    BOOST_TEST(sstr(employee->direct_bases) == sstr(role));

    {
        std::vector<rt_class*> expected = {employee};
        BOOST_TEST(manager->direct_bases == expected);
    }

    {
        std::vector<rt_class*> expected = {role};
        BOOST_TEST(founder->direct_bases == expected);
    }

    {
        std::vector<rt_class*> expected = {expense};
        BOOST_TEST(public_->direct_bases == expected);
    }

    {
        std::vector<rt_class*> expected = {public_};
        BOOST_TEST(bus->direct_bases == expected);
    }

    {
        std::vector<rt_class*> expected = {public_};
        BOOST_TEST(metro->direct_bases == expected);
    }

    {
        std::vector<rt_class*> expected = {expense};
        BOOST_TEST(taxi->direct_bases == expected);
    }

    {
        std::vector<rt_class*> expected = {expense};
        BOOST_TEST(jet->direct_bases == expected);
    }

    {
        std::vector<rt_class*> expected = {employee, founder};
        BOOST_TEST(sstr(role->direct_derived) == sstr(expected));
    }

    {
        std::vector<rt_class*> expected = {manager};
        BOOST_TEST(sstr(employee->direct_derived) == sstr(expected));
    }

    {
        std::vector<rt_class*> expected = {public_, taxi, jet};
        BOOST_TEST(sstr(expense->direct_derived) == sstr(expected));
    }

    {
        std::vector<rt_class*> expected = {bus, metro};
        BOOST_TEST(sstr(public_->direct_derived) == sstr(expected));
    }

    BOOST_TEST(bus->direct_derived.size() == 0);
    BOOST_TEST(metro->direct_derived.size() == 0);
    BOOST_TEST(taxi->direct_derived.size() == 0);
    BOOST_TEST(jet->direct_derived.size() == 0);

    BOOST_TEST(
        sstr(role->covariant_classes) == sstr(role, employee, founder, manager));
    BOOST_TEST(sstr(founder->covariant_classes) == sstr(founder));
    BOOST_TEST(
        sstr(expense->covariant_classes) ==
        sstr(expense, public_, taxi, jet, bus, metro));
    BOOST_TEST(sstr(public_->covariant_classes) == sstr(public_, bus, metro));

    rt.augment_methods();

    BOOST_TEST_REQUIRE(rt.methods.size() == 2);
    auto method_iter = rt.methods.begin();
    rt_method& pay_method = *method_iter++;
    BOOST_TEST_REQUIRE(pay_method.vp.size() == 1);
    rt_method& approve_method = *method_iter++;
    BOOST_TEST_REQUIRE(approve_method.vp.size() == 2);

    {
        std::vector<rt_class*> expected = {employee};
        BOOST_TEST_INFO("result   = " + sstr(pay_method.vp));
        BOOST_TEST_INFO("expected = " + sstr(expected));
        BOOST_TEST(pay_method.vp == expected);
    }

    {
        std::vector<rt_class*> expected = {role, expense};
        BOOST_TEST_INFO("result   = " << sstr(approve_method.vp));
        BOOST_TEST_INFO("expected = " << sstr(expected));
        BOOST_TEST(approve_method.vp == expected);
    }

    {
        auto c_iter = test_policy::catalog.methods.begin();
        auto r_iter = rt.methods.rbegin();

        for (int i = 0; i < rt.methods.size(); ++i) {
            BOOST_TEST_INFO("i = " << i);
            auto& c_meth = *c_iter++;
            auto& r_meth = *r_iter++;
            BOOST_TEST_REQUIRE(r_meth.specs.size() == c_meth.specs.size());

            auto c_spec_iter = c_meth.specs.begin();
            auto r_spec_iter = r_meth.specs.begin();

            for (int j = 0; j < r_meth.specs.size(); ++j) {
                BOOST_TEST_INFO("i = " << i);
                BOOST_TEST_INFO("j = " << j);
                auto& c_spec = *c_spec_iter++;
                auto& r_spec = *r_spec_iter++;
                BOOST_TEST_REQUIRE(
                    r_spec.vp.size() == c_spec.vp_end - c_spec.vp_begin);
            }
        }
    }

    BOOST_TEST_REQUIRE(role->used_by_vp.size() == 1);
    BOOST_TEST(role->used_by_vp[0].method == &approve_method);
    BOOST_TEST(role->used_by_vp[0].param == 0);

    BOOST_TEST_REQUIRE(employee->used_by_vp.size() == 1);
    BOOST_TEST(employee->used_by_vp[0].method == &pay_method);
    BOOST_TEST(employee->used_by_vp[0].param == 0);

    BOOST_TEST_REQUIRE(expense->used_by_vp.size() == 1);
    BOOST_TEST(expense->used_by_vp[0].method == &approve_method);
    BOOST_TEST(expense->used_by_vp[0].param == 1);

    rt.allocate_slots();

    {
        const std::vector<int> expected = {1};
        BOOST_TEST(expected == pay_method.slots);
    }

    {
        const std::vector<int> expected = {0, 0};
        BOOST_TEST(expected == approve_method.slots);
    }

    BOOST_TEST_REQUIRE(role->mtbl.size() == 1);
    BOOST_TEST_REQUIRE(employee->mtbl.size() == 2);
    BOOST_TEST_REQUIRE(manager->mtbl.size() == 2);
    BOOST_TEST_REQUIRE(founder->mtbl.size() == 1);

    BOOST_TEST_REQUIRE(expense->mtbl.size() == 1);
    BOOST_TEST_REQUIRE(public_->mtbl.size() == 1);
    BOOST_TEST_REQUIRE(bus->mtbl.size() == 1);
    BOOST_TEST_REQUIRE(metro->mtbl.size() == 1);
    BOOST_TEST_REQUIRE(taxi->mtbl.size() == 1);
    BOOST_TEST_REQUIRE(jet->mtbl.size() == 1);

    auto pay_method_iter = pay_method.specs.begin();
    auto pay_Employee = pay_method_iter++;
    auto pay_Manager = pay_method_iter++;

    auto spec_iter = approve_method.specs.begin();
    auto approve_Role_Expense = spec_iter++;
    auto approve_Employee_public = spec_iter++;
    auto approve_Manager_Taxi = spec_iter++;
    auto approve_Founder_Expense = spec_iter++;

    {
        BOOST_TEST(runtime::is_more_specific(
            &*approve_Founder_Expense, &*approve_Role_Expense));
        BOOST_TEST(runtime::is_more_specific(
            &*approve_Manager_Taxi, &*approve_Role_Expense));
        BOOST_TEST(!runtime::is_more_specific(
            &*approve_Role_Expense, &*approve_Role_Expense));

        {
            std::vector<const rt_spec*> expected = {&*approve_Manager_Taxi};
            std::vector<const rt_spec*> specs = {
                &*approve_Role_Expense, &*approve_Manager_Taxi};
            BOOST_TEST(expected == runtime::best(specs));
        }
    }

    {
        BOOST_TEST(runtime::is_base(
            &*approve_Role_Expense, &*approve_Founder_Expense));
        BOOST_TEST(
            !runtime::is_base(&*approve_Role_Expense, &*approve_Role_Expense));
    }

    rt.build_dispatch_tables();

    {
        BOOST_TEST_REQUIRE(pay_method.dispatch_table.size() == 2);
        BOOST_TEST(pay_method.dispatch_table[0] == pay_Employee->info->pf);
        BOOST_TEST(pay_method.dispatch_table[1] == pay_Manager->info->pf);
    }

    {
        // expected dispatch table here:
        // https://www.codeproject.com/Articles/859492/Open-Multi-Methods-for-Cplusplus-Part-Inside-Yomm
        BOOST_TEST_REQUIRE(approve_method.dispatch_table.size() == 12);
        BOOST_TEST_REQUIRE(approve_method.strides.size() == 1);
        BOOST_TEST_REQUIRE(approve_method.strides[0] == 4);

        auto dp_iter = approve_method.dispatch_table.begin();
        BOOST_TEST(*dp_iter++ == approve_Role_Expense->info->pf);
        BOOST_TEST(*dp_iter++ == approve_Role_Expense->info->pf);
        BOOST_TEST(*dp_iter++ == approve_Role_Expense->info->pf);
        BOOST_TEST(*dp_iter++ == approve_Founder_Expense->info->pf);
        BOOST_TEST(*dp_iter++ == approve_Role_Expense->info->pf);
        BOOST_TEST(*dp_iter++ == approve_Employee_public->info->pf);
        BOOST_TEST(*dp_iter++ == approve_Employee_public->info->pf);
        BOOST_TEST(*dp_iter++ == approve_Founder_Expense->info->pf);
        BOOST_TEST(*dp_iter++ == approve_Role_Expense->info->pf);
        BOOST_TEST(*dp_iter++ == approve_Role_Expense->info->pf);
        BOOST_TEST(*dp_iter++ == approve_Manager_Taxi->info->pf);
        BOOST_TEST(*dp_iter++ == approve_Founder_Expense->info->pf);
    }

    {
        const std::vector<int> expected = {0};
        BOOST_TEST(expected == role->mtbl);
    }

    {
        const std::vector<int> expected = {1, 0};
        BOOST_TEST(expected == employee->mtbl);
    }

    {
        const std::vector<int> expected = {2, 1};
        BOOST_TEST(expected == manager->mtbl);
    }

    {
        const std::vector<int> expected = {3};
        BOOST_TEST(expected == founder->mtbl);
    }

    {
        const std::vector<int> expected = {0};
        BOOST_TEST(expected == expense->mtbl);
    }

    {
        const std::vector<int> expected = {1};
        BOOST_TEST(expected == public_->mtbl);
    }

    {
        const std::vector<int> expected = {1};
        BOOST_TEST(expected == bus->mtbl);
    }

    {
        const std::vector<int> expected = {1};
        BOOST_TEST(expected == metro->mtbl);
    }

    {
        const std::vector<int> expected = {2};
        BOOST_TEST(expected == taxi->mtbl);
    }

    {
        const std::vector<int> expected = {0};
        BOOST_TEST(expected == jet->mtbl);
    }

    BOOST_TEST_REQUIRE(pay_Employee->info->next != nullptr);
    BOOST_TEST_REQUIRE(pay_Manager->info->next != nullptr);
    BOOST_TEST(*pay_Manager->info->next == pay_Employee->info->pf);

    rt.find_hash_function(rt.classes, rt.ctx.hash, rt.metrics);
    rt.install_gv();

    {
        // pay
        // clang-format off
        BOOST_TEST_REQUIRE(test_policy::context.gv.size() ==
                           1       // ptr to control table
                           + rt.metrics.hash_table_size // mptr table
                           + rt.metrics.hash_table_size // control table
                           + 15    // approve: 3 slots and 12 cells for dispatch table
                           + 12);  // 3 mtbl of 2 cells for Roles + 6 mtbl of 1 cells for Expenses
        // clang-format on

        auto gv_iter =
            test_policy::context.hash_table + 2 * rt.metrics.hash_table_size;
        // no slots nor fun* for 1-method

        // approve
        BOOST_TEST(gv_iter++->i == 0); // slot for approve/0
        BOOST_TEST(gv_iter++->i == 0); // slot for approve/1
        BOOST_TEST(gv_iter++->i == 4); // stride for approve/1
        // 12 fun*
        auto approve_dispatch_table = gv_iter;
        BOOST_TEST(std::equal(
            approve_method.dispatch_table.begin(),
            approve_method.dispatch_table.end(), gv_iter,
            [](const void* pf, word w) { return w.pf == pf; }));
        gv_iter += approve_method.dispatch_table.size();

        // auto opt_iter = gv_iter;

        // // Role
        // BOOST_TEST(gv_iter++->i == 0); // approve/0
        // // Employee
        // BOOST_TEST(gv_iter++->i == 1); // approve/0
        // BOOST_TEST(gv_iter++->i == 0); // pay
        // // Manager
        // BOOST_TEST(gv_iter++->i == 2); // approve/0
        // BOOST_TEST(gv_iter++->i == 1); // pay
        // // owner
        // BOOST_TEST(gv_iter++->i == 3); // approve/0
        // // Expense
        // BOOST_TEST(gv_iter++->i == 0); // approve/1
        // // Public
        // BOOST_TEST(gv_iter++->i == 1); // approve/1
        // // Bus
        // BOOST_TEST(gv_iter++->i == 1); // approve/1
        // // Metro
        // BOOST_TEST(gv_iter++->i == 1); // approve/1
        // // Taxi
        // BOOST_TEST(gv_iter++->i == 2); // approve/1
        // // Plane
        // BOOST_TEST(gv_iter++->i == 0); // approve/1

        rt.optimize();

        // // Role
        // BOOST_TEST(opt_iter++->pw == 0 + approve_dispatch_table); //
        // approve/0
        // // Employee
        // BOOST_TEST(opt_iter++->pw == 1 + approve_dispatch_table); //
        // approve/0 BOOST_TEST(opt_iter++->pf == pay_Employee->info->pf); //
        // pay
        // // Manager
        // BOOST_TEST(opt_iter++->pw == 2 + approve_dispatch_table); //
        // approve/0 BOOST_TEST(opt_iter++->pf == pay_Manager->info->pf); // pay
        // // owner
        // BOOST_TEST(opt_iter++->pw == 3 + approve_dispatch_table); //
        // approve/0
        // // Expense
        // BOOST_TEST(opt_iter++->i == 0); // approve/1
        // // Public
        // BOOST_TEST(opt_iter++->i == 1); // approve/1
        // // Bus
        // BOOST_TEST(opt_iter++->i == 1); // approve/1
        // // Metro
        // BOOST_TEST(opt_iter++->i == 1); // approve/1
        // // Taxi
        // BOOST_TEST(opt_iter++->i == 2); // approve/1
        // // Plane
        // BOOST_TEST(opt_iter++->i == 0); // approve/1

        // BOOST_TEST(mptr(test_policy::context, &typeid(Role)) == role->mptr);
        // BOOST_TEST(
        //     mptr(test_policy::context, &typeid(Employee)) == employee->mptr);
        // BOOST_TEST(
        //     mptr(test_policy::context, &typeid(Manager)) == manager->mptr);
        // BOOST_TEST(
        //     mptr(test_policy::context, &typeid(Founder)) == founder->mptr);

        // BOOST_TEST(
        //     mptr(test_policy::context, &typeid(Expense)) == expense->mptr);
        // BOOST_TEST(
        //     mptr(test_policy::context, &typeid(Public)) == public_->mptr);
        // BOOST_TEST(mptr(test_policy::context, &typeid(Bus)) == bus->mptr);
        // BOOST_TEST(mptr(test_policy::context, &typeid(Metro)) ==
        // metro->mptr); BOOST_TEST(mptr(test_policy::context, &typeid(Taxi)) ==
        // taxi->mptr); BOOST_TEST(mptr(test_policy::context, &typeid(Jet)) ==
        // jet->mptr);

        {
            const Role& role = Role();
            const Employee& employee = Employee();
            const Employee& manager = Manager();
            const Role& founder = Founder();

            const Expense& expense = Expense();
            const Expense& public_transport = Public();
            const Expense& bus = Bus();
            const Expense& metro = Metro();
            const Expense& taxi = Taxi();
            const Expense& jet = Jet();

            const auto& pay_method =
                decltype(yOMM2_SELECTOR(pay)(Employee()))::fn;
            BOOST_TEST(pay_method.arity == 1);
            BOOST_TEST(pay_method.resolve(employee) == pay_Employee->info->pf);
            BOOST_TEST(&typeid(manager) == &typeid(Manager));
            BOOST_TEST(pay_method.resolve(manager) == pay_Manager->info->pf);

            using approve_method =
                decltype(yOMM2_SELECTOR(approve)(Role(), Expense(), 0.));
            BOOST_TEST(approve_method::fn.arity == 2);

            BOOST_TEST(
                approve_method::fn.resolve(role, expense, 0.) ==
                approve_Role_Expense->info->pf);

            {
                std::vector<const Role*> Roles = {
                    &role, &employee, &manager, &founder};

                std::vector<const Expense*> Expenses = {
                    &expense, &public_transport, &bus, &metro, &taxi, &jet};

                int i = 0;

                for (auto r : Roles) {
                    int j = 0;
                    for (auto e : Expenses) {
                        BOOST_TEST_INFO("i = " << i << " j = " << j);
                        auto expected = typeid(*r) == typeid(Founder)
                            ? approve_Founder_Expense
                            : typeid(*r) == typeid(Manager)
                            ? (typeid(*e) == typeid(Taxi) ? approve_Manager_Taxi
                                   : dynamic_cast<const Public*>(e)
                                   ? approve_Employee_public
                                   : approve_Role_Expense)
                            : typeid(*r) == typeid(Employee) &&
                                dynamic_cast<const Public*>(e)
                            ? approve_Employee_public
                            : approve_Role_Expense;
                        BOOST_TEST(
                            approve_method::fn.resolve(*r, *e, 0.) ==
                            expected->info->pf);
                        ++j;
                    }
                    ++i;
                }
            }

            BOOST_TEST(pay(employee) == 3000);
            BOOST_TEST(pay(manager) == 5000);
        }
    }
}
} // namespace rolex

namespace multiple_inheritance {

// A   B
//  \ / \
//  AB   D
//  |    |
//  C    E

struct A {
    virtual ~A() {
    }
};

struct B {
    virtual ~B() {
    }
};

struct AB : A, B {};

struct C : AB {};

struct D : B {};

struct E : D {};

struct key;
using test_policy = test_policy_<key>;

use_classes<test_policy, A, B, AB, C, D, E> YOMM2_GENSYM;

BOOST_AUTO_TEST_CASE(test_use_classes_mi) {
    std::vector<rt_class*> actual, expected;

    runtime rt(test_policy::catalog, test_policy::context);
    rt.augment_classes();

    auto a = get<A>(rt);
    auto b = get<B>(rt);
    auto ab = get<AB>(rt);
    auto c = get<C>(rt);
    auto d = get<D>(rt);
    auto e = get<E>(rt);

    // -----------------------------------------------------------------------
    // A
    BOOST_REQUIRE_EQUAL(sstr(a->direct_bases), empty);
    BOOST_REQUIRE_EQUAL(sstr(a->direct_derived), sstr(ab));
    BOOST_REQUIRE_EQUAL(sstr(a->covariant_classes), sstr(a, ab, c));

    // -----------------------------------------------------------------------
    // B
    BOOST_REQUIRE_EQUAL(sstr(b->direct_bases), empty);
    BOOST_REQUIRE_EQUAL(sstr(b->direct_derived), sstr(ab, d));
    BOOST_REQUIRE_EQUAL(sstr(b->covariant_classes), sstr(b, ab, c, d, e));

    // -----------------------------------------------------------------------
    // AB
    BOOST_REQUIRE_EQUAL(sstr(ab->direct_bases), sstr(a, b));
    BOOST_REQUIRE_EQUAL(sstr(ab->direct_derived), sstr(c));
    BOOST_REQUIRE_EQUAL(sstr(ab->covariant_classes), sstr(ab, c));

    // -----------------------------------------------------------------------
    // C
    BOOST_REQUIRE_EQUAL(sstr(c->direct_bases), sstr(ab));
    BOOST_REQUIRE_EQUAL(sstr(c->direct_derived), empty);
    BOOST_REQUIRE_EQUAL(sstr(c->covariant_classes), sstr(c));

    // -----------------------------------------------------------------------
    // D
    BOOST_REQUIRE_EQUAL(sstr(d->direct_bases), sstr(b));
    BOOST_REQUIRE_EQUAL(sstr(d->direct_derived), sstr(e));
    BOOST_REQUIRE_EQUAL(sstr(d->covariant_classes), sstr(d, e));

    // -----------------------------------------------------------------------
    // E
    BOOST_REQUIRE_EQUAL(sstr(e->direct_bases), sstr(d));
    BOOST_REQUIRE_EQUAL(sstr(e->direct_derived), empty);
    BOOST_REQUIRE_EQUAL(sstr(e->covariant_classes), sstr(e));
}

method<key, void(virtual_<A&>), test_policy> m_a("m_a");
method<key, void(virtual_<B&>), test_policy> m_b("m_b");
method<key, void(virtual_<A&>, virtual_<B&>), test_policy> m_ab("m_ab");
method<key, void(virtual_<C&>), test_policy> m_c("m_c");
method<key, void(virtual_<D&>), test_policy> m_d("m_d");

BOOST_AUTO_TEST_CASE(test_allocate_slots_mi) {
    runtime rt(test_policy::catalog, test_policy::context);
    rt.augment_classes();
    rt.augment_methods();
    rt.allocate_slots();

    auto m_iter = rt.methods.begin();
    auto m_a = m_iter++;
    auto m_b = m_iter++;
    auto m_ab = m_iter++;
    auto m_c = m_iter++;
    auto m_d = m_iter++;

    // lattice:
    // A   B
    //  \ / \
    //  AB   D
    //  |    |
    //  C    E

    // 1-methods use one slot:
    BOOST_TEST(m_a->slots.size() == 1);
    BOOST_TEST(m_b->slots.size() == 1);
    BOOST_TEST(m_c->slots.size() == 1);
    BOOST_TEST(m_d->slots.size() == 1);

    // 2-method uses two slots
    BOOST_TEST(m_ab->slots.size() == 2);
    // two *different* slots
    BOOST_TEST(m_ab->slots[0] != m_ab->slots[1]);

    // m_c and m_d use the same slot
    BOOST_TEST(str(m_c->slots) == str(m_d->slots));

    // check that no two methods methods (except m_d) use a same slot

    decltype(m_a) methods[] = {m_a, m_b, m_ab, m_c};

    for (auto m1 : methods) {
        std::vector<bool> in_use(5); // in total methods use 5 slots

        for (auto s1 : m1->slots) {
            BOOST_REQUIRE(s1 <= 4);
            in_use[s1] = true;
        }
            
        for (auto m2 : methods) {
            if (m1 == m2) {
                continue;
            }

            for (auto s2 : m2->slots) {
                BOOST_REQUIRE(s2 <= 4);
                BOOST_TEST(!in_use[s2]);
            }
        }
    }
}

} // namespace multiple_inheritance
