package jlang.ast;

import polyglot.ast.Expr;
import polyglot.ast.Ext;
import polyglot.util.CodeWriter;
import polyglot.util.Position;
import polyglot.visit.PrettyPrinter;

public class SynchronizedExit_c extends SynchronizedChange implements SynchronizedExit {

    SynchronizedExit_c(Position pos, Expr expr, Ext ext) {
        super(pos, expr, ext);
    }

    @Override
    public void prettyPrint(CodeWriter w, PrettyPrinter pp) {
        w.write("synchronized_exit(");
        expr.prettyPrint(w, pp);
        w.write(")");
    }

    @Override
    public String toString() {
        return "synchronized_exit(" + expr.toString() + ");";
    }
}
