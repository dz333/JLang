package jlang.extension;

import jlang.ast.JLangExt;
import jlang.ast.SynchronizedEnter;
import jlang.util.Constants;
import jlang.visit.LLVMTranslator;
import org.bytedeco.javacpp.LLVM.LLVMTypeRef;
import org.bytedeco.javacpp.LLVM.LLVMValueRef;
import polyglot.ast.Node;

import static jlang.extension.JLangSynchronizedExt.buildMonitorFunc;

public class JLangSynchronizedEnterExt extends JLangExt {

    @Override
    public Node leaveTranslateLLVM(LLVMTranslator v) {
        SynchronizedEnter n = (SynchronizedEnter) node();

        LLVMValueRef obj = v.getTranslation(n.expr());

        buildMonitorFunc(v, Constants.MONITOR_ENTER, obj);

        return n;
    }
}
