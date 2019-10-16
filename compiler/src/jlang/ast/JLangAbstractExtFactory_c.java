//Copyright (C) 2018 Cornell University

package jlang.ast;

import jlang.extension.JLangSynchronizedEnterExt;
import jlang.extension.JLangSynchronizedExitExt;
import polyglot.ast.Ext;
import polyglot.ast.ExtFactory;
import polyglot.ext.jl7.ast.JL7AbstractExtFactory_c;

public abstract class JLangAbstractExtFactory_c
        extends JL7AbstractExtFactory_c
        implements JLangExtFactory {

    public JLangAbstractExtFactory_c() {
        super();
    }

    public JLangAbstractExtFactory_c(ExtFactory nextExtFactory) {
        super(nextExtFactory);
    }

    @Override
    public Ext extESeq() {
        Ext e = extESeqImpl();

        if (nextExtFactory() != null) {
            Ext e2;
            if (nextExtFactory() instanceof JLangExtFactory) {
                e2 = ((JLangExtFactory) nextExtFactory()).extESeq();
            }
            else {
                e2 = nextExtFactory().extExpr();
            }
            e = composeExts(e, e2);
        }
        return postExtESeq(e);
    }

    @Override
    public Ext extAddressOf() {
        Ext e = extAddressOfImpl();

        if (nextExtFactory() != null) {
            Ext e2;
            if (nextExtFactory() instanceof JLangExtFactory) {
                e2 = ((JLangExtFactory) nextExtFactory()).extAddressOf();
            }
            else {
                e2 = nextExtFactory().extExpr();
            }
            e = composeExts(e, e2);
        }
        return postExtAddressOf(e);
    }

    @Override
    public Ext extLoad() {
        Ext e = extLoadImpl();

        if (nextExtFactory() != null) {
            Ext e2;
            if (nextExtFactory() instanceof JLangExtFactory) {
                e2 = ((JLangExtFactory) nextExtFactory()).extLoad();
            }
            else {
                e2 = nextExtFactory().extExpr();
            }
            e = composeExts(e, e2);
        }
        return postExtLoad(e);
    }

    @Override
    public Ext extSynchronizedEnter() {
        Ext e = extSynchronizedEnterImpl();

        if (nextExtFactory() != null) {
            Ext e2;
            if (nextExtFactory() instanceof JLangExtFactory) {
                e2 = ((JLangExtFactory) nextExtFactory()).extSynchronizedEnter();
            }
            else {
                e2 = nextExtFactory().extStmt();
            }
            e = composeExts(e, e2);
        }
        return postExtSynchronizedEnter(e);
    }

    @Override
    public Ext extSynchronizedExit() {
        Ext e = extSynchronizedExitImpl();

        if (nextExtFactory() != null) {
            Ext e2;
            if (nextExtFactory() instanceof JLangExtFactory) {
                e2 = ((JLangExtFactory) nextExtFactory()).extSynchronizedExit();
            }
            else {
                e2 = nextExtFactory().extStmt();
            }
            e = composeExts(e, e2);
        }
        return postExtSynchronizedExit(e);
    }

    protected Ext extAddressOfImpl() {
        return extExprImpl();
    }

    protected Ext extESeqImpl() {
        return extExprImpl();
    }
    protected Ext extLoadImpl() {
        return extExprImpl();
    }

    protected Ext extSynchronizedEnterImpl() {
        return extStmtImpl();
    }

    protected Ext extSynchronizedExitImpl() {
        return extStmtImpl();
    }

    protected Ext postExtAddressOf(Ext e) {
        return postExtExpr(e);
    }

    protected Ext postExtESeq(Ext e) {
        return postExtExpr(e);
    }

    protected Ext postExtLoad(Ext e) {
        return postExtExpr(e);
    }

    protected Ext postExtSynchronizedEnter(Ext e) {
        return postExtStmt(e);
    }

    protected Ext postExtSynchronizedExit(Ext e) {
        return postExtStmt(e);
    }
}
