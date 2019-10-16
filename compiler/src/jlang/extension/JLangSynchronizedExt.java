//Copyright (C) 2018 Cornell University

package jlang.extension;

import jlang.ast.JLangExt;
import jlang.visit.LLVMTranslator;
import org.bytedeco.javacpp.LLVM.LLVMTypeRef;
import org.bytedeco.javacpp.LLVM.LLVMValueRef;
import polyglot.ast.Node;

import static jlang.util.Constants.JNI_ENV_VAR_NAME;
import static org.bytedeco.javacpp.LLVM.LLVMBuildBitCast;
import static org.bytedeco.javacpp.LLVM.LLVMTypeOf;

public class JLangSynchronizedExt extends JLangExt {
    private static boolean printedWarning = false;

    @Override
    public Node leaveTranslateLLVM(LLVMTranslator v) {
        if (!printedWarning) {
            System.err.println("WARNING: synchronized keywords should have already been desugared.");
            printedWarning = true;
        }
        return super.leaveTranslateLLVM(v);
    }

    public static void buildMonitorFunc(LLVMTranslator v, String op, LLVMValueRef syncObj) {
        LLVMValueRef cast = LLVMBuildBitCast(v.builder, syncObj, v.utils.toLL(v.ts.Object()), "cast_l");
        LLVMTypeRef monitorFuncType = v.utils.functionType(
                v.utils.voidType(),
                v.utils.ptrTypeRef(v.utils.jniEnvType()),
                v.utils.toLL(v.ts.Object())
        );
        LLVMValueRef monitorFunc = v.utils.getFunction(op, monitorFuncType);
        LLVMValueRef env = v.utils.getGlobal(JNI_ENV_VAR_NAME, v.utils.jniEnvType());
        v.utils.buildProcCall(monitorFunc, env, cast);
    }
}
