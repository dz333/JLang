package jlang.ast;

import polyglot.ast.Expr;
import polyglot.ast.Stmt;

public interface SynchronizedExit extends Stmt {
    Expr expr();
}
