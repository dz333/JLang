//Copyright (C) 2018 Cornell University

package jlang.ast;

import polyglot.ast.Expr;
import polyglot.ast.Stmt;
import polyglot.ext.jl7.ast.JL7NodeFactory;
import polyglot.util.Position;

import java.util.List;

/** NodeFactory for JLang. */
public interface JLangNodeFactory extends JL7NodeFactory {

    ESeq ESeq(Position pos, List<Stmt> statements, Expr expr);

    AddressOf AddressOf(Position pos, Expr expr);

    Load Load(Position pos, Expr expr);

    SynchronizedEnter SynchronizedEnter(Position pos, Expr expr);

    SynchronizedExit SynchronizedExit(Position pos, Expr expr);
}
