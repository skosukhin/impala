/*
 * trait.cpp
 *
 *  Created on: Dec 14, 2013
 *      Author: David Poetzsch-Heffter <s9dapoet@stud.uni-saarland.de>
 */

#include "trait.h"

const std::string TypeTrait::top_trait_name = std::string("");

bool TypeTrait::equal(const GenericElement* other) const {
    // TODO is this correct for a instanceof-equivalent?
    if (const TypeTrait* t = other->isa<TypeTrait>()) {
        return equal(t);
    }
    return false;
}

bool TypeTrait::equal(const TypeTrait* other) const {
    return name_.compare(other->name_) == 0;
}

size_t TypeTrait::hash() const { return thorin::hash_value(name_); }

// TODO
std::string TypeTrait::to_string() const {
    return name_;
}

void TypeTrait::add_method(const std::string name, const FnType* type) {
    if (! type->is_unified()) {
        throw IllegalTypeException("Method types must be closed");
    }
    assert(type->is_closed());
    TypeTraitMethod* m = new TypeTraitMethod();
    m->name = name;
    m->type = type;
    methods_.push_back(m);
}


TypeTraitInstance::TypeTraitInstance(const TypeTrait* trait, TypeArray var_instances)
    : trait_(trait)
    , var_instances_(var_instances.size())
{
    if (var_instances.size() != trait->bound_vars().size())
        throw IllegalTypeException("Wrong number of instances for bound type variables");

    size_t i = 0;
    for (auto elem : var_instances)
        var_instances_[i++] = elem;
}

bool TypeTraitInstance::equal(const TypeTraitInstance* other) const {
    if (this->is_unified() && other->is_unified()) {
        return this->get_representative() == other->get_representative();
    }

    // TODO use equal?
    if (trait_ != other->trait_)
        return false;

    assert(var_instances_.size() == other->var_instances_.size());
    for (int i = 0; i < var_instances_.size(); ++i) {
        if (! var_instances_[i]->equal(other->var_instances_[i])) {
            return false;
        }
    }
    return true;
}

// TODO better hash function
size_t TypeTraitInstance::hash() const { return trait_->hash(); }

bool TypeTraitInstance::is_closed() const {
    for (auto i : var_instances_) {
        if (!i->is_closed())
            return false;
    }
    return true;
}

// TODO
std::string TypeTraitInstance::to_string() const {
    std::string result = trait_->name();

    const char* separator = "<";
    for (auto v : var_instances_) {
        result += separator + v->to_string();
        separator = ",";
    }

    return result + ">";
}
