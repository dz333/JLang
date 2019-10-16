package jlang.ast;

import polyglot.ast.*;
import polyglot.types.SemanticException;
import polyglot.util.Position;
import polyglot.visit.CFGBuilder;
import polyglot.visit.NodeVisitor;
import polyglot.visit.TypeChecker;

import java.util.List;

public abstract class SynchronizedChange extends Stmt_c {
    protected Expr expr;

    public SynchronizedChange(Position pos, Expr expr, Ext ext) {
        super(pos, ext);
        this.expr = expr;
    }

    @Override
    public Node typeCheck(TypeChecker tc) throws SemanticException {
        if (!expr.type().isReference()) {
            throw new SemanticException("Cannot synchronize on an expression of type \""
                    + expr.type() + "\".",
                    expr.position());
        }

        return this;
    }

    public Expr expr() {
        return expr;
    }

    protected <N extends SynchronizedChange> N expr(N n, Expr expr) {
        if (n.expr == expr) return n;
        n = copyIfNeeded(n);
        n.expr = expr;
        return n;
    }

    protected <N extends SynchronizedChange> N reconstruct(N n, Expr expr) {
        return expr(n, expr);
    }

    @Override
    public Node visitChildren(NodeVisitor v) {
        Expr expr = visitChild(this.expr, v);
        return reconstruct(this, expr);
    }

    @Override
    public Term firstChild() {
        return expr;
    }

    @Override
    public <T> List<T> acceptCFG(CFGBuilder<?> v, List<T> succs) {
        v.visitCFG(expr, this, EXIT);
        return succs;
    }
}
