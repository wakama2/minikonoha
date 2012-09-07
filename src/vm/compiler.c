/****************************************************************************
 * Copyright (c) 2012, the Konoha project authors. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *	this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/
#include <minikonoha/minikonoha.h>
#include <minikonoha/sugar.h>
#include <stdio.h>
#include "vm.h"
#ifdef _CLASSICVM
#include "../../module/classicvm/classicvm_gen.h"
#include "../../module/classicvm/classicvm.h"
#else
#include "minivm.h"
#endif

#include <minikonoha/local.h>
/* ************************************************************************ */

#ifdef __cplusplus
extern "C" {
#endif

int verbose_code = 0;  // global variable

/*static void EXPR_asm(KonohaContext *kctx, kStmt *stmt, int a, kExpr *expr, int shift, int espidx)
  {
  DBG_ASSERT(expr != NULL);
//DBG_P("a=%d, shift=%d, espidx=%d", a, shift, espidx);
switch(expr->build) {
case TEXPR_CONST :
break;
case TEXPR_NEW   :
break;
case TEXPR_NULL  :
break;
case TEXPR_NCONST :
break;
case TEXPR_LOCAL :
break;
case TEXPR_BLOCK :
break;
case TEXPR_FIELD :
break;
case TEXPR_CALL  :
break;
case TEXPR_AND  :
break;
case TEXPR_OR  :
break;
case TEXPR_LET  :
break;
case TEXPR_STACKTOP  :
break;
default:
DBG_ABORT("unknown expr=%d", expr->build);
}
}
 */
void emit_string(const char *str, const char *prefix, const char *suffix, int indent) {
	int i;
	for (i = 0; i < indent; i++){
		putchar(' ');
	}
	printf("%s%s%s\n", prefix, str, suffix);
}


//IfStmt_getThenBlock();
//IfStmt_getElseBlock();

struct IRBuilder;
typedef void (*VisitStmt_t)(struct IRBuilder *self, kStmt *stmt);
typedef void (*VisitExpr_t)(struct IRBuilder *self, kExpr *expr);
struct IRBuilderBase {
	VisitStmt_t visitErrStmt;
	VisitStmt_t visitExprStmt;
	VisitStmt_t visitBlockStmt;
	VisitStmt_t visitReturnStmt;
	VisitStmt_t visitIfStmt;
	VisitStmt_t visitLoopStmt;
	VisitStmt_t visitJumpStmt;
	VisitStmt_t visitUndefinedStmt;
	VisitExpr_t visitConstExpr;
	VisitExpr_t visitNConstExpr;
	VisitExpr_t visitNewExpr;
	VisitExpr_t visitNullExpr;
	VisitExpr_t visitLocalExpr;
	VisitExpr_t visitBlockExpr;
	VisitExpr_t visitFieldExpr;
	VisitExpr_t visitCallExpr;
	VisitExpr_t visitAndExpr;
	VisitExpr_t visitOrExpr;
	VisitExpr_t visitLetExpr;
	VisitExpr_t visitStackTopExpr;
};

static void visitBlock(struct IRBuilder *builder, kBlock *bk);
static void handleExpr(struct IRBuilder *builder, kExpr *expr);
/*
typedef struct MiniVMBlock {
	unsigned id;
	char	*bytecode;
	struct MiniVMBlock *nextBlock;
	struct MiniVMBlock *jumpBlock;
} MiniVMBlock;

static void MiniVMBlock_init(MiniVMBlock* block)
{
	block->id = 0;
	block->bytecode = NULL;
	block->nextBlock = NULL;
	block->jumpBlock = NULL;
}
*/

struct IRBuilder {
	struct IRBuilderBase base;
	//MiniVMBlock *blocks;
	//MiniVMBlock *workingBlock;
	KonohaContext *kctx;
	union{
		int indent;
		int espidx;
	};
	kStmt* currentStmt;
};

static kBlock* Stmt_getFirstBlock(KonohaContext *kctx, kStmt *stmt)
{
	return SUGAR kStmt_getBlock(kctx, stmt, NULL, KW_BlockPattern, K_NULLBLOCK);
}

static kBlock* Stmt_getElseBlock(KonohaContext *kctx, kStmt *stmt)
{
	return SUGAR kStmt_getBlock(kctx, stmt, NULL, KW_else, K_NULLBLOCK);
}

static kExpr* Stmt_getFirstExpr(KonohaContext *kctx, kStmt *stmt)
{
	return SUGAR kStmt_getExpr(kctx, stmt, KW_ExprPattern, NULL); 
}

static kMethod* CallExpr_getMethod(kExpr *expr)
{
	return expr->cons->methodItems[0];
}

static int CallExpr_getArgCount(kExpr *expr)
{
	return kArray_size(expr->cons) - 2;
}

static kString* Stmt_getErrorMessage(KonohaContext *kctx, kStmt *stmt)
{
	kString* msg = (kString*)kStmt_getObjectNULL(kctx, stmt, KW_ERR);
	DBG_ASSERT(IS_String(msg));
	return msg;
}

/* ************************************************************************ */
static void DumpVisitor_visitErrStmt(struct IRBuilder *self, kStmt *stmt)
{
	KonohaContext *kctx = self->kctx;
	emit_string(S_text(kStmt_getObjectNULL(kctx, stmt, KW_ERR)), "", "", self->indent);
}

static void DumpVisitor_visitExprStmt(struct IRBuilder *self, kStmt *stmt)
{
	KonohaContext *kctx = self->kctx;
	handleExpr(self, Stmt_getFirstExpr(kctx, stmt));
}

static void DumpVisitor_visitBlockStmt(struct IRBuilder *self, kStmt *stmt)
{
	KonohaContext *kctx = self->kctx;
	visitBlock(self, Stmt_getFirstBlock(kctx, stmt)); 
}

static void DumpVisitor_visitReturnStmt(struct IRBuilder *self, kStmt *stmt)
{
	KonohaContext *kctx = self->kctx;
	emit_string("Return", "", "", self->indent);
	kExpr* expr = Stmt_getFirstExpr(kctx, stmt);
	if (expr != NULL && IS_Expr(expr)){
		handleExpr(self, expr);
	}
}

static void DumpVisitor_visitIfStmt(struct IRBuilder *self, kStmt *stmt)
{
	KonohaContext *kctx = self->kctx;
	self->indent++;
	emit_string("If", "", "", self->indent - 1);
	handleExpr(self, Stmt_getFirstExpr(kctx, stmt));
	emit_string("Then", "", "", self->indent - 1);
	visitBlock(self, Stmt_getFirstBlock(kctx, stmt)); 
	emit_string("Else", "", "", self->indent - 1);
	visitBlock(self, Stmt_getElseBlock(kctx, stmt)); 
	self->indent--;
}

static void DumpVisitor_visitLoopStmt(struct IRBuilder *self, kStmt *stmt)
{
	KonohaContext *kctx = self->kctx;
	self->indent++;
	emit_string("Loop", "", "", self->indent - 1);
	handleExpr(self, Stmt_getFirstExpr(kctx, stmt));
	emit_string("Body", "", "", self->indent - 1);
	visitBlock(self, Stmt_getFirstBlock(kctx, stmt)); 
	self->indent--;
}

static void DumpVisitor_visitJumpStmt(struct IRBuilder *self, kStmt *stmt)
{
	//KonohaContext *kctx = self->kctx;
	emit_string("Jump", "", "", self->indent);
}

static void DumpVisitor_visitUndefinedStmt(struct IRBuilder *self, kStmt *stmt)
{
	//KonohaContext *kctx = self->kctx;
}

static void DumpVisitor_visitConstExpr(struct IRBuilder *self, kExpr *expr)
{
	KonohaContext *kctx = self->kctx;
	KUtilsWriteBuffer wb;
	KLIB Kwb_init(&(kctx->stack->cwb), &wb);
	KonohaStack sfp[1];
	kObject *obj = expr->objectConstValue;
	sfp[0].o = obj;
	O_ct(obj)->p(kctx, sfp, 0, &wb, 0);
	char  *str = (char *) KLIB Kwb_top(kctx, &wb, 0);
	char buf[128];
	snprintf(buf, 128, "CONST:%s:'%s'", CT_t(O_ct(obj)), str);
	emit_string(buf, "", "", self->indent);
	KLIB Kwb_free(&wb);
}

static void DumpVisitor_visitNConstExpr(struct IRBuilder *self, kExpr *expr)
{
	KonohaContext *kctx = self->kctx;
	KUtilsWriteBuffer wb;
	KLIB Kwb_init(&(kctx->stack->cwb), &wb);
	KonohaStack sfp[1];
	unsigned long unboxVal = expr->unboxConstValue;
	KonohaClass *ct = CT_(expr->ty);
	sfp[0].unboxValue = unboxVal;
	ct->p(kctx, sfp, 0, &wb, 0);
	char  *str = (char *) KLIB Kwb_top(kctx, &wb, 0);
	char buf[128];
	snprintf(buf, 128, "NCONST:'%s'", str);
	emit_string(buf, "", "", self->indent);
	KLIB Kwb_free(&wb);
}

static void DumpVisitor_visitNewExpr(struct IRBuilder *self, kExpr *expr)
{
	//KonohaContext *kctx = self->kctx;
	emit_string("NEW", "", "", self->indent);
}

static void DumpVisitor_visitNullExpr(struct IRBuilder *self, kExpr *expr)
{
	KonohaContext *kctx = self->kctx;
	char buf[128];
	snprintf(buf, 128, "%s.NULL", CT_t(CT_(expr->ty)));
	emit_string(buf, "", "", self->indent);
}

static void DumpVisitor_visitLocalExpr(struct IRBuilder *self, kExpr *expr)
{
	KonohaContext *kctx = self->kctx;
	char buf[128];
	snprintf(buf, 128, "LOCAL(%d, %s)", (int)expr->index, CT_t(CT_(expr->ty)));
	emit_string(buf, "", "", self->indent);

}

static void DumpVisitor_visitBlockExpr(struct IRBuilder *self, kExpr *expr)
{
	//KonohaContext *kctx = self->kctx;
	emit_string("BLOCK", "", "", self->indent);
	visitBlock(self, expr->block);
}

static void DumpVisitor_visitFieldExpr(struct IRBuilder *self, kExpr *expr)
{
	//KonohaContext *kctx = self->kctx;
	emit_string("FIELD", "", "", self->indent);
}

static void DumpVisitor_visitCallExpr(struct IRBuilder *self, kExpr *expr)
{
	KonohaContext *kctx = self->kctx;
	KUtilsWriteBuffer wb;
	KLIB Kwb_init(&(kctx->stack->cwb), &wb);
	kMethod *mtd = CallExpr_getMethod(expr);
	KLIB Kwb_printf(kctx, &wb, "CALL: '%s%s'", T_mn(mtd->mn));
	self->indent++;
	emit_string(KLIB Kwb_top(kctx, &wb, 1), "(", "", self->indent);
	self->indent++;
	unsigned i;
	for(i = 1; i < kArray_size(expr->cons); ++i) {
		handleExpr(self, kExpr_at(expr, i));
	}
	self->indent--;
	emit_string(")", "", "", self->indent);
	self->indent--;
	KLIB Kwb_free(&wb);
}

static void DumpVisitor_visitAndExpr(struct IRBuilder *self, kExpr *expr)
{
	//KonohaContext *kctx = self->kctx;
	emit_string("AND", "", "", self->indent);
	self->indent++;
	unsigned i;
	for(i = 1; i < kArray_size(expr->cons); ++i) {
		handleExpr(self, kExpr_at(expr, i));
	}
	self->indent--;
}

static void DumpVisitor_visitOrExpr(struct IRBuilder *self, kExpr *expr)
{
	//KonohaContext *kctx = self->kctx;
	emit_string("OR", "", "", self->indent);
	self->indent++;
	unsigned i;
	for(i = 1; i < kArray_size(expr->cons); ++i) {
		handleExpr(self, kExpr_at(expr, i));
	}
	self->indent--;
}

static void DumpVisitor_visitLetExpr(struct IRBuilder *self, kExpr *expr)
{
	self->indent++;
	emit_string("LET ", "", "", self->indent);
	self->indent++;
	handleExpr(self, kExpr_at(expr, 1));
	emit_string(":=", "", "", self->indent);
	handleExpr(self, kExpr_at(expr, 2));
	self->indent--;
	self->indent--;
}

static void DumpVisitor_visitStackTopExpr(struct IRBuilder *self, kExpr *expr)
{
}

struct IRBuilder *createCppVisitor(struct IRBuilder *builder);
struct IRBuilder *createLLVMVisitor(struct IRBuilder *builder);
struct IRBuilder *createJSVisitor(struct IRBuilder *builder);

struct IRBuilder *createDumpVisitor(struct IRBuilder *builder)
{
#define VISITOR_LIST(OP) \
	OP(ErrStmt)\
	OP(ExprStmt)\
	OP(BlockStmt)\
	OP(ReturnStmt)\
	OP(IfStmt)\
	OP(LoopStmt)\
	OP(JumpStmt)\
	OP(UndefinedStmt)\
	OP(ConstExpr)\
	OP(NConstExpr)\
	OP(NewExpr)\
	OP(NullExpr)\
	OP(LocalExpr)\
	OP(BlockExpr)\
	OP(FieldExpr)\
	OP(CallExpr)\
	OP(AndExpr)\
	OP(OrExpr)\
	OP(LetExpr)\
	OP(StackTopExpr)

#define DEFINE_BUILDER_API(NAME) builder->base.visit##NAME = DumpVisitor_visit##NAME;
	VISITOR_LIST(DEFINE_BUILDER_API);
#undef DEFINE_BUILDER_API
	builder->indent = 0;
	return builder;
}

static void handleStmt(struct IRBuilder *builder, kStmt *stmt)
{
	kStmt *beforeStmt = builder->currentStmt;
	builder->currentStmt = stmt;
	switch(stmt->build) {
		case TSTMT_ERR:	builder->base.visitErrStmt(builder, stmt);       return;
		case TSTMT_EXPR:   builder->base.visitExprStmt(builder, stmt);   break;
		case TSTMT_BLOCK:  builder->base.visitBlockStmt(builder, stmt);  break;
		case TSTMT_RETURN: builder->base.visitReturnStmt(builder, stmt); return;
		case TSTMT_IF:	 builder->base.visitIfStmt(builder, stmt);       break;
		case TSTMT_LOOP:   builder->base.visitLoopStmt(builder, stmt);   break;
		case TSTMT_JUMP:   builder->base.visitJumpStmt(builder, stmt);   break;
		default: builder->base.visitUndefinedStmt(builder, stmt);        break;
	}
	builder->currentStmt = beforeStmt;
}


static void visitBlock(struct IRBuilder *builder, kBlock *bk)
{
	int i;
	for(i = 0; i < kArray_size(bk->stmtList); i++) {
		kStmt *stmt = bk->stmtList->stmtItems[i];
		if(stmt->syn == NULL) continue;
		handleStmt(builder, stmt);
	}
}

static void handleExpr(struct IRBuilder *builder, kExpr *expr)
{
	switch(expr->build) {
		case TEXPR_CONST:    builder->base.visitConstExpr(builder, expr);  break;
		case TEXPR_NEW:      builder->base.visitNewExpr(builder, expr);    break;
		case TEXPR_NULL:     builder->base.visitNullExpr(builder, expr);   break;
		case TEXPR_NCONST:   builder->base.visitNConstExpr(builder, expr); break;
		case TEXPR_LOCAL:    builder->base.visitLocalExpr(builder, expr);  break;
		case TEXPR_BLOCK:    builder->base.visitBlockExpr(builder, expr);  break;
		case TEXPR_FIELD:    builder->base.visitFieldExpr(builder, expr);  break;
		case TEXPR_CALL:     builder->base.visitCallExpr(builder, expr);   break;
		case TEXPR_AND:      builder->base.visitAndExpr(builder, expr);    break;
		case TEXPR_OR:       builder->base.visitOrExpr(builder, expr);     break;
		case TEXPR_LET:      builder->base.visitLetExpr(builder, expr);    break;
		case TEXPR_STACKTOP:
		default: builder->base.visitStackTopExpr(builder, expr);    break;
	}
}

/* ************************************************************************ */

#if defined(K_USING_THCODE_)
#define TADDR   NULL, 0/*counter*/
#else
#define TADDR   0/*counter*/
#endif/*K_USING_THCODE_*/
#define ASMLINE  0

#define OP_T(T) union { VirtualMachineInstruction op; T op_; }

#define NC_(sfpidx)    (((sfpidx) * 2) + 1)
#define OC_(sfpidx)    ((sfpidx) * 2)
#define SFP_(sfpidx)   ((sfpidx) * 2)
#define RIX_(rix)      rix

#define BasicBlock_codesize(BB)  ((BB)->codeTable.bytesize / sizeof(VirtualMachineInstruction))
#define BBOP(BB)     (BB)->codeTable.codeItems

#define ESP_(sfpidx, args)   SFP_(sfpidx + args + K_CALLDELTA + 1)

#define ASM(T, ...) do {\
	OP##T op_ = {TADDR, OPCODE_##T, ASMLINE, ## __VA_ARGS__};\
	union { VirtualMachineInstruction op; OP##T op_; } tmp_; tmp_.op_ = op_;\
	BUILD_asm(kctx, &tmp_.op, sizeof(OP##T));\
} while (0)

#define ASMop(T, OP, ...) do {\
	OP##T op_ = {TADDR, OP, ASMLINE, ## __VA_ARGS__};\
	union { VirtualMachineInstruction op; OP##T op_; } tmp_; tmp_.op_ = op_;\
	BUILD_asm(kctx, &tmp_.op, sizeof(OP##T));\
} while (0)

#define ASMbranch(T, lb, ...) do {\
	OP##T op_ = {TADDR, OPCODE_##T, ASMLINE, NULL, ## __VA_ARGS__};\
	union { VirtualMachineInstruction op; OP##T op_; } tmp_; tmp_.op_ = op_;\
	ASM_BRANCH_(kctx, lb, &tmp_.op, sizeof(OP##T)); \
} while (0)

#define kBasicBlock_add(bb, T, ...) do { \
	OP##T op_ = {TADDR, OPCODE_##T, ASMLINE, ## __VA_ARGS__};\
	union { VirtualMachineInstruction op; OP##T op_; } tmp_; tmp_.op_ = op_;\
	BasicBlock_add(kctx, bb, 0, &tmp_.op, sizeof(OP##T));\
} while (0)

static kBasicBlock* new_BasicBlockLABEL(KonohaContext *kctx)
{
	kBasicBlock *bb = new_(BasicBlock, 0);
	bb->id = kArray_size(ctxcode->codeList);
	KLIB kArray_add(kctx, ctxcode->codeList, bb);
	return bb;
}

static void BasicBlock_add(KonohaContext *kctx, kBasicBlock *bb, kushort_t line, VirtualMachineInstruction *op, size_t size)
{
	if(bb->codeTable.bytemax == 0) {
		KLIB Karray_init(kctx, &(bb->codeTable), 1 * sizeof(VirtualMachineInstruction));
	}
	else if(bb->codeTable.bytesize == bb->codeTable.bytemax) {
		KLIB Karray_expand(kctx, &(bb->codeTable), 4 * sizeof(VirtualMachineInstruction));
	}
	VirtualMachineInstruction *tailcode = bb->codeTable.codeItems + (bb->codeTable.bytesize/sizeof(VirtualMachineInstruction));
	memcpy(tailcode, op, size == 0 ? sizeof(VirtualMachineInstruction) : size);
	tailcode->line = line;
	bb->codeTable.bytesize += sizeof(VirtualMachineInstruction);
}

static kObject* BUILD_addConstPool(KonohaContext *kctx, kObject *o)
{
	KLIB kArray_add(kctx, ctxcode->constPools, o);
	return o;
}

static void BUILD_asm(KonohaContext *kctx, VirtualMachineInstruction *op, size_t opsize)
{
	assert(op->opcode != OPCODE_JMPF);
	BasicBlock_add(kctx, ctxcode->currentWorkingBlock, ctxcode->uline, op, opsize);
}

static int BUILD_asmJMPF(KonohaContext *kctx, OPJMPF *op)
{
	kBasicBlock *bb = ctxcode->currentWorkingBlock;
	DBG_ASSERT(op->opcode == OPCODE_JMPF);
	int swap = 0;
#ifdef _CLASSICVM
	if (CLASSICVM_BUILD_asmJMPF(kctx, bb, op, &swap)) {
		return swap;
	}
#endif
	BasicBlock_add(kctx, bb, ctxcode->uline, (VirtualMachineInstruction*)op, 0);
	return swap;
}


static void ASM_LABEL(KonohaContext *kctx, kBasicBlock *labelBlock)
{
	if(labelBlock != NULL) {
		kBasicBlock *bb = ctxcode->currentWorkingBlock;
		if(bb != NULL) {
			bb->nextBlock = labelBlock;
			bb->branchBlock = NULL;
			labelBlock->incoming += 1;
		}
		ctxcode->currentWorkingBlock = labelBlock;
	}
}

static void ASM_JMP(KonohaContext *kctx, kBasicBlock *labelBlock)
{
	kBasicBlock *bb = ctxcode->currentWorkingBlock;
	if(bb != NULL) {
		bb->nextBlock = NULL;
		bb->branchBlock = labelBlock;
		labelBlock->incoming += 1;
	}
	ctxcode->currentWorkingBlock = NULL;
}

static kBasicBlock* ASM_JMPF(KonohaContext *kctx, int flocal, kBasicBlock *lbJUMP)
{
	kBasicBlock *bb = ctxcode->currentWorkingBlock;
	kBasicBlock *lbNEXT = new_BasicBlockLABEL(kctx);
	OPJMPF op = {TADDR, OPCODE_JMPF, ASMLINE, NULL, NC_(flocal)};
	if(BUILD_asmJMPF(kctx, &op)) {
		bb->branchBlock = lbNEXT;
		bb->nextBlock = lbJUMP;
	}
	else {
		bb->branchBlock = lbJUMP;
		bb->nextBlock = lbNEXT;
	}
	lbNEXT->incoming += 1;
	ctxcode->currentWorkingBlock = lbNEXT;
	lbJUMP->incoming += 1;
	return lbJUMP;
}

static void ASM_SAFEPOINT(KonohaContext *kctx, int espidx)
{
	kBasicBlock *bb = ctxcode->currentWorkingBlock;
	size_t i;
	for(i = 0; i < BasicBlock_codesize(bb); i++) {
		VirtualMachineInstruction *op = BBOP(bb) + i;
		if(op->opcode == OPCODE_SAFEPOINT) return;
	}
	ASM(SAFEPOINT, SFP_(espidx));
}

static void NMOV_asm(KonohaContext *kctx, int a, ktype_t ty, int b)
{
	if(TY_isUnbox(ty)) {
		ASM(NMOV, NC_(a), NC_(b), CT_(ty));
	}
	else {
		ASM(NMOV, OC_(a), OC_(b), CT_(ty));
	}
}

static kBasicBlock* KonohaVisitor_asmJMPIF(struct IRBuilder *self, int a, kExpr *expr, int isTRUE, kBasicBlock* label)
{
	KonohaContext *kctx = self->kctx;
	handleExpr(self, expr);
	if(isTRUE) {
		ASM(BNOT, NC_(a), NC_(a));
	}
	return ASM_JMPF(kctx, a, label);
}


static void KonohaVisitor_visitErrStmt(struct IRBuilder *self, kStmt *stmt)
{
	KonohaContext *kctx = self->kctx;
	ASM(ERROR, SFP_(self->espidx), Stmt_getErrorMessage(kctx, stmt));
}

static void KonohaVisitor_visitExprStmt(struct IRBuilder *self, kStmt *stmt)
{
	KonohaContext *kctx = self->kctx;
	handleExpr(self, Stmt_getFirstExpr(kctx, stmt));
}

static void KonohaVisitor_visitBlockStmt(struct IRBuilder *self, kStmt *stmt)
{
	KonohaContext *kctx = self->kctx;
	visitBlock(self, Stmt_getFirstBlock(kctx, stmt)); 
}

static void KonohaVisitor_visitReturnStmt(struct IRBuilder *self, kStmt *stmt)
{
	KonohaContext *kctx = self->kctx;
	kExpr *expr = (kExpr*)kStmt_getObjectNULL(kctx, stmt, KW_ExprPattern);
	if(expr != NULL && IS_Expr(expr) && expr->ty != TY_void) {
		handleExpr(self, expr);
	}
	ASM_JMP(kctx, ctxcode->lbEND);  // RET
}

static void KonohaVisitor_visitIfStmt(struct IRBuilder *self, kStmt *stmt)
{
	KonohaContext *kctx = self->kctx;
	int espidx = self->espidx;
	kBasicBlock*  lbELSE = new_BasicBlockLABEL(kctx);
	kBasicBlock*  lbEND  = new_BasicBlockLABEL(kctx);
	/* if */
	lbELSE = KonohaVisitor_asmJMPIF(self, espidx, Stmt_getFirstExpr(kctx, stmt), 0/*FALSE*/, lbELSE);
	/* then */
	visitBlock(self, Stmt_getFirstBlock(kctx, stmt));
	ASM_JMP(kctx, lbEND);
	/* else */
	ASM_LABEL(kctx, lbELSE);
	visitBlock(self, Stmt_getElseBlock(kctx, stmt));
	/* endif */
	ASM_LABEL(kctx, lbEND);
}

static void KonohaVisitor_visitLoopStmt(struct IRBuilder *self, kStmt *stmt)
{
	KonohaContext *kctx = self->kctx;
	int espidx = self->espidx;
	kBasicBlock* lbCONTINUE = new_BasicBlockLABEL(kctx);
	kBasicBlock* lbBREAK = new_BasicBlockLABEL(kctx);
	KLIB kObject_setObject(kctx, stmt, SYM_("continue"), TY_BasicBlock, lbCONTINUE);
	KLIB kObject_setObject(kctx, stmt, SYM_("break"), TY_BasicBlock, lbBREAK);
	ASM_LABEL(kctx, lbCONTINUE);
	ASM_SAFEPOINT(kctx, espidx);
	KonohaVisitor_asmJMPIF(self, espidx, Stmt_getFirstExpr(kctx, stmt), 0/*FALSE*/, lbBREAK);
	
	visitBlock(self, Stmt_getFirstBlock(kctx, stmt));
	ASM_JMP(kctx, lbCONTINUE);
	ASM_LABEL(kctx, lbBREAK);
}

static void KonohaVisitor_visitJumpStmt(struct IRBuilder *self, kStmt *stmt)
{
	KonohaContext *kctx = self->kctx;
	SugarSyntax *syn = stmt->syn;
	kStmt *jump = (kStmt*)kStmt_getObject(kctx, stmt, syn->keyword, NULL);
	DBG_ASSERT(jump != NULL);
	DBG_ASSERT(IS_Stmt(jump));
	kBasicBlock* lbJUMP = (kBasicBlock*)KLIB kObject_getObject(kctx, jump, syn->keyword, NULL);
	DBG_ASSERT(lbJUMP != NULL);
	DBG_ASSERT(IS_BasicBlock(lbJUMP));
	ASM_JMP(kctx, lbJUMP);
}

static void KonohaVisitor_visitUndefinedStmt(struct IRBuilder *self, kStmt *stmt)
{
	KonohaContext *kctx = self->kctx;
	DBG_P("undefined asm syntax kw='%s'", SYM_t(stmt->syn->keyword));
}

static void KonohaVisitor_visitConstExpr(struct IRBuilder *self, kExpr *expr)
{
	KonohaContext *kctx = self->kctx;
	int a = self->espidx;
	kObject *v = expr->objectConstValue;
	DBG_ASSERT(!TY_isUnbox(expr->ty));
	DBG_ASSERT(Expr_hasObjectConstValue(expr));
	v = BUILD_addConstPool(kctx, v);
	ASM(NSET, OC_(a), (uintptr_t)v, CT_(expr->ty));
}

static void KonohaVisitor_visitNConstExpr(struct IRBuilder *self, kExpr *expr)
{
	KonohaContext *kctx = self->kctx;
	int a = self->espidx;
	ASM(NSET, NC_(a), expr->unboxConstValue, CT_(expr->ty));
}

static void KonohaVisitor_visitNewExpr(struct IRBuilder *self, kExpr *expr)
{
	KonohaContext *kctx = self->kctx;
	int a = self->espidx;
	ASM(NEW, OC_(a), expr->index, CT_(expr->ty));
}

static void KonohaVisitor_visitNullExpr(struct IRBuilder *self, kExpr *expr)
{
	KonohaContext *kctx = self->kctx;
	int a = self->espidx;
	if(TY_isUnbox(expr->ty)) {
		ASM(NSET, NC_(a), 0, CT_(expr->ty));
	}
	else {
		ASM(NULL, OC_(a), CT_(expr->ty));
	}
}

static void KonohaVisitor_visitLocalExpr(struct IRBuilder *self, kExpr *expr)
{
	KonohaContext *kctx = self->kctx;
	int a = self->espidx;
	NMOV_asm(kctx, a, expr->ty, expr->index);
}

static void KonohaVisitor_visitBlockExpr(struct IRBuilder *self, kExpr *expr)
{
	KonohaContext *kctx = self->kctx;
	int a = self->espidx;
	DBG_ASSERT(IS_Block(expr->block));
	visitBlock(self, expr->block);
	NMOV_asm(kctx, a, expr->ty, self->espidx);
}

static void KonohaVisitor_visitFieldExpr(struct IRBuilder *self, kExpr *expr)
{
	KonohaContext *kctx = self->kctx;
	int a = self->espidx;
	kshort_t index = (kshort_t)expr->index;
	kshort_t xindex = (kshort_t)(expr->index >> (sizeof(kshort_t)*8));
	if(TY_isUnbox(expr->ty)) {
		ASM(NMOVx, NC_(a), OC_(index), xindex, CT_(expr->ty));
	}
	else {
		ASM(NMOVx, OC_(a), OC_(index), xindex, CT_(expr->ty));
	}
}

static void KonohaVisitor_visitCallExpr(struct IRBuilder *self, kExpr *expr)
{
	KonohaContext *kctx = self->kctx;
	kMethod *mtd = CallExpr_getMethod(expr);
	DBG_ASSERT(IS_Method(mtd));

	int i;
	int s = Method_isStatic(mtd) ? 2 : 1;
	int espidx = self->espidx;
	int thisidx = espidx + K_CALLDELTA;
	for(i = s; i < kArray_size(expr->cons); i++) {
		kExpr *exprN = kExpr_at(expr, i);
		DBG_ASSERT(IS_Expr(exprN));
		handleExpr(self, exprN);
	}

	int argc = CallExpr_getArgCount(expr);
	if(Method_isFinal(mtd) || !Method_isVirtual(mtd)) {
		ASM(NSET, NC_(thisidx-1), (intptr_t)mtd, CT_Method);
	}
	else {
		ASM(LOOKUP, SFP_(thisidx), Stmt_nameSpace(self->currentStmt), mtd);
	}

	ASM(CALL, ctxcode->uline, SFP_(thisidx), ESP_(espidx, argc), KLIB Knull(kctx, CT_(expr->ty)));
}


static void KonohaVisitor_visitAndExpr(struct IRBuilder *self, kExpr *expr)
{
	KonohaContext *kctx = self->kctx;
	int a = self->espidx;
	int i, size = kArray_size(expr->cons);
	kBasicBlock*  lbTRUE = new_BasicBlockLABEL(kctx);
	kBasicBlock*  lbFALSE = new_BasicBlockLABEL(kctx);
	for(i = 1; i < size; i++) {
		KonohaVisitor_asmJMPIF(self, a, kExpr_at(expr, i), 0/*FALSE*/, lbFALSE);
	}
	ASM(NSET, NC_(a), 1/*O_data(K_TRUE)*/, CT_Boolean);
	ASM_JMP(kctx, lbTRUE);
	ASM_LABEL(kctx, lbFALSE); // false
	ASM(NSET, NC_(a), 0/*O_data(K_FALSE)*/, CT_Boolean);
	ASM_LABEL(kctx, lbTRUE);   // TRUE
}

static void KonohaVisitor_visitOrExpr(struct IRBuilder *self, kExpr *expr)
{
	KonohaContext *kctx = self->kctx;
	int a = self->espidx;
	int i, size = kArray_size(expr->cons);
	kBasicBlock*  lbTRUE = new_BasicBlockLABEL(kctx);
	kBasicBlock*  lbFALSE = new_BasicBlockLABEL(kctx);
	for(i = 1; i < size; i++) {
		KonohaVisitor_asmJMPIF(self, a, kExpr_at(expr, i), 1/*TRUE*/, lbFALSE);
	}
	ASM(NSET, NC_(a), 0/*O_data(K_FALSE)*/, CT_Boolean);
	ASM_JMP(kctx, lbFALSE);
	ASM_LABEL(kctx, lbTRUE);
	ASM(NSET, NC_(a), 1/*O_data(K_TRUE)*/, CT_Boolean);
	ASM_LABEL(kctx, lbFALSE); // false
}

static void KonohaVisitor_visitLetExpr(struct IRBuilder *self, kExpr *expr)
{
	KonohaContext *kctx = self->kctx;
	int a = self->espidx;
	int espidx = self->espidx;
	kExpr *exprL = kExpr_at(expr, 1);
	kExpr *exprR = kExpr_at(expr, 2);
	if(exprL->build == TEXPR_LOCAL) {
		handleExpr(self, exprR);
		if(a != espidx) {
			NMOV_asm(kctx, a, exprL->ty, espidx);
		}
	}
	else if(exprL->build == TEXPR_STACKTOP) {
		DBG_P("LET TEXPR_STACKTOP a=%d, exprL->index=%d, espidx=%d", a, exprL->index, espidx);
		handleExpr(self, exprR);
		if(a != espidx) {
			NMOV_asm(kctx, a, exprL->ty, exprL->index + espidx);
		}
	}
	else{
		assert(exprL->build == TEXPR_FIELD);
		handleExpr(self, exprR);
		kshort_t index = (kshort_t)exprL->index;
		kshort_t xindex = (kshort_t)(exprL->index >> (sizeof(kshort_t)*8));
		if(TY_isUnbox(exprR->ty)) {
			ASM(XNMOV, OC_(index), xindex, NC_(espidx), CT_(exprL->ty));
		}
		else {
			ASM(XNMOV, OC_(index), xindex, OC_(espidx), CT_(exprL->ty));
		}
		if(a != espidx) {
			NMOV_asm(kctx, a, exprL->ty, espidx);
		}
	}

}

static void KonohaVisitor_visitStackTopExpr(struct IRBuilder *self, kExpr *expr)
{
	//KonohaContext *kctx = self->kctx;
}


struct IRBuilder *createKonohaVisitor(struct IRBuilder *builder)
{
#define DEFINE_BUILDER_API(NAME) builder->base.visit##NAME = KonohaVisitor_visit##NAME;
	VISITOR_LIST(DEFINE_BUILDER_API);
#undef DEFINE_BUILDER_API
	builder->indent = 0;
	builder->espidx = 0;
	//builder->blocks = NULL;
	//builder->workingBlock = NULL;
	builder->currentStmt = NULL;
	return builder;
}

static void kMethod_genCode(KonohaContext *kctx, kMethod *mtd, kBlock *bk)
{
	DBG_P("START CODE GENERATION..");
	INIT_GCSTACK();
	//if(ctxcode == NULL) {
	//	kmodcode->h.setup(kctx, NULL, 0);
	//}
	struct IRBuilder *builder, builderbuf;
	builder = createDumpVisitor(&builderbuf);
	builder->kctx = kctx;
	visitBlock(builder, bk);
	RESET_GCSTACK();
}

/* ------------------------------------------------------------------------ */
/* [datatype] */

//#define PACKSUGAR	.packageId = 1, .packageDomain = 1

static KMETHOD MethodFunc_invokeAbstractMethod(KonohaContext *kctx, KonohaStack *sfp)
{
	RETURNi_(0);
}

static void kMethod_setFunc(KonohaContext *kctx, kMethod *mtd, MethodFunc func)
{
	func = (func == NULL) ? MethodFunc_invokeAbstractMethod : func;
	((kMethodVar*)mtd)->invokeMethodFunc = func;
	//((kMethodVar*)mtd)->pc_start = CODE_NCALL;
}

void MODCODE_init(KonohaContext *kctx, KonohaContextVar *ctx)
{
	KonohaLibVar *l = (KonohaLibVar*)kctx->klib;
	l->kMethod_genCode = kMethod_genCode;
	l->kMethod_setFunc = kMethod_setFunc;
}

#ifdef __cplusplus
}
#endif
