#ifndef IMPALA_SEMA_GENERIC_H
#define IMPALA_SEMA_GENERIC_H

#include <vector>

#include "thorin/util/array.h"
#include "thorin/util/cast.h"
#include "thorin/util/hash.h"

namespace impala {

class TypeTable;
template<class T> class Unifiable;

template<class T> struct NodeHash {
    size_t operator () (const T t) const { return thorin::hash_value(t.node()); }
};
template<class T> struct NodeEqual {
    bool operator () (const T t1, const T t2) const { return t1.node() == t2.node(); }
};
template<class T> using NodeSet = thorin::HashSet<T, NodeHash<T>, NodeEqual<T>>;

template<class T> struct UniHash {
    size_t operator () (const T t) const { return t->hash(); }
};
template<class T> struct UniEqual {
    bool operator () (const T t1, const T t2) const { return (t1->is_unified() && t2->is_unified()) ? t1 == t2 : t1->equal(*t2); }
};
template<class T> using UniSet = thorin::HashSet<T, UniHash<T>, UniEqual<T>>;

class TypeNode;

template<class T> class Proxy;
template<class T> void unify(TypeTable&, const Proxy<T>&);

template<class T>
class Proxy {
public:
    typedef T BaseType;

    Proxy()
        : node_(nullptr)
    {}
    explicit Proxy(T* node)
        : node_(node)
    {}

    bool empty() const { return node_ == nullptr; }
    bool operator == (const Proxy<T>& other) const {
        assert(node_ != nullptr);         
        assert(&node()->typetable() == &other.node()->typetable());
        unify(node()->typetable(), *this);
        unify(node()->typetable(), other);
        return representative() == other.representative();
    }
    bool operator != (const Proxy<T>& other) const { assert(node_ != nullptr); return !(*this == other); }
    T* representative() const { assert(node_ != nullptr); return node_->representative()->template as<T>(); }
    T* node() const { assert(node_ != nullptr); return node_; }
    T* operator  * () const { assert(node_ != nullptr); return node_->is_unified() ? representative() : node_->template as<T>(); }
    T* operator -> () const { assert(node_ != nullptr); return *(*this); }
    /// Automatic up-cast in the class hierarchy.
    template<class U> operator Proxy<U>() {
        static_assert(std::is_base_of<U, T>::value, "R is not a base type of L");
        assert(node_ != nullptr); return Proxy<U>((U*) node_);
    }
    template<class U> Proxy<typename U::BaseType> isa() { 
        assert(node_ != nullptr); return Proxy<typename U::BaseType>((*this)->isa<typename U::BaseType>()); 
    }
    template<class U> Proxy<typename U::BaseType> as() { 
        assert(node_ != nullptr); return Proxy<typename U::BaseType>((*this)->as <typename U::BaseType>()); 
    }
    operator bool() { return !empty(); }
    Proxy<T>& operator= (Proxy<T> other) { assert(node_ == nullptr); node_ = *other; return *this; }
    void clear() { node_ = nullptr; }

private:
    T* node_;
};

class UnknownTypeNode;
class TypeVarNode;
class TraitNode;
class TraitImplNode;
typedef Proxy<TypeNode> Type;
typedef Proxy<UnknownTypeNode> UnknownType;
typedef Proxy<TypeVarNode> TypeVar;
typedef Proxy<TraitNode> Trait;
typedef Proxy<TraitImplNode> TraitImpl;

class Generic;
typedef thorin::HashMap<const Generic*, Generic*> SpecializeMapping;

//------------------------------------------------------------------------------

class Generic : public thorin::MagicCast<Generic> {
protected:
    Generic(TypeTable& tt)
        : typetable_(tt)
    {}

public:
    TypeTable& typetable() const { return typetable_; }
    virtual size_t num_bound_vars() const { return bound_vars_.size(); }
    virtual thorin::ArrayRef<TypeVar> bound_vars() const { return thorin::ArrayRef<TypeVar>(bound_vars_); }
    virtual TypeVar bound_var(size_t i) const { return bound_vars_[i]; }
    /// Returns true if this \p Type does have any bound type variabes (\p bound_vars_).
    virtual bool is_generic() const { return !bound_vars_.empty(); }
    virtual bool is_closed() const = 0; // TODO
    virtual void add_bound_var(TypeVar v);
    virtual bool equal(const Generic*) const = 0;
    virtual size_t hash() const = 0;
    virtual std::string to_string() const = 0;

    /**
     * Try to fill in missing type information by matching this possibly incomplete Generic with a complete Generic.
     * Example: fn(?0, ?1) unified_with fn(int, bool)  will set ?0=int and ?1=bool
     * @return \p true if unification worked, i.e. both generics were structurally equal
     *         and there were no contradictions during unification (a contradiction
     *         would be fn(?0, ?0) unified with fn(int, bool)).
     */
    virtual bool unify_with(Generic*) = 0;

    /**
     * replace any \p UnknownTypeNodes within this Generic with their instances
     * and set the representatives of these nodes to their instances
     */
    virtual void make_real() = 0;
    /// a \p Generic is real if it does not contain any \p UnknownTypeNodes
    virtual bool is_real() const = 0;

    void dump() const;

protected:
    std::vector<TypeVar> bound_vars_;
    TypeTable& typetable_;

    std::string bound_vars_to_string() const;
    bool unify_bound_vars(thorin::ArrayRef<TypeVar>);
    void make_bound_vars_real();
    bool bound_vars_real() const;

    Generic* ginstantiate(SpecializeMapping& var_instances);
    Generic* gspecialize(SpecializeMapping&); // TODO one could always assert that this is only called on final representatives!

    /// like specialize but does not care about generics (this method is called by specialize)
    virtual Generic* vspecialize(SpecializeMapping&) = 0;

private:
    /// raise error if a type does not implement the required traits;
    void check_instantiation(SpecializeMapping&) const;

    friend class TypeVarNode;
    friend class TraitInstanceNode;
};

template<class T>
class Unifiable : public Generic {
protected:
    Unifiable(TypeTable& tt)
        : Generic(tt)
        , representative_(nullptr)
    {
        static_assert(std::is_base_of<Unifiable<T>, T>::value, "Unifiable<T> is not a base type of T");
    }

public:
    T* representative() const { return representative_; }
    bool is_final_representative() const { return representative() == this->template as<T>(); }
    bool is_unified() const { return representative_ != nullptr; }
    virtual bool equal(const T*) const = 0;
    virtual bool equal(const Generic* other) const {
        if (const T* t = other->isa<T>())
            return equal(t);
        return false;
    }

    /// @see Generic::unify_with(Generic*)
    virtual bool unify_with(T*) = 0;
    bool unify_with(Proxy<T> other) {
        assert(other->is_closed());
        bool b = unify_with(*other);
        assert(!b || is_closed());
        return b;
    }
    virtual bool unify_with(Generic* other) {
        assert(other->is_closed());
        if (T* t = other->isa<T>()) {
            bool b = unify_with(t);
            assert(!b || is_closed());
            return b;
        }
        return false;
    }

    /**
     * Instantiate a generic element using the mapping from TypeVar -> Type
     * @param var_instances A mapping that assigns each type variable that is bound at this generic an instance
     * @return the instantiated type
     * @see TypeTable::create_spec_mapping()
     */
    Proxy<T> instantiate(SpecializeMapping& var_instances) {
//        assert(is_final_representative()); CHECK it seems nothing gets broken w.o. this assertion - still I don't feel comfortable w.o. it
        // we can not unify yet because it could be that this type is not closed yet
        return Proxy<T>(ginstantiate(var_instances)->as<T>());
    }
    /**
     * if this element is in the mapping return the mapped one;
     * otherwise copy this element with specialized sub-elements
     */
    Proxy<T> specialize(SpecializeMapping& mapping) {
//        assert(is_final_representative()); CHECK it seems nothing gets broken w.o. this assertion - still I don't feel comfortable w.o. it
        return Proxy<T>(gspecialize(mapping)->as<T>());
    }

private:
    T* representative_;

    void set_representative(T* representative) { representative_ = representative; }

protected:
    std::vector<TypeVar> bound_vars_;

    friend class TypeTable;
};

template<class T>
std::ostream& operator << (std::ostream& o, Proxy<T> u) { return o << u->to_string(); }

}

#endif
