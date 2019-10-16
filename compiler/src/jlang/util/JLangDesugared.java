//Copyright (C) 2018 Cornell University

package jlang.util;

import polyglot.frontend.*;
import polyglot.frontend.goals.AbstractGoal;
import polyglot.frontend.goals.Goal;
import polyglot.frontend.goals.VisitorGoal;
import polyglot.types.ParsedClassType;
import polyglot.types.SemanticException;
import polyglot.util.InternalCompilerError;

import java.util.stream.Stream;

import jlang.ast.JLangNodeFactory;
import jlang.types.JLangTypeSystem;
import jlang.visit.*;

/**
 * Runs all JLang desugar passes so that the AST is
 * fully desugared and ready for emitting LLVM IR.
 */
public class JLangDesugared extends AbstractGoal {
    private boolean attempted = false;
    private final Goal[] goals;

    public JLangDesugared(Job job) {
        super(job);

        ExtensionInfo extInfo = job.extensionInfo();
        JLangTypeSystem ts = (JLangTypeSystem) extInfo.typeSystem();
        JLangNodeFactory nf = (JLangNodeFactory) extInfo.nodeFactory();

        goals = new Goal[] {
                // Visitor passes running after type checking must preserve type information.
                // However, running type check again after these passes can fail, for example
                // because the type checker could complain about visibility issues. That's ok.

                // Future desugar passes assume that anonymous classes have constructors and names.
                new VisitorGoal(job, new NameAnonClasses(job, ts, nf)),
                new VisitorGoal(job, new DeclareExplicitAnonCtors(job, ts, nf)),

                // Translate enums to normal classes.
                new VisitorGoal(job, new DesugarEnums(job, ts, nf)),

                // Translate captures to field accesses.
                new DesugarLocalClasses(job, ts, nf),

                // Translate accesses to enclosing instances. Future desugar passes
                // should not create qualified Special nodes.
                new DesugarInnerClasses(job, ts, nf),

                // Future desugar passes assume that instance initialization code
                // is called at the beginning of each constructor.
                new VisitorGoal(job, new DesugarInstanceInitializers(job, ts, nf)),

                new VisitorGoal(job, new DesugarSynchronized(job, ts, nf)),

                // Local desugar transformations.
                // It's usually unsafe to run a pass after this one, since
                // additional passes might create new non-desugared nodes.
                new VisitorGoal(job, new DesugarLocally(job, ts, nf)),
        };
    }

    /**
     * List of types needed by desugar transformations.
     * Used to find missing dependencies ahead-of-time.
     */
    private static final String[] neededTypes = {
            "java.lang.ArrayIndexOutOfBoundsException",
            "java.lang.AssertionError",
            "java.lang.Class",
            "java.lang.ClassCastException",
            "java.lang.Error",
            "java.lang.Exception",
            "java.lang.IndexOutOfBoundsException",
            "java.lang.Iterable",
            "java.lang.NullPointerException",
            "java.lang.String",
            "java.lang.Throwable",
            "java.util.Iterator",
            "java.lang.Integer",
            Constants.RUNTIME_ARRAY,
            Constants.RUNTIME_HELPER
    };

    @Override
    public Pass createPass(ExtensionInfo extInfo) {

        Pass[] passes = Stream.of(goals)
                .map(g -> g.createPass(extInfo))
                .toArray(Pass[]::new);


        return new AbstractPass(this) {

            @Override
            public boolean run() {

                // Eagerly query the members for types needed by desugar transformations.
                // This catches missing dependency exceptions ahead-of-time.
                for (String t : neededTypes) {
                    try {
                        ParsedClassType ct = (ParsedClassType) extInfo.typeSystem().typeForName(t);
                        ct.members(); // Force missing dependency exceptions.
                    }
                    catch (SemanticException e) {
                        throw new InternalCompilerError(e);
                    }
                }

                // Make sure we have not already run the desugar passes.
                if (attempted)
                    throw new InternalCompilerError(
                            "Desugaring passes should only be run once per job.\n" +
                                    "They are not idempotent!\n" + job.toString());
                attempted = true;

                // Run all sub-passes in sequence.
                for (Pass p : passes) {
                    try {
                        if (!p.run()) {
                            throw new InternalCompilerError("Desugar pass did not succeed: " + p);
                        }
                    } catch (MissingDependencyException e) {
                        // Intercept missing dependency exceptions, since these would
                        // lead to rerunning all desugar passes.
                        throw new InternalCompilerError(
                                "Missing dependency while running desugar pass: " + p + "." +
                                        "\nMay need to add the dependency explicitly.", e);
                    }
                }

                return true;
            }
        };
    }
}
