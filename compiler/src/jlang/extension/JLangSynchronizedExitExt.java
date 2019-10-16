package jlang.extension;

import jlang.ast.JLangExt;
import jlang.visit.LLVMTranslator;
import polyglot.ast.Node;

public class JLangSynchronizedExitExt extends JLangExt {

    @Override
    public Node leaveTranslateLLVM(LLVMTranslator v) {
        return super.leaveTranslateLLVM(v);
    }
}
