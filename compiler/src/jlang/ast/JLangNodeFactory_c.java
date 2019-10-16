//Copyright (C) 2018 Cornell University

package jlang.ast;

import polyglot.ast.*;
import polyglot.ext.jl7.ast.JL7NodeFactory_c;
import polyglot.util.Position;

import java.util.List;

/** NodeFactory for JLang. */
public class JLangNodeFactory_c extends JL7NodeFactory_c implements JLangNodeFactory {

    public JLangNodeFactory_c(JLangLang lang, JLangExtFactory extFactory) {
        super(lang, extFactory);
    }

    @Override
    public ESeq ESeq(Position pos, List<Stmt> statements, Expr expr) {
        return new ESeq_c(pos, statements, expr, extFactory().extESeq());
    }

    @Override
    public AddressOf AddressOf(Position pos, Expr expr) {
        return new AddressOf_c(pos, expr, extFactory().extAddressOf());
    }

    @Override
    public Load Load(Position pos, Expr expr) {
        return new Load_c(pos, expr, extFactory().extLoad());
    }

    @Override
    public SynchronizedEnter SynchronizedEnter(Position pos, Expr expr) {
        return new SynchronizedEnter_c(pos, expr, extFactory().extSynchronizedEnter());
    }

    @Override
    public SynchronizedExit SynchronizedExit(Position pos, Expr expr) {
        return new SynchronizedExit_c(pos, expr, extFactory().extSynchronizedExit());
    }

    @Override
    public JLangExtFactory extFactory() {
        return (JLangExtFactory) super.extFactory();
    }
}
