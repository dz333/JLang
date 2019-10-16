//Copyright (C) 2018 Cornell University

package jlang.ast;

import jlang.extension.*;
import polyglot.ast.Ext;
import polyglot.ast.ExtFactory;

public final class JLangExtFactory_c extends JLangAbstractExtFactory_c {

    public JLangExtFactory_c() {
        super();
    }

    @SuppressWarnings("unused")
    public JLangExtFactory_c(ExtFactory nextExtFactory) {
        super(nextExtFactory);
    }

    @Override
    protected Ext extTryWithResourcesImpl() {
        return new JLangTryWithResourcesExt();
    }

    @Override
    protected Ext extNodeImpl() {
        return new JLangExt();
    }

    @Override
    protected Ext extClassLitImpl() {
        return new JLangClassLitExt();
    }

    @Override
    protected Ext extESeqImpl() {
        return new JLangEseqExt();
    }

    @Override
    protected Ext extNormalAnnotationElemImpl() {
        return new JLangAnnotationElemExt();
    }

    @Override
    protected Ext extInstanceofImpl() {
        return new JLangInstanceofExt();
    }

    @Override
    protected Ext extIntLitImpl() {
        return new JLangIntLitExt();
    }

    @Override
    protected Ext extLocalDeclImpl() {
        return new JLangLocalDeclExt();
    }

    @Override
    protected Ext extAssignImpl() {
        return new JLangAssignExt();
    }

    @Override
    protected Ext extBinaryImpl() {
        return new JLangBinaryExt();
    }

    @Override
    protected Ext extExtendedForImpl() {
        return new JLangExtendedForExt();
    }

    @Override
    protected Ext extLocalImpl() {
        return new JLangLocalExt();
    }

    @Override
    protected Ext extAssertImpl() {
        return new JLangAssertExt();
    }

    @Override
    protected Ext extReturnImpl() {
        return new JLangReturnExt();
    }

    @Override
    protected Ext extSourceFileImpl() {
        return new JLangSourceFileExt();
    }

    @Override
    protected Ext extClassDeclImpl() {
        return new JLangClassDeclExt();
    }

    @Override
    protected Ext extCallImpl() {
        return new JLangCallExt();
    }

    @Override
    protected Ext extIfImpl() {
        return new JLangIfExt();
    }

    @Override
    public Ext extLoadImpl() {
        return new JLangLoadExt();
    }

    @Override
    protected Ext extBooleanLitImpl() {
        return new JLangBooleanLitExt();
    }

    @Override
    protected Ext extSwitchImpl() {
        return new JLangSwitchExt();
    }

    @Override
    protected Ext extWhileImpl() {
        return new JLangWhileExt();
    }

    @Override
    protected Ext extUnaryImpl() {
        return new JLangUnaryExt();
    }

    @Override
    protected Ext extCharLitImpl() {
        return new JLangCharLitExt();
    }

    @Override
    protected Ext extCastImpl() {
        return new JLangCastExt();
    }

    @Override
    public Ext extAddressOfImpl() {
        return new JLangAddressOfExt();
    }

    @Override
    protected Ext extEnumConstantDeclImpl() {
        return new JLangDesugaredNodeExt();
    }

    @Override
    protected Ext extBranchImpl() {
        return new JLangBranchExt();
    }

    @Override
    protected Ext extLabeledImpl() {
        return new JLangLabeledExt();
    }

    @Override
    protected Ext extFloatLitImpl() {
        return new JLangFloatLitExt();
    }

    @Override
    protected Ext extNullLitImpl() {
        return new JLangNullLitExt();
    }

    @Override
    protected Ext extNewImpl() {
        return new JLangNewExt();
    }

    @Override
    protected Ext extFieldImpl() {
        return new JLangFieldExt();
    }

    @Override
    protected Ext extFieldDeclImpl() {
        return new JLangFieldDeclExt();
    }

    @Override
    protected Ext extSpecialImpl() {
        return new JLangSpecialExt();
    }

    @Override
    protected Ext extNewArrayImpl() {
        return new JLangNewArrayExt();
    }

    @Override
    protected Ext extProcedureDeclImpl() {
        return new JLangProcedureDeclExt();
    }

    @Override
    protected Ext extConstructorCallImpl() {
        return new JLangConstructorCallExt();
    }

    @Override
    protected Ext extArrayAccessImpl() {
        return new JLangArrayAccessExt();
    }

    @Override
    protected Ext extInitializerImpl() {
        return new JLangInitializerExt();
    }

    @Override
    protected Ext extArrayInitImpl() {
        return new JLangArrayInitExt();
    }

    @Override
    protected Ext extDoImpl() {
        return new JLangDoExt();
    }

    @Override
    protected Ext extForImpl() {
        return new JLangForExt();
    }

    @Override
    protected Ext extConditionalImpl() {
        return new JLangConditionalExt();
    }

    @Override
    protected Ext extTryImpl() {
        return new JLangTryExt();
    }

    @Override
    protected Ext extThrowImpl() {
        return new JLangThrowExt();
    }

    @Override
    protected Ext extSynchronizedImpl() {
        return new JLangSynchronizedExt();
    }

    @Override
    public Ext extSynchronizedEnterImpl() {
        return new JLangSynchronizedEnterExt();
    }

    @Override
    public Ext extSynchronizedExitImpl() {
        return new JLangSynchronizedExitExt();
    }

    @Override
    protected Ext extStringLitImpl() {
        return new JLangStringLitExt();
    }
}
