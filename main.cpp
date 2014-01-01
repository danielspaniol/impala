#include <iostream>

#include "TypeTable.h"

using namespace std;

void simple_tests() {
    TypeTable tt;

    /*
    // create some test types
    Type* t1 = tt.tupletype( { tt.type_int(), tt.type_bool() }); // tuple(int, bool)
    Type* t2 = tt.tupletype( { tt.type_float(), tt.type_float(), t1 }); // tuple(float, float, tuple(int, bool))
    Type* t3 = tt.tupletype( { tt.type_float(), tt.type_float(), t1 }); // tuple(float, float, tuple(int, bool))

    // dump those types
    t1->dump();
    t2->dump();
    t3->dump();

    // check for equality
    std::cout << (t1 == t2) << std::endl; // 0
    std::cout << (t1 == t3) << std::endl; // 0
    std::cout << (t2 == t3) << std::endl; // 1*/

    // create an fn<A,B>(A, B)
    TypeVar* A = tt.typevar();
    TypeVar* B = tt.typevar();
    FnType* f = tt.fntype({A, B});
    f->add_bound_var(A);
    f->add_bound_var(B);
    tt.unify(f);
    f->dump();

    // create an fn<C:Clonable+Equality, D>(C)
    const TypeTrait* clonable = tt.typetrait(std::string("Clonable"));
    const TypeTrait* eq = tt.typetrait(std::string("Equality"));

    TypeVar* C = tt.typevar();
    TypeVar* D = tt.typevar();

    const TypeTraitInstance* clonableInst = tt.instantiate_trait(clonable, {});
    const TypeTraitInstance* eqInst = tt.instantiate_trait(eq, {});

    C->add_restriction(clonableInst);
    C->add_restriction(eqInst);

    FnType* g = tt.fntype({C, D});
    g->add_bound_var(C);
    g->add_bound_var(D);

    tt.unify(g);

    g->dump();

    tt.check_sanity();

    cout << "simple_tests [okay]" << endl;
}

void test_unification1() {
    TypeTable tt;

    TypeVar* A = tt.typevar();
    FnType* f = tt.fntype({A});       // fn(A)
    FnType* g = tt.fntype({A});       // fn(A)
    FnType* h = tt.fntype({f, g});    // fn(fn(A), fn(A))
    h->add_bound_var(A);              // fn<A>(fn(A), fn(A))
    FnType* gh = tt.unify(h);

    //gh->dump();
    //gh->get_representative()->dump();
    //h->dump();
    //h->get_representative()->dump();

    assert(f->is_unified());
    assert(g->is_unified());
    assert(h->is_unified());
    assert(gh->is_unified());

    assert(gh == h);

    assert(f->equal(g));
    assert(f->get_representative() == g->get_representative());

    tt.check_sanity();
    check_sanity({A, f, g, h, gh});

    cout << "test_unification1 [okay]" << endl;
}


void test_unification2() {
    TypeTable tt;

    TypeVar* A = tt.typevar();
    FnType* f = tt.fntype({A});     // fn(A)
    f->add_bound_var(A);            // fn<A>(A)
    FnType* uf = tt.unify(f);

    TypeVar* B = tt.typevar();
    FnType* g = tt.fntype({B});     // fn(B)
    g->add_bound_var(B);            // fn<B>(B)
    FnType* ug = tt.unify(g);

    //uf->dump();
    //uf->get_representative()->dump();
    //g->dump();
    //g->get_representative()->dump();

    assert(uf == f);
    assert(uf == ug);

    assert(g != ug);
    assert(g->equal(ug));
    assert(g->get_representative() == ug->get_representative());

    assert(A->get_representative() == B->get_representative());

    tt.check_sanity();
    check_sanity({A, f, uf, B, g, ug});

    cout << "test_unification2 [okay]" << endl;
}

/*void test_unification3() {
    TypeTable tt;

    const TypeVar* A = tt.typevar();
    const TypeVar* B = tt.typevar();
    const FnType* f1 = tt.fntype({A, B});     // fn(A, B)
    const FnType* gf1 = tt.gentype({B}, f1);  // fn<B>(A, B)
    const FnType* f2 = tt.fntype({A, gf1});   // fn(A, fn<B>(A, B))
    const FnType* gf2 = tt.gentype({A}, f2);  // fn<A>(A, fn<B>(A, B))

    const TypeVar* C = tt.typevar();
    const TypeVar* D = tt.typevar();
    const FnType* g1 = tt.fntype({C, D});     // fn(C, D)
    const FnType* gg1 = tt.gentype({D}, g1);  // fn<D>(C, D)
    const FnType* g2 = tt.fntype({C, gg1});   // fn(C, fn<D>(C, D))
    const FnType* gg2 = tt.gentype({C}, g2);  // fn<C>(C, fn<D>(C, D))

    //gf2->dump();
    //gg2->dump();

    assert(gf2 == gg2);
    assert(gf1 != gg1);
    assert(gf1->equal(gg1));
    assert(gf1->get_representative() == gg1->get_representative());

    const TypeVar* E = tt.typevar();
    const TypeVar* F = tt.typevar();
    const FnType* h1 = tt.fntype({F, E});     // fn(F, E)
    const FnType* gh1 = tt.gentype({F}, h1);  // fn<F>(F, E)
    const FnType* h2 = tt.fntype({E, gh1});   // fn(E, fn<F>(F, E))
    const FnType* gh2 = tt.gentype({E}, h2);  // fn<E>(E, fn<F>(F, E))

    //gh2->dump();

    assert(gf2 != gh2);
    assert(!gf2->equal(gh2));
    assert(gf2->get_representative() != gh2->get_representative());

    assert(gf1 != gh1);
    assert(!gf1->equal(gh1));
    assert(gf1->get_representative() != gh1->get_representative());

    const TypeVar* G = tt.typevar();
    const TypeVar* H = tt.typevar();
    const FnType* k1 = tt.fntype({G, H});         // fn(G, H)
    const FnType* k2 = tt.fntype({G, k1});        // fn(G, fn(G, H))
    const FnType* gk2 = tt.gentype({G, H}, k2);   // fn<G,H>(G, fn(G, H))

    //gk2->dump();

    assert(gf2 != gk2);
    assert(!gf2->equal(gk2));
    assert(gf2->get_representative() != gk2->get_representative());

    assert(gf1 != k1);
    assert(!gf1->equal(k1));
    assert(gf1->get_representative() != k1->get_representative());

    tt.check_sanity();
    check_sanity({A, B, f1, gf1, f2, gf2, C, D, g1, gg1, g2, gg2, E, F, h1, gh1, h2, gh2, G, H, k1, k2, gk2});

    cout << "test_unification3 [okay]" << endl;
}

void test_unification4() {
    TypeTable tt;

    const TypeTrait* clonable = tt.typetrait(std::string("Clonable"));
    const TypeTraitInstance* clonableInst = tt.instantiate_trait(clonable, {});
    const TypeVar* A = tt.typevar({clonableInst});
    const FnType* f = tt.gentype({A}, tt.fntype({A})); // fn<A:Clonable>(A)

    const TypeTrait* clonable2 = tt.typetrait(std::string("Clonable"));
    const TypeTraitInstance* clonable2Inst = tt.instantiate_trait(clonable2, {});
    const TypeVar* B = tt.typevar({clonable2Inst});
    const FnType* g = tt.gentype({B}, tt.fntype({B})); // fn<B:Clonable>(B)

    assert(clonable == clonable2);
    assert(clonableInst == clonable2Inst);

    assert(f->get_representative() == g->get_representative());

    const TypeTrait* st = tt.typetrait(std::string("SomeTrait"));
    const TypeTraitInstance* stInst = tt.instantiate_trait(st, {});
    const TypeVar* C = tt.typevar({stInst});
    const FnType* h = tt.gentype({C}, tt.fntype({C})); // fn<B:SomeTrait>(B)

    assert(st != clonable);
    assert(!st->equal(clonable));

    assert(h->get_representative() != g->get_representative());

    tt.check_sanity();
    check_sanity({A, f, B, g, C, h});

    cout << "test_unification4 [okay]" << endl;
}

/// fn<A:S<B>, B:S<A>>(A, B)
void test_unification5() {
    TypeTable tt;

    const TypeVar* X = tt.typevar();
    const TypeTrait* S = tt.typetrait(std::string("S"), {X});   // trait S<X>

    const TypeVar* A = tt.typevar();
    const TypeVar* B = tt.typevar();

    const TypeTraitInstance* SA = tt.instantiate_trait(S, {A}); // S<A>
    const TypeTraitInstance* SB = tt.instantiate_trait(S, {B}); // S<B>

    A->add_restriction(SB);
    B->add_restriction(SA);

    /// fn<A:S<B>, B:S<A>>(A, B)
    const FnType* f = tt.gentype({A, B}, tt.fntype({A, B}));

    f->dump();

    tt.check_sanity();
    check_sanity({A, B, f});

    cout << "test_unification5 [okay]" << endl;
}

void test_trait_instatiation1() {
    TypeTable tt;

    const TypeVar* A = tt.typevar();
    const TypeTrait* T = tt.typetrait(std::string("T"), {A});   // trait T<A>
    try {
        // illegal, must instantiate A
        const TypeTraitInstance* Ti = tt.instantiate_trait(T, {});
        assert(false && "Previous statement should have failed!");
    } catch (exception& e) {
    }

    tt.check_sanity();

    cout << "test_type_sanity1 [okay]" << endl;
}

void test_type_sanity1() {
    TypeTable tt;

    TypeVar* A = tt.typevar();
    FnType* g = tt.fntype({tt.type_int()}); // fn(int)

    try {
        // illegal
        g->add_bound_var(A);    // fn<A>(int)

        assert(false && "Previous statement should have failed!");
    } catch (IllegalTypeException& e) {
        assert(g->is_closed());
        assert(g->is_unified());
        assert(!A->is_unified());
        assert(A->bound_at() == nullptr);
    }

    tt.check_sanity();

    cout << "test_type_sanity1 [okay]" << endl;
}

void test_type_sanity2() {
    TypeTable tt;

    const TypeVar* A = tt.typevar();

    try {
        A->add_bound_var(A);         // forall A, A
        assert(false && "Previous statement should have failed!");
    } catch (IllegalTypeException& e) {
        assert(!A->is_closed());
        assert(!A->is_unified());
        assert(A->bound_at() == nullptr);
    }

    const TypeVar* B = tt.typevar();
    const TypeVar* C = tt.typevar();
    try {
        C->add_bound_var(B); // forall B, C
        assert(false && "Previous statement should have failed!");
    } catch (IllegalTypeException& e) {
        assert(B->bound_at() == nullptr);
    }


    tt.check_sanity();

    cout << "test_type_sanity2 [okay]" << endl;
}

void test_type_sanity3() {
    TypeTable tt;

    TypeVar* A = tt.typevar();
    FnType* f = tt.fntype({A}); // fn(A)
    f->add_bound_var(A);        // fn<A>(A)

    FnType* g = tt.fntype({A}); // fn(A)

    // TODO unify

    assert(g->is_unified());
    assert(g != gf);
    assert(g->get_representative() != gf->get_representative());
    assert(!g->equal(gf));

    try {
        FnType* h = tt.fntype({gf, g});     // fn(fn<A>(A), fn(A)) -> INVALID
        // TODO unify

        h->dump();

        assert(false && "Previous statement should have failed!");
    } catch (IllegalTypeException& e) {
    }

    tt.check_sanity();
    check_sanity({A, f, gf, g});

    cout << "test_type_sanity3 [okay]" << endl;
}

void test_type_sanity4() {
    TypeTable tt;

    TypeVar* A = tt.typevar();
    FnType* f = tt.fntype({A}); // fn(A)
    f->add_bound_var(A);        // fn<A>(A)

    TypeVar* B = tt.typevar();
    FnType* g = tt.fntype({A, B});   // fn(A, B)

    try {
        // must fail because A is already bound!
        g->add_bound_var(A);

        assert(false && "Previous statement should have failed!");
    } catch (IllegalTypeException& e) {
    }

    tt.check_sanity();

    cout << "test_type_sanity4 [okay]" << endl;
}

void test_type_sanity5() {
    TypeTable tt;

    TypeVar* A = tt.typevar();
    TypeVar* B = tt.typevar();
    FnType* g = tt.fntype({B});   // fn(B)

    try {
        // must fail because A is not a subtype of g
        g->add_bound_var(A);

        assert(false && "Previous statement should have failed!");
    } catch (IllegalTypeException& e) {
    }

    tt.check_sanity();

    cout << "test_type_sanity5 [okay]" << endl;
}

void test_type_sanity6() {
    TypeTable tt;

    TypeVar* A = tt.typevar();
    FnType* f = tt.fntype({A});         // fn(A)
    FnType* g = tt.fntype({A});         // fn(A)
    g->add_bound_var(A);
    // TODO unify g
    // TODO f is now a type that is sane but not unified!

    //tt.check_sanity();

    //cout << "test_type_sanity6 [okay]" << endl;
}*/

int main() {
    simple_tests();
    //return 0;

    test_unification1();
    test_unification2();
    /*test_unification3();
    test_unification4();
    test_unification5();
    test_type_sanity1();
    test_type_sanity2();
    //test_type_sanity3();
    test_type_sanity4();
    test_type_sanity5();
    //test_type_sanity6();*/
}
