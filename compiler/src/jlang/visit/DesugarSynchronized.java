package jlang.visit;

import jlang.ast.JLangNodeFactory;
import jlang.ast.SynchronizedEnter;
import jlang.ast.SynchronizedExit;
import jlang.types.JLangTypeSystem;
import polyglot.ast.*;
import polyglot.frontend.Job;
import polyglot.types.SemanticException;
import polyglot.util.Position;

import java.util.Collections;

public class DesugarSynchronized extends DesugarVisitor {

    public DesugarSynchronized(Job job, JLangTypeSystem ts, JLangNodeFactory nf) {
        super(job, ts, nf);
    }

    @Override
    protected Node leaveDesugar(Node n) throws SemanticException {

        if (n instanceof Synchronized) {
            // Desugar
            // synchronized(o) {...}
            // To
            // {
            //   Object temp = o;
            //   try { synchronized_enter(o); ... }
            //   finally {synchronized_exit(o); }
            // }
            Synchronized node = (Synchronized) n;
            Position pos = node.position();
            // TempSSA or TempVar?
            LocalDecl declObj = tnf.TempSSA("syncObj", node.expr());

            SynchronizedEnter syncEnter = nf.SynchronizedEnter(pos, tnf.Local(pos, declObj));
            SynchronizedExit syncExit = nf.SynchronizedExit(pos, tnf.Local(pos, declObj));

            // Should we copy the node?
            Block tryBlock = nf.Block(pos, syncEnter, node.body());
            Block finallyBlock = nf.Block(pos, syncExit);

            Try tryFinally = nf.Try(pos, tryBlock, Collections.emptyList(), finallyBlock);

            Block outerBlock = nf.Block(pos, declObj, tryFinally);

            return outerBlock;
        }

        return n;

    }
}
