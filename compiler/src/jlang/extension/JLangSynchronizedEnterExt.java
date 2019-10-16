package jlang.extension;

import jlang.ast.JLangExt;
import jlang.visit.LLVMTranslator;
import polyglot.ast.Node;

public class JLangSynchronizedEnterExt extends JLangExt {

    @Override
    public Node leaveTranslateLLVM(LLVMTranslator v) {
        return super.leaveTranslateLLVM(v);
    }
}
