#ifndef IMPALA_TYPE_H
#define IMPALA_TYPE_H

#include <unordered_set>

#include "anydsl2/node.h"
#include "anydsl2/util/hash.h"

#include "impala/token.h"

namespace anydsl2 {
    class GenericMap;
    class Type;
    class World;
}

namespace impala {

class FnType;
class Generic;
class GenericRef;
class NoRet;
class Void;
class PrimType;
class TupleType;
class Type;
class TypeError;

class TypeTable {
public:
    TypeTable();

    const TypeError* type_error() { return type_error_; }
    const NoRet* noret() { return noret_; }
    const Void* type_void() { return void_; }
    const PrimType* primtype(TokenKind kind);
#define IMPALA_TYPE(itype, atype) const PrimType* type_##itype() { return itype##_; }
#include "impala/tokenlist.h"
    const FnType* fntype(const Type*);
    const FnType* fntype(anydsl2::ArrayRef<const Type*> elems = anydsl2::ArrayRef<const Type*>(nullptr, 0));
    const TupleType* tupletype(anydsl2::ArrayRef<const Type*> elems);
    const Generic* generic(size_t index);

private:
    const Type* unify_base(const Type* type);
    template<class T> const T* unify(const T* type) { return unify_base(type)->template as<T>(); }

    std::unordered_set<const Type*> types_;

#define IMPALA_TYPE(itype, atype) const PrimType* itype##_;
#include "impala/tokenlist.h"
    const TypeError* type_error_;
    const NoRet* noret_;
    const Void* void_;
};

//------------------------------------------------------------------------------

class GenericBuilder {
public:
    GenericBuilder(TypeTable& typetable)
        : typetable_(typetable)
        , index_(0)
    {}

    size_t new_def();
    const Generic* use(size_t handle);
    void pop();

private:
    TypeTable& typetable_;
    size_t index_;
    typedef std::vector<const Generic*> Index2Generic;
    Index2Generic index2generic_;
};

//------------------------------------------------------------------------------

class GenericMap {
public:
    GenericMap() {}

    const Type*& operator [] (const Generic* generic) const;
    bool is_empty() const;
    const char* to_string() const;

private:
    mutable std::vector<const Type*> types_;
};

inline std::ostream& operator << (std::ostream& o, const GenericMap& map) { o << map.to_string(); return o; }

//------------------------------------------------------------------------------

class Type : public anydsl2::Node {
protected:
    Type(TypeTable& typetable, TokenKind kind, size_t size, const std::string& name)
        : Node((int) kind, size, name)
        , typetable_(typetable)
    {}

public:
    TokenKind kind() const { return (TokenKind) anydsl2::Node::kind(); }
    anydsl2::ArrayRef<const Type*> elems() const { return ops_ref<const Type*>(); }
    const Type* elem(size_t i) const { return elems()[i]; }
    virtual const Type* specialize(const GenericMap& generic_map) const = 0;
    virtual const anydsl2::Type* convert(anydsl2::World&) const = 0;
    bool is_bool() const;
    bool is_int() const;
    bool is_float() const;
    bool is_void() const;
    bool check_with(const Type*) const;
    bool infer_with(GenericMap& map, const Type* type) const;
    std::ostream& dump() const;

protected:
    TypeTable& typetable_;
};

class TypeError : public Type {
private:
    TypeError(TypeTable& typetable) 
        : Type(typetable, Token::TYPE_error, 0, "<error>")
    {}

    virtual const Type* specialize(const GenericMap& generic_map) const { return this; }
    virtual const anydsl2::Type* convert(anydsl2::World&) const { assert(false); }

    friend class TypeTable;
};

class Void : public Type {
private:
    Void(TypeTable& typetable) 
        : Type(typetable, Token::TYPE_void, 0, "void")
    {}

    virtual const Type* specialize(const GenericMap& generic_map) const { return this; }
    virtual const anydsl2::Type* convert(anydsl2::World&) const { assert(false); }

    friend class TypeTable;
};

class NoRet : public Type {
private:
    NoRet(TypeTable& typetable) 
        : Type(typetable, Token::TYPE_noret, 0, "!")
    {}

    virtual const Type* specialize(const GenericMap& generic_map) const { return this; }
    virtual const anydsl2::Type* convert(anydsl2::World&) const { assert(false); }

    friend class TypeTable;
};

class PrimType : public Type {
private:
    PrimType(TypeTable& typetable, TokenKind kind)
        : Type(typetable, kind, 0, "<primitive type>")
    {}

public:
    virtual const Type* specialize(const GenericMap& generic_map) const { return this; }
    virtual const anydsl2::Type* convert(anydsl2::World&) const;

    friend class TypeTable;
};

class Generic : public Type {
private:
    Generic(TypeTable& typetable, size_t index)
        : Type(typetable, Token::TYPE_generic, 0, "<generic>")
        , index_(index)
    {}

    virtual const Type* specialize(const GenericMap& generic_map) const;
    virtual const anydsl2::Type* convert(anydsl2::World&) const;
    virtual size_t hash() const { return anydsl2::hash_combine(Type::hash(), index()); }
    virtual bool equal(const Node* other) const { 
        return Type::equal(other) ? index() == other->as<Generic>()->index() : false; 
    }

public:
    size_t index() const { return index_; }
    static std::string to_string(size_t index);

private:
    size_t index_;

    friend class TypeTable;
};

class CompoundType : public Type {
protected:
    CompoundType(TypeTable& typetable, TokenKind kind, anydsl2::ArrayRef<const Type*> elems, const std::string& name)
        : Type(typetable, kind, elems.size(), name)
    {
        size_t i = 0;
        for (auto elem : elems)
            set(i++, elem);
    }
};

class FnType : public CompoundType {
private:
    FnType(TypeTable& typetable, anydsl2::ArrayRef<const Type*> elems)
        : CompoundType(typetable, Token::PI, elems, "<function type>")
    {}

public:
    virtual const Type* specialize(const GenericMap& generic_map) const;
    virtual const anydsl2::Type* convert(anydsl2::World&) const;
    const Type* return_type() const;

    friend class TypeTable;
};

class TupleType : public CompoundType {
private:
    TupleType(TypeTable& typetable, anydsl2::ArrayRef<const Type*> elems)
        : CompoundType(typetable, Token::SIGMA, elems, "<tuple type>")
    {}

    virtual const Type* specialize(const GenericMap& generic_map) const;
    virtual const anydsl2::Type* convert(anydsl2::World&) const;

    friend class TypeTable;
};

} // namespace impala

#endif
