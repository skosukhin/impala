#include <fstream>
#include <vector>
#include <cctype>
#include <stdexcept>

#include "thorin/analyses/looptree.h"
#include "thorin/analyses/scope.h"
#include "thorin/analyses/verify.h"
#include "thorin/transform/import.h"
#include "thorin/transform/vectorize.h"
#include "thorin/transform/partial_evaluation.h"
#include "thorin/be/thorin.h"
#include "thorin/be/il.h"
#include "thorin/be/llvm/llvm.h"
#include "thorin/util/args.h"

#include "impala/ast.h"
#include "impala/dump.h"
#include "impala/impala.h"

//------------------------------------------------------------------------------

using namespace thorin;
using namespace std;

typedef vector<string> Names;

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
    try {
        if (argc < 1)
            throw logic_error("bad number of arguments");

        string prgname = argv[0];
        Names infiles;
#ifndef NDEBUG
        Names breakpoints;
#endif
        string outfile;
        bool help, emit_all, emit_thorin, emit_il, emit_ast, emit_annotated, emit_llvm, emit_looptree, fancy, nocolor, opt, nocleanup, nossa = false;
        int vectorlength = 0;
        auto cmd_parser = ArgParser()
            .implicit_option("infiles", "input files", infiles)
            // specify options
            .add_option<bool>("help", "produce this help message", help, false)
            .add_option<string>("o", "specifies the output file", outfile, "-")
#ifndef NDEBUG
            .add_option<vector<string>>("break", "breakpoint at definition generation of number arg", breakpoints)
#endif
            .add_option<bool>("nocleanup", "no clean-up phase", nocleanup, false)
            .add_option<bool>("nossa", "use slots + load/store instead of SSA construction", nossa, false)
            .add_option<int>("vectorize", "run vectorizer on main with given vector length (experimantal!!!), arg=<vector length>", vectorlength, false)
            .add_option<bool>("emit-thorin", "emit textual THORIN representation of impala program", emit_thorin, false)
            .add_option<bool>("emit-il", "emit textual IL representation of impala program", emit_il, false)
            .add_option<bool>("emit-all", "emit AST, THORIN, LLVM and loop tree", emit_all, false)
            .add_option<bool>("emit-ast", "emit AST of impala program", emit_ast, false)
            .add_option<bool>("emit-annotated", "emit AST of impala program after semantical analysis", emit_annotated, false)
            .add_option<bool>("emit-looptree", "emit loop tree", emit_looptree, false)
            .add_option<bool>("emit-llvm", "emit llvm from THORIN representation (implies -O)", emit_llvm, false)
            .add_option<bool>("f", "use fancy output", fancy, false)
            .add_option<bool>("nc", "use uncolored output", nocolor, false)
            .add_option<bool>("O", "optimize", opt, false);

        // do cmdline parsing
        cmd_parser.parse(argc, argv);

#if 0
        thorin::World w("unsolvable compiler error");
        auto T = w.type_var();
        auto ffn = w.fn_type({T});
        ffn.bind(T);
        auto f = w.lambda(ffn, Lambda::Attribute(Lambda::Extern), "f");
        auto gfn = w.fn_type({T});
        auto g = w.lambda(gfn, "g");
        g->jump(f, {g->param(0)});
        f->jump(g, {f->param(0)});
        thorin::emit_thorin(w, true, true);
        std::cout << "ffn" << std::endl;
        for (auto tv : ffn->free_type_vars())
            tv->dump();
        std::cout << "gfn" << std::endl;
        for (auto tv : gfn->free_type_vars())
            tv->dump();
        //auto f2 = w.fn_type({f1});
        //f2->bind(T);
#endif

        if (emit_all)
            emit_thorin = emit_looptree = emit_ast = emit_annotated = emit_llvm = true;
        opt |= emit_llvm;

        if (infiles.empty() && !help) {
            std::cerr << "no input files" << std::endl;
            return EXIT_FAILURE;
        }

        if (help) {
            std::cout << "Usage: " + prgname + " [options] file..." << std::endl;
            cmd_parser.print_help();
            return EXIT_SUCCESS;
        }

        std::string module_name;
        for (auto infile : infiles) {
            auto i = infile.find_last_of('.');
            if (infile.substr(i + 1) != "impala")
                throw logic_error("input file '" + infile + "' does not have '.impala' extension");
            auto rest = infile.substr(0, i);
            if (rest.empty())
                throw logic_error("input file '" + infile + "' has empty module name");
            module_name = rest;
        }

        impala::Init init(module_name);

#ifndef NDEBUG
        for (auto b : breakpoints) {
            assert(b.size() > 0);
            size_t num = 0;
            for (size_t i = 0, e = b.size(); i != e; ++i) {
                char c = b[i];
                if (!std::isdigit(c)) {
                    std::cerr << "invalid breakpoint '" << b << "'" << std::endl;
                    return EXIT_FAILURE;
                }
                num = num*10 + c - '0';
            }

            init.world.breakpoint(num);
        }
#endif

        //thorin::AutoPtr<impala::Scope> prg = new impala::Scope();
        //prg->set_loc(thorin::Location(infiles[0], 1, 1, 1, 1));

        bool result;
        thorin::AutoPtr<const impala::ModContents> prg;
        for (auto infile : infiles) {
            std::string filename = infile.c_str();
            ifstream file(filename);
            prg = impala::parse(result, file, filename);
            break;
        }

        if (emit_ast)
            impala::dump(prg, fancy);

        result &= check(init, prg, nossa);

        if (result && emit_annotated)
            impala::dump(prg, fancy);

        if (result && emit_thorin)
            emit(init.world, prg);

        if (result) {
            if (!nocleanup)
                init.world.cleanup();
            if (opt)
                init.world.opt();
            //if (vectorlength != 0) {
                //Lambda* impala_main = top_level_lambdas(init.world)[0];
                //Scope scope(impala_main);
                //thorin::vectorize(scope, vectorlength);
                //init.world.cleanup();
            //}
            if (emit_thorin)
                thorin::emit_thorin(init.world, fancy, !nocolor);
            if (emit_il)
                thorin::emit_il(init.world, fancy);
            //if (emit_looptree) {
                //for (auto top : top_level_lambdas(init.world)) {
                    //Scope scope(top);
                    //const LoopTree looptree(scope);
                    //std::cout << looptree.root() << std::endl; // TODO
                //}
            //}

            if (emit_llvm)
                thorin::emit_llvm(init.world);
        } else
            return EXIT_FAILURE;

        return EXIT_SUCCESS;
    } catch (exception const& e) {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    } catch (...) {
        cerr << "unknown exception" << endl;
        return EXIT_FAILURE;
    }
}
