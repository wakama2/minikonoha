/****************************************************************************
 * Copyright (c) 2012, the Konoha project authors. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef SUGAR_H_
#define SUGAR_H_

/* ------------------------------------------------------------------------ */
/* sugar.h */

#include "minikonoha.h"
#include "klib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KW_END              ((ksymbol_t)-1)
#define KW_ERR              (((ksymbol_t)0)|0) /**/
#define KW_ExprPattern      (((ksymbol_t)1)|KW_PATTERN) /*$Expr*/
#define KW_SymbolPattern    (((ksymbol_t)2)|KW_PATTERN) /*$Symbol*/
#define KW_TextPattern      (((ksymbol_t)3)|KW_PATTERN) /*$Text*/
#define KW_NumberPattern    (((ksymbol_t)4)|KW_PATTERN) /*$Number*/
#define KW_TypePattern      (((ksymbol_t)5)|KW_PATTERN) /*$Type*/

#define KW_ParenthesisGroup (((ksymbol_t)6)) /*()*/
#define KW_BracketGroup     (((ksymbol_t)7)) /*[]*/
#define KW_BraceGroup       (((ksymbol_t)8)) /*{}*/
#define KW_TypeCastGroup    (((ksymbol_t)6)|KW_PATTERN)    /*$()*/
#define KW_TypeParamGroup   (((ksymbol_t)7)|KW_PATTERN)    /*$[]*/
#define KW_OptionalGroup    (((ksymbol_t)7)|KW_ATMARK)     /*@[]*/
#define KW_BlockPattern     (((ksymbol_t)9)|KW_PATTERN)    /*$Block*/
#define KW_ParamPattern     (((ksymbol_t)10)|KW_PATTERN)   /*$Param*/
#define KW_TypeDeclPattern  (((ksymbol_t)11)|KW_PATTERN)   /*$TypeDecl*/
#define KW_MethodDeclPattern  (((ksymbol_t)12)|KW_PATTERN) /*$MethodDecl*/
#define KW_TokenPattern     (((ksymbol_t)13)|KW_PATTERN)   /*$Token*/

#define KW_ExprOperator        KW_ParamPattern
#define KW_ExprTerm            KW_SymbolPattern
#define KW_ExprMethodCall      KW_ParamPattern

#define KW_DOT     14
#define KW_DIV     (1+KW_DOT)
#define KW_MOD     (2+KW_DOT)
#define KW_MUL     (3+KW_DOT)
#define KW_ADD     (4+KW_DOT)
#define KW_SUB     (5+KW_DOT)
#define KW_LT      (6+KW_DOT)
#define KW_LTE     (7+KW_DOT)
#define KW_GT      (8+KW_DOT)
#define KW_GTE     (9+KW_DOT)
#define KW_EQ      (10+KW_DOT)
#define KW_NEQ     (11+KW_DOT)
#define KW_AND     (12+KW_DOT)
#define KW_OR      (13+KW_DOT)
#define KW_NOT     (14+KW_DOT)
#define KW_LET     (15+KW_DOT)
#define KW_COMMA   (16+KW_DOT)
#define KW_DOLLAR  KW_PATTERN
#define KW_ATMARK  MN_Annotation
#define KW_COLON   (17+KW_DOT)
#define KW_SEMICOLON (18+KW_DOT)

#define KW_true      33
#define KW_false     (1+KW_true)
#define KW_if        (2+KW_true)
#define KW_else      (3+KW_true)
#define KW_return    (4+KW_true)

typedef enum {
	TokenType_INDENT = 1,
	TokenType_SYMBOL = KW_SymbolPattern,
	TokenType_TEXT   = KW_TextPattern,
	TokenType_INT    = KW_NumberPattern,
	TokenType_CODE   = KW_BlockPattern,
	TokenType_ERR    = KW_TokenPattern
} kTokenType;

// reserved
//#define MN_new       (8+KW_void)
#define FN_this      FN_("this")

// Tokenizer

typedef enum {
	KonohaChar_Null                 = 0,
	KonohaChar_Undefined            = 1,
	KonohaChar_Digit                = 2,
	KonohaChar_UpperCaseAlphabet    = 3,
	KonohaChar_LowerCaseAlphabet    = 4,
	KonohaChar_Unicode              = 5,
	KonohaChar_NewLine              = 6,
	KonohaChar_Tab                  = 7,
	KonohaChar_Space                = 8,
	KonohaChar_OpenParenthesis      =9,
	KonohaChar_CloseParenthesis      =10,
	KonohaChar_OpenBracket       =11,
	KonohaChar_CloseBracket       =12,
	KonohaChar_OpenBrace       =13,
	KonohaChar_CloseBrace       =14,
	KonohaChar_LessThan        =15,
	KonohaChar_GreaterThan        =16,
	KonohaChar_Quote     =17,
	KonohaChar_DoubleQuote    =18,
	KonohaChar_BackQuote   =19,
	KonohaChar_Surprised   =20,
	KonohaChar_Sharp     =21,
	KonohaChar_Dollar    =22,
	KonohaChar_Percent       =23,
	KonohaChar_And       =24,
	KonohaChar_Star      =25,
	KonohaChar_Plus      =26,
	KonohaChar_Comma     =27,
	KonohaChar_Minus     =28,
	KonohaChar_Dot       =29,
	KonohaChar_Slash     =30,
	KonohaChar_Colon     =31,
	KonohaChar_SemiColon =32,
	KonohaChar_Equal        =33,
	KonohaChar_Question  =34,
	KonohaChar_AtMark       =35,
	KonohaChar_Var       =36,
	KonohaChar_Childer   =37,
	KonohaChar_BackSlash   =38,
	KonohaChar_Hat       =39,
	KonohaChar_UnderBar     =40,
	KonohaChar_MAX = 41
} KonohaChar;

#define KCHAR_MAX  41
#define SIZEOF_TOKENMATRIX   (sizeof(void *) * KCHAR_MAX * 2)
typedef struct Tokenizer Tokenizer;
typedef int (*TokenizeFunc)(KonohaContext *, kTokenVar *, Tokenizer *, int);

struct Tokenizer {
	kNameSpace         *ns;
	const char         *source;
	size_t              sourceLength;
	kfileline_t         currentLine;
	kArray             *tokenList;
	int                 tabsize;
	const TokenizeFunc *cFuncItems;
	union {
		kFunc         **FuncItems;
		kArray        **funcListItems;
	};
	kString            *preparedString;
};

//#define VAR_TRACE
#ifndef VAR_TRACE
#define VAR_TRACE DBG_P("tracing..")
#endif

// int TokenFunc(Token tk, Source s)
#define VAR_TokenFunc(TK, S)\
		kTokenVar *TK = (kTokenVar *)sfp[1].asObject;\
		kString *S = sfp[2].asString;\
		Tokenizer *tokenizer = (Tokenizer *)sfp[1].unboxValue;\
		int tok_start = (ksymbol_t)sfp[2].intValue;\
		VAR_TRACE; (void)TK; (void)S; (void)tok_start; (void)tokenizer;

// int PatternMatch(Stmt stmt, int classNameSymbol, Token[] toks, int s, int e)
#define VAR_PatternMatch(STMT, NAME, TLS, S, E)\
		kStmt *STMT = (kStmt *)sfp[1].asObject;\
		ksymbol_t NAME = (ksymbol_t)sfp[2].intValue;\
		kArray *TLS = (kArray *)sfp[3].asObject;\
		int S = (int)sfp[4].intValue;\
		int E = (int)sfp[5].intValue;\
		VAR_TRACE; (void)STMT; (void)NAME; (void)TLS; (void)S; (void)E

// Expr Expression(Stmt stmt, Token[] tokenList, int s, int c, int e)
#define VAR_Expression(STMT, TLS, S, C, E)\
		SugarSyntax *syn = (SugarSyntax *)sfp[0].unboxValue;\
		kStmt *STMT = (kStmt *)sfp[1].asObject;\
		kArray *TLS = (kArray *)sfp[2].asObject;\
		int S = (int)sfp[3].intValue;\
		int C = (int)sfp[4].intValue;\
		int E = (int)sfp[5].intValue;\
		VAR_TRACE; (void)syn; (void)STMT; (void)TLS; (void)S; (void)C; (void)E

// boolean Statement(Stmt stmt, Gamma gma)
#define VAR_Statement(STMT, GMA)\
		kStmt *STMT = (kStmt *)sfp[1].asObject;\
		kGamma *GMA = (kGamma *)sfp[2].asObject;\
		VAR_TRACE; (void)STMT; (void)GMA

// Expr TypeCheck(Stmt stmt, Expr expr, Gamma gma, int typeid)
#define VAR_TypeCheck(STMT, EXPR, GMA, TY) \
		kStmt *STMT = (kStmt *)sfp[1].asObject;\
		kExprVar *EXPR = (kExprVar *)sfp[2].asObject;\
		kGamma *GMA = (kGamma *)sfp[3].asObject;\
		kattrtype_t TY = (kattrtype_t)sfp[4].intValue;\
		VAR_TRACE; (void)STMT; (void)EXPR; (void)GMA; (void)TY


typedef const struct SugarSyntaxVar   SugarSyntax;
typedef struct SugarSyntaxVar         SugarSyntaxVar;

typedef enum {
	SugarFunc_TokenFunc         = 0,
	SugarFunc_PatternMatch      = 1,
	SugarFunc_Expression        = 2,
	SugarFunc_TopLevelStatement = 3,
	SugarFunc_Statement         = 4,
	SugarFunc_TypeCheck         = 5,
	SugarFunc_SIZE              = 6
} SugerFunc;


#define SYNFLAG_Macro               ((kshortflag_t)1)

#define SYNFLAG_ExprLeftJoinOp2     ((kshortflag_t)1 << 1)
#define SYNFLAG_ExprPostfixOp2      ((kshortflag_t)1 << 2)

#define SYNFLAG_StmtBreakExec       ((kshortflag_t)1 << 8)  /* return, throw */
#define SYNFLAG_StmtJumpAhead0      ((kshortflag_t)1 << 9)  /* continue */
#define SYNFLAG_StmtJumpSkip0       ((kshortflag_t)1 << 10)  /* break */

#define SYN_Is(P, o)      (TFLAG_is(uintptr_t,(o)->flag, SYNFLAG_##P))
#define SYN_Set(P,o,B)     TFLAG_set(uintptr_t,(o)->flag, SYNFLAG_##P, B)

struct SugarSyntaxVar {
	ksymbol_t  keyword;               kshortflag_t  flag;
	const struct SugarSyntaxVar      *parentSyntaxNULL;
	kArray                           *syntaxPatternListNULL_OnList;
	union {
		kFunc                        *sugarFuncTable[SugarFunc_SIZE];
		kArray                       *sugarFuncListTable[SugarFunc_SIZE];
	};
	kshort_t tokenKonohaChar;
	kshort_t precedence_op2;          kshort_t precedence_op1;
	kpackageId_t lastLoadedPackageId;
	kshort_t macroParamSize;
	kArray                           *macroDataNULL_OnList;
};

#define PatternMatch_(NAME)    .PatternMatch   = PatternMatch_##NAME
#define Expression_(NAME)       .Expression      = Expression_##NAME
#define TopLevelStatement_(NAME)  .TopLevelStatement = Statement_##NAME
#define Statement_(NAME)     .Statement    = Statement_##NAME
#define TypeCheck_(NAME)     .TypeCheck    = TypeCheck_##NAME

#define _OPLeft   .flag = (SYNFLAG_ExprLeftJoinOp2)

// operator prcedence

typedef enum {
	Precedence_CStyleCALL     = 100,  /*x(), x[], x.x x->x x++ */
	Precedence_CStylePREUNARY = 200,  /*++x, --x, sizeof x &x +x -x !x */
	Precedence_CStyleCAST     = 300,  /* (T)x */
	Precedence_CStyleMUL      = 400,  /* x * x, x / x, x % x*/
	Precedence_CStyleADD      = 500,  /* x + x, x - x */
	Precedence_CStyleSHIFT    = 600,  /* x << x, x >> x */
	Precedence_CStyleCOMPARE  = 700,
	Precedence_CStyleEQUALS   = 800,
	Precedence_CStyleBITAND   = 900,
	Precedence_CStyleBITXOR   = 1000,
	Precedence_CStyleBITOR    = 1100,
	Precedence_CStyleAND      = 1200,
	Precedence_CStyleOR       = 1300,
	Precedence_CStyleTRINARY  = 1400,  /* ? : */
	Precedence_CStyleASSIGN   = 1500,
	Precedence_CStyleCOMMA    = 1600
} Precedence;

typedef struct KDEFINE_SYNTAX {
	ksymbol_t keyword;  kshortflag_t flag;
	const char *rule;
	int precedence_op2;
	int precedence_op1;
	MethodFunc PatternMatch;
	MethodFunc Expression;
	MethodFunc TopLevelStatement;
	MethodFunc Statement;
	MethodFunc TypeCheck;
} KDEFINE_SYNTAX;

#define new_SugarFunc(ns, F)     new_(Func, KLIB new_kMethod(kctx, (ns)->NameSpaceConstList, 0, 0, 0, F), (ns)->NameSpaceConstList)

#define kToken_SetHintChar(tk, ch, ch2)           tk->hintChar = ((ch << 8) | ((char)ch2))
#define kToken_GetOpenHintChar(tk)                ((int)(tk->hintChar >> 8))
#define kToken_GetCloseHintChar(tk)               ((char)tk->hintChar)

/* Token */
struct kTokenVar {
	KonohaObjectHeader h;
	union {
		kString *text;
		kArray  *subTokenList;
		kExpr   *parsedExpr;
	};
	kfileline_t     uline;
	SugarSyntax    *resolvedSyntaxInfo;
	union {
		ksymbol_t   unresolvedTokenType; // (resolvedSyntaxInfo == NULL)
		ksymbol_t   resolvedSymbol;      // symbol (resolvedSyntaxInfo != NULL)
		kattrtype_t     resolvedTypeId;      // typeid if KW_TypePattern
	};
	union {
		kushort_t   indent;               // indent when kw == TokenType_INDENT
		kshort_t    hintChar;
		ksymbol_t   stmtEntryKey;         // pattern name for 'setting key in Stmt'
	};
};

#define kToken_isIndent(T)  ((T)->unresolvedTokenType == TokenType_INDENT && (T)->resolvedSyntaxInfo == NULL)

#define kTokenFlag_StatementSeparator    kObject_Local1
#define kTokenFlag_MatchPreviousPattern  kObject_Local1
#define kTokenFlag_RequiredReformat      kObject_Local2
#define kTokenFlag_BeforeWhiteSpace      kObject_Local3

#define kToken_is(P, o)      (TFLAG_is(uintptr_t,(o)->h.magicflag, kTokenFlag_##P))
#define kToken_set(P,o,B)    TFLAG_set(uintptr_t,(o)->h.magicflag, kTokenFlag_##P, B)

typedef struct MacroSet {
	int/*ksymbol_t*/          symbol;
	kArray                   *tokenList;
	int                       beginIdx;
	int                       endIdx;
} MacroSet;

struct TokenSeqSource {
	kToken *openToken;
	int     stopChar;
	kToken *foundErrorToken;
};

struct TokenSeqTarget {
	int RemovingIndent;
	int ExpandingBraceGroup;
	SugarSyntax *syntaxSymbolPattern;
};

typedef struct TokenSeq {
	kNameSpace *ns;
	kArray *tokenList;
	int beginIdx;
	int endIdx;
	union {
		struct TokenSeqSource SourceConfig;
		struct TokenSeqTarget TargetPolicy;
	};
} TokenSeq;

#define TokenSeq_end(kctx, range)   range->endIdx = kArray_size(range->tokenList)

#define TokenSeq_Push(kctx, range) \
	size_t _PopCheckIdx = kArray_size(range.tokenList);\
	range.beginIdx = kArray_size(range.tokenList);\
	range.endIdx   = 0;\

#define TokenSeq_Pop(kctx, range)   do {\
	KLIB kArray_Clear(kctx, range.tokenList, _PopCheckIdx);\
	DBG_ASSERT(_PopCheckIdx == kArray_size(range.tokenList));\
} while(0)

typedef kbool_t (*CheckEndOfStmtFunc2)(KonohaContext *, TokenSeq *range, TokenSeq *sourceRange, int *currentIdxRef, int *indentRef);

#define Token_isVirtualTypeLiteral(TK)     ((TK)->resolvedSyntaxInfo->keyword == KW_TypePattern)
#define Token_typeLiteral(TK)              (TK)->resolvedTypeId

typedef enum {
	TEXPR_CONST,
	TEXPR_NEW,
	TEXPR_NULL,
	TEXPR_NCONST,
	TEXPR_LOCAL,
	TEXPR_BLOCK,
	TEXPR_FIELD,
	TEXPR_CALL,
	TEXPR_AND,
	TEXPR_OR,
	TEXPR_LET,
	TEXPR_STACKTOP,

	TSTMT_EXPR,  // this must be the first stmt
	TSTMT_BLOCK,
	TSTMT_RETURN,
	TSTMT_IF,
	TSTMT_LOOP,
	TSTMT_JUMP,
	TSTMT_TRY,
	TSTMT_ERR   // this must be last
} kvisit_t;

#define TEXPR_UNTYPED       -1   /*THIS MUST NOT HAPPEN*/
#define TEXPR_MAX           12

#define kExpr_IsConstValue(o)     (TEXPR_CONST <= (o)->build && (o)->build <= TEXPR_NCONST)
#define Expr_isTerm(o)      (TFLAG_is(uintptr_t,(o)->h.magicflag,kObject_Local1))
#define Expr_setTerm(o,B)   TFLAG_set(uintptr_t,(o)->h.magicflag,kObject_Local1,B)

#define Expr_hasObjectConstValue(o)     (TFLAG_is(uintptr_t,(o)->h.magicflag,kObject_Local2))
#define Expr_setObjectConstValue(o,B)   TFLAG_set(uintptr_t,(o)->h.magicflag,kObject_Local2,B)

#define kExpr_at(E,N)                   ((E)->cons->ExprItems[(N)])

typedef kshort_t    kexpr_t;

struct kExprVar {
	KonohaObjectHeader h;
	SugarSyntax *syn;
	union {
		kToken  *termToken;     // Term
		kArray*  cons;          // Cons
		kBlock*  block;
	};
	kattrtype_t attrTypeId;    kexpr_t build;
	union {
		kObject*   objectConstValue;
		uintptr_t  unboxConstValue;
		intptr_t   index;
	};
};

#define TSTMT_UNDEFINED      0
#define kStmt_isERR(STMT)       ((STMT)->build == TSTMT_ERR)

struct kStmtVar {
	KonohaObjectHeader h;
	kfileline_t        uline;
	SugarSyntax       *syn;
	kBlock            *parentBlockNULL;
	kushort_t          build;
};

#define kStmt_GetObjectNULL(CTX, O, K)            (KLIB kObject_getObject(CTX, UPCAST(O), K, NULL))
#define kStmt_GetObject(CTX, O, K, DEF)           (KLIB kObject_getObject(CTX, UPCAST(O), K, DEF))
#define kStmt_setObject(CTX, O, K, V)             KLIB kObjectProto_SetObject(CTX, UPCAST(O), K, O_typeId(V), UPCAST(V))
#define kStmt_setUnboxValue(CTX, O, K, T, V)      KLIB kObjectProto_SetUnboxValue(CTX, UPCAST(O), K, T, V)
#define kStmt_removeKey(CTX, O, K)                KLIB kObjectProto_RemoveKey(CTX, UPCAST(O), K)
#define kStmt_protoEach(CTX, O, THUNK, F)         KLIB kObjectProto_DoEach(CTX, UPCAST(O), THUNK, F)

#define kStmt_Message(kctx, STMT, PE, FMT, ...)            SUGAR kStmt_Message2(kctx, STMT, NULL, PE, FMT, ## __VA_ARGS__)
#define kStmtToken_Message(kctx, STMT, TK, PE, FMT, ...)   SUGAR kStmt_Message2(kctx, STMT, TK, PE, FMT, ## __VA_ARGS__)
#define kStmtExpr_Message(kctx, STMT, EXPR, PE, FMT, ...)  SUGAR kStmt_Message2(kctx, STMT, (kToken *)EXPR, PE, FMT, ## __VA_ARGS__)

#define kStmtFlag_RedoLoop           kObject_Local1
#define kStmtFlag_CatchContinue      kObject_Local2
#define kStmtFlag_CatchBreak         kObject_Local3

#define kStmt_Is(P, O)       (TFLAG_is(uintptr_t, (O)->h.magicflag, kStmtFlag_##P))
#define kStmt_Set(P, O, B)   TFLAG_set(uintptr_t,((kStmtVar *)O)->h.magicflag, kStmtFlag_##P, B)

struct kBlockVar {
	KonohaObjectHeader   h;
	kNameSpace          *BlockNameSpace;
	kStmt               *parentStmtNULL;
	kArray              *StmtList;
	kExpr               *esp;
};

typedef struct {
	kattrtype_t    attrTypeId;    ksymbol_t  name;
} GammaStackDecl;

#define kGamma_TopLevel        (kshortflag_t)(1)
#define Gamma_isTopLevel(GMA)  TFLAG_is(kshortflag_t, GMA->genv->flag, kGamma_TopLevel)
#define kGamma_ERROR           (kshortflag_t)(1<<1)
#define Gamma_hasERROR(GMA)    TFLAG_is(kshortflag_t, GMA->genv->flag, kGamma_ERROR)
#define Gamma_setERROR(GMA,B) TFLAG_set(kshortflag_t, GMA->genv->flag, kGamma_ERROR, B)

typedef struct {
	GammaStackDecl *varItems;
	size_t varsize;
	size_t capacity;
	size_t allocsize;  // set size if not allocated  (by default on stack)
} GammaStack;

typedef struct  {
	kshortflag_t  flag;      kshortflag_t  cflag;
	KonohaClass   *thisClass;
	//kattrtype_t      static_cid;
	kMethod      *currentWorkingMethod;
	GammaStack    localScope;
	int           blockScopeShiftSize;
} GammaAllocaData;

struct kGammaVar {
	KonohaObjectHeader h;
	GammaAllocaData *genv;
};

/* ------------------------------------------------------------------------ */

#define GetSugarContext(kctx)    ((SugarContext *)kctx->modlocal[MOD_sugar])
#define kmodsugar       ((KModuleSugar *)kctx->modshare[MOD_sugar])
#define CT_Symbol       kmodsugar->cSymbol
#define CT_Token        kmodsugar->cToken
#define CT_Expr         kmodsugar->cExpr
#define CT_Stmt         kmodsugar->cStmt
#define CT_Block        kmodsugar->cBlock
#define CT_Gamma        kmodsugar->cGamma

#define CT_SymbolVar        kmodsugar->cSymbol
#define CT_TokenVar        kmodsugar->cToken
#define CT_ExprVar         kmodsugar->cExpr
#define CT_StmtVar         kmodsugar->cStmt
#define CT_BlockVar        kmodsugar->cBlock
#define CT_GammaVar        kmodsugar->cGamma

#define CT_TokenArray           kmodsugar->cTokenArray
#define kTokenArray             kArray
#define CT_ExprArray            CT_Array
#define kExprArray              kArray
#define CT_StmtArray            CT_Array
#define kStmtArray              kArray

#define IS_Token(O)  (O_ct(O) == CT_Token)
#define IS_Expr(O)   (O_ct(O) == CT_Expr)
#define IS_Stmt(O)   (O_ct(O) == CT_Stmt)
#define IS_Block(O)  (O_ct(O) == CT_Block)
#define IS_Gamma(O)  (O_ct(O) == CT_Gamma)

#define K_NULLTOKEN  (kToken *)((CT_Token)->defaultNullValue_OnGlobalConstList)
#define K_NULLEXPR   (kExpr *)((CT_Expr)->defaultNullValue_OnGlobalConstList)
#define K_NULLBLOCK  (kBlock *)((CT_Block)->defaultNullValue_OnGlobalConstList)

typedef kStmt* (*TypeDeclFunc)(KonohaContext *kctx, kStmt *stmt, kGamma *gma, kattrtype_t ty, kExpr *termExpr, kExpr *vexpr, kObject *thunk);
struct KBuilder;

typedef struct {
	KonohaModule  h;
	KonohaClass *cSymbol;
	KonohaClass *cToken;
	KonohaClass *cExpr;
	KonohaClass *cStmt;
	KonohaClass *cBlock;
	KonohaClass *cGamma;
	KonohaClass *cTokenArray;

	SugarSyntax*    (*kNameSpace_GetSyntax)(KonohaContext *, kNameSpace *, ksymbol_t, int);
	void            (*kNameSpace_DefineSyntax)(KonohaContext *, kNameSpace *, KDEFINE_SYNTAX *, KTraceInfo *);
	kbool_t         (*kArray_AddSyntaxRule)(KonohaContext *, kArray *ruleList, TokenSeq *sourceRange);
	SugarSyntaxVar* (*kNameSpace_SetTokenFunc)(KonohaContext *, kNameSpace *, ksymbol_t, int ch, kFunc *);
	SugarSyntaxVar* (*kNameSpace_AddSugarFunc)(KonohaContext *, kNameSpace *, ksymbol_t kw, size_t idx, kFunc *);
	void            (*kNameSpace_SetMacroData)(KonohaContext *, kNameSpace *, ksymbol_t, int, const char *);

	void        (*TokenSeq_Tokenize)(KonohaContext *, TokenSeq *, const char *, kfileline_t);
	kbool_t     (*TokenSeq_ApplyMacro)(KonohaContext *, TokenSeq *, kArray *, int, int, size_t, MacroSet *);
	int         (*TokenSeq_Resolve)(KonohaContext *, TokenSeq *, MacroSet *, TokenSeq *, int);
	kstatus_t   (*TokenSeq_Eval)(KonohaContext *, TokenSeq *, KTraceInfo *);

	int         (*TokenUtils_ParseTypePattern)(KonohaContext *, kNameSpace *, kArray *, int , int , KonohaClass **classRef);
	kTokenVar*  (*kToken_ToBraceGroup)(KonohaContext *, kTokenVar *, kNameSpace *, MacroSet *);

	void        (*kStmt_AddParsedObject)(KonohaContext *, kStmt *, ksymbol_t, kObject *o);
	int         (*kNameSpace_FindEndOfStatement)(KonohaContext *, kNameSpace *, kArray *, int, int);

	uintptr_t   (*kStmt_ParseFlag)(KonohaContext *kctx, kStmt *stmt, KonohaFlagSymbolData *flagData, uintptr_t flag);
	kToken*     (*kStmt_GetToken)(KonohaContext *, kStmt *, ksymbol_t kw, kToken *def);
	kExpr*      (*kStmt_GetExpr)(KonohaContext *, kStmt *, ksymbol_t kw, kExpr *def);
	const char* (*kStmt_GetText)(KonohaContext *, kStmt *, ksymbol_t kw, const char *def);
	kBlock*     (*kStmt_GetBlock)(KonohaContext *, kStmt *, kNameSpace *, ksymbol_t kw, kBlock *def);

	kBlock*      (*new_kBlock)(KonohaContext *, kStmt *, MacroSet *, TokenSeq *);
	kStmtVar*    (*new_kStmt)(KonohaContext *kctx, kArray *gcstack, SugarSyntax *syn, ...);
	void         (*kBlock_InsertAfter)(KonohaContext *, kBlock *, kStmtNULL *target, kStmt *);

	kExpr*       (*new_TermExpr)(KonohaContext *, kToken *tk);
	kExprVar*       (*new_UntypedCallStyleExpr)(KonohaContext *, SugarSyntax *syn, int n, ...);
	kExpr*       (*kStmt_ParseOperatorExpr)(KonohaContext *, kStmt *, SugarSyntax *, kArray *tokenList, int beginIdx, int operatorIdx, int endIdx);
	kExpr*       (*kStmt_ParseExpr)(KonohaContext *, kStmt *, kArray *tokenList, int s, int e, const char *hintBeforeText);
	kExpr*       (*kStmt_AddExprParam)(KonohaContext *, kStmt *, kExpr *, kArray *tokenList, int, int, const char *hintBeforeText);
	kExpr*       (*kStmt_RightJoinExpr)(KonohaContext *, kStmt *, kExpr *, kArray *, int, int);

	kExpr*       (*kExpr_SetConstValue)(KonohaContext *, kExprVar *, KonohaClass *, kObject *o);
	kExpr*       (*kExpr_SetUnboxConstValue)(KonohaContext *, kExprVar *, kattrtype_t, uintptr_t unboxValue);
	kExpr*       (*kExpr_SetVariable)(KonohaContext *, kExprVar *, kGamma *, kexpr_t build, kattrtype_t, intptr_t index);
	kExpr *      (*new_TypedCallExpr)(KonohaContext *, kStmt *, kGamma *, KonohaClass *, kMethod *mtd, int n, ...);

	kbool_t     (*kBlock_TypeCheckAll)(KonohaContext *, kBlock *, kGamma *);
	kbool_t     (*kStmt_TypeCheckByName)(KonohaContext *, kStmt*, ksymbol_t, kGamma *, KonohaClass *, int);
	kExpr*      (*kStmt_TypeCheckExprAt)(KonohaContext *, kStmt *, kExpr *, size_t, kGamma *, KonohaClass *, int);
	kExpr *     (*kStmtkExpr_TypeCheckCallParam)(KonohaContext *, kStmt *, kExprVar *, kMethod *, kGamma *, KonohaClass *);
	int         (*kGamma_AddLocalVariable)(KonohaContext *, kGamma *, kattrtype_t, ksymbol_t);
	kbool_t     (*kStmt_DeclType)(KonohaContext *, kStmt *, kGamma *, kattrtype_t, kExpr *, kObject *, TypeDeclFunc, kStmt **);
	kExpr*      (*kStmt_TypeCheckVariableNULL)(KonohaContext *, kStmt *, kExprVar *, kGamma *, KonohaClass *);

	void       (*kToken_ToError)(KonohaContext *, kTokenVar *, kinfotag_t, const char *fmt, ...);
	kExpr *    (*kStmt_Message2)(KonohaContext *, kStmt *, kToken *, kinfotag_t, const char *fmt, ...);

	kbool_t (*VisitBlock)(KonohaContext *, struct KBuilder *, kBlock *block);
	kbool_t (*VisitStmt)(KonohaContext *, struct KBuilder *, kStmt *stmt);
	void    (*VisitExpr)(KonohaContext *, struct KBuilder *, kStmt *stmt, kExpr *expr);

	void (*dumpToken)(KonohaContext *kctx, kToken *tk, int n);
	void (*dumpTokenArray)(KonohaContext *kctx, int nest, kArray *a, int s, int e);
	void (*dumpExpr)(KonohaContext *kctx, int n, int nest, kExpr *expr);
	void (*dumpStmt)(KonohaContext *kctx, kStmt *stmt);

} KModuleSugar;

typedef struct {
	KonohaModuleContext h;
	kArray            *preparedTokenList;
	KGrowingArray errorMessageBuffer;
	kArray            *errorMessageList;
	int                errorMessageCount;
	kbool_t            isBlockedErrorMessage;
	kGamma            *preparedGamma;
	kArray            *definedMethodList;
} SugarContext;

#define CT_INFER    CT_(TY_var)

typedef enum {
	TypeCheckPolicy_NOPOLICY              = 0,
	TypeCheckPolicy_NOCHECK               = 1,
	TypeCheckPolicy_ALLOWVOID      = (1 << 1),
	TypeCheckPolicy_COERCION       = (1 << 2),
	TypeCheckPolicy_CONST          = (1 << 4)
} TypeCheckPolicy;

#define new_ConstValueExpr(CTX, T, O)              SUGAR kExpr_SetConstValue(CTX, NULL, T, O)
#define new_UnboxConstValueExpr(CTX, T, D)         SUGAR kExpr_SetUnboxConstValue(CTX, NULL, T, D)
#define new_VariableExpr(CTX, GMA, BLD, TY, IDX)   SUGAR kExpr_SetVariable(CTX, NULL, GMA, BLD, TY, IDX)

#ifdef USING_SUGAR_AS_BUILTIN

#define SUGAR

//static kExpr* kExpr_SetConstValue(KonohaContext *kctx, kExprVar *expr, kattrtype_t ty, kObject *o);
//static kExpr* kExpr_SetUnboxConstValue(KonohaContext *kctx, kExprVar *expr, kattrtype_t ty, uintptr_t unboxValue);
//static kExpr* kExpr_SetVariable(KonohaContext *kctx, kExpr *expr, kGamma *gma, kexpr_t build, kattrtype_t ty, intptr_t index);

#define TY_Symbol                          kmodsugar->cSymbol->typeId
#define TY_Token                           kmodsugar->cToken->typeId
#define TY_Stmt                            kmodsugar->cStmt->typeId
#define TY_Block                           kmodsugar->cBlock->typeId
#define TY_Expr                            kmodsugar->cExpr->typeId
#define TY_Gamma                           kmodsugar->cGamma->typeId
#define TY_TokenArray                      kmodsugar->cTokenArray->typeId

#define SYN_(KS, KW)                       kNameSpace_GetSyntax(kctx, KS, KW, 0)


#else/*SUGAR_EXPORTS*/

#define SUGAR        ((const KModuleSugar *)kmodsugar)->
#define TY_Symbol                            SUGAR cSymbol->typeId
#define TY_Token                             SUGAR cToken->typeId
#define TY_Stmt                              SUGAR cStmt->typeId
#define TY_Block                             SUGAR cBlock->typeId
#define TY_Expr                              SUGAR cExpr->typeId
#define TY_Gamma                             SUGAR cGamma->typeId
#define TY_TokenArray                        SUGAR cTokenArray->typeId

//#define KW_(T)                               _e->keyword(kctx, T, sizeof(T)-1, SYM_NONAME)
#define SYN_(KS, KW)                         SUGAR kNameSpace_GetSyntax(kctx, KS, KW, 0)
#define NEWSYN_(KS, KW)                      (SugarSyntaxVar *)(SUGAR kNameSpace_GetSyntax(kctx, KS, KW, 1))

#endif/*SUGAR_EXPORTS*/

#ifdef USE_SMALLBUILD
#define KdumpToken(ctx, tk)
#define KdumpTokenArray(CTX, TLS, S, E)
#define KdumpTokenSeq(CTX, MSG, R)
#define KdumpExpr(CTX, EXPR)
#else
#define KDump(O)                         KLIB DumpObject(kctx, (kObject *)O, __FILE__, __FUNCTION__, __LINE__)
#define KdumpToken(ctx, tk)              ((const KModuleSugar *)kmodsugar)->dumpToken(ctx, tk, 0)
#define KdumpTokenArray(CTX, TLS, S, E)  DBG_P("@"); ((const KModuleSugar *)kmodsugar)->dumpTokenArray(CTX, 1, TLS, S, E)
#define KdumpTokenSeq(CTX, MSG, R)     DBG_P(MSG); ((const KModuleSugar *)kmodsugar)->dumpTokenArray(CTX, 1, R->tokenList, R->beginIdx, R->endIdx)
#define KdumpExpr(CTX, EXPR)             ((const KModuleSugar *)kmodsugar)->dumpExpr(CTX, 0, 0, EXPR)
#endif

/* ------------------------------------------------------------------------ */
/* BuilderAPI */

struct KBuilder;
typedef struct KBuilder KBuilder;

typedef kbool_t (*VisitStmtFunc)(KonohaContext *kctx, KBuilder *builder, kStmt *stmt);
typedef void (*VisitExprFunc)(KonohaContext *kctx, KBuilder *builder, kStmt *stmt, kExpr *expr);

struct KBuilderCommon {
	struct KBuilderAPI2* api;
	int option;
	kfileline_t uline;
	int a; /* whatis a ? */
	int shift;
	int espidx;
};

struct KBuilderAPI2 {
	const char *target;
	struct VirtualCode*   (*GenerateVirtualCode)(KonohaContext *, kMethod *mtd, kBlock *block, int option);
	MethodFunc            (*GenerateMethodFunc)(KonohaContext *, struct VirtualCode *);
	struct VirtualCode *  (*RunVirtualMachine)(KonohaContext *kctx, struct KonohaValueVar *sfp, struct VirtualCode *pc);

	VisitStmtFunc visitErrStmt;
	VisitStmtFunc visitExprStmt;
	VisitStmtFunc visitBlockStmt;
	VisitStmtFunc visitReturnStmt;
	VisitStmtFunc visitIfStmt;
	VisitStmtFunc visitLoopStmt;
	VisitStmtFunc visitJumpStmt;
	VisitStmtFunc visitTryStmt;
	VisitStmtFunc visitUndefinedStmt;
	VisitExprFunc visitConstExpr;
	VisitExprFunc visitNConstExpr;
	VisitExprFunc visitNewExpr;
	VisitExprFunc visitNullExpr;
	VisitExprFunc visitLocalExpr;
	VisitExprFunc visitBlockExpr;
	VisitExprFunc visitFieldExpr;
	VisitExprFunc visitCallExpr;
	VisitExprFunc visitAndExpr;
	VisitExprFunc visitOrExpr;
	VisitExprFunc visitLetExpr;
	VisitExprFunc visitStackTopExpr;
	size_t allocSize;
};

#define VISITOR_LIST(OP) \
	OP(ErrStmt)\
	OP(ExprStmt)\
	OP(BlockStmt)\
	OP(ReturnStmt)\
	OP(IfStmt)\
	OP(LoopStmt)\
	OP(JumpStmt)\
	OP(TryStmt)\
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


/* ------------------------------------------------------------------------ */

static inline void kToken_setTypeId(KonohaContext *kctx, kToken *tk, kNameSpace *ns, kattrtype_t type)
{
	((kTokenVar *)tk)->resolvedTypeId = type;
	((kTokenVar *)tk)->resolvedSyntaxInfo = kmodsugar->kNameSpace_GetSyntax(kctx, ns, KW_TypePattern, 0);
}

#define Stmt_ns(STMT)   kStmt_ns(STMT)
static inline kNameSpace *kStmt_ns(/*KonohaContext *kctx, */kStmt *stmt)
{
	return stmt->parentBlockNULL->BlockNameSpace;
}

#define kStmt_setsyn(STMT, S)  Stmt_setsyn(kctx, STMT, S)
#define kStmt_done(kctx, STMT) Stmt_setsyn(kctx, STMT, NULL)
static inline void Stmt_setsyn(KonohaContext *kctx, kStmt *stmt, SugarSyntax *syn)
{
	//if(syn == NULL && stmt->syn != NULL) {
	//	DBG_P("DONE: STMT='%s'", PSYM_t(syn->keyword));
	//}
	((kStmtVar *)stmt)->syn = syn;
	(void)kctx;
}
static inline kbool_t Stmt_isDone(kStmt *stmt)
{
	return (stmt->syn == NULL);
}

#define kStmt_typed(STMT, T)  Stmt_typed(STMT, TSTMT_##T)
static inline void Stmt_typed(kStmt *stmt, int build)
{
	if(stmt->build != TSTMT_ERR) {
		((kStmtVar *)stmt)->build = build;
	}
}

static inline kbool_t kExpr_isSymbolTerm(kExpr *expr)
{
	return (Expr_isTerm(expr) && (expr->termToken->resolvedSyntaxInfo->keyword == KW_SymbolPattern));
}

static inline void kExpr_Setsyn(kExpr *expr, SugarSyntax *syn)
{
	((kExprVar *)expr)->syn = syn;
}

#define kExpr_typed(E, B, TY)   Expr_typed(E, TEXPR_##B, TY)
static inline kExpr *Expr_typed(kExprVar *expr, int build, kattrtype_t ty)
{
	expr->build = build;
	expr->attrTypeId = ty;
	return expr;
}

#ifdef __cplusplus
}
#endif

/* ------------------------------------------------------------------------ */
#endif /* SUGAR_H_ */
