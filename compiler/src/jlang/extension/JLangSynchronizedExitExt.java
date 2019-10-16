package jlang.extension;

import jlang.ast.JLangExt;
import jlang.ast.SynchronizedEnter;
import jlang.ast.SynchronizedExit;
import jlang.util.Constants;
import jlang.visit.LLVMTranslator;
import org.bytedeco.javacpp.LLVM.LLVMValueRef;
import polyglot.ast.Node;

import static jlang.extension.JLangSynchronizedExt.buildMonitorFunc;

public class JLangSynchronizedExitExt extends JLangExt {

    @Override
    public Node leaveTranslateLLVM(LLVMTranslator v) {
        SynchronizedExit n = (SynchronizedExit) node();

        LLVMValueRef obj = v.getTranslation(n.expr());

        buildMonitorFunc(v, Constants.MONITOR_EXIT, obj);

        return n;
    }
}
