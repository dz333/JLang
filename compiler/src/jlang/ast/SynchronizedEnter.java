package jlang.ast;

import polyglot.ast.Expr;
import polyglot.ast.Stmt;

public interface SynchronizedEnter extends Stmt {
    Expr expr();
}
