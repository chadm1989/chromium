/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JIT.h"

#if ENABLE(JIT)

#include "CodeBlock.h"
#include "JITInlineMethods.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "Interpreter.h"
#include "ResultType.h"
#include "SamplingTool.h"

#ifndef NDEBUG
#include <stdio.h>
#endif

using namespace std;

namespace JSC {

#if COMPILER(GCC) && PLATFORM(X86)

COMPILE_ASSERT(STUB_ARGS_code == 0x0C, STUB_ARGS_code_is_0x0C);
COMPILE_ASSERT(STUB_ARGS_callFrame == 0x0E, STUB_ARGS_callFrame_is_0x0E);

#if PLATFORM(DARWIN)
#define SYMBOL_STRING(name) "_" #name
#else
#define SYMBOL_STRING(name) #name
#endif

asm(
".globl " SYMBOL_STRING(ctiTrampoline) "\n"
SYMBOL_STRING(ctiTrampoline) ":" "\n"
    "pushl %ebp" "\n"
    "movl %esp, %ebp" "\n"
    "pushl %esi" "\n"
    "pushl %edi" "\n"
    "pushl %ebx" "\n"
    "subl $0x1c, %esp" "\n"
    "movl $512, %esi" "\n"
    "movl 0x38(%esp), %edi" "\n" // Ox38 = 0x0E * 4, 0x0E = STUB_ARGS_callFrame (see assertion above)
    "call *0x30(%esp)" "\n" // Ox30 = 0x0C * 4, 0x0C = STUB_ARGS_code (see assertion above)
    "addl $0x1c, %esp" "\n"
    "popl %ebx" "\n"
    "popl %edi" "\n"
    "popl %esi" "\n"
    "popl %ebp" "\n"
    "ret" "\n"
);

asm(
".globl " SYMBOL_STRING(ctiVMThrowTrampoline) "\n"
SYMBOL_STRING(ctiVMThrowTrampoline) ":" "\n"
#if USE(JIT_STUB_ARGUMENT_VA_LIST)
    "call " SYMBOL_STRING(_ZN3JSC8JITStubs12cti_vm_throwEPvz) "\n"
#else
#if USE(JIT_STUB_ARGUMENT_REGISTER)
    "movl %esp, %ecx" "\n"
#else // JIT_STUB_ARGUMENT_STACK
    "movl %esp, 0(%esp)" "\n"
#endif
    "call " SYMBOL_STRING(_ZN3JSC8JITStubs12cti_vm_throwEPPv) "\n"
#endif
    "addl $0x1c, %esp" "\n"
    "popl %ebx" "\n"
    "popl %edi" "\n"
    "popl %esi" "\n"
    "popl %ebp" "\n"
    "ret" "\n"
);
    
#elif COMPILER(GCC) && PLATFORM(X86_64)

COMPILE_ASSERT(STUB_ARGS_code == 0x10, STUB_ARGS_code_is_0x10);
COMPILE_ASSERT(STUB_ARGS_callFrame == 0x12, STUB_ARGS_callFrame_is_0x12);

#if PLATFORM(DARWIN)
#define SYMBOL_STRING(name) "_" #name
#else
#define SYMBOL_STRING(name) #name
#endif

asm(
".globl " SYMBOL_STRING(ctiTrampoline) "\n"
SYMBOL_STRING(ctiTrampoline) ":" "\n"
    "pushq %rbp" "\n"
    "movq %rsp, %rbp" "\n"
    "pushq %r12" "\n"
    "pushq %r13" "\n"
    "pushq %r14" "\n"
    "pushq %r15" "\n"
    "pushq %rbx" "\n"
    "subq $0x48, %rsp" "\n"
    "movq $512, %r12" "\n"
    "movq $0xFFFF000000000000, %r14" "\n"
    "movq $0xFFFF000000000002, %r15" "\n"
    "movq 0x90(%rsp), %r13" "\n" // Ox90 = 0x12 * 8, 0x12 = STUB_ARGS_callFrame (see assertion above)
    "call *0x80(%rsp)" "\n" // Ox80 = 0x10 * 8, 0x10 = STUB_ARGS_code (see assertion above)
    "addq $0x48, %rsp" "\n"
    "popq %rbx" "\n"
    "popq %r15" "\n"
    "popq %r14" "\n"
    "popq %r13" "\n"
    "popq %r12" "\n"
    "popq %rbp" "\n"
    "ret" "\n"
);

asm(
".globl " SYMBOL_STRING(ctiVMThrowTrampoline) "\n"
SYMBOL_STRING(ctiVMThrowTrampoline) ":" "\n"
#if USE(JIT_STUB_ARGUMENT_REGISTER)
    "movq %rsp, %rdi" "\n"
    "call " SYMBOL_STRING(_ZN3JSC8JITStubs12cti_vm_throwEPPv) "\n"
#else // JIT_STUB_ARGUMENT_VA_LIST or JIT_STUB_ARGUMENT_STACK
#error "JIT_STUB_ARGUMENT configuration not supported."
#endif
    "addq $0x48, %rsp" "\n"
    "popq %rbx" "\n"
    "popq %r15" "\n"
    "popq %r14" "\n"
    "popq %r13" "\n"
    "popq %r12" "\n"
    "popq %rbp" "\n"
    "ret" "\n"
);
    
#elif COMPILER(MSVC)

extern "C" {
    
    __declspec(naked) JSValueEncodedAsPointer* ctiTrampoline(void* code, RegisterFile*, CallFrame*, JSValuePtr* exception, Profiler**, JSGlobalData*)
    {
        __asm {
            push ebp;
            mov ebp, esp;
            push esi;
            push edi;
            push ebx;
            sub esp, 0x1c;
            mov esi, 512;
            mov ecx, esp;
            mov edi, [esp + 0x38];
            call [esp + 0x30]; // Ox30 = 0x0C * 4, 0x0C = STUB_ARGS_code (see assertion above)
            add esp, 0x1c;
            pop ebx;
            pop edi;
            pop esi;
            pop ebp;
            ret;
        }
    }
    
    __declspec(naked) void ctiVMThrowTrampoline()
    {
        __asm {
#if USE(JIT_STUB_ARGUMENT_REGISTER)
            mov ecx, esp;
#else // JIT_STUB_ARGUMENT_VA_LIST or JIT_STUB_ARGUMENT_STACK
#error "JIT_STUB_ARGUMENT configuration not supported."
#endif
            call JSC::JITStubs::cti_vm_throw;
            add esp, 0x1c;
            pop ebx;
            pop edi;
            pop esi;
            pop ebp;
            ret;
        }
    }
    
}

#endif

void ctiSetReturnAddress(void** addressOfReturnAddress, void* newDestinationToReturnTo)
{
    *addressOfReturnAddress = newDestinationToReturnTo;
}

void ctiPatchCallByReturnAddress(MacroAssembler::ProcessorReturnAddress returnAddress, void* newCalleeFunction)
{
    returnAddress.relinkCallerToFunction(newCalleeFunction);
}

void ctiPatchNearCallByReturnAddress(MacroAssembler::ProcessorReturnAddress returnAddress, void* newCalleeFunction)
{
    returnAddress.relinkNearCallerToFunction(newCalleeFunction);
}

JIT::JIT(JSGlobalData* globalData, CodeBlock* codeBlock)
    : m_interpreter(globalData->interpreter)
    , m_globalData(globalData)
    , m_codeBlock(codeBlock)
    , m_labels(codeBlock ? codeBlock->instructions().size() : 0)
    , m_propertyAccessCompilationInfo(codeBlock ? codeBlock->numberOfStructureStubInfos() : 0)
    , m_callStructureStubCompilationInfo(codeBlock ? codeBlock->numberOfCallLinkInfos() : 0)
    , m_lastResultBytecodeRegister(std::numeric_limits<int>::max())
    , m_jumpTargetsPosition(0)
{
}

void JIT::compileOpStrictEq(Instruction* currentInstruction, CompileOpStrictEqType type)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned src1 = currentInstruction[2].u.operand;
    unsigned src2 = currentInstruction[3].u.operand;

    emitGetVirtualRegisters(src1, regT0, src2, regT1);

#if USE(ALTERNATE_JSIMMEDIATE)
    // Jump to a slow case if either operand is a number, or if both are JSCell*s.
    move(regT0, regT2);
    orPtr(regT1, regT2);
    addSlowCase(emitJumpIfJSCell(regT2));
    addSlowCase(emitJumpIfImmediateNumber(regT2));

    if (type == OpStrictEq)
        set32(Equal, regT1, regT0, regT0);
    else
        set32(NotEqual, regT1, regT0, regT0);
    emitTagAsBoolImmediate(regT0);
#else
    bool negated = (type == OpNStrictEq);

    // Check that both are immediates, if so check if they're equal
    Jump firstNotImmediate = emitJumpIfJSCell(regT0);
    Jump secondNotImmediate = emitJumpIfJSCell(regT1);
    Jump bothWereImmediatesButNotEqual = branchPtr(NotEqual, regT1, regT0);

    // They are equal - set the result to true. (Or false, if negated).
    move(ImmPtr(JSValuePtr::encode(jsBoolean(!negated))), regT0);
    Jump bothWereImmediatesAndEqual = jump();

    // eax was not an immediate, we haven't yet checked edx.
    // If edx is also a JSCell, or is 0, then jump to a slow case,
    // otherwise these values are not equal.
    firstNotImmediate.link(this);
    emitJumpSlowCaseIfJSCell(regT1);
    addSlowCase(branchPtr(Equal, regT1, ImmPtr(JSValuePtr::encode(js0()))));
    Jump firstWasNotImmediate = jump();

    // eax was an immediate, but edx wasn't.
    // If eax is 0 jump to a slow case, otherwise these values are not equal.
    secondNotImmediate.link(this);
    addSlowCase(branchPtr(Equal, regT0, ImmPtr(JSValuePtr::encode(js0()))));

    // We get here if the two values are different immediates, or one is 0 and the other is a JSCell.
    // Vaelues are not equal, set the result to false.
    bothWereImmediatesButNotEqual.link(this);
    firstWasNotImmediate.link(this);
    move(ImmPtr(JSValuePtr::encode(jsBoolean(negated))), regT0);
    
    bothWereImmediatesAndEqual.link(this);
#endif

    emitPutVirtualRegister(dst);
}

void JIT::emitTimeoutCheck()
{
    Jump skipTimeout = branchSub32(NonZero, Imm32(1), timeoutCheckRegister);
    emitCTICall(JITStubs::cti_timeout_check);
    move(regT0, timeoutCheckRegister);
    skipTimeout.link(this);

    killLastResultRegister();
}


#define NEXT_OPCODE(name) \
    m_bytecodeIndex += OPCODE_LENGTH(name); \
    break;

#define CTI_COMPILE_BINARY_OP(name) \
    case name: { \
        emitPutJITStubArgFromVirtualRegister(currentInstruction[2].u.operand, 1, regT2); \
        emitPutJITStubArgFromVirtualRegister(currentInstruction[3].u.operand, 2, regT2); \
        emitCTICall(JITStubs::cti_##name); \
        emitPutVirtualRegister(currentInstruction[1].u.operand); \
        NEXT_OPCODE(name); \
    }

#define CTI_COMPILE_UNARY_OP(name) \
    case name: { \
        emitPutJITStubArgFromVirtualRegister(currentInstruction[2].u.operand, 1, regT2); \
        emitCTICall(JITStubs::cti_##name); \
        emitPutVirtualRegister(currentInstruction[1].u.operand); \
        NEXT_OPCODE(name); \
    }

void JIT::privateCompileMainPass()
{
    Instruction* instructionsBegin = m_codeBlock->instructions().begin();
    unsigned instructionCount = m_codeBlock->instructions().size();
    unsigned propertyAccessInstructionIndex = 0;
    unsigned globalResolveInfoIndex = 0;
    unsigned callLinkInfoIndex = 0;

    for (m_bytecodeIndex = 0; m_bytecodeIndex < instructionCount; ) {
        Instruction* currentInstruction = instructionsBegin + m_bytecodeIndex;
        ASSERT_WITH_MESSAGE(m_interpreter->isOpcode(currentInstruction->u.opcode), "privateCompileMainPass gone bad @ %d", m_bytecodeIndex);

#if ENABLE(OPCODE_SAMPLING)
        if (m_bytecodeIndex > 0) // Avoid the overhead of sampling op_enter twice.
            sampleInstruction(currentInstruction);
#endif

        m_labels[m_bytecodeIndex] = label();
        OpcodeID opcodeID = m_interpreter->getOpcodeID(currentInstruction->u.opcode);

        switch (opcodeID) {
        case op_mov: {
            emitGetVirtualRegister(currentInstruction[2].u.operand, regT0);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_mov);
        }
        case op_add: {
            compileFastArith_op_add(currentInstruction);
            NEXT_OPCODE(op_add);
        }
        case op_end: {
            if (m_codeBlock->needsFullScopeChain())
                emitCTICall(JITStubs::cti_op_end);
            ASSERT(returnValueRegister != callFrameRegister);
            emitGetVirtualRegister(currentInstruction[1].u.operand, returnValueRegister);
            push(Address(callFrameRegister, RegisterFile::ReturnPC * static_cast<int>(sizeof(Register))));
            ret();
            NEXT_OPCODE(op_end);
        }
        case op_jmp: {
            unsigned target = currentInstruction[1].u.operand;
            addJump(jump(), target + 1);
            NEXT_OPCODE(op_jmp);
        }
        case op_pre_inc: {
            compileFastArith_op_pre_inc(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_pre_inc);
        }
        case op_loop: {
            emitTimeoutCheck();

            unsigned target = currentInstruction[1].u.operand;
            addJump(jump(), target + 1);
            NEXT_OPCODE(op_end);
        }
        case op_loop_if_less: {
            emitTimeoutCheck();

            unsigned op1 = currentInstruction[1].u.operand;
            unsigned op2 = currentInstruction[2].u.operand;
            unsigned target = currentInstruction[3].u.operand;
            if (isOperandConstantImmediateInt(op2)) {
                emitGetVirtualRegister(op1, regT0);
                emitJumpSlowCaseIfNotImmediateInteger(regT0);
#if USE(ALTERNATE_JSIMMEDIATE)
                int32_t op2imm = getConstantOperandImmediateInt(op2);
#else
                int32_t op2imm = static_cast<int32_t>(JSImmediate::rawValue(getConstantOperand(op2)));
#endif
                addJump(branch32(LessThan, regT0, Imm32(op2imm)), target + 3);
            } else {
                emitGetVirtualRegisters(op1, regT0, op2, regT1);
                emitJumpSlowCaseIfNotImmediateInteger(regT0);
                emitJumpSlowCaseIfNotImmediateInteger(regT1);
                addJump(branch32(LessThan, regT0, regT1), target + 3);
            }
            NEXT_OPCODE(op_loop_if_less);
        }
        case op_loop_if_lesseq: {
            emitTimeoutCheck();

            unsigned op1 = currentInstruction[1].u.operand;
            unsigned op2 = currentInstruction[2].u.operand;
            unsigned target = currentInstruction[3].u.operand;
            if (isOperandConstantImmediateInt(op2)) {
                emitGetVirtualRegister(op1, regT0);
                emitJumpSlowCaseIfNotImmediateInteger(regT0);
#if USE(ALTERNATE_JSIMMEDIATE)
                int32_t op2imm = getConstantOperandImmediateInt(op2);
#else
                int32_t op2imm = static_cast<int32_t>(JSImmediate::rawValue(getConstantOperand(op2)));
#endif
                addJump(branch32(LessThanOrEqual, regT0, Imm32(op2imm)), target + 3);
            } else {
                emitGetVirtualRegisters(op1, regT0, op2, regT1);
                emitJumpSlowCaseIfNotImmediateInteger(regT0);
                emitJumpSlowCaseIfNotImmediateInteger(regT1);
                addJump(branch32(LessThanOrEqual, regT0, regT1), target + 3);
            }
            NEXT_OPCODE(op_loop_if_less);
        }
        case op_new_object: {
            emitCTICall(JITStubs::cti_op_new_object);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_new_object);
        }
        case op_put_by_id: {
            compilePutByIdHotPath(currentInstruction[1].u.operand, &(m_codeBlock->identifier(currentInstruction[2].u.operand)), currentInstruction[3].u.operand, propertyAccessInstructionIndex++);
            NEXT_OPCODE(op_put_by_id);
        }
        case op_get_by_id: {
            compileGetByIdHotPath(currentInstruction[1].u.operand, currentInstruction[2].u.operand, &(m_codeBlock->identifier(currentInstruction[3].u.operand)), propertyAccessInstructionIndex++);
            NEXT_OPCODE(op_get_by_id);
        }
        case op_instanceof: {
            emitGetVirtualRegister(currentInstruction[2].u.operand, regT0); // value
            emitGetVirtualRegister(currentInstruction[3].u.operand, regT2); // baseVal
            emitGetVirtualRegister(currentInstruction[4].u.operand, regT1); // proto

            // check if any are immediates
            move(regT0, regT3);
            orPtr(regT2, regT3);
            orPtr(regT1, regT3);
            emitJumpSlowCaseIfNotJSCell(regT3);

            // check that all are object type - this is a bit of a bithack to avoid excess branching;
            // we check that the sum of the three type codes from Structures is exactly 3 * ObjectType,
            // this works because NumberType and StringType are smaller
            move(Imm32(3 * ObjectType), regT3);
            loadPtr(Address(regT0, FIELD_OFFSET(JSCell, m_structure)), regT0);
            loadPtr(Address(regT2, FIELD_OFFSET(JSCell, m_structure)), regT2);
            loadPtr(Address(regT1, FIELD_OFFSET(JSCell, m_structure)), regT1);
            sub32(Address(regT0, FIELD_OFFSET(Structure, m_typeInfo.m_type)), regT3);
            sub32(Address(regT2, FIELD_OFFSET(Structure, m_typeInfo.m_type)), regT3);
            addSlowCase(branch32(NotEqual, Address(regT1, FIELD_OFFSET(Structure, m_typeInfo.m_type)), regT3));

            // check that baseVal's flags include ImplementsHasInstance but not OverridesHasInstance
            load32(Address(regT2, FIELD_OFFSET(Structure, m_typeInfo.m_flags)), regT2);
            and32(Imm32(ImplementsHasInstance | OverridesHasInstance), regT2);
            addSlowCase(branch32(NotEqual, regT2, Imm32(ImplementsHasInstance)));

            emitGetVirtualRegister(currentInstruction[2].u.operand, regT2); // reload value
            emitGetVirtualRegister(currentInstruction[4].u.operand, regT1); // reload proto

            // optimistically load true result
            move(ImmPtr(JSValuePtr::encode(jsBoolean(true))), regT0);

            Label loop(this);

            // load value's prototype
            loadPtr(Address(regT2, FIELD_OFFSET(JSCell, m_structure)), regT2);
            loadPtr(Address(regT2, FIELD_OFFSET(Structure, m_prototype)), regT2);

            Jump exit = branchPtr(Equal, regT2, regT1);

            branchPtr(NotEqual, regT2, ImmPtr(JSValuePtr::encode(jsNull())), loop);

            move(ImmPtr(JSValuePtr::encode(jsBoolean(false))), regT0);

            exit.link(this);

            emitPutVirtualRegister(currentInstruction[1].u.operand);

            NEXT_OPCODE(op_instanceof);
        }
        case op_del_by_id: {
            emitPutJITStubArgFromVirtualRegister(currentInstruction[2].u.operand, 1, regT2);
            Identifier* ident = &(m_codeBlock->identifier(currentInstruction[3].u.operand));
            emitPutJITStubArgConstant(ident, 2);
            emitCTICall(JITStubs::cti_op_del_by_id);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_del_by_id);
        }
        case op_mul: {
            compileFastArith_op_mul(currentInstruction);
            NEXT_OPCODE(op_mul);
        }
        case op_new_func: {
            FuncDeclNode* func = m_codeBlock->function(currentInstruction[2].u.operand);
            emitPutJITStubArgConstant(func, 1);
            emitCTICall(JITStubs::cti_op_new_func);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_new_func);
        }
        case op_call: {
            compileOpCall(opcodeID, currentInstruction, callLinkInfoIndex++);
            NEXT_OPCODE(op_call);
        }
        case op_call_eval: {
            compileOpCall(opcodeID, currentInstruction, callLinkInfoIndex++);
            NEXT_OPCODE(op_call_eval);
        }
        case op_construct: {
            compileOpCall(opcodeID, currentInstruction, callLinkInfoIndex++);
            NEXT_OPCODE(op_construct);
        }
        case op_get_global_var: {
            JSVariableObject* globalObject = static_cast<JSVariableObject*>(currentInstruction[2].u.jsCell);
            move(ImmPtr(globalObject), regT0);
            emitGetVariableObjectRegister(regT0, currentInstruction[3].u.operand, regT0);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_get_global_var);
        }
        case op_put_global_var: {
            emitGetVirtualRegister(currentInstruction[3].u.operand, regT1);
            JSVariableObject* globalObject = static_cast<JSVariableObject*>(currentInstruction[1].u.jsCell);
            move(ImmPtr(globalObject), regT0);
            emitPutVariableObjectRegister(regT1, regT0, currentInstruction[2].u.operand);
            NEXT_OPCODE(op_put_global_var);
        }
        case op_get_scoped_var: {
            int skip = currentInstruction[3].u.operand + m_codeBlock->needsFullScopeChain();

            emitGetFromCallFrameHeader(RegisterFile::ScopeChain, regT0);
            while (skip--)
                loadPtr(Address(regT0, FIELD_OFFSET(ScopeChainNode, next)), regT0);

            loadPtr(Address(regT0, FIELD_OFFSET(ScopeChainNode, object)), regT0);
            emitGetVariableObjectRegister(regT0, currentInstruction[2].u.operand, regT0);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_get_scoped_var);
        }
        case op_put_scoped_var: {
            int skip = currentInstruction[2].u.operand + m_codeBlock->needsFullScopeChain();

            emitGetFromCallFrameHeader(RegisterFile::ScopeChain, regT1);
            emitGetVirtualRegister(currentInstruction[3].u.operand, regT0);
            while (skip--)
                loadPtr(Address(regT1, FIELD_OFFSET(ScopeChainNode, next)), regT1);

            loadPtr(Address(regT1, FIELD_OFFSET(ScopeChainNode, object)), regT1);
            emitPutVariableObjectRegister(regT0, regT1, currentInstruction[1].u.operand);
            NEXT_OPCODE(op_put_scoped_var);
        }
        case op_tear_off_activation: {
            emitPutJITStubArgFromVirtualRegister(currentInstruction[1].u.operand, 1, regT2);
            emitCTICall(JITStubs::cti_op_tear_off_activation);
            NEXT_OPCODE(op_tear_off_activation);
        }
        case op_tear_off_arguments: {
            emitCTICall(JITStubs::cti_op_tear_off_arguments);
            NEXT_OPCODE(op_tear_off_arguments);
        }
        case op_ret: {
            // We could JIT generate the deref, only calling out to C when the refcount hits zero.
            if (m_codeBlock->needsFullScopeChain())
                emitCTICall(JITStubs::cti_op_ret_scopeChain);

            ASSERT(callFrameRegister != regT1);
            ASSERT(regT1 != returnValueRegister);
            ASSERT(returnValueRegister != callFrameRegister);

            // Return the result in %eax.
            emitGetVirtualRegister(currentInstruction[1].u.operand, returnValueRegister);

            // Grab the return address.
            emitGetFromCallFrameHeader(RegisterFile::ReturnPC, regT1);

            // Restore our caller's "r".
            emitGetFromCallFrameHeader(RegisterFile::CallerFrame, callFrameRegister);

            // Return.
            push(regT1);
            ret();

            NEXT_OPCODE(op_ret);
        }
        case op_new_array: {
            emitPutJITStubArgConstant(currentInstruction[2].u.operand, 1);
            emitPutJITStubArgConstant(currentInstruction[3].u.operand, 2);
            emitCTICall(JITStubs::cti_op_new_array);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_new_array);
        }
        case op_resolve: {
            Identifier* ident = &(m_codeBlock->identifier(currentInstruction[2].u.operand));
            emitPutJITStubArgConstant(ident, 1);
            emitCTICall(JITStubs::cti_op_resolve);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_resolve);
        }
        case op_construct_verify: {
            emitGetVirtualRegister(currentInstruction[1].u.operand, regT0);

            emitJumpSlowCaseIfNotJSCell(regT0);
            loadPtr(Address(regT0, FIELD_OFFSET(JSCell, m_structure)), regT2);
            addSlowCase(branch32(NotEqual, Address(regT2, FIELD_OFFSET(Structure, m_typeInfo) + FIELD_OFFSET(TypeInfo, m_type)), Imm32(ObjectType)));

            NEXT_OPCODE(op_construct_verify);
        }
        case op_get_by_val: {
            emitGetVirtualRegisters(currentInstruction[2].u.operand, regT0, currentInstruction[3].u.operand, regT1);
            emitJumpSlowCaseIfNotImmediateInteger(regT1);
#if USE(ALTERNATE_JSIMMEDIATE)
            // This is technically incorrect - we're zero-extending an int32.  On the hot path this doesn't matter.
            // We check the value as if it was a uint32 against the m_fastAccessCutoff - which will always fail if
            // number was signed since m_fastAccessCutoff is always less than intmax (since the total allocation
            // size is always less than 4Gb).  As such zero extending wil have been correct (and extending the value
            // to 64-bits is necessary since it's used in the address calculation.  We zero extend rather than sign
            // extending since it makes it easier to re-tag the value in the slow case.
            zeroExtend32ToPtr(regT1, regT1);
#else
            emitFastArithImmToInt(regT1);
#endif
            emitJumpSlowCaseIfNotJSCell(regT0);
            addSlowCase(branchPtr(NotEqual, Address(regT0), ImmPtr(m_interpreter->m_jsArrayVptr)));

            // This is an array; get the m_storage pointer into ecx, then check if the index is below the fast cutoff
            loadPtr(Address(regT0, FIELD_OFFSET(JSArray, m_storage)), regT2);
            addSlowCase(branch32(AboveOrEqual, regT1, Address(regT0, FIELD_OFFSET(JSArray, m_fastAccessCutoff))));

            // Get the value from the vector
            loadPtr(BaseIndex(regT2, regT1, ScalePtr, FIELD_OFFSET(ArrayStorage, m_vector[0])), regT0);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_get_by_val);
        }
        case op_resolve_func: {
            Identifier* ident = &(m_codeBlock->identifier(currentInstruction[3].u.operand));
            emitPutJITStubArgConstant(ident, 1);
            emitCTICall(JITStubs::cti_op_resolve_func);
            emitPutVirtualRegister(currentInstruction[2].u.operand, regT1);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_resolve_func);
        }
        case op_sub: {
            compileFastArith_op_sub(currentInstruction);
            NEXT_OPCODE(op_sub);
        }
        case op_put_by_val: {
            emitGetVirtualRegisters(currentInstruction[1].u.operand, regT0, currentInstruction[2].u.operand, regT1);
            emitJumpSlowCaseIfNotImmediateInteger(regT1);
#if USE(ALTERNATE_JSIMMEDIATE)
            // See comment in op_get_by_val.
            zeroExtend32ToPtr(regT1, regT1);
#else
            emitFastArithImmToInt(regT1);
#endif
            emitJumpSlowCaseIfNotJSCell(regT0);
            addSlowCase(branchPtr(NotEqual, Address(regT0), ImmPtr(m_interpreter->m_jsArrayVptr)));

            // This is an array; get the m_storage pointer into ecx, then check if the index is below the fast cutoff
            loadPtr(Address(regT0, FIELD_OFFSET(JSArray, m_storage)), regT2);
            Jump inFastVector = branch32(Below, regT1, Address(regT0, FIELD_OFFSET(JSArray, m_fastAccessCutoff)));
            // No; oh well, check if the access if within the vector - if so, we may still be okay.
            addSlowCase(branch32(AboveOrEqual, regT1, Address(regT2, FIELD_OFFSET(ArrayStorage, m_vectorLength))));

            // This is a write to the slow part of the vector; first, we have to check if this would be the first write to this location.
            // FIXME: should be able to handle initial write to array; increment the the number of items in the array, and potentially update fast access cutoff. 
            addSlowCase(branchTestPtr(Zero, BaseIndex(regT2, regT1, ScalePtr, FIELD_OFFSET(ArrayStorage, m_vector[0]))));

            // All good - put the value into the array.
            inFastVector.link(this);
            emitGetVirtualRegister(currentInstruction[3].u.operand, regT0);
            storePtr(regT0, BaseIndex(regT2, regT1, ScalePtr, FIELD_OFFSET(ArrayStorage, m_vector[0])));
            NEXT_OPCODE(op_put_by_val);
        }
        CTI_COMPILE_BINARY_OP(op_lesseq)
        case op_loop_if_true: {
            emitTimeoutCheck();

            unsigned target = currentInstruction[2].u.operand;
            emitGetVirtualRegister(currentInstruction[1].u.operand, regT0);

            Jump isZero = branchPtr(Equal, regT0, ImmPtr(JSValuePtr::encode(js0())));
            addJump(emitJumpIfImmediateInteger(regT0), target + 2);

            addJump(branchPtr(Equal, regT0, ImmPtr(JSValuePtr::encode(jsBoolean(true)))), target + 2);
            addSlowCase(branchPtr(NotEqual, regT0, ImmPtr(JSValuePtr::encode(jsBoolean(false)))));

            isZero.link(this);
            NEXT_OPCODE(op_loop_if_true);
        };
        case op_resolve_base: {
            Identifier* ident = &(m_codeBlock->identifier(currentInstruction[2].u.operand));
            emitPutJITStubArgConstant(ident, 1);
            emitCTICall(JITStubs::cti_op_resolve_base);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_resolve_base);
        }
        case op_negate: {
            emitPutJITStubArgFromVirtualRegister(currentInstruction[2].u.operand, 1, regT2);
            emitCTICall(JITStubs::cti_op_negate);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_negate);
        }
        case op_resolve_skip: {
            Identifier* ident = &(m_codeBlock->identifier(currentInstruction[2].u.operand));
            emitPutJITStubArgConstant(ident, 1);
            emitPutJITStubArgConstant(currentInstruction[3].u.operand + m_codeBlock->needsFullScopeChain(), 2);
            emitCTICall(JITStubs::cti_op_resolve_skip);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_resolve_skip);
        }
        case op_resolve_global: {
            // Fast case
            void* globalObject = currentInstruction[2].u.jsCell;
            Identifier* ident = &(m_codeBlock->identifier(currentInstruction[3].u.operand));
            
            unsigned currentIndex = globalResolveInfoIndex++;
            void* structureAddress = &(m_codeBlock->globalResolveInfo(currentIndex).structure);
            void* offsetAddr = &(m_codeBlock->globalResolveInfo(currentIndex).offset);

            // Check Structure of global object
            move(ImmPtr(globalObject), regT0);
            loadPtr(structureAddress, regT1);
            Jump noMatch = branchPtr(NotEqual, regT1, Address(regT0, FIELD_OFFSET(JSCell, m_structure))); // Structures don't match

            // Load cached property
            loadPtr(Address(regT0, FIELD_OFFSET(JSGlobalObject, m_propertyStorage)), regT0);
            load32(offsetAddr, regT1);
            loadPtr(BaseIndex(regT0, regT1, ScalePtr), regT0);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            Jump end = jump();

            // Slow case
            noMatch.link(this);
            emitPutJITStubArgConstant(globalObject, 1);
            emitPutJITStubArgConstant(ident, 2);
            emitPutJITStubArgConstant(currentIndex, 3);
            emitCTICall(JITStubs::cti_op_resolve_global);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            end.link(this);
            NEXT_OPCODE(op_resolve_global);
        }
        CTI_COMPILE_BINARY_OP(op_div)
        case op_pre_dec: {
            compileFastArith_op_pre_dec(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_pre_dec);
        }
        case op_jnless: {
            unsigned op1 = currentInstruction[1].u.operand;
            unsigned op2 = currentInstruction[2].u.operand;
            unsigned target = currentInstruction[3].u.operand;
            if (isOperandConstantImmediateInt(op2)) {
                emitGetVirtualRegister(op1, regT0);
                emitJumpSlowCaseIfNotImmediateInteger(regT0);
#if USE(ALTERNATE_JSIMMEDIATE)
                int32_t op2imm = getConstantOperandImmediateInt(op2);
#else
                int32_t op2imm = static_cast<int32_t>(JSImmediate::rawValue(getConstantOperand(op2)));
#endif
                addJump(branch32(GreaterThanOrEqual, regT0, Imm32(op2imm)), target + 3);
            } else {
                emitGetVirtualRegisters(op1, regT0, op2, regT1);
                emitJumpSlowCaseIfNotImmediateInteger(regT0);
                emitJumpSlowCaseIfNotImmediateInteger(regT1);
                addJump(branch32(GreaterThanOrEqual, regT0, regT1), target + 3);
            }
            NEXT_OPCODE(op_jnless);
        }
        case op_not: {
            emitGetVirtualRegister(currentInstruction[2].u.operand, regT0);
            xorPtr(Imm32(static_cast<int32_t>(JSImmediate::FullTagTypeBool)), regT0);
            addSlowCase(branchTestPtr(NonZero, regT0, Imm32(static_cast<int32_t>(~JSImmediate::ExtendedPayloadBitBoolValue))));
            xorPtr(Imm32(static_cast<int32_t>(JSImmediate::FullTagTypeBool | JSImmediate::ExtendedPayloadBitBoolValue)), regT0);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_not);
        }
        case op_jfalse: {
            unsigned target = currentInstruction[2].u.operand;
            emitGetVirtualRegister(currentInstruction[1].u.operand, regT0);

            addJump(branchPtr(Equal, regT0, ImmPtr(JSValuePtr::encode(js0()))), target + 2);
            Jump isNonZero = emitJumpIfImmediateInteger(regT0);

            addJump(branchPtr(Equal, regT0, ImmPtr(JSValuePtr::encode(jsBoolean(false)))), target + 2);
            addSlowCase(branchPtr(NotEqual, regT0, ImmPtr(JSValuePtr::encode(jsBoolean(true)))));

            isNonZero.link(this);
            NEXT_OPCODE(op_jfalse);
        };
        case op_jeq_null: {
            unsigned src = currentInstruction[1].u.operand;
            unsigned target = currentInstruction[2].u.operand;

            emitGetVirtualRegister(src, regT0);
            Jump isImmediate = emitJumpIfNotJSCell(regT0);

            // First, handle JSCell cases - check MasqueradesAsUndefined bit on the structure.
            loadPtr(Address(regT0, FIELD_OFFSET(JSCell, m_structure)), regT2);
            addJump(branchTest32(NonZero, Address(regT2, FIELD_OFFSET(Structure, m_typeInfo.m_flags)), Imm32(MasqueradesAsUndefined)), target + 2);
            Jump wasNotImmediate = jump();

            // Now handle the immediate cases - undefined & null
            isImmediate.link(this);
            andPtr(Imm32(~JSImmediate::ExtendedTagBitUndefined), regT0);
            addJump(branchPtr(Equal, regT0, ImmPtr(JSValuePtr::encode(jsNull()))), target + 2);            

            wasNotImmediate.link(this);
            NEXT_OPCODE(op_jeq_null);
        };
        case op_jneq_null: {
            unsigned src = currentInstruction[1].u.operand;
            unsigned target = currentInstruction[2].u.operand;

            emitGetVirtualRegister(src, regT0);
            Jump isImmediate = emitJumpIfNotJSCell(regT0);

            // First, handle JSCell cases - check MasqueradesAsUndefined bit on the structure.
            loadPtr(Address(regT0, FIELD_OFFSET(JSCell, m_structure)), regT2);
            addJump(branchTest32(Zero, Address(regT2, FIELD_OFFSET(Structure, m_typeInfo.m_flags)), Imm32(MasqueradesAsUndefined)), target + 2);
            Jump wasNotImmediate = jump();

            // Now handle the immediate cases - undefined & null
            isImmediate.link(this);
            andPtr(Imm32(~JSImmediate::ExtendedTagBitUndefined), regT0);
            addJump(branchPtr(NotEqual, regT0, ImmPtr(JSValuePtr::encode(jsNull()))), target + 2);            

            wasNotImmediate.link(this);
            NEXT_OPCODE(op_jneq_null);
        }
        case op_post_inc: {
            compileFastArith_op_post_inc(currentInstruction[1].u.operand, currentInstruction[2].u.operand);
            NEXT_OPCODE(op_post_inc);
        }
        case op_unexpected_load: {
            JSValuePtr v = m_codeBlock->unexpectedConstant(currentInstruction[2].u.operand);
            move(ImmPtr(JSValuePtr::encode(v)), regT0);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_unexpected_load);
        }
        case op_jsr: {
            int retAddrDst = currentInstruction[1].u.operand;
            int target = currentInstruction[2].u.operand;
            DataLabelPtr storeLocation = storePtrWithPatch(Address(callFrameRegister, sizeof(Register) * retAddrDst));
            addJump(jump(), target + 2);
            m_jsrSites.append(JSRInfo(storeLocation, label()));
            NEXT_OPCODE(op_jsr);
        }
        case op_sret: {
            jump(Address(callFrameRegister, sizeof(Register) * currentInstruction[1].u.operand));
            NEXT_OPCODE(op_sret);
        }
        case op_eq: {
            emitGetVirtualRegisters(currentInstruction[2].u.operand, regT0, currentInstruction[3].u.operand, regT1);
            emitJumpSlowCaseIfNotImmediateIntegers(regT0, regT1, regT2);
            set32(Equal, regT1, regT0, regT0);
            emitTagAsBoolImmediate(regT0);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_eq);
        }
        case op_lshift: {
            compileFastArith_op_lshift(currentInstruction[1].u.operand, currentInstruction[2].u.operand, currentInstruction[3].u.operand);
            NEXT_OPCODE(op_lshift);
        }
        case op_bitand: {
            compileFastArith_op_bitand(currentInstruction[1].u.operand, currentInstruction[2].u.operand, currentInstruction[3].u.operand);
            NEXT_OPCODE(op_bitand);
        }
        case op_rshift: {
            compileFastArith_op_rshift(currentInstruction[1].u.operand, currentInstruction[2].u.operand, currentInstruction[3].u.operand);
            NEXT_OPCODE(op_rshift);
        }
        case op_bitnot: {
            emitGetVirtualRegister(currentInstruction[2].u.operand, regT0);
            emitJumpSlowCaseIfNotImmediateInteger(regT0);
#if USE(ALTERNATE_JSIMMEDIATE)
            not32(regT0);
            emitFastArithIntToImmNoCheck(regT0, regT0);
#else
            xorPtr(Imm32(~JSImmediate::TagTypeNumber), regT0);
#endif
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_bitnot);
        }
        case op_resolve_with_base: {
            Identifier* ident = &(m_codeBlock->identifier(currentInstruction[3].u.operand));
            emitPutJITStubArgConstant(ident, 1);
            emitCTICall(JITStubs::cti_op_resolve_with_base);
            emitPutVirtualRegister(currentInstruction[2].u.operand, regT1);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_resolve_with_base);
        }
        case op_new_func_exp: {
            FuncExprNode* func = m_codeBlock->functionExpression(currentInstruction[2].u.operand);
            emitPutJITStubArgConstant(func, 1);
            emitCTICall(JITStubs::cti_op_new_func_exp);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_new_func_exp);
        }
        case op_mod: {
            compileFastArith_op_mod(currentInstruction[1].u.operand, currentInstruction[2].u.operand, currentInstruction[3].u.operand);
            NEXT_OPCODE(op_mod);
        }
        case op_jtrue: {
            unsigned target = currentInstruction[2].u.operand;
            emitGetVirtualRegister(currentInstruction[1].u.operand, regT0);

            Jump isZero = branchPtr(Equal, regT0, ImmPtr(JSValuePtr::encode(js0())));
            addJump(emitJumpIfImmediateInteger(regT0), target + 2);

            addJump(branchPtr(Equal, regT0, ImmPtr(JSValuePtr::encode(jsBoolean(true)))), target + 2);
            addSlowCase(branchPtr(NotEqual, regT0, ImmPtr(JSValuePtr::encode(jsBoolean(false)))));

            isZero.link(this);
            NEXT_OPCODE(op_jtrue);
        }
        CTI_COMPILE_BINARY_OP(op_less)
        case op_neq: {
            emitGetVirtualRegisters(currentInstruction[2].u.operand, regT0, currentInstruction[3].u.operand, regT1);
            emitJumpSlowCaseIfNotImmediateIntegers(regT0, regT1, regT2);
            set32(NotEqual, regT1, regT0, regT0);
            emitTagAsBoolImmediate(regT0);

            emitPutVirtualRegister(currentInstruction[1].u.operand);

            NEXT_OPCODE(op_neq);
        }
        case op_post_dec: {
            compileFastArith_op_post_dec(currentInstruction[1].u.operand, currentInstruction[2].u.operand);
            NEXT_OPCODE(op_post_dec);
        }
        CTI_COMPILE_BINARY_OP(op_urshift)
        case op_bitxor: {
            emitGetVirtualRegisters(currentInstruction[2].u.operand, regT0, currentInstruction[3].u.operand, regT1);
            emitJumpSlowCaseIfNotImmediateIntegers(regT0, regT1, regT2);
            xorPtr(regT1, regT0);
            emitFastArithReTagImmediate(regT0, regT0);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_bitxor);
        }
        case op_new_regexp: {
            RegExp* regExp = m_codeBlock->regexp(currentInstruction[2].u.operand);
            emitPutJITStubArgConstant(regExp, 1);
            emitCTICall(JITStubs::cti_op_new_regexp);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_new_regexp);
        }
        case op_bitor: {
            emitGetVirtualRegisters(currentInstruction[2].u.operand, regT0, currentInstruction[3].u.operand, regT1);
            emitJumpSlowCaseIfNotImmediateIntegers(regT0, regT1, regT2);
            orPtr(regT1, regT0);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_bitor);
        }
        case op_throw: {
            emitPutJITStubArgFromVirtualRegister(currentInstruction[1].u.operand, 1, regT2);
            emitCTICall(JITStubs::cti_op_throw);
            ASSERT(regT0 == returnValueRegister);
#if PLATFORM(X86_64)
            addPtr(Imm32(0x48), X86::esp);
            pop(X86::ebx);
            pop(X86::r15);
            pop(X86::r14);
            pop(X86::r13);
            pop(X86::r12);
            pop(X86::ebp);
            ret();
#else
            addPtr(Imm32(0x1c), X86::esp);
            pop(X86::ebx);
            pop(X86::edi);
            pop(X86::esi);
            pop(X86::ebp);
            ret();
#endif
            NEXT_OPCODE(op_throw);
        }
        case op_get_pnames: {
            emitPutJITStubArgFromVirtualRegister(currentInstruction[2].u.operand, 1, regT2);
            emitCTICall(JITStubs::cti_op_get_pnames);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_get_pnames);
        }
        case op_next_pname: {
            emitPutJITStubArgFromVirtualRegister(currentInstruction[2].u.operand, 1, regT2);
            unsigned target = currentInstruction[3].u.operand;
            emitCTICall(JITStubs::cti_op_next_pname);
            Jump endOfIter = branchTestPtr(Zero, regT0);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            addJump(jump(), target + 3);
            endOfIter.link(this);
            NEXT_OPCODE(op_next_pname);
        }
        case op_push_scope: {
            emitPutJITStubArgFromVirtualRegister(currentInstruction[1].u.operand, 1, regT2);
            emitCTICall(JITStubs::cti_op_push_scope);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_push_scope);
        }
        case op_pop_scope: {
            emitCTICall(JITStubs::cti_op_pop_scope);
            NEXT_OPCODE(op_pop_scope);
        }
        CTI_COMPILE_UNARY_OP(op_typeof)
        CTI_COMPILE_UNARY_OP(op_is_undefined)
        CTI_COMPILE_UNARY_OP(op_is_boolean)
        CTI_COMPILE_UNARY_OP(op_is_number)
        CTI_COMPILE_UNARY_OP(op_is_string)
        CTI_COMPILE_UNARY_OP(op_is_object)
        CTI_COMPILE_UNARY_OP(op_is_function)
        case op_stricteq: {
            compileOpStrictEq(currentInstruction, OpStrictEq);
            NEXT_OPCODE(op_stricteq);
        }
        case op_nstricteq: {
            compileOpStrictEq(currentInstruction, OpNStrictEq);
            NEXT_OPCODE(op_nstricteq);
        }
        case op_to_jsnumber: {
            int srcVReg = currentInstruction[2].u.operand;
            emitGetVirtualRegister(srcVReg, regT0);
            
            Jump wasImmediate = emitJumpIfImmediateInteger(regT0);

            emitJumpSlowCaseIfNotJSCell(regT0, srcVReg);
            loadPtr(Address(regT0, FIELD_OFFSET(JSCell, m_structure)), regT2);
            addSlowCase(branch32(NotEqual, Address(regT2, FIELD_OFFSET(Structure, m_typeInfo.m_type)), Imm32(NumberType)));
            
            wasImmediate.link(this);

            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_to_jsnumber);
        }
        CTI_COMPILE_BINARY_OP(op_in)
        case op_push_new_scope: {
            Identifier* ident = &(m_codeBlock->identifier(currentInstruction[2].u.operand));
            emitPutJITStubArgConstant(ident, 1);
            emitPutJITStubArgFromVirtualRegister(currentInstruction[3].u.operand, 2, regT2);
            emitCTICall(JITStubs::cti_op_push_new_scope);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_push_new_scope);
        }
        case op_catch: {
            emitGetCTIParam(STUB_ARGS_callFrame, callFrameRegister);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_catch);
        }
        case op_jmp_scopes: {
            unsigned count = currentInstruction[1].u.operand;
            emitPutJITStubArgConstant(count, 1);
            emitCTICall(JITStubs::cti_op_jmp_scopes);
            unsigned target = currentInstruction[2].u.operand;
            addJump(jump(), target + 2);
            NEXT_OPCODE(op_jmp_scopes);
        }
        case op_put_by_index: {
            emitPutJITStubArgFromVirtualRegister(currentInstruction[1].u.operand, 1, regT2);
            emitPutJITStubArgConstant(currentInstruction[2].u.operand, 2);
            emitPutJITStubArgFromVirtualRegister(currentInstruction[3].u.operand, 3, regT2);
            emitCTICall(JITStubs::cti_op_put_by_index);
            NEXT_OPCODE(op_put_by_index);
        }
        case op_switch_imm: {
            unsigned tableIndex = currentInstruction[1].u.operand;
            unsigned defaultOffset = currentInstruction[2].u.operand;
            unsigned scrutinee = currentInstruction[3].u.operand;

            // create jump table for switch destinations, track this switch statement.
            SimpleJumpTable* jumpTable = &m_codeBlock->immediateSwitchJumpTable(tableIndex);
            m_switches.append(SwitchRecord(jumpTable, m_bytecodeIndex, defaultOffset, SwitchRecord::Immediate));
            jumpTable->ctiOffsets.grow(jumpTable->branchOffsets.size());

            emitPutJITStubArgFromVirtualRegister(scrutinee, 1, regT2);
            emitPutJITStubArgConstant(tableIndex, 2);
            emitCTICall(JITStubs::cti_op_switch_imm);
            jump(regT0);
            NEXT_OPCODE(op_switch_imm);
        }
        case op_switch_char: {
            unsigned tableIndex = currentInstruction[1].u.operand;
            unsigned defaultOffset = currentInstruction[2].u.operand;
            unsigned scrutinee = currentInstruction[3].u.operand;

            // create jump table for switch destinations, track this switch statement.
            SimpleJumpTable* jumpTable = &m_codeBlock->characterSwitchJumpTable(tableIndex);
            m_switches.append(SwitchRecord(jumpTable, m_bytecodeIndex, defaultOffset, SwitchRecord::Character));
            jumpTable->ctiOffsets.grow(jumpTable->branchOffsets.size());

            emitPutJITStubArgFromVirtualRegister(scrutinee, 1, regT2);
            emitPutJITStubArgConstant(tableIndex, 2);
            emitCTICall(JITStubs::cti_op_switch_char);
            jump(regT0);
            NEXT_OPCODE(op_switch_char);
        }
        case op_switch_string: {
            unsigned tableIndex = currentInstruction[1].u.operand;
            unsigned defaultOffset = currentInstruction[2].u.operand;
            unsigned scrutinee = currentInstruction[3].u.operand;

            // create jump table for switch destinations, track this switch statement.
            StringJumpTable* jumpTable = &m_codeBlock->stringSwitchJumpTable(tableIndex);
            m_switches.append(SwitchRecord(jumpTable, m_bytecodeIndex, defaultOffset));

            emitPutJITStubArgFromVirtualRegister(scrutinee, 1, regT2);
            emitPutJITStubArgConstant(tableIndex, 2);
            emitCTICall(JITStubs::cti_op_switch_string);
            jump(regT0);
            NEXT_OPCODE(op_switch_string);
        }
        case op_del_by_val: {
            emitPutJITStubArgFromVirtualRegister(currentInstruction[2].u.operand, 1, regT2);
            emitPutJITStubArgFromVirtualRegister(currentInstruction[3].u.operand, 2, regT2);
            emitCTICall(JITStubs::cti_op_del_by_val);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_del_by_val);
        }
        case op_put_getter: {
            emitPutJITStubArgFromVirtualRegister(currentInstruction[1].u.operand, 1, regT2);
            Identifier* ident = &(m_codeBlock->identifier(currentInstruction[2].u.operand));
            emitPutJITStubArgConstant(ident, 2);
            emitPutJITStubArgFromVirtualRegister(currentInstruction[3].u.operand, 3, regT2);
            emitCTICall(JITStubs::cti_op_put_getter);
            NEXT_OPCODE(op_put_getter);
        }
        case op_put_setter: {
            emitPutJITStubArgFromVirtualRegister(currentInstruction[1].u.operand, 1, regT2);
            Identifier* ident = &(m_codeBlock->identifier(currentInstruction[2].u.operand));
            emitPutJITStubArgConstant(ident, 2);
            emitPutJITStubArgFromVirtualRegister(currentInstruction[3].u.operand, 3, regT2);
            emitCTICall(JITStubs::cti_op_put_setter);
            NEXT_OPCODE(op_put_setter);
        }
        case op_new_error: {
            JSValuePtr message = m_codeBlock->unexpectedConstant(currentInstruction[3].u.operand);
            emitPutJITStubArgConstant(currentInstruction[2].u.operand, 1);
            emitPutJITStubArgConstant(JSValuePtr::encode(message), 2);
            emitPutJITStubArgConstant(m_bytecodeIndex, 3);
            emitCTICall(JITStubs::cti_op_new_error);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_new_error);
        }
        case op_debug: {
            emitPutJITStubArgConstant(currentInstruction[1].u.operand, 1);
            emitPutJITStubArgConstant(currentInstruction[2].u.operand, 2);
            emitPutJITStubArgConstant(currentInstruction[3].u.operand, 3);
            emitCTICall(JITStubs::cti_op_debug);
            NEXT_OPCODE(op_debug);
        }
        case op_eq_null: {
            unsigned dst = currentInstruction[1].u.operand;
            unsigned src1 = currentInstruction[2].u.operand;

            emitGetVirtualRegister(src1, regT0);
            Jump isImmediate = emitJumpIfNotJSCell(regT0);

            loadPtr(Address(regT0, FIELD_OFFSET(JSCell, m_structure)), regT2);
            setTest32(NonZero, Address(regT2, FIELD_OFFSET(Structure, m_typeInfo.m_flags)), Imm32(MasqueradesAsUndefined), regT0);

            Jump wasNotImmediate = jump();

            isImmediate.link(this);

            andPtr(Imm32(~JSImmediate::ExtendedTagBitUndefined), regT0);
            setPtr(Equal, regT0, Imm32(JSImmediate::FullTagTypeNull), regT0);

            wasNotImmediate.link(this);

            emitTagAsBoolImmediate(regT0);
            emitPutVirtualRegister(dst);

            NEXT_OPCODE(op_eq_null);
        }
        case op_neq_null: {
            unsigned dst = currentInstruction[1].u.operand;
            unsigned src1 = currentInstruction[2].u.operand;

            emitGetVirtualRegister(src1, regT0);
            Jump isImmediate = emitJumpIfNotJSCell(regT0);

            loadPtr(Address(regT0, FIELD_OFFSET(JSCell, m_structure)), regT2);
            setTest32(Zero, Address(regT2, FIELD_OFFSET(Structure, m_typeInfo.m_flags)), Imm32(MasqueradesAsUndefined), regT0);

            Jump wasNotImmediate = jump();

            isImmediate.link(this);

            andPtr(Imm32(~JSImmediate::ExtendedTagBitUndefined), regT0);
            setPtr(NotEqual, regT0, Imm32(JSImmediate::FullTagTypeNull), regT0);

            wasNotImmediate.link(this);

            emitTagAsBoolImmediate(regT0);
            emitPutVirtualRegister(dst);

            NEXT_OPCODE(op_neq_null);
        }
        case op_enter: {
            // Even though CTI doesn't use them, we initialize our constant
            // registers to zap stale pointers, to avoid unnecessarily prolonging
            // object lifetime and increasing GC pressure.
            size_t count = m_codeBlock->m_numVars + m_codeBlock->numberOfConstantRegisters();
            for (size_t j = 0; j < count; ++j)
                emitInitRegister(j);

            NEXT_OPCODE(op_enter);
        }
        case op_enter_with_activation: {
            // Even though CTI doesn't use them, we initialize our constant
            // registers to zap stale pointers, to avoid unnecessarily prolonging
            // object lifetime and increasing GC pressure.
            size_t count = m_codeBlock->m_numVars + m_codeBlock->numberOfConstantRegisters();
            for (size_t j = 0; j < count; ++j)
                emitInitRegister(j);

            emitCTICall(JITStubs::cti_op_push_activation);
            emitPutVirtualRegister(currentInstruction[1].u.operand);

            NEXT_OPCODE(op_enter_with_activation);
        }
        case op_create_arguments: {
            if (m_codeBlock->m_numParameters == 1)
                emitCTICall(JITStubs::cti_op_create_arguments_no_params);
            else
                emitCTICall(JITStubs::cti_op_create_arguments);
            NEXT_OPCODE(op_create_arguments);
        }
        case op_convert_this: {
            emitGetVirtualRegister(currentInstruction[1].u.operand, regT0);

            emitJumpSlowCaseIfNotJSCell(regT0);
            loadPtr(Address(regT0, FIELD_OFFSET(JSCell, m_structure)), regT1);
            addSlowCase(branchTest32(NonZero, Address(regT1, FIELD_OFFSET(Structure, m_typeInfo.m_flags)), Imm32(NeedsThisConversion)));

            NEXT_OPCODE(op_convert_this);
        }
        case op_profile_will_call: {
            emitGetCTIParam(STUB_ARGS_profilerReference, regT0);
            Jump noProfiler = branchTestPtr(Zero, Address(regT0));
            emitPutJITStubArgFromVirtualRegister(currentInstruction[1].u.operand, 1, regT0);
            emitCTICall(JITStubs::cti_op_profile_will_call);
            noProfiler.link(this);

            NEXT_OPCODE(op_profile_will_call);
        }
        case op_profile_did_call: {
            emitGetCTIParam(STUB_ARGS_profilerReference, regT0);
            Jump noProfiler = branchTestPtr(Zero, Address(regT0));
            emitPutJITStubArgFromVirtualRegister(currentInstruction[1].u.operand, 1, regT0);
            emitCTICall(JITStubs::cti_op_profile_did_call);
            noProfiler.link(this);

            NEXT_OPCODE(op_profile_did_call);
        }
        case op_get_array_length:
        case op_get_by_id_chain:
        case op_get_by_id_generic:
        case op_get_by_id_proto:
        case op_get_by_id_proto_list:
        case op_get_by_id_self:
        case op_get_by_id_self_list:
        case op_get_string_length:
        case op_put_by_id_generic:
        case op_put_by_id_replace:
        case op_put_by_id_transition:
            ASSERT_NOT_REACHED();
        }
    }

    ASSERT(propertyAccessInstructionIndex == m_codeBlock->numberOfStructureStubInfos());
    ASSERT(callLinkInfoIndex == m_codeBlock->numberOfCallLinkInfos());

#ifndef NDEBUG
    // reset this, in order to guard it's use with asserts
    m_bytecodeIndex = (unsigned)-1;
#endif
}


void JIT::privateCompileLinkPass()
{
    unsigned jmpTableCount = m_jmpTable.size();
    for (unsigned i = 0; i < jmpTableCount; ++i)
        m_jmpTable[i].from.linkTo(m_labels[m_jmpTable[i].toBytecodeIndex], this);
    m_jmpTable.clear();
}

void JIT::privateCompileSlowCases()
{
    Instruction* instructionsBegin = m_codeBlock->instructions().begin();
    unsigned propertyAccessInstructionIndex = 0;
    unsigned callLinkInfoIndex = 0;

    for (Vector<SlowCaseEntry>::iterator iter = m_slowCases.begin(); iter != m_slowCases.end();) {
        // FIXME: enable peephole optimizations for slow cases when applicable
        killLastResultRegister();

        m_bytecodeIndex = iter->to;
#ifndef NDEBUG
        unsigned firstTo = m_bytecodeIndex;
#endif
        Instruction* currentInstruction = instructionsBegin + m_bytecodeIndex;

        switch (OpcodeID opcodeID = m_interpreter->getOpcodeID(currentInstruction->u.opcode)) {
        case op_convert_this: {
            linkSlowCase(iter);
            linkSlowCase(iter);
            emitPutJITStubArg(regT0, 1);
            emitCTICall(JITStubs::cti_op_convert_this);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_convert_this);
        }
        case op_add: {
            compileFastArithSlow_op_add(currentInstruction, iter);
            NEXT_OPCODE(op_add);
        }
        case op_construct_verify: {
            linkSlowCase(iter);
            linkSlowCase(iter);
            emitGetVirtualRegister(currentInstruction[2].u.operand, regT0);
            emitPutVirtualRegister(currentInstruction[1].u.operand);

            NEXT_OPCODE(op_construct_verify);
        }
        case op_get_by_val: {
            // The slow case that handles accesses to arrays (below) may jump back up to here. 
            Label beginGetByValSlow(this);

            Jump notImm = getSlowCase(iter);
            linkSlowCase(iter);
            linkSlowCase(iter);
            emitFastArithIntToImmNoCheck(regT1, regT1);
            notImm.link(this);
            emitPutJITStubArg(regT0, 1);
            emitPutJITStubArg(regT1, 2);
            emitCTICall(JITStubs::cti_op_get_by_val);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_get_by_val));

            // This is slow case that handles accesses to arrays above the fast cut-off.
            // First, check if this is an access to the vector
            linkSlowCase(iter);
            branch32(AboveOrEqual, regT1, Address(regT2, FIELD_OFFSET(ArrayStorage, m_vectorLength)), beginGetByValSlow);

            // okay, missed the fast region, but it is still in the vector.  Get the value.
            loadPtr(BaseIndex(regT2, regT1, ScalePtr, FIELD_OFFSET(ArrayStorage, m_vector[0])), regT2);
            // Check whether the value loaded is zero; if so we need to return undefined.
            branchTestPtr(Zero, regT2, beginGetByValSlow);
            move(regT2, regT0);
            emitPutVirtualRegister(currentInstruction[1].u.operand, regT0);

            NEXT_OPCODE(op_get_by_val);
        }
        case op_sub: {
            compileFastArithSlow_op_sub(currentInstruction, iter);
            NEXT_OPCODE(op_sub);
        }
        case op_rshift: {
            compileFastArithSlow_op_rshift(currentInstruction[1].u.operand, currentInstruction[2].u.operand, currentInstruction[3].u.operand, iter);
            NEXT_OPCODE(op_rshift);
        }
        case op_lshift: {
            compileFastArithSlow_op_lshift(currentInstruction[1].u.operand, currentInstruction[2].u.operand, currentInstruction[3].u.operand, iter);
            NEXT_OPCODE(op_lshift);
        }
        case op_loop_if_less: {
            unsigned op2 = currentInstruction[2].u.operand;
            unsigned target = currentInstruction[3].u.operand;
            if (isOperandConstantImmediateInt(op2)) {
                linkSlowCase(iter);
                emitPutJITStubArg(regT0, 1);
                emitPutJITStubArgFromVirtualRegister(op2, 2, regT2);
                emitCTICall(JITStubs::cti_op_loop_if_less);
                emitJumpSlowToHot(branchTest32(NonZero, regT0), target + 3);
            } else {
                linkSlowCase(iter);
                linkSlowCase(iter);
                emitPutJITStubArg(regT0, 1);
                emitPutJITStubArg(regT1, 2);
                emitCTICall(JITStubs::cti_op_loop_if_less);
                emitJumpSlowToHot(branchTest32(NonZero, regT0), target + 3);
            }
            NEXT_OPCODE(op_loop_if_less);
        }
        case op_put_by_id: {
            compilePutByIdSlowCase(currentInstruction[1].u.operand, &(m_codeBlock->identifier(currentInstruction[2].u.operand)), currentInstruction[3].u.operand, iter, propertyAccessInstructionIndex++);
            NEXT_OPCODE(op_put_by_id);
        }
        case op_get_by_id: {
            compileGetByIdSlowCase(currentInstruction[1].u.operand, currentInstruction[2].u.operand, &(m_codeBlock->identifier(currentInstruction[3].u.operand)), iter, propertyAccessInstructionIndex++);
            NEXT_OPCODE(op_get_by_id);
        }
        case op_loop_if_lesseq: {
            unsigned op2 = currentInstruction[2].u.operand;
            unsigned target = currentInstruction[3].u.operand;
            if (isOperandConstantImmediateInt(op2)) {
                linkSlowCase(iter);
                emitPutJITStubArg(regT0, 1);
                emitPutJITStubArgFromVirtualRegister(currentInstruction[2].u.operand, 2, regT2);
                emitCTICall(JITStubs::cti_op_loop_if_lesseq);
                emitJumpSlowToHot(branchTest32(NonZero, regT0), target + 3);
            } else {
                linkSlowCase(iter);
                linkSlowCase(iter);
                emitPutJITStubArg(regT0, 1);
                emitPutJITStubArg(regT1, 2);
                emitCTICall(JITStubs::cti_op_loop_if_lesseq);
                emitJumpSlowToHot(branchTest32(NonZero, regT0), target + 3);
            }
            NEXT_OPCODE(op_loop_if_lesseq);
        }
        case op_pre_inc: {
            compileFastArithSlow_op_pre_inc(currentInstruction[1].u.operand, iter);
            NEXT_OPCODE(op_pre_inc);
        }
        case op_put_by_val: {
            // Normal slow cases - either is not an immediate imm, or is an array.
            Jump notImm = getSlowCase(iter);
            linkSlowCase(iter);
            linkSlowCase(iter);
            emitFastArithIntToImmNoCheck(regT1, regT1);
            notImm.link(this);
            emitGetVirtualRegister(currentInstruction[3].u.operand, regT2);
            emitPutJITStubArg(regT0, 1);
            emitPutJITStubArg(regT1, 2);
            emitPutJITStubArg(regT2, 3);
            emitCTICall(JITStubs::cti_op_put_by_val);
            emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_put_by_val));

            // slow cases for immediate int accesses to arrays
            linkSlowCase(iter);
            linkSlowCase(iter);
            emitGetVirtualRegister(currentInstruction[3].u.operand, regT2);
            emitPutJITStubArg(regT0, 1);
            emitPutJITStubArg(regT1, 2);
            emitPutJITStubArg(regT2, 3);
            emitCTICall(JITStubs::cti_op_put_by_val_array);

            NEXT_OPCODE(op_put_by_val);
        }
        case op_loop_if_true: {
            linkSlowCase(iter);
            emitPutJITStubArg(regT0, 1);
            emitCTICall(JITStubs::cti_op_jtrue);
            unsigned target = currentInstruction[2].u.operand;
            emitJumpSlowToHot(branchTest32(NonZero, regT0), target + 2);
            NEXT_OPCODE(op_loop_if_true);
        }
        case op_pre_dec: {
            compileFastArithSlow_op_pre_dec(currentInstruction[1].u.operand, iter);
            NEXT_OPCODE(op_pre_dec);
        }
        case op_jnless: {
            unsigned op2 = currentInstruction[2].u.operand;
            unsigned target = currentInstruction[3].u.operand;
            if (isOperandConstantImmediateInt(op2)) {
                linkSlowCase(iter);
                emitPutJITStubArg(regT0, 1);
                emitPutJITStubArgFromVirtualRegister(currentInstruction[2].u.operand, 2, regT2);
                emitCTICall(JITStubs::cti_op_jless);
                emitJumpSlowToHot(branchTest32(Zero, regT0), target + 3);
            } else {
                linkSlowCase(iter);
                linkSlowCase(iter);
                emitPutJITStubArg(regT0, 1);
                emitPutJITStubArg(regT1, 2);
                emitCTICall(JITStubs::cti_op_jless);
                emitJumpSlowToHot(branchTest32(Zero, regT0), target + 3);
            }
            NEXT_OPCODE(op_jnless);
        }
        case op_not: {
            linkSlowCase(iter);
            xorPtr(Imm32(static_cast<int32_t>(JSImmediate::FullTagTypeBool)), regT0);
            emitPutJITStubArg(regT0, 1);
            emitCTICall(JITStubs::cti_op_not);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_not);
        }
        case op_jfalse: {
            linkSlowCase(iter);
            emitPutJITStubArg(regT0, 1);
            emitCTICall(JITStubs::cti_op_jtrue);
            unsigned target = currentInstruction[2].u.operand;
            emitJumpSlowToHot(branchTest32(Zero, regT0), target + 2); // inverted!
            NEXT_OPCODE(op_jfalse);
        }
        case op_post_inc: {
            compileFastArithSlow_op_post_inc(currentInstruction[1].u.operand, currentInstruction[2].u.operand, iter);
            NEXT_OPCODE(op_post_inc);
        }
        case op_bitnot: {
            linkSlowCase(iter);
            emitPutJITStubArg(regT0, 1);
            emitCTICall(JITStubs::cti_op_bitnot);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_bitnot);
        }
        case op_bitand: {
            compileFastArithSlow_op_bitand(currentInstruction[1].u.operand, currentInstruction[2].u.operand, currentInstruction[3].u.operand, iter);
            NEXT_OPCODE(op_bitand);
        }
        case op_jtrue: {
            linkSlowCase(iter);
            emitPutJITStubArg(regT0, 1);
            emitCTICall(JITStubs::cti_op_jtrue);
            unsigned target = currentInstruction[2].u.operand;
            emitJumpSlowToHot(branchTest32(NonZero, regT0), target + 2);
            NEXT_OPCODE(op_jtrue);
        }
        case op_post_dec: {
            compileFastArithSlow_op_post_dec(currentInstruction[1].u.operand, currentInstruction[2].u.operand, iter);
            NEXT_OPCODE(op_post_dec);
        }
        case op_bitxor: {
            linkSlowCase(iter);
            emitPutJITStubArg(regT0, 1);
            emitPutJITStubArg(regT1, 2);
            emitCTICall(JITStubs::cti_op_bitxor);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_bitxor);
        }
        case op_bitor: {
            linkSlowCase(iter);
            emitPutJITStubArg(regT0, 1);
            emitPutJITStubArg(regT1, 2);
            emitCTICall(JITStubs::cti_op_bitor);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_bitor);
        }
        case op_eq: {
            linkSlowCase(iter);
            emitPutJITStubArg(regT0, 1);
            emitPutJITStubArg(regT1, 2);
            emitCTICall(JITStubs::cti_op_eq);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_eq);
        }
        case op_neq: {
            linkSlowCase(iter);
            emitPutJITStubArg(regT0, 1);
            emitPutJITStubArg(regT1, 2);
            emitCTICall(JITStubs::cti_op_neq);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_neq);
        }
        case op_stricteq: {
            linkSlowCase(iter);
            linkSlowCase(iter);
#if !USE(ALTERNATE_JSIMMEDIATE)
            linkSlowCase(iter);
#endif
            emitPutJITStubArg(regT0, 1);
            emitPutJITStubArg(regT1, 2);
            emitCTICall(JITStubs::cti_op_stricteq);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_stricteq);
        }
        case op_nstricteq: {
            linkSlowCase(iter);
            linkSlowCase(iter);
#if !USE(ALTERNATE_JSIMMEDIATE)
            linkSlowCase(iter);
#endif
            emitPutJITStubArg(regT0, 1);
            emitPutJITStubArg(regT1, 2);
            emitCTICall(JITStubs::cti_op_nstricteq);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_nstricteq);
        }
        case op_instanceof: {
            linkSlowCase(iter);
            linkSlowCase(iter);
            linkSlowCase(iter);
            emitPutJITStubArgFromVirtualRegister(currentInstruction[2].u.operand, 1, regT2);
            emitPutJITStubArgFromVirtualRegister(currentInstruction[3].u.operand, 2, regT2);
            emitPutJITStubArgFromVirtualRegister(currentInstruction[4].u.operand, 3, regT2);
            emitCTICall(JITStubs::cti_op_instanceof);
            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_instanceof);
        }
        case op_mod: {
            compileFastArithSlow_op_mod(currentInstruction[1].u.operand, currentInstruction[2].u.operand, currentInstruction[3].u.operand, iter);
            NEXT_OPCODE(op_mod);
        }
        case op_mul: {
            compileFastArithSlow_op_mul(currentInstruction, iter);
            NEXT_OPCODE(op_mul);
        }

        case op_call: {
            compileOpCallSlowCase(currentInstruction, iter, callLinkInfoIndex++, opcodeID);
            NEXT_OPCODE(op_call);
        }
        case op_call_eval: {
            compileOpCallSlowCase(currentInstruction, iter, callLinkInfoIndex++, opcodeID);
            NEXT_OPCODE(op_call_eval);
        }
        case op_construct: {
            compileOpCallSlowCase(currentInstruction, iter, callLinkInfoIndex++, opcodeID);
            NEXT_OPCODE(op_construct);
        }
        case op_to_jsnumber: {
            linkSlowCaseIfNotJSCell(iter, currentInstruction[2].u.operand);
            linkSlowCase(iter);

            emitPutJITStubArg(regT0, 1);
            emitCTICall(JITStubs::cti_op_to_jsnumber);

            emitPutVirtualRegister(currentInstruction[1].u.operand);
            NEXT_OPCODE(op_to_jsnumber);
        }

        default:
            ASSERT_NOT_REACHED();
        }

        ASSERT_WITH_MESSAGE(iter == m_slowCases.end() || firstTo != iter->to,"Not enough jumps linked in slow case codegen.");
        ASSERT_WITH_MESSAGE(firstTo == (iter - 1)->to, "Too many jumps linked in slow case codegen.");

        emitJumpSlowToHot(jump(), 0);
    }

#if ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS)
    ASSERT(propertyAccessInstructionIndex == m_codeBlock->numberOfStructureStubInfos());
#endif
    ASSERT(callLinkInfoIndex == m_codeBlock->numberOfCallLinkInfos());

#ifndef NDEBUG
    // reset this, in order to guard it's use with asserts
    m_bytecodeIndex = (unsigned)-1;
#endif
}

void JIT::privateCompile()
{
    sampleCodeBlock(m_codeBlock);
#if ENABLE(OPCODE_SAMPLING)
    sampleInstruction(m_codeBlock->instructions().begin());
#endif

    // Could use a pop_m, but would need to offset the following instruction if so.
    pop(regT2);
    emitPutToCallFrameHeader(regT2, RegisterFile::ReturnPC);

    Jump slowRegisterFileCheck;
    Label afterRegisterFileCheck;
    if (m_codeBlock->codeType() == FunctionCode) {
        // In the case of a fast linked call, we do not set this up in the caller.
        emitPutImmediateToCallFrameHeader(m_codeBlock, RegisterFile::CodeBlock);

        emitGetCTIParam(STUB_ARGS_registerFile, regT0);
        addPtr(Imm32(m_codeBlock->m_numCalleeRegisters * sizeof(Register)), callFrameRegister, regT1);
        
        slowRegisterFileCheck = branch32(GreaterThan, regT1, Address(regT0, FIELD_OFFSET(RegisterFile, m_end)));
        afterRegisterFileCheck = label();
    }

    privateCompileMainPass();
    privateCompileLinkPass();
    privateCompileSlowCases();

    if (m_codeBlock->codeType() == FunctionCode) {
        slowRegisterFileCheck.link(this);
        m_bytecodeIndex = 0; // emitCTICall will add to the map, but doesn't actually need this...
        emitCTICall(JITStubs::cti_register_file_check);
#ifndef NDEBUG
        // reset this, in order to guard it's use with asserts
        m_bytecodeIndex = (unsigned)-1;
#endif
        jump(afterRegisterFileCheck);
    }

    ASSERT(m_jmpTable.isEmpty());

    RefPtr<ExecutablePool> allocator = m_globalData->poolForSize(m_assembler.size());
    void* code = m_assembler.executableCopy(allocator.get());
    JITCodeRef codeRef(code, allocator);
#ifndef NDEBUG
    codeRef.codeSize = m_assembler.size();
#endif

    PatchBuffer patchBuffer(code);

    // Translate vPC offsets into addresses in JIT generated code, for switch tables.
    for (unsigned i = 0; i < m_switches.size(); ++i) {
        SwitchRecord record = m_switches[i];
        unsigned bytecodeIndex = record.bytecodeIndex;

        if (record.type != SwitchRecord::String) {
            ASSERT(record.type == SwitchRecord::Immediate || record.type == SwitchRecord::Character); 
            ASSERT(record.jumpTable.simpleJumpTable->branchOffsets.size() == record.jumpTable.simpleJumpTable->ctiOffsets.size());

            record.jumpTable.simpleJumpTable->ctiDefault = patchBuffer.locationOf(m_labels[bytecodeIndex + 3 + record.defaultOffset]);

            for (unsigned j = 0; j < record.jumpTable.simpleJumpTable->branchOffsets.size(); ++j) {
                unsigned offset = record.jumpTable.simpleJumpTable->branchOffsets[j];
                record.jumpTable.simpleJumpTable->ctiOffsets[j] = offset ? patchBuffer.locationOf(m_labels[bytecodeIndex + 3 + offset]) : record.jumpTable.simpleJumpTable->ctiDefault;
            }
        } else {
            ASSERT(record.type == SwitchRecord::String);

            record.jumpTable.stringJumpTable->ctiDefault = patchBuffer.locationOf(m_labels[bytecodeIndex + 3 + record.defaultOffset]);

            StringJumpTable::StringOffsetTable::iterator end = record.jumpTable.stringJumpTable->offsetTable.end();            
            for (StringJumpTable::StringOffsetTable::iterator it = record.jumpTable.stringJumpTable->offsetTable.begin(); it != end; ++it) {
                unsigned offset = it->second.branchOffset;
                it->second.ctiOffset = offset ? patchBuffer.locationOf(m_labels[bytecodeIndex + 3 + offset]) : record.jumpTable.stringJumpTable->ctiDefault;
            }
        }
    }

    for (size_t i = 0; i < m_codeBlock->numberOfExceptionHandlers(); ++i) {
        HandlerInfo& handler = m_codeBlock->exceptionHandler(i);
        handler.nativeCode = patchBuffer.locationOf(m_labels[handler.target]);
    }

    for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter) {
        if (iter->to)
            patchBuffer.link(iter->from, iter->to);
    }

    if (m_codeBlock->hasExceptionInfo()) {
        m_codeBlock->callReturnIndexVector().reserveCapacity(m_calls.size());
        for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter)
            m_codeBlock->callReturnIndexVector().append(CallReturnOffsetToBytecodeIndex(patchBuffer.returnAddressOffset(iter->from), iter->bytecodeIndex));
    }

    // Link absolute addresses for jsr
    for (Vector<JSRInfo>::iterator iter = m_jsrSites.begin(); iter != m_jsrSites.end(); ++iter)
        patchBuffer.patch(iter->storeLocation, patchBuffer.locationOf(iter->target).addressForJSR());

#if ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS)
    for (unsigned i = 0; i < m_codeBlock->numberOfStructureStubInfos(); ++i) {
        StructureStubInfo& info = m_codeBlock->structureStubInfo(i);
        info.callReturnLocation = patchBuffer.locationOf(m_propertyAccessCompilationInfo[i].callReturnLocation);
        info.hotPathBegin = patchBuffer.locationOf(m_propertyAccessCompilationInfo[i].hotPathBegin);
    }
#endif
#if ENABLE(JIT_OPTIMIZE_CALL)
    for (unsigned i = 0; i < m_codeBlock->numberOfCallLinkInfos(); ++i) {
        CallLinkInfo& info = m_codeBlock->callLinkInfo(i);
        info.callReturnLocation = patchBuffer.locationOfNearCall(m_callStructureStubCompilationInfo[i].callReturnLocation);
        info.hotPathBegin = patchBuffer.locationOf(m_callStructureStubCompilationInfo[i].hotPathBegin);
        info.hotPathOther = patchBuffer.locationOfNearCall(m_callStructureStubCompilationInfo[i].hotPathOther);
        info.coldPathOther = patchBuffer.locationOf(m_callStructureStubCompilationInfo[i].coldPathOther);
    }
#endif

    m_codeBlock->setJITCode(codeRef);
}

void JIT::privateCompileCTIMachineTrampolines()
{
#if ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS)
    // (1) The first function provides fast property access for array length
    Label arrayLengthBegin = align();

    // Check eax is an array
    Jump array_failureCases1 = emitJumpIfNotJSCell(regT0);
    Jump array_failureCases2 = branchPtr(NotEqual, Address(regT0), ImmPtr(m_interpreter->m_jsArrayVptr));

    // Checks out okay! - get the length from the storage
    loadPtr(Address(regT0, FIELD_OFFSET(JSArray, m_storage)), regT0);
    load32(Address(regT0, FIELD_OFFSET(ArrayStorage, m_length)), regT0);

    Jump array_failureCases3 = branch32(Above, regT0, Imm32(JSImmediate::maxImmediateInt));

    // regT0 contains a 64 bit value (is positive, is zero extended) so we don't need sign extend here.
    emitFastArithIntToImmNoCheck(regT0, regT0);

    ret();

    // (2) The second function provides fast property access for string length
    Label stringLengthBegin = align();

    // Check eax is a string
    Jump string_failureCases1 = emitJumpIfNotJSCell(regT0);
    Jump string_failureCases2 = branchPtr(NotEqual, Address(regT0), ImmPtr(m_interpreter->m_jsStringVptr));

    // Checks out okay! - get the length from the Ustring.
    loadPtr(Address(regT0, FIELD_OFFSET(JSString, m_value) + FIELD_OFFSET(UString, m_rep)), regT0);
    load32(Address(regT0, FIELD_OFFSET(UString::Rep, len)), regT0);

    Jump string_failureCases3 = branch32(Above, regT0, Imm32(JSImmediate::maxImmediateInt));

    // regT0 contains a 64 bit value (is positive, is zero extended) so we don't need sign extend here.
    emitFastArithIntToImmNoCheck(regT0, regT0);
    
    ret();
#endif

#if !(PLATFORM(X86) || PLATFORM(X86_64))
#error "This code is less portable than it looks this code assumes that regT3 is callee preserved, which happens to be true on x86/x86-64."
#endif

    // (3) Trampolines for the slow cases of op_call / op_call_eval / op_construct.
    
    Label virtualCallPreLinkBegin = align();

    // Load the callee CodeBlock* into eax
    loadPtr(Address(regT2, FIELD_OFFSET(JSFunction, m_body)), regT0);
    loadPtr(Address(regT0, FIELD_OFFSET(FunctionBodyNode, m_code)), regT0);
    Jump hasCodeBlock1 = branchTestPtr(NonZero, regT0);
    pop(regT3);
    restoreArgumentReference();
    Call callJSFunction1 = call();
    emitGetJITStubArg(1, regT2);
    emitGetJITStubArg(3, regT1);
    push(regT3);
    hasCodeBlock1.link(this);

    // Check argCount matches callee arity.
    Jump arityCheckOkay1 = branch32(Equal, Address(regT0, FIELD_OFFSET(CodeBlock, m_numParameters)), regT1);
    pop(regT3);
    emitPutJITStubArg(regT3, 2);
    emitPutJITStubArg(regT0, 4);
    restoreArgumentReference();
    Call callArityCheck1 = call();
    move(regT1, callFrameRegister);
    emitGetJITStubArg(1, regT2);
    emitGetJITStubArg(3, regT1);
    push(regT3);
    arityCheckOkay1.link(this);
    
    compileOpCallInitializeCallFrame();

    pop(regT3);
    emitPutJITStubArg(regT3, 2);
    restoreArgumentReference();
    Call callDontLazyLinkCall = call();
    push(regT3);

    jump(regT0);

    Label virtualCallLinkBegin = align();

    // Load the callee CodeBlock* into eax
    loadPtr(Address(regT2, FIELD_OFFSET(JSFunction, m_body)), regT0);
    loadPtr(Address(regT0, FIELD_OFFSET(FunctionBodyNode, m_code)), regT0);
    Jump hasCodeBlock2 = branchTestPtr(NonZero, regT0);
    pop(regT3);
    restoreArgumentReference();
    Call callJSFunction2 = call();
    emitGetJITStubArg(1, regT2);
    emitGetJITStubArg(3, regT1);
    push(regT3);
    hasCodeBlock2.link(this);

    // Check argCount matches callee arity.
    Jump arityCheckOkay2 = branch32(Equal, Address(regT0, FIELD_OFFSET(CodeBlock, m_numParameters)), regT1);
    pop(regT3);
    emitPutJITStubArg(regT3, 2);
    emitPutJITStubArg(regT0, 4);
    restoreArgumentReference();
    Call callArityCheck2 = call();
    move(regT1, callFrameRegister);
    emitGetJITStubArg(1, regT2);
    emitGetJITStubArg(3, regT1);
    push(regT3);
    arityCheckOkay2.link(this);

    compileOpCallInitializeCallFrame();

    pop(regT3);
    emitPutJITStubArg(regT3, 2);
    restoreArgumentReference();
    Call callLazyLinkCall = call();
    push(regT3);

    jump(regT0);

    Label virtualCallBegin = align();

    // Load the callee CodeBlock* into eax
    loadPtr(Address(regT2, FIELD_OFFSET(JSFunction, m_body)), regT0);
    loadPtr(Address(regT0, FIELD_OFFSET(FunctionBodyNode, m_code)), regT0);
    Jump hasCodeBlock3 = branchTestPtr(NonZero, regT0);
    pop(regT3);
    restoreArgumentReference();
    Call callJSFunction3 = call();
    emitGetJITStubArg(1, regT2);
    emitGetJITStubArg(3, regT1);
    push(regT3);
    hasCodeBlock3.link(this);

    // Check argCount matches callee arity.
    Jump arityCheckOkay3 = branch32(Equal, Address(regT0, FIELD_OFFSET(CodeBlock, m_numParameters)), regT1);
    pop(regT3);
    emitPutJITStubArg(regT3, 2);
    emitPutJITStubArg(regT0, 4);
    restoreArgumentReference();
    Call callArityCheck3 = call();
    move(regT1, callFrameRegister);
    emitGetJITStubArg(1, regT2);
    emitGetJITStubArg(3, regT1);
    push(regT3);
    arityCheckOkay3.link(this);

    compileOpCallInitializeCallFrame();

    // load ctiCode from the new codeBlock.
    loadPtr(Address(regT0, FIELD_OFFSET(CodeBlock, m_jitCode)), regT0);

    jump(regT0);

#if ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS)
    Call array_failureCases1Call = makeTailRecursiveCall(array_failureCases1);
    Call array_failureCases2Call = makeTailRecursiveCall(array_failureCases2);
    Call array_failureCases3Call = makeTailRecursiveCall(array_failureCases3);
    Call string_failureCases1Call = makeTailRecursiveCall(string_failureCases1);
    Call string_failureCases2Call = makeTailRecursiveCall(string_failureCases2);
    Call string_failureCases3Call = makeTailRecursiveCall(string_failureCases3);
#endif

    // All trampolines constructed! copy the code, link up calls, and set the pointers on the Machine object.
    m_interpreter->m_executablePool = m_globalData->poolForSize(m_assembler.size());
    void* code = m_assembler.executableCopy(m_interpreter->m_executablePool.get());
    PatchBuffer patchBuffer(code);

#if ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS)
    patchBuffer.link(array_failureCases1Call, JITStubs::cti_op_get_by_id_array_fail);
    patchBuffer.link(array_failureCases2Call, JITStubs::cti_op_get_by_id_array_fail);
    patchBuffer.link(array_failureCases3Call, JITStubs::cti_op_get_by_id_array_fail);
    patchBuffer.link(string_failureCases1Call, JITStubs::cti_op_get_by_id_string_fail);
    patchBuffer.link(string_failureCases2Call, JITStubs::cti_op_get_by_id_string_fail);
    patchBuffer.link(string_failureCases3Call, JITStubs::cti_op_get_by_id_string_fail);

    m_interpreter->m_ctiArrayLengthTrampoline = patchBuffer.trampolineAt(arrayLengthBegin);
    m_interpreter->m_ctiStringLengthTrampoline = patchBuffer.trampolineAt(stringLengthBegin);
#endif
    patchBuffer.link(callArityCheck1, JITStubs::cti_op_call_arityCheck);
    patchBuffer.link(callArityCheck2, JITStubs::cti_op_call_arityCheck);
    patchBuffer.link(callArityCheck3, JITStubs::cti_op_call_arityCheck);
    patchBuffer.link(callJSFunction1, JITStubs::cti_op_call_JSFunction);
    patchBuffer.link(callJSFunction2, JITStubs::cti_op_call_JSFunction);
    patchBuffer.link(callJSFunction3, JITStubs::cti_op_call_JSFunction);
    patchBuffer.link(callDontLazyLinkCall, JITStubs::cti_vm_dontLazyLinkCall);
    patchBuffer.link(callLazyLinkCall, JITStubs::cti_vm_lazyLinkCall);

    m_interpreter->m_ctiVirtualCallPreLink = patchBuffer.trampolineAt(virtualCallPreLinkBegin);
    m_interpreter->m_ctiVirtualCallLink = patchBuffer.trampolineAt(virtualCallLinkBegin);
    m_interpreter->m_ctiVirtualCall = patchBuffer.trampolineAt(virtualCallBegin);
}

void JIT::emitGetVariableObjectRegister(RegisterID variableObject, int index, RegisterID dst)
{
    loadPtr(Address(variableObject, FIELD_OFFSET(JSVariableObject, d)), dst);
    loadPtr(Address(dst, FIELD_OFFSET(JSVariableObject::JSVariableObjectData, registers)), dst);
    loadPtr(Address(dst, index * sizeof(Register)), dst);
}

void JIT::emitPutVariableObjectRegister(RegisterID src, RegisterID variableObject, int index)
{
    loadPtr(Address(variableObject, FIELD_OFFSET(JSVariableObject, d)), variableObject);
    loadPtr(Address(variableObject, FIELD_OFFSET(JSVariableObject::JSVariableObjectData, registers)), variableObject);
    storePtr(src, Address(variableObject, index * sizeof(Register)));
}

} // namespace JSC

#endif // ENABLE(JIT)
