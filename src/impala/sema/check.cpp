#include <iostream>

#include "impala/ast.h"
#include "impala/dump.h"
#include "impala/sema/scopetable.h"
#include "impala/sema/typetable.h"

namespace impala {

class Sema : public ScopeTable, public TypeTable {
public:
    Sema(bool nossa)
        : cur_fn_(nullptr)
        , nossa_(nossa)
    {}

    bool nossa() const { return nossa_; }
    const Fn* cur_fn() const { return cur_fn_; }

private:
    const Fn* cur_fn_;
    bool nossa_;
};

inline bool match_types(Sema& sema, Type expected, const Expr* found) {
    assert(!expected.empty());
    assert(!found->type().empty());
    if (found->type() != expected) {
        sema.error(found) << "Wrong argument type; expected " << expected << " but found " << found->type() << "\n";
        return false;
    } else
        return true;
}

//------------------------------------------------------------------------------

void ParametricASTType::check_type_params(Sema& sema) const {
    // we need two runs for types like fn[A:T[B], B:T[A]](A, B)

    for (const TypeParam* tp : type_params()) {
        tp->type_var_ = sema.typevar();
        sema.insert(tp);
    }

    // check bounds
    for (const TypeParam* tp : type_params()) {
        for (const ASTType* b : tp->bounds()) {
            if (auto trait_inst = b->isa<ASTTypeApp>()) {
                tp->type_var()->add_bound(trait_inst->to_trait_instance(sema));
            } else {
                sema.error(tp) << "Bounds must be trait instances, not types\n";
            }
        }
    }
}

Type ErrorASTType::to_type(Sema& sema) const { return sema.type_error(); }

Type PrimASTType::to_type(Sema& sema) const {
    switch (kind()) {
#define IMPALA_TYPE(itype, atype) case TYPE_##itype: return sema.primtype(PrimType_##itype);
#include "impala/tokenlist.h"
        default: THORIN_UNREACHABLE;
    }
}

Type PtrASTType::to_type(Sema& sema) const {
    return Type(); // FEATURE
}

Type IndefiniteArrayASTType::to_type(Sema& sema) const {
    return Type(); // FEATURE
}

Type DefiniteArrayASTType::to_type(Sema& sema) const {
    return Type(); // FEATURE
}

Type TupleASTType::to_type(Sema& sema) const {
    return Type(); // FEATURE
}

Type ASTTypeApp::to_type(Sema& sema) const {
    if (auto decl = sema.lookup(this, symbol())) {
        if (auto tp = decl->isa<TypeParam>()) {
            assert(elems().empty());
            assert(!tp->type_var().empty());
            return tp->type_var();
        } else
            sema.error(this) << "cannot convert a trait instance into a type\n";
    }

    return sema.type_error();
}

Type FnASTType::to_type(Sema& sema) const {
    sema.push_scope();

    check_type_params(sema);

    std::vector<Type> params;
    for (auto p : elems())
        params.push_back(p->to_type(sema));

    FnType t = sema.fntype(params);
    for (auto tp : type_params())
        t->add_bound_var(tp->type_var());

    sema.pop_scope();

    return t;
}

TraitInstance ASTTypeApp::to_trait_instance(Sema& sema) const {
    if (auto decl = sema.lookup(this, symbol())) {
        if (auto trait_decl = decl->isa<TraitDecl>()) {
            std::vector<Type> type_args;
            for (auto e : elems())
                type_args.push_back(e->to_type(sema));

            return sema.instantiate_trait(trait_decl->trait(), type_args);
        } else
            sema.error(this) << "cannot convert a type variable into a trait instance\n";
    }
    // return error_trait_instance <- we need sth like that
}

//------------------------------------------------------------------------------

/*
 * items - check_head
 */

void ModDecl::check_head(Sema& sema) const {
    sema.insert(this);
}

void ModContents::check(Sema& sema) const {
    for (auto item : items()) item->check_head(sema);
    for (auto item : items()) item->check(sema);
}

void ForeignMod::check_head(Sema& sema) const {
    sema.insert(this);
}

void EnumDecl::check_head(Sema& sema) const {
    sema.insert(this);
}

void FnDecl::check_head(Sema& sema) const {
    sema.insert(this);
}

void StaticItem::check_head(Sema& sema) const {
    sema.insert(this);
}

void StructDecl::check_head(Sema& sema) const {
    sema.insert(this);
}

void TraitDecl::check_head(Sema& sema) const {
    sema.insert(this);
}

void Typedef::check_head(Sema& sema) const {
    sema.insert(this);
}

void Impl::check_head(Sema& sema) const {
    sema.insert(this);
}

/*
 * items - check
 */

void ModDecl::check(Sema& sema) const {
    sema.push_scope();
    if (mod_contents())
        mod_contents()->check(sema);
    sema.pop_scope();
}

void ForeignMod::check(Sema& sema) const {
}

void Typedef::check(Sema& sema) const {
}

void EnumDecl::check(Sema& sema) const {
}

void StaticItem::check(Sema& sema) const {
}

void FnDecl::check(Sema& sema) const {
    sema.push_scope();
    check_type_params(sema);
    // check parameters
    std::vector<Type> par_types;
    for (const Param* p : fn().params()) {
        sema.insert(p);
        Type pt = p->asttype()->to_type(sema);
        p->set_type(pt);
        par_types.push_back(pt);
    }
    // create FnType
    Type fn_type = sema.fntype(par_types);
    for (auto tp : type_params()) {
        assert(!tp->type_var().empty());
        fn_type->add_bound_var(tp->type_var());
    }
    sema.unify(fn_type);
    set_type(fn_type);

    // CHECK set sema.cur_fn_?
    fn().body()->check(sema);

    // FEATURE check for correct return type
    sema.pop_scope();
}

void StructDecl::check(Sema& sema) const {
}

void TraitDecl::check(Sema& sema) const {
    // FEATURE consider super traits and check methods
    trait_ = sema.trait(this, TraitSet());

    check_type_params(sema);
    for (auto tp : type_params()) {
        assert(!tp->type_var().empty());
        trait_->add_bound_var(tp->type_var());
    }
}

void Impl::check(Sema& sema) const {
}

/*
 * expressions
 */

void EmptyExpr::check(Sema& sema) const {
    // set_type(sema.unit(); yes: empty expression returns unit - the empty tuple type '()'
}

void BlockExpr::check(Sema& sema) const {
    for (auto stmt : stmts()) {
        if (auto item_stmt = stmt->isa<ItemStmt>())
            item_stmt->item()->check_head(sema);
    }

    for (auto stmt : stmts())
        stmt->check(sema);

    expr()->check(sema);
    assert(!expr()->type().empty());
    set_type(expr()->type());
}

void LiteralExpr::check(Sema& sema) const {
}

void FnExpr::check(Sema& sema) const {
}

void PathExpr::check(Sema& sema) const {
    // FEATURE consider longer paths
    auto last_item = path()->path_items().back();

    if ((decl_ = sema.lookup(this, last_item->symbol()))) {
        if (auto vdec = decl_->isa<ValueDecl>()) {
            // consider type expressions
            if (!last_item->types().empty()) {
                std::vector<Type> type_args;
                for (const ASTType* t : last_item->types())
                    type_args.push_back(t->to_type(sema));

                set_type(vdec->type()->instantiate(type_args));
            } else
                set_type(vdec->type());
        }
    } else
        set_type(sema.type_error());

    assert(!type().empty());
}

void PrefixExpr::check(Sema& sema) const {
}

void InfixExpr::check(Sema& sema) const {
}

void PostfixExpr::check(Sema& sema) const {
}

void FieldExpr::check(Sema& sema) const {
}

void CastExpr::check(Sema& sema) const {
}

void DefiniteArrayExpr::check(Sema& sema) const {
}

void RepeatedDefiniteArrayExpr::check(Sema& sema) const {
}

void IndefiniteArrayExpr::check(Sema& sema) const {
}

void TupleExpr::check(Sema& sema) const {
}

void StructExpr::check(Sema& sema) const {
}

void MapExpr::check(Sema& sema) const {
    // FEATURE this currently only considers function calls
    lhs()->check(sema);
    Type lhs_type = lhs()->type();
    assert(!lhs_type.empty());

    switch (lhs_type->kind()) {
        case Type_error:
            set_type(sema.type_error());
            return;
        case Type_fn:
            if ((lhs_type->size() != (args().size()+1)) && (lhs_type->size() != args().size())) {
                // CHECK how would the result type look like if the continuation is explicitly passed?
                sema.error(this) << "Wrong number of arguments\n";
                set_type(sema.type_error());
            } else {
                for (size_t i = 0; i < args().size(); ++i) {
                    auto arg = args()[i];
                    arg->check(sema);
                    if (!match_types(sema, lhs_type->elem(i), arg)) set_type(sema.type_error());
                }

                // set return type
                Type ret_func = lhs_type->elem(lhs_type->size() - 1);
                assert(ret_func->kind() == Type_fn); // CHECK can there be function types w/o return function?

                switch (ret_func->size()) {
                case 0:
                    // FEATURE set void/unit type or something
                    break;
                case 1:
                    set_type(ret_func->elem(0));
                    break;
                default:
                    // FEATURE return tuple type
                    break;
                }
            }
            break;
        default:
            set_type(sema.type_error());
    }
    assert(!type().empty());
}

void IfExpr::check(Sema& sema) const {
}

void ForExpr::check(Sema& sema) const {
}

/*
 * statements
 */

void ExprStmt::check(Sema& sema) const {
    expr()->check(sema);
}

void ItemStmt::check(Sema& sema) const {
    item()->check(sema);
}

void LetStmt::check(Sema& sema) const {
    if (init())
        init()->check(sema);
}

//------------------------------------------------------------------------------

bool check(const ModContents* mod, bool nossa) {
    Sema sema(nossa);
    mod->check(sema);
    return sema.result();
}

}
