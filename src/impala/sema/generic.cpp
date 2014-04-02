#include "impala/sema/generic.h"

#include "thorin/util/assert.h"
#include "impala/sema/type.h"
#include "impala/sema/trait.h"
#include "impala/sema/typetable.h"

namespace impala {

template<class T> void unify(TypeTable& tt, const Proxy<T>& p) { tt.unify(p); }

template void unify(TypeTable&, const Proxy<TypeNode>&);
template void unify(TypeTable&, const Proxy<TraitNode>&);
//template void unify(TypeTable&, const Proxy<TraitImplNode>&);

//------------------------------------------------------------------------------

bool Generic::unify_bound_vars(thorin::ArrayRef<TypeVar> other_vars) {
    if (num_bound_vars() == other_vars.size())
        return !is_generic(); // TODO enable unification of generic elements!
    return false;
}

void Generic::refine_bound_vars() {
    for (auto v : bound_vars())
        v->refine();
}

bool Generic::bound_vars_known() const {
    for (auto v : bound_vars()) {
        if (!v->is_known())
            return false;
    }
    return true;
}

void Generic::add_bound_var(TypeVar v) {
    assert(!v->is_closed() && "Type variables already bound");

    // CHECK should variables only be bound in this case? does this also hold for traits?
    //assert(v->is_subtype(this) && "Type variables can only be bound at t if they are a subtype of t!");
    // CHECK should 'forall a, a' be forbidden?
    //assert(type->kind() != Type_var && "Types like 'forall a, a' are forbidden!");

    v->bind(this);
    bound_vars_.push_back(v);
}

void Generic::verify_instantiation(SpecializeMap& map) const {
    assert(map.size() == num_bound_vars());

    // check the bounds
    for (auto v : bound_vars()) {
        auto it = map.find(*v);
        assert(it != map.end());
        Type instance = Type(it->second->as<TypeNode>());

        for (auto bound : v->bounds()) {
            SpecializeMap m(map); // copy the map
            Trait spec_bound = Trait(bound->specialize(m)->as<TraitNode>());
            spec_bound->typetable().unify(spec_bound);
            assert(instance->implements(spec_bound));
        }
    }
}

Generic* Generic::ginstantiate(SpecializeMap& var_instances) {
/*#ifndef NDEBUG
    check_instantiation(var_instances);
#endif*/
    assert(var_instances.size() == num_bound_vars());
    return vspecialize(var_instances);
}

Generic* Generic::gspecialize(SpecializeMap& map) {
    // FEATURE this could be faster if we copy only types where something changed inside
    if (auto result = thorin::find(map, this))
        return result;

    for (auto v : bound_vars()) {
        // CHECK is representative really correct or do we need node()? -- see also below!
        assert(!map.contains(v.representative()));
        v->clone(map); // CHECK is node() correct here?
    }

    Generic* t = vspecialize(map);

    for (auto v : bound_vars()) {
        assert(map.find(v.representative()) != map.end());
        t->add_bound_var(TypeVar(map[v.representative()]->as<TypeVarNode>()));
    }

    return t;
}

}
