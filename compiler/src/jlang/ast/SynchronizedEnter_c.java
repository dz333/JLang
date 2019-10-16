package jlang.ast;

import polyglot.ast.*;
import polyglot.util.CodeWriter;
import polyglot.util.Position;
import polyglot.visit.PrettyPrinter;

public class SynchronizedEnter_c extends SynchronizedChange implements SynchronizedEnter {

    SynchronizedEnter_c(Position pos, Expr expr, Ext ext) {
        super(pos, expr, ext);
    }

    protected <N extends SynchronizedEnter_c> N expr(N n, Expr expr) {
        return super.expr(n, expr);
    }

    @Override
    public void prettyPrint(CodeWriter w, PrettyPrinter pp) {
        w.write("synchronized_enter(");
        expr.prettyPrint(w, pp);
        w.write(")");
    }

    @Override
    public String toString() {
        return "synchronized_enter(" + expr.toString() + ");";
    }
}
