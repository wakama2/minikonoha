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

#include <minikonoha/minikonoha.h>
#include <minikonoha/sugar.h>

// for isCommand
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// --------------------------------------------------------------------------

static const char *expandDollarToken(KonohaContext *kctx, kTokenArray *tokens, size_t start, size_t end)
{
	// TODO: Expand $token by using konoha variable, return char pointer.
	return NULL;
}

static kbool_t checkPath(const char *path, const char *cmd)
{
	struct stat sb;
	char buf[PATH_MAX];
	const char *fullpath;
	if(path != NULL) {
		snprintf(buf, PATH_MAX, "%s/%s", path, cmd);
		fullpath = buf;
	}
	else {
		fullpath = cmd;
	}
	if(lstat(fullpath, &sb) == -1) {
		return false;
	}
	return (sb.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH));
}

static kbool_t isCommand(const char *cmd)
{
	if(strchr(cmd, ' ') != NULL) {
		return false;
	}
	if(cmd[0] == '/') {
		return checkPath(NULL, cmd);
	}
	size_t bufsize = confstr(_CS_PATH, NULL, 0);
	char buf[bufsize];
	confstr(_CS_PATH, buf, bufsize);
	char *pos, *p = buf;
	while(pos < buf + bufsize) {
		if((pos = strchr(p, ':')) == NULL) {
			if(checkPath(p, cmd)) return true;
			break;
		}
		p[pos - p] = '\0';
		if(checkPath(p, cmd)) return true;
		p = pos + 1;
	}
	return false;
}

static kString *splitWhiteSpace(KonohaContext *kctx, kTokenArray *tokenList)
{
	size_t i;
	KGrowingBuffer wb;
	KLIB KBuffer_Init(&(kctx->stack->cwb), &wb);
	if(O_typeId(tokenList) == TY_Token) {
		/* Single token was passed (e.g. "dsh ls;"). */
		kToken *token = (kToken *)tokenList;
		KLIB KBuffer_Write(kctx, &wb, S_text(token->text), S_size(token->text));
	}
	else {
		/* Multiple tokens was passed (e.g. "dsh ls -la;"). */
		for(i = 0; i < kArray_size(tokenList); i++) {
			kToken *token = tokenList->TokenItems[i];
			if(token->resolvedSymbol == SYM_("|")) {
				// TODO: PIPE
			}
			else if(token->resolvedSymbol == SYM_("$")) {
				// TODO: parse dollar token ($token)
				size_t start = i;
				while(i < kArray_size(tokenList) && !kToken_is(BeforeWhiteSpace, tokenList->TokenItems[i])) {
					++i;
				}
				const char *dollarstr = expandDollarToken(kctx, tokenList, start, i-1);
				if(dollarstr == NULL) {
					KLIB KBuffer_Free(&wb);
					return NULL;
				}
				else {
					KLIB KBuffer_Write(kctx, &wb, dollarstr, strlen(dollarstr));
				}
			}
			else {
				KLIB KBuffer_Write(kctx, &wb, S_text(token->text), S_size(token->text));
			}
			if(kToken_is(BeforeWhiteSpace, token)) {
				KLIB KBuffer_Write(kctx, &wb, " ", 1);
			}
		}
	}
	kString *cmd = KLIB new_kString(kctx, GcUnsafe, KLIB KBuffer_Stringfy(kctx, &wb, 0), KBuffer_bytesize(&wb), 0);
	KLIB KBuffer_Free(&wb);
	return cmd;
}

static KMETHOD Statement_dsh(KonohaContext *kctx, KonohaStack *sfp)
{
	VAR_Statement(stmt, gma);
	kTokenArray *tokenList = (kTokenArray *)kStmt_GetObjectNULL(kctx, stmt, KW_TokenPattern);
	if(tokenList == NULL) {
		KReturnUnboxValue(false);
	}

	kString *cmd = splitWhiteSpace(kctx, tokenList);
	if(cmd == NULL) {
		KReturnUnboxValue(false);
	}
	DBG_P("cmd=%s", S_text(cmd));
	DBG_P("iscommand=%d", isCommand(S_text(cmd)));

	//TODO: generate eval("cmd") syntax

	kNameSpace *ns = Stmt_ns(stmt);
	SugarSyntaxVar *syn = (SugarSyntaxVar *) SYN_(ns, KW_ExprMethodCall);
	kTokenVar *callToken = new_(TokenVar, 0, OnGcStack);
	kExpr *callExpr = new_ConstValueExpr(kctx, TY_String, UPCAST(cmd));
	callToken->resolvedSymbol = MN_("call");
	const char cname[] = "Subproc";
	kExpr *expr = SUGAR new_UntypedCallStyleExpr(kctx, syn, 3, callToken,
			new_UnboxConstValueExpr(kctx, KLIB kNameSpace_GetClassByFullName(kctx, ns, cname, strlen(cname), NULL)->typeId, 0), callExpr);
	KLIB kObjectProto_SetObject(kctx, stmt, KW_ExprPattern, TY_Expr, expr);
	kbool_t ret = SUGAR kStmt_TypeCheckByName(kctx, stmt, KW_ExprPattern, gma, TY_int, 0);
	if(ret) {
		kStmt_typed(stmt, EXPR);
	}
	KReturnUnboxValue(ret);
}

static kbool_t DSLib_CheckExecutablePath(KonohaContext *kctx, const char *path, const char *cmd)
{
	char buf[PATH_MAX];
	struct stat sb;
	const char *fullpath;
	if(path != NULL) {
		snprintf(buf, PATH_MAX, "%s/%s", path, cmd);
		fullpath = buf;
	}
	else {
		fullpath = cmd;
	}
	DBG_P("path='%s'", fullpath);
	if(lstat(fullpath, &sb) == -1) {
		return false;
	}
	return (sb.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH));
}

static kbool_t DSLib_isCommand(KonohaContext *kctx, const char *cmd)
{
	size_t bufsize = confstr(_CS_PATH, NULL, 0);
	char buf[bufsize];
	confstr(_CS_PATH, buf, bufsize);
	char *pos, *p = buf;
	while(p < buf + bufsize) {
		if((pos = strchr(p, ':')) == NULL) {
			if(DSLib_CheckExecutablePath(kctx, p, cmd)) return true;
			break;
		}
		p[pos - p] = '\0';
		if(DSLib_CheckExecutablePath(kctx, p, cmd)) return true;
		p = pos + 1;
	}
	return DSLib_CheckExecutablePath(kctx, "/bin", cmd);
}


static KMETHOD PatternMatch_Shell(KonohaContext *kctx, KonohaStack *sfp)
{
	VAR_PatternMatch(stmt, nameid, tokenList, beginIdx, endIdx);
	kToken *firstToken = tokenList->TokenItems[beginIdx];
	DBG_P("firstToken='%s', isCommand=%d", S_text(firstToken->text), DSLib_isCommand(kctx, S_text(firstToken->text)));
	KReturnUnboxValue((firstToken->resolvedSyntaxInfo->keyword == KW_SymbolPattern && DSLib_isCommand(kctx, S_text(firstToken->text))) ? beginIdx : -1);
}

static KMETHOD Statement_Shell(KonohaContext *kctx, KonohaStack *sfp)
{
	VAR_Statement(stmt, gma);
	kTokenArray *tokenList = (kTokenArray *)kStmt_GetObjectNULL(kctx, stmt, KW_TokenPattern);
	if(tokenList != NULL) {
		kString *cmd = NULL;
		if(IS_Token(tokenList)) {
			cmd = ((kToken *)tokenList)->text;
		}
		else {
			DBG_ASSERT(IS_Array(tokenList));
			cmd = splitWhiteSpace(kctx, tokenList);  // forget GC
		}
		DBG_P("cmd=%s", S_text(cmd));
		system(S_text(cmd));  // FIXME: This is for demo
		kStmt_done(kctx, stmt);
	}
	KReturnUnboxValue(false);
}

// ----------------------------------------------------------------------------
/* define class */

static kbool_t shell_defineSyntax(KonohaContext *kctx, kNameSpace *ns, KTraceInfo *trace)
{
	KRequirePackage("dscript.subproc", trace);
	//KDEFINE_SYNTAX SYNTAX[] = {
	//	{ SYM_("dsh"), 0, "\"dsh\" $Token*", 0, 0, NULL, NULL, Statement_dsh, Statement_dsh, NULL, },
	//	{ SYM_("$Shell"), 0, "$Shell $Token*", 0, 0, PatternMatch_Shell, NULL, Statement_Shell, Statement_Shell},
	//	{ KW_END, },
	//};
	//SUGAR kNameSpace_DefineSyntax(kctx, ns, SYNTAX, trace);
	return true;
}

static kbool_t shell_PackupNameSpace(KonohaContext *kctx, kNameSpace *ns, int option, KTraceInfo *trace)
{
	shell_defineSyntax(kctx, ns, trace);
	return true;
}

static kbool_t shell_ExportNameSpace(KonohaContext *kctx, kNameSpace *ns, kNameSpace *exportNS, int option, KTraceInfo *trace)
{
	return true;
}

KDEFINE_PACKAGE* shell_Init(void)
{
	static KDEFINE_PACKAGE d = {0};
	KSetPackageName(d, "dshell", "1.0");
	d.PackupNameSpace    = shell_PackupNameSpace;
	d.ExportNameSpace   = shell_ExportNameSpace;
	return &d;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
