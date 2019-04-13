# `make all` creates solution files, llvm ir, and output files. Add the -k flag
#            to make all targets regardless of errors.
# `make <file>` to produce any individual generated file, such as .ll (LLVM IR),
#               .binary (executable), etc.
# `make sol` to generate solution (.sol) files.
# `make ir` to compile all programs down to LLVM IR (.ll) files.
# `make output` to compile LLVM IR to executable files and produce program
#               output (.output) files.

# Uses absolute paths to llvm commands, because Java's ProcessBuilder will
# otherwise not be able to find them.
BASE_DIR := ../..
RUNTIME := $(BASE_DIR)/runtime

ifndef TOP_LEVEL_MAKEFILE_INVOKED
JDK ?= jdk-lite
JDK := $(realpath $(BASE_DIR)/$(JDK))
JDK_CLASSES := $(realpath $(JDK)/out/classes)
JDK7 ?= /Library/Java/JavaVirtualMachines/jdk1.7.0_80.jdk/Contents/Home/
endif

# Setup variables for travis CI
ifdef TRAVIS
ADDITIONAL_JLANGC_OPTIONS:=-compiler-classpath $(realpath $(BASE_DIR)/lib/llvm-linux-x86_64-Ubuntu12.04.jar)
endif

SRC := $(shell find * -name "*.java")

CLASSES := $(SRC:.java=.class)
SOL := $(SRC:.java=.sol)
LL := $(SRC:.java=.ll)
BINARY := $(SRC:.java=.binary)
OUTPUT := $(SRC:.java=.output)

ifndef CLANG
CLANG := clang++
endif

CLANG_FLAGS := \
	-Wno-override-module -lgc -g \
	-L$(RUNTIME)/out -ljvm \
	-L$(JDK)/out -ljdk \
	-Wl,-rpath,$(JDK7_LIB_PATH) \
	-Wl,-rpath,$(JDK)/out -Wl,-rpath,$(RUNTIME)/out \
	-rdynamic

all: check

check: sol output
	@echo "******** CHECKING CORRECTNESS *********"
	@./checkDiffs.sh $(realpath expected_fails)


sol: $(SOL)

ir: $(LL)

ll: $(LL)

binary: $(BINARY)

output: $(OUTPUT)

# Compiling Java with a Makefile is never ideal, but this is sufficient.
# Recall that $? expands to all dependencies newer than the target.
$(SOL): $(SRC)
	@echo "Compiling $(words $?) test(s) with javac"
	@javac $?
	@echo "Generating solutions for $(words $?) test(s)"
	@for f in $?; do \
		java -ea -cp . $(subst /,.,$${f%.java}) > $${f%.java}.sol; \
	done
	@touch $(SOL)

$(LL): $(SRC)
	@echo "Compiling $(words $?) Java file(s) down to LLVM IR"
	@$(BASE_DIR)/bin/jlangc $(ADDITIONAL_JLANGC_OPTIONS) -cp $(JDK_CLASSES) -assert $?
	@touch $(LL)

%.ll: %.java
	@echo "Compiling file to LLVM IR"
	@$(BASE_DIR)/bin/jlangc $(ADDITIONAL_JLANGC_OPTIONS) -cp $(JDK_CLASSES) -assert $?

%.binary: %.ll
	@echo "Creating binary for $<"
	@$(CLANG) $(CLANG_FLAGS) -o $@ $<

%.output: %.binary phony
	@echo "Generating output for $*"
	@JAVA_HOME=$(JDK7) bash -c './$<' | tee >/dev/null $@

%.debug: %.binary phony
	@JAVA_HOME=$(JDK7) bash -c 'gdb $<'

clean: phony
	@echo "Removing files generated for Unit Tests"
	@rm -rf *.dSYM *.sol *.ll *.binary *.output *.class check

.PHONY: phony
