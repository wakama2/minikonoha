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
#include <minikonoha/bytes.h>
#include <minikonoha/posix.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <sys/time.h>

#if defined(__linux__)
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
#define __USE_LOCAL_PIPE2__
#endif
#endif /* __linux__ */

#include "subproc_resourcemonitor.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined (__APPLE__) && !defined(__USE_LOCAL_PIPE2__)
extern int pipe2 (int __pipedes[2], int __flags);
#endif
extern int sigignore (int __sig);

typedef struct {
	int mode;                              // the kind of identifier
	FILE* fp;                              // file stream pointer
} pfd_t;                                   // pipe fd structure

/*
typedef struct {
	kbool_t shell;                         // shell mode [true/false]
	kbool_t closefds;                      // closefds   [true/false]
	kbool_t bg;                            // bg mode    [true/false]
	kArray *env;                           // child process environment
	kString *command;                      // child process command
	kString *cwd;                          // child process current working directory
	pfd_t r;                               // child process output identifier
	pfd_t w;                               // child process input identifier
	pfd_t e;                               // child process error identifier
	int cpid;                              // child process ID
	int bufferSize;                        // buffer size (unused)
	int timeout;                           // child process timeout value
	int status;                            // waitpid status
	int timeoutKill;                       // child process Timeout ending flag [true/false]
	SUBPROC_RESOURCEMON_INSTANCE;
} subprocData_t;                           // subproc data structure
*/

#define SUBPROC_CLOSEFDS         ((kshortflag_t)(1<<0))
#define SUBPROC_BACKGROUND       ((kshortflag_t)(1<<1))
#define SUBPROC_SHELL            ((kshortflag_t)(1<<2))

#define SUBPROC_IsCloseFds(P)    (FLAG_is((P)->flag, SUBPROC_CLOSEFDS))
#define SUBPROC_IsBackground(P)  (FLAG_is((P)->flag, SUBPROC_BACKGROUND))
#define SUBPROC_IsShell(P)       (FLAG_is((P)->flag, SUBPROC_SHELL))

#define SUBPROC_setCloseFds(P)   (FLAG_set((P)->flag, SUBPROC_CLOSEFDS))
#define SUBPROC_setBackground(P) (FLAG_set((P)->flag, SUBPROC_BACKGROUND))
#define SUBPROC_setShell(P)      (FLAG_set((P)->flag, SUBPROC_SHELL))

#define SUBPROC_unsetCloseFds(P) (FLAG_unset((P)->flag, SUBPROC_CLOSEFDS))
#define SUBPROC_unsetBackground(P) (FLAG_unset((P)->flag, SUBPROC_BACKGROUND))
#define SUBPROC_unsetShell(P) (FLAG_unset((P)->flag, SUBPROC_SHELL))

typedef const struct kSubprocVar kSubproc;
struct kSubprocVar {
	KonohaObjectHeader h;
	kshortflag_t   flag;
	//	kbool_t shell;                         // shell mode [true/false]
	//	kbool_t closefds;                      // closefds   [true/false]
	//	kbool_t bg;                            // bg mode    [true/false]
	kArray *env;                           // child process environment
	kString *command;                      // child process command
	kString *cwd;                          // child process current working directory
	int   rmode;
	FILE *rfp;                               // child process output identifier
	int   wmode;
	FILE  *wfp;                               // child process input identifier
	int   emode;
	FILE  *efp;                               // child process error identifier
	int cpid;                              // child process ID
	int bufferSize;                        // buffer size (unused)
	int timeout;                           // child process timeout value
	int status;                            // waitpid status
	int timeoutKill;                       // child process Timeout ending flag [true/false]
	//SUBPROC_RESOURCEMON_INSTANCE;
};

/* ------------------------------------------------------------------------ */
/* [class defs] */

#define CT_Subproc         cSubproc
#define TY_Subproc         cSubproc->typeId
#define IS_Subproc(O)      ((O)->h.ct == CT_Subproc)

/* ------------------------------------------------------------------------ */
/* [global varibals]
 */

static jmp_buf env;

static void alarmHandler(int sig) {
		siglongjmp(env, 1);
}
static int fgPid;
static void keyIntHandler(int sig) { kill(fgPid, SIGINT); }

/* ------------------------------------------------------------------------ */

// child process IO mode
//#define M_DEFAULT          -2			// initialization state
//#define M_NREDIRECT        -1			// parent process succession
//#define M_PIPE             0			// pipe
//#define M_STDOUT           1			// standard OUT
//#define M_FILE             2			// file

enum {
	M_DEFAULT   = -2,
	M_NREDIRECT = -1,
	M_PIPE      =  0,
	M_STDOUT    =  1,
	M_FILE      =  2
};

// child process status code
#define S_RUNNING          -300			// running
#define S_PREEXECUTION     -400			// preexecution
#define S_TIMEOUT          -500			// tiomeout
#define S_EXIT             0			// terminate

// subproc macro
#define MAXARGS            128				// the number maximum of parameters for spSplit
#define DELAY              1000				// the adjustment value at the time of signal transmission
#define DEF_TIMEOUT        10 * 1000		// default timeout valx
//#define DEF_TIMEOUT -1
#define ONEXEC(p)          (p->cpid > 0) ? 1 : 0
#define PREEXEC(p)         (p->cpid == -1) ? 1 : 0
#define WORD2INT(val)      (sizeof(val)==8) ? (val&0x7FFFFFFF)|((val>>32)&0x80000000) : val

/* ------------------------------------------------------------------------ */

#if defined (__APPLE__) || defined(__USE_LOCAL_PIPE2__)
// for fg & bg & exec & restart
static int pipe2( int *fd, int flags ) {
	int val;
	int p[2];
	if( pipe(p) < 0 ) {
		return -1;
	}
	if( (val=fcntl(p[0], F_GETFL, 0)) < 0 ) {
		goto L_ERR;
	}
	if( fcntl(p[0], F_SETFL, val | flags) < 0 ) {
		goto L_ERR;
	}
	if( (val=fcntl(p[1], F_GETFL, 0)) < 0 ) {
		goto L_ERR;
	}
	if( fcntl(p[1], F_SETFL, val | flags) < 0 ) {
		goto L_ERR;
	}
	fd[0] = p[0];
	fd[1] = p[1];
	return 0;
L_ERR:;
	{
		close(p[0]);
		close(p[1]);
		return -1;
	}
}
#endif

// for argSplit & fg & bg & exec & restart
// ===========================================================================
// <example>
// char str[12], *args[2];
// strcpy(str, "Hello world");
// int param = spSplit( str, args );
// if( param > 0 ) {
//    printf("param:%d args[0]:%s args[1]:%s\n", param, args[0], args[1]);
// } else {
//     printf("spSplit error[%d]\n", param);
// }
// ---------------------------------------------------------------------------
// output:
//     param:2 args[0]:Hello args[1]:world
// ===========================================================================
/**
 * @return number of parameter (zero or more)
 *         -1 is Internal Error
 *         -2 is the maximum error of the number of parameters
 */
static int spSplit(char* str, char* args[]) {

	if( (str == NULL) || (args == NULL) ) {
		return -1;
	}
	int indx;
	char *cp = str;
	for (indx = 0; indx <= MAXARGS; indx++) {
		if( indx == MAXARGS ) {
			return -2;
		} else if( (args[indx] = strtok(cp, " ")) == NULL ) {
			break;
		} else {
			cp = NULL;
		}
	}
	// number of parameter
	return indx;
}

// for getIN & getOUT & getERR
/**
 * @return "konoha.posix.File" class id
 */
//static KonohaClass khn_getFileClass(KonohaContext *kctx) {
//	char *name = "konoha.posix.File";
//	kbytes_t lname;
//	lname.ubuf = (kchar_t *)name;
//	lname.len = knh_strlen(lname.text);
//	return knh_getcid(kctx, lname);
//}

// for fg & bg & exec & restart
/**
 * @return child process id
 *         -1 is Internal Error
 */

static int subproc_popen(KonohaContext *kctx, kString* command, kSubproc *p, int defaultMode)
{
	struct kSubprocVar *sp = (struct kSubprocVar *)p;
	if(IS_NULL(command) || S_size(command) == 0) {
		return -1;
	}
	pid_t pid  = -1;
	int rmode = (sp->rmode==M_DEFAULT) ? defaultMode : sp->rmode;
	int wmode = (sp->wmode==M_DEFAULT) ? defaultMode : sp->wmode;
	int emode = (sp->emode==M_DEFAULT) ? defaultMode : sp->emode;
	int c2p[2];
	int p2c[2];
	int err[2];
	if(rmode == M_PIPE) {
		if(pipe2(c2p, O_NONBLOCK) != 0) {
			return -1;
		}
	}
	if(wmode == M_PIPE) {
		if(pipe(p2c) != 0) {
			// TODO: trace
			close(c2p[0]); close(c2p[1]);
			return -1;
		}
	}

	if(emode == M_PIPE) {
		if(pipe2(err, O_NONBLOCK) != 0) {
			// TODO: trace
			close(c2p[0]); close(c2p[1]);
			close(p2c[0]); close(p2c[1]);
			return -1;
		}
	}

	//	SETUP_RESOURCE_MONITOR(sp);

	switch(pid = fork()) {
	case -1:
		// parent process illegal route
		if(rmode == M_PIPE) {
			close(c2p[0]); close(c2p[1]);
		}
		if(wmode == M_PIPE) {
			close(p2c[0]); close(p2c[1]);
		}
		if(emode == M_PIPE) {
			close(err[0]); close(err[1]);
		}
		//		CLEANUP_RESOURCE_MONITOR(sp);
		break;
	case 0:
		// child process normal route
		//	SETUP_RESOURCE_MONITOR_FOR_CHILD(sp);
		if(wmode == M_PIPE){
			close(0);
			if(dup2(p2c[0], 0) == -1) {
				//TODO: trace
			}
			close(p2c[0]); close(p2c[1]);
		}
		else if(wmode == M_FILE) {
			close(0);
			if(dup2(fileno(sp->wfp), 0) == -1) {
				// TODO: trace
			}
		}
		if(rmode == M_PIPE) {
			close(1);
			if(dup2(c2p[1], 1) == -1){
				// TODO: trace dup2
			}
			close(c2p[0]); close(c2p[1]);
		}
		else if(rmode == M_FILE) {
			close(1);
			if(dup2(fileno(sp->rfp), 1) == -1) {
				// TODO: trace
			}
		}
		if(emode == M_PIPE) {
			close(2);
			dup2(err[1], 2);
			close(err[0]); close(err[1]);
		}
		else if(emode == M_FILE) {
			close(2);
			dup2(fileno(sp->efp), 2);
		}
		else if(emode == M_STDOUT) {
			close(2);
			dup2(1, 2);
		}
		if(SUBPROC_IsCloseFds(sp)) {
			// close other than 0, 1, and 2
			int cfd = 3;
			int maxFd = sysconf(_SC_OPEN_MAX);
			do {
				close(cfd);
			} while(++cfd < maxFd);
		}
		setsid(); // separate from tty
		if(!IS_NULL(sp->cwd)) { // TODO!!
			if(chdir(S_text((sp->cwd))) != 0) {
				//TODO: trace
				_exit(1);
			}
		}
		char *args[MAXARGS];
		//		if(sp->shell == 0) {
		if(!SUBPROC_IsShell(sp)) {
			// division of a commnad parameter
			if(spSplit((char*)S_text(command), args) < 0){
				//TODO: error
				args[0] = NULL;
			}
		}
		if(!IS_NULL(sp->env)) {
			// division of a environment parameter
			kArray *a = sp->env;
			int num = kArray_size(a);
			char *envs[num+1];
			int i;
			for(i = 0; i < num; i++) {
				envs[i] = (char *)S_text(a->stringItems[i]);
			}
			envs[num] = NULL;
			// exec load new process image if success.
			// if its not, they will return with -1.
			//			if(sp->shell == 0) {
			if(!SUBPROC_IsShell(sp)) {
				if(execve(args[0], args, envs) == -1) {
					//todo: trace
				}
			}
			else {
				if(execle("/bin/sh", "sh", "-c", S_text(command), NULL, envs) == -1) {
					//todo: trace
				}
			}
		} else {
			//			if(sp->shell == 0) {
			if(!SUBPROC_IsShell(sp)) {
				if(execvp(args[0], args) == -1) {
					//todo trace
				}
			}
			else {
				if(execlp("sh", "sh", "-c", S_text(command), NULL) == -1) {
					//todo trace
				}
			}
		}
		perror("subproc_popen :");
		_exit(1);
	default:
		// parent process normal route
#if defined(SUBPROC_ENABLE_RESOURCEMONITOR)
/		ATTACH_RESOURCE_MONITOR_FOR_CHILD(sp, pid);
//		size_t mem = FETCH_MEM_FROM_RESOURCE_MONITOR(sp);
//		fprintf(stderr, "menusage:%.1fM\n", (double)mem / (1024.0 * 1024.0));
		CLEANUP_RESOURCE_MONITOR(sp);
#endif
		if(rmode == M_PIPE) {
			sp->rfp = fdopen(c2p[0], "r");
			close(c2p[1]);
		}
		if(wmode == M_PIPE) {
			sp->wfp = fdopen(p2c[1], "w");
			close(p2c[0]);
		}
		if(emode == M_PIPE) {
			sp->efp = fdopen(err[0], "r");
			close(err[1]);
		}
	}
	return pid;
}


// for wait & fg & exec & communicate
/**
 * @return termination status of a child process
 */
static int subproc_wait(KonohaContext *kctx, int pid, kshortflag_t flag, int timeout, int *status ) {

#ifndef __APPLE__
	__sighandler_t alarm_oldset  = SIG_ERR;
	__sighandler_t keyInt_oldset = SIG_ERR;
	__sighandler_t ret = SIG_ERR;
#else
	sig_t alarm_oldset  = SIG_ERR;
	sig_t keyInt_oldset = SIG_ERR;
	sig_t ret = SIG_ERR;
#endif
	if(timeout > 0) {
		if(sigsetjmp(env, 1)) {
			// wait timeout return route
			setitimer(ITIMER_REAL, NULL, NULL);
			if(alarm_oldset != SIG_ERR) {
				ret = signal(SIGALRM, alarm_oldset);
			}
			else {
				ret = signal(SIGALRM, SIG_DFL);
			}
			if(ret == SIG_ERR) {
				//todo : tracesetitimer
			}
			return S_TIMEOUT;
		}
	}
	if(!FLAG_is(flag, SUBPROC_BACKGROUND)/*bg != 1*/) {
		// SIGINT registration
		fgPid = pid;
		keyInt_oldset = signal(SIGINT, keyIntHandler);
	}
	if(timeout > 0) {
		// SIGALRM registration
		struct itimerval its;
		its.it_value.tv_sec  = timeout / 1000;
		its.it_value.tv_usec = (timeout % 1000) * 1000;
		its.it_interval.tv_sec  = 0;
		its.it_interval.tv_usec = 0;
		setitimer(ITIMER_REAL, &its, NULL);
		alarm_oldset = signal(SIGALRM, alarmHandler);
	}
	int stat;
	waitpid(pid, &stat, WUNTRACED);
	if(timeout > 0) {
		// SIGALRM release
		struct itimerval its;
		its.it_value.tv_sec = 0;
		its.it_value.tv_usec = 0;
		its.it_interval.tv_sec = 0;
		its.it_interval.tv_usec = 0;
		setitimer(ITIMER_REAL, &its, NULL);
		if(alarm_oldset != SIG_ERR) {
			signal(SIGALRM, alarm_oldset);
		}
		else {
			signal(SIGALRM, SIG_DFL);
		}
	}
	if(FLAG_is(flag, SUBPROC_BACKGROUND)/*bg != 1*/) {
		// SIGINT release
		if(keyInt_oldset != SIG_ERR) {
			ret = signal(SIGINT, keyInt_oldset);
		}
		else {
			ret = signal(SIGINT, SIG_DFL);
		}
		if(ret == SIG_ERR) {
			// todo: error
		}
	}
	if(status != NULL) {
		*status = stat;
	}
	// return value creation
	if(WIFSIGNALED(stat)) {
		return WTERMSIG(stat) * -1;
	} else if( WIFSTOPPED(stat) ) {
		return WSTOPSIG(stat) * -1;
	} else {
		return S_EXIT;
	}
}

// for fg & bg & restart
/**
 * @return in the case of foreground, it is start status of a child process
 *         in the case of background, it is termination status of a child process
 */
static int proc_start(KonohaContext *kctx, struct kSubprocVar *sp) {
	int ret = S_PREEXECUTION;
	int pid = subproc_popen(kctx, sp->command, sp, M_NREDIRECT );
	if(pid > 0) {
		sp->cpid  = pid;
		//		if(sp->bg != 1) {
		if(!SUBPROC_IsBackground(sp)) {
			ret = subproc_wait(kctx, sp->cpid, sp->flag, sp->timeout, &sp->status );
		} else {
			// nomal end status for bg
			ret = 0;
		}
	} else {
		DBG_P("failed");
	}
	return ret;
}

// for new & enablePipemodeXXX(false) & enableStandardXXX(false) & enableERR2StdOUT(false)
static inline void initFd(pfd_t *p) {
	p->mode = M_DEFAULT;
	p->fp   = NULL;
}

// for setFileXXX & PipemodeXXX(true) & enableStandardXXX(true) & enableERR2StdOUT(true)
static void setFd(KonohaContext *kctx, pfd_t *p, int changeMode, FILE* ptr) {
	if(((p->mode == M_PIPE) || (p->mode == M_FILE)) && !(p->mode == changeMode)) {
		// warning of the pipe or file mode overwrite
		//char *msg = (p->mode == M_PIPE) ? "pipe has already set, but we overwrite it." :
		//"file has already set, but we overwrite it.";
		//WarnTagPackageMessage(kctx, msg );
		//fprintf(stderr, "%s\n", msg);
	}
	p->mode = changeMode;
	p->fp = ptr;
}

// for exec & restart
static void clearFd(pfd_t *p) {
	if(((p->mode == M_PIPE) || (p->mode == M_DEFAULT)) && (p->fp != NULL)) {
		// a file identification child does not close
		fclose(p->fp);
		p->fp = NULL;
	}
}

// for poll
static int getPidStatus(int pid, int *status) {
	return waitpid(pid, status, WNOHANG | WUNTRACED | WCONTINUED);
}

// for Subproc_free & fg & exec & communicate & restart
static void killWait(int pid) {
	int status;
	kill(pid, SIGKILL);
	usleep(DELAY);
	getPidStatus(pid, &status); // this wait is in order not to leave a zombie process.
}

static kbool_t kPipeReadArray(KonohaContext *kctx, kArray *a, FILE *fp)
{
	char buf[K_PAGESIZE];
	while(1) {
		size_t size = fread(buf, 1, sizeof(buf), fp);
		if(size > 0) {
			KLIB kArray_add(kctx, a, KLIB new_kString(kctx, GcUnsafe, buf, size, 0));
		}
		else {
			break;
		}
	}
	if(ferror(fp) == 0) {
		KLIB kArray_add(kctx, a, KNULL(String));
		return false;
	}
	return true;
}

static kString *kPipeReadStringNULL(KonohaContext *kctx, FILE *fp)
{
	char buf[K_PAGESIZE];
	KGrowingBuffer wb;
	KLIB Kwb_init(&(kctx->stack->cwb), &wb);
	while(1) {
		size_t size = fread(buf, 1, sizeof(buf), fp);
		if(size > 0) {
			KLIB Kwb_write(kctx, &wb, buf, size);
		}
		else {
			break;
		}
	}
	if(ferror(fp)) {
		return NULL;
	}
	kString *ret = KLIB new_kString(kctx, GcUnsafe, KLIB Kwb_top(kctx, &wb, 0), Kwb_bytesize(&wb), 0);
	KLIB Kwb_free(&wb);
	return ret;
}

static kString *kSubproc_checkOutput(KonohaContext *kctx, kSubproc *p, kString *command)
{
	//	subprocData_t *p = sp->spd;
	struct kSubprocVar *sp = (struct kSubprocVar*)p;
	kString *ret_s = KNULL(String);
	if(PREEXEC(sp)) {
		sp->timeoutKill = 0;
		int pid = subproc_popen(kctx, command, sp, M_PIPE );
		if(pid > 0 ) {
			if(subproc_wait(kctx, pid, 0, sp->timeout, NULL ) == S_TIMEOUT ) {
				sp->timeoutKill = 1;
				killWait(pid);
				clearFd((pfd_t*)(&sp->rmode));
				clearFd((pfd_t*)&sp->wmode);
				clearFd((pfd_t*)&sp->emode);
				//todo: error
			}
			else if( (sp->rmode == M_PIPE) || (sp->rmode == M_DEFAULT) ) {
				ret_s = kPipeReadStringNULL(kctx, sp->rfp);
				if(ret_s == NULL) {
					// todo: error
					ret_s = KNULL(String);
				}
				clearFd((pfd_t*)&sp->rmode);
				clearFd((pfd_t*)&sp->wmode);
				clearFd((pfd_t*)&sp->emode);
			} else if(sp->rmode == M_FILE) {
				char *msg = " will be ignored.";
				char *cmd = (char *)command;
				char mbuf[strlen(msg)+strlen(cmd)+1];
				snprintf(mbuf, sizeof(mbuf), "'%s'%s", cmd, msg);
			}
		} else {
			//toro popen error
		}
		// remove alarm
		struct itimerval val;
		getitimer(ITIMER_REAL, &val);
		val.it_value.tv_sec = 0;
		setitimer(ITIMER_REAL, &val, NULL);
	}
	return ret_s;
}

/* ------------------------------------------------------------------------ */

//## Subproc Subproc.new(String cmd, boolean closefds);
static KMETHOD Subproc_new(KonohaContext *kctx, KonohaStack *sfp)
{
	struct kSubprocVar *sp = (struct kSubprocVar *)sfp[0].asObject;
	KFieldSet(sp, sp->command, sfp[1].asString);
	if (sfp[2].boolValue) SUBPROC_setCloseFds(sp);
	KReturn(sp);
}

//## boolean Subproc.bg();
static KMETHOD Subproc_bg(KonohaContext *kctx, KonohaStack *sfp)
{
	struct kSubprocVar *sp = (struct kSubprocVar *)sfp[0].asObject;
	int ret = -1;
	if(PREEXEC(sp)) {
		sp->timeoutKill = 0;
		//sp->bg = 1;
		SUBPROC_setBackground(sp);
		if( (ret = proc_start(kctx, sp)) != 0 ) {
			// todo : proc_strat error
		}
	}
	KReturnUnboxValue(ret == 0);
}

//## int Subproc.fg();
static KMETHOD Subproc_fg(KonohaContext *kctx, KonohaStack *sfp)
{
	struct kSubprocVar *sp = (struct kSubprocVar *)sfp[0].asObject;
	int ret = S_PREEXECUTION;
	if(PREEXEC(sp)) {
		sp->timeoutKill = 0;
		//sp->bg = 0;
		SUBPROC_unsetBackground(sp);
		if( (ret = proc_start(kctx, sp)) == S_TIMEOUT ) {
			sp->timeoutKill = 1;
			killWait(sp->cpid);
		}
	}
	KReturnUnboxValue(ret);
}

//## @Throwable String Subproc.exec(String data);
static KMETHOD Subproc_exec(KonohaContext *kctx, KonohaStack *sfp)
{
	KReturn(kSubproc_checkOutput(kctx, (kSubproc *)sfp[0].asObject, sfp[1].asString));
}

//## @Throwable String[] Subproc.communicate(String input);
static KMETHOD Subproc_communicate(KonohaContext *kctx, KonohaStack *sfp)
{
	kArray *ret_a = KNULL(Array);
	struct kSubprocVar *sp = (struct kSubprocVar *)sfp[0].asObject;
	KMakeTrace(trace, sfp);
	if(ONEXEC(sp)) {
		if((sp->wmode == M_PIPE) && (S_size(sfp[1].asString) > 0)) {
			kString *s = sfp[1].asString;
			// The measure against panic,
			// if "Broken Pipe" is detected at the time of writing.
#ifndef __APPLE__
			__sighandler_t oldset = signal(SIGPIPE, SIG_IGN);
#else
			sig_t oldset = signal(SIGPIPE, SIG_IGN);
#endif
			if(fwrite(S_text(s), sizeof(char), S_size(s), sp->wfp) > 0) {
//				fputc('\n', p->w.fp);
//				fflush(p->w.fp);
//				fsync(fileno(p->w.fp));
				fclose(sp->wfp);
			} else {
				KTraceApi(trace, SystemFault, "fwrite", LogErrno);
			}
			if(oldset != SIG_ERR) {
				signal(SIGPIPE, oldset);
			}
		}
		if(subproc_wait(kctx, sp->cpid, sp->flag, sp->timeout, &sp->status ) == S_TIMEOUT) {
			sp->timeoutKill = 1;
			killWait(sp->cpid);
			//TODO: raise error
		} else {
			ret_a = (kArray *)KLIB new_kObject(kctx, GcUnsafe, CT_Array, 0);
			if(sp->rmode == M_PIPE) {
				if(!kPipeReadArray(kctx, ret_a, sp->rfp)) {
					//TODO;  raise error
				}
			}
			else {
				KLIB kArray_add(kctx, ret_a, KNULL(String));
			}
			if(sp->emode == M_PIPE) {
				if(!kPipeReadArray(kctx, ret_a, sp->efp)) {
					// TODO: raise error
				}
			}
			else {
				KLIB kArray_add(kctx, ret_a, KNULL(String));
			}
		}
	}
	KReturn(ret_a);
}

//## @Restricted boolean Subproc.enableShellmode(boolean isShellmode);
static KMETHOD Subproc_enableShellmode(KonohaContext *kctx, KonohaStack *sfp)
{
	struct kSubprocVar *sp = (struct kSubprocVar *)sfp[0].asObject;
	int ret = PREEXEC(sp);
	if(ret) {
		//		sp->shell = sfp[1].boolValue;
		if (sfp[1].boolValue) SUBPROC_setShell(sp);
	}
	KReturnUnboxValue(ret);
}

//## boolean Subproc.setEnv(Map env);
//KMETHOD Subproc_setEnv(KonohaContext *kctx, KonohaStack *sfp)
//{
//	kSubproc *sp = (kSubproc *)sfp[0].asObject;
//	subprocData_t *p = sp->spd;
//	int ret = PREEXEC(p);
//	if( ret ) {
//		kDictMap *env = (kDictMap *)sfp[1].asObject;
//		int i;
//		size_t msize = env->spi->size(ctx, env->mapptr);
//		if( p->env != (kArray *)KNH_NULVAL(TY_Array) ) {
//			knh_Array_clear( ctx, p->env, 0 );
//		}
//		p->env = new_Array(ctx, TY_String, msize);
//		for (i = 0; i < msize; i++) {
//			kString *key = (kString *)knh_DictMap_keyAt(env, i);
//			kString *val = (kString *)knh_DictMap_valueAt(env, i);
//			char buf[key->str.len + val->str.len + 2];
//			snprintf(buf, sizeof(buf), "%s=%s", key->str.buf, val->str.buf);
//			knh_Array_add( ctx, p->env, new_String(ctx, buf) );
//		}
//	}
//	KReturnUnboxValue( ret );
//}

//## boolean Subproc.setCwd(String cwd);
static KMETHOD Subproc_setCwd(KonohaContext *kctx, KonohaStack *sfp)
{
	struct kSubprocVar *sp = (struct kSubprocVar *)sfp[0].asObject;
	int ret = PREEXEC(sp);
	if(ret) {
		sp->cwd = KLIB new_kString(kctx, GcUnsafe, S_text(sfp[1].asString), S_size(sfp[1].asString), 0);
	}
	KReturnUnboxValue(ret);
}

//## boolean Subproc.setBufsize(int size);
static KMETHOD Subproc_setBufsize(KonohaContext *kctx, KonohaStack *sfp)
{
	struct kSubprocVar *sp = (struct kSubprocVar *)sfp[0].asObject;
	int ret = PREEXEC(sp);
	if(ret) {
		sp->bufferSize = WORD2INT(sfp[1].intValue);
	}
	KReturnUnboxValue(ret);
}

//## boolean Subproc.setFileIN(File in);
static KMETHOD Subproc_setFileIN(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	int ret = PREEXEC(sp);
	if(ret) {
		kFile *kfile = (kFile *)sfp[1].asObject;
		setFd(kctx,(pfd_t*)&sp->wmode, M_FILE, kfile->fp);
	}
	KReturnUnboxValue(ret);
}

//## boolean Subproc.setFileOUT(File out);
KMETHOD Subproc_setFileOUT(KonohaContext *kctx, KonohaStack *sfp)
{
	struct kSubprocVar *sp = (struct kSubprocVar *)sfp[0].asObject;
	int ret = PREEXEC(sp);
	if(ret) {
		kFile *kfile = (kFile *)sfp[1].asObject;
		setFd(kctx, (pfd_t*)&sp->rmode, M_FILE, kfile->fp);
	}
	KReturnUnboxValue(ret);
}

//## boolean Subproc.setFileERR(File err);
KMETHOD Subproc_setFileERR(KonohaContext *kctx, KonohaStack *sfp)
{
	struct kSubprocVar *sp = (struct kSubprocVar *)sfp[0].asObject;
	int ret = PREEXEC(sp);
	if(ret) {
			kFile *kfile = (kFile *)sfp[1].asObject;
			setFd(kctx, (pfd_t*)&sp->emode, M_FILE, kfile->fp);
	}
	KReturnUnboxValue(ret);
}

//## int Subproc.getPid();
static KMETHOD Subproc_getPid(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	KReturnUnboxValue(ONEXEC(sp) == 1 ? sp->cpid : -1 );
}

//## int Subproc.getTimeout();
static KMETHOD Subproc_getTimeout(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	KReturnUnboxValue(sp->timeout);
}

//## int Subproc.getReturncode();
static KMETHOD Subproc_getReturncode(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	KReturnUnboxValue(sp->status);
}

//## boolean Subproc.enablePipemodeIN(Boolean isPipemode);
static KMETHOD Subproc_enablePipemodeIN(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	int ret = PREEXEC(sp);
	if(ret) {
		if(sfp[1].boolValue == 1) {
			setFd(kctx, (pfd_t*)&sp->wmode, M_PIPE, NULL);
		} else {
			if(sp->wmode == M_PIPE) {
				initFd((pfd_t*)(&sp->wmode));
			}
		}
	}
	KReturnUnboxValue(ret);
}

//## boolean Subproc.enablePipemodeOUT(Boolean isPipemode);
static KMETHOD Subproc_enablePipemodeOUT(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	int ret = PREEXEC(sp);
	if(ret) {
		if(sfp[1].boolValue == 1) {
			setFd(kctx, (pfd_t*)&sp->rmode, M_PIPE, NULL);
		} else {
			if(sp->rmode == M_PIPE) {
				initFd((pfd_t*)(&sp->rmode));
			}
		}
	}
	KReturnUnboxValue(ret);
}

//## boolean Subproc.enablePipemodeERR(Boolean isPipemode);
static KMETHOD Subproc_enablePipemodeERR(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	int ret = PREEXEC(sp);
	if(ret) {
		if(sfp[1].boolValue == 1) {
			setFd(kctx, (pfd_t*)&sp->emode, M_PIPE, NULL);
		}
		else {
			if(sp->emode == M_PIPE) {
				initFd((pfd_t*)&sp->emode);
			}
		}
	}
	KReturnUnboxValue(ret);
}

//## boolean Subproc.enableStandardIN(Boolean isStandard);
static KMETHOD Subproc_enableStandardIN(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	int ret = PREEXEC(sp);
	if(ret) {
		if(sfp[1].boolValue == 1) {
			setFd(kctx, (pfd_t*)&sp->wmode, M_NREDIRECT, NULL);
		}
		else {
			if(sp->wmode == M_NREDIRECT) {
				initFd((pfd_t*)&sp->wmode);
			}
		}
	}
	KReturnUnboxValue(ret);
}

//## boolean Subproc.enableStandardOUT(Boolean isStandard);
static KMETHOD Subproc_enableStandardOUT(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	int ret = PREEXEC(sp);
	if(ret) {
		if(sfp[1].boolValue == 1) {
			setFd(kctx, (pfd_t*)&sp->rmode, M_NREDIRECT, NULL);
		} else {
			if(sp->rmode == M_NREDIRECT) {
				initFd((pfd_t*)&sp->rmode);
			}
		}
	}
	KReturnUnboxValue(ret);
}

//## boolean Subproc.enableStandardERR(Boolean isStandard);
static KMETHOD Subproc_enableStandardERR(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	int ret = PREEXEC(sp);
	if(ret) {
		if(sfp[1].boolValue == 1) {
			setFd(kctx, (pfd_t*)&sp->emode, M_NREDIRECT, NULL);
		} else {
			if(sp->emode == M_NREDIRECT) {
				initFd((pfd_t*)&sp->emode);
			}
		}
	}
	KReturnUnboxValue(ret);
}

//## boolean Subproc.enableERR2StdOUT(Boolean isStdout);
static KMETHOD Subproc_enableERR2StdOUT(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	int ret = PREEXEC(sp);
	if(ret) {
		if(sfp[1].boolValue == 1) {
			setFd(kctx, (pfd_t*)&sp->emode, M_STDOUT, NULL);
		} else {
			if(sp->emode == M_STDOUT) {
				initFd((pfd_t*)&sp->emode);
			}
		}
	}
	KReturnUnboxValue(ret);
}

//## boolean Subproc.isShellmode();
static KMETHOD Subproc_isShellmode(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	//	KReturnUnboxValue(sp->shell == 1);
	KReturnUnboxValue(SUBPROC_IsShell(sp));
}

//## boolean Subproc.isPipemodeIN();
static KMETHOD Subproc_isPipemodeIN(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	KReturnUnboxValue(sp->wmode == M_PIPE);
}

//## boolean Subproc.isPipemodeOUT();
static KMETHOD Subproc_isPipemodeOUT(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	KReturnUnboxValue(sp->rmode == M_PIPE);
}

//## boolean Subproc.isPipemodeERR();
static KMETHOD Subproc_isPipemodeERR(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	KReturnUnboxValue(sp->emode == M_PIPE);
}

//## boolean Subproc.isStandardIN();
static KMETHOD Subproc_isStandardIN(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	KReturnUnboxValue(sp->wmode == M_NREDIRECT);
}

//## boolean Subproc.isStandardOUT();
static KMETHOD Subproc_isStandardOUT(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	KReturnUnboxValue(sp->rmode == M_NREDIRECT);
}

//## boolean Subproc.isStandardERR();
static KMETHOD Subproc_isStandardERR(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	KReturnUnboxValue(sp->emode == M_NREDIRECT);
}

//## boolean Subproc.isERR2StdOUT();
static KMETHOD Subproc_isERR2StdOUT(KonohaContext *kctx, KonohaStack *sfp)
{
	kSubproc *sp = (kSubproc *)sfp[0].asObject;
	KReturnUnboxValue(sp->emode == M_STDOUT);
}

//## @Static int Subproc.call(String args);
//static KMETHOD Subproc_call(KonohaContext *kctx, KonohaStack *sfp)
//{
//	kSubproc *sp = /*G*/new_(Subproc, NULL);
//	subprocData_t *p = sp->spd;
//	int ret = S_PREEXECUTION;
//	KFieldSet(sp, p->command, sfp[1].asString);
//	if( (ret = proc_start(kctx, p)) == S_TIMEOUT ) {
//		p->timeoutKill = 1;
//		killWait(p->cpid);
////		KNH_NTHROW2(kctx, sfp, "Script!!", "subproc.fg :: timeout", K_FAILED, KNH_LDATA0);
//	}
//	int status = p->status;
//	if(WIFEXITED(status)) {
//		ret = WEXITSTATUS(status);
//	} else if(WIFSIGNALED(status)) {
//		ret = WTERMSIG(status) * -1;
//	} else if(WIFSTOPPED(status)) {
//		ret = WSTOPSIG(status) * -1;
//	}
//	KReturnUnboxValue( ret );
//}

//## @Static String Subproc.checkOutput(String args);
//static KMETHOD Subproc_checkOutput(KonohaContext *kctx, KonohaStack *sfp)
//{
//	KReturn(kSubproc_checkOutput(kctx, /*G*/new_(Subproc, NULL), sfp[1].asString));
//}

/* ------------------------------------------------------------------------ */

static void kSubproc_init(KonohaContext *kctx, kObject *o, void *conf)
{
	//	struct kSubprocVar *proc = (struct kSubprocVar *)o;
	struct kSubprocVar *p = (struct kSubprocVar *)o;
	/*	if(conf != NULL) {
		proc->spd = (subprocData_t *)conf;
	} else {
	*/
	//	subprocData_t *p = (subprocData_t *)KCalloc_UNTRACE(sizeof(subprocData_t), 1);
	p->command     = KNULL(String);
	p->cwd         = KNULL(String);
	p->env         = KNULL(Array);
	p->cpid        = -1;
	//	p->closefds    = 0;
	//	p->bg          = 0;
	//	p->shell       = 0;
	p->flag        = 0;
	p->timeout     = DEF_TIMEOUT;
	p->bufferSize  = 0;
	p->timeoutKill = 0;
	initFd((pfd_t*)&p->rmode);
	initFd((pfd_t*)&p->wmode);
	initFd((pfd_t*)&p->emode);
	INIT_RESOURCE_MONITOR(p);
}

static void kSubproc_free(KonohaContext *kctx, kObject *o)
{
	/*	struct kSubprocVar *proc = (struct _kSubproc *)o;
		if(proc->spd != NULL) {
		KFree(proc->spd, sizeof(subprocData_t));
		proc->spd = NULL;
	}
	*/
}

static void kSubproc_reftrace(KonohaContext *kctx, kObject *o, KObjectVisitor *visitor)
{
	struct kSubprocVar *proc = (struct kSubprocVar *)o;
	BEGIN_REFTRACE(3);
	KREFTRACEv(proc->env);
	KREFTRACEv(proc->command);
	KREFTRACEv(proc->cwd);
	END_REFTRACE();
}

/* ------------------------------------------------------------------------ */

#define _Public   kMethod_Public
#define _Static   kMethod_Static
#define _Im kMethod_Immutable
#define _F(F)   (intptr_t)(F)

#define CT_FILE   cFILE
#define TY_FILE   cFILE->typeId

static kbool_t subproc_initPackage(KonohaContext *kctx, kNameSpace *ns, int argc, const char**args, KTraceInfo *trace)
{
	KRequirePackage("posix.file", trace);
	KDEFINE_CLASS defSubproc = {
		STRUCTNAME(Subproc),
		.cflag    = kClass_Final,
		.init     = kSubproc_init,
		.free     = kSubproc_free,
		.reftrace = kSubproc_reftrace,
	};

	KonohaClass *cSubproc = KLIB kNameSpace_defineClass(kctx, ns, NULL, &defSubproc, trace);
//	base->cSubproc= KLIB kNameSpace_defineClass(kctx, ns, NULL, &defSubproc, trace);
	KonohaClass *cFILE = KLIB kNameSpace_getClass(kctx, ns, "posix.file.FILE", strlen("posix.file.FILE"), NULL);

	kparamtype_t ps = {TY_String, FN_("str")};
	KonohaClass *CT_StringArray2 = KLIB KonohaClass_Generics(kctx, CT_Array, TY_String, 1, &ps);
	ktype_t TY_StringArray = CT_StringArray2->typeId;

	KDEFINE_METHOD MethodData[] = {
		_Public, _F(Subproc_new), TY_Subproc, TY_Subproc,MN_("new"), 2, TY_String, FN_("path"), TY_boolean, FN_("mode"),
		_Public|_Im, _F(Subproc_fg), TY_int, TY_Subproc, MN_("fg"), 0,
		_Public|_Im, _F(Subproc_bg), TY_boolean, TY_Subproc, MN_("bg"), 0,
		_Public|_Im, _F(Subproc_exec), TY_String, TY_Subproc, MN_("exec"), 1, TY_String, FN_("data"),
		_Public|_Im, _F(Subproc_communicate), TY_StringArray, TY_Subproc, MN_("communicate"), 1, TY_String, FN_("input"),
//		_Public|_Im, _F(Subproc_poll), TY_int, TY_Subproc, MN_("poll"), 0,
//		_Public|_Im, _F(Subproc_wait), TY_int, TY_Subproc, MN_("wait"), 0,
//		_Public|_Im, _F(Subproc_sendSignal), TY_boolean, TY_Subproc, MN_("sendSignal"), 1 TY_int, FN_("signal"),
//		_Public|_Im, _F(Subproc_terminate), TY_boolean, TY_Subproc, MN_("terminate"), 0,
		_Public|_Im, _F(Subproc_getPid), TY_int, TY_Subproc, MN_("getPid"), 0,
		_Public|_Im, _F(Subproc_enableShellmode), TY_boolean, TY_Subproc, MN_("enableShellmode"), 1, TY_boolean, FN_("isShellmode"),
		_Public|_Im, _F(Subproc_enablePipemodeIN), TY_boolean, TY_Subproc, MN_("enablePipemodeIN"), 1, TY_boolean, FN_("isPipemode"),
		_Public|_Im, _F(Subproc_enablePipemodeOUT), TY_boolean, TY_Subproc, MN_("enablePipemodeOUT"), 1, TY_boolean, FN_("isPipemode"),
		_Public|_Im, _F(Subproc_enablePipemodeERR), TY_boolean, TY_Subproc, MN_("enablePipemodeERR"), 1, TY_boolean, FN_("isPipemode"),
		_Public|_Im, _F(Subproc_enableStandardIN), TY_boolean, TY_Subproc, MN_("enableStandardIN"), 1, TY_boolean, FN_("isStandard"),
		_Public|_Im, _F(Subproc_enableStandardOUT), TY_boolean, TY_Subproc, MN_("enableStandardOUT"), 1, TY_boolean, FN_("isStandard"),
		_Public|_Im, _F(Subproc_enableStandardERR), TY_boolean, TY_Subproc, MN_("enableStandardERR"), 1, TY_boolean, FN_("isStandard"),
		_Public|_Im, _F(Subproc_isShellmode), TY_boolean, TY_Subproc, MN_("isShellmode"), 0,
		_Public|_Im, _F(Subproc_isPipemodeIN), TY_boolean, TY_Subproc, MN_("isPipemodeIN"), 0,
		_Public|_Im, _F(Subproc_isPipemodeOUT), TY_boolean, TY_Subproc, MN_("isPipemodeOUT"), 0,
		_Public|_Im, _F(Subproc_isPipemodeERR), TY_boolean, TY_Subproc, MN_("isPipemodeERR"), 0,
		_Public|_Im, _F(Subproc_isStandardIN), TY_boolean, TY_Subproc, MN_("isStandardIN"), 0,
		_Public|_Im, _F(Subproc_isStandardOUT), TY_boolean, TY_Subproc, MN_("isStandardOUT"), 0,
		_Public|_Im, _F(Subproc_isStandardERR), TY_boolean, TY_Subproc, MN_("isStandardERR"), 0,
		_Public|_Im, _F(Subproc_isERR2StdOUT), TY_boolean, TY_Subproc, MN_("isERR2StdOUT"), 0,
//		_Public|_Static|_Im, _F(Subproc_call), TY_int, TY_Subproc, MN_("call"), 1, TY_String, FN_("args"),
//		_Public|_Static|_Im, _F(Subproc_checkOutput), TY_String, TY_Subproc, MN_("checkOutput"), 1, TY_String, FN_("data"),
		_Public|_Im, _F(Subproc_setCwd), TY_boolean, TY_Subproc, MN_("setCwd"), 1, TY_String, FN_("cwd"),
		_Public|_Im, _F(Subproc_setBufsize), TY_boolean, TY_Subproc, MN_("setBufsize"), 1, TY_int, FN_("bufsize"),
		_Public|_Im, _F(Subproc_setFileIN), TY_boolean, TY_Subproc, MN_("setFileIN"), 1, TY_FILE, FN_("file"),
		_Public|_Im, _F(Subproc_setFileOUT), TY_boolean, TY_Subproc, MN_("setFileOUT"), 1, TY_FILE, FN_("file"),
		_Public|_Im, _F(Subproc_setFileERR), TY_boolean, TY_Subproc, MN_("setFileERR"), 1, TY_FILE, FN_("file"),
		_Public|_Im, _F(Subproc_getTimeout), TY_int, TY_Subproc, MN_("getTimeout"), 0,
		_Public|_Im, _F(Subproc_getReturncode), TY_int, TY_Subproc, MN_("getReturncode"), 0,
		_Public|_Im, _F(Subproc_enableERR2StdOUT), TY_boolean, TY_Subproc, MN_("enableERR2StdOUT"), 1, TY_boolean, FN_("isStdout"),
		DEND,
	};
	KLIB kNameSpace_loadMethodData(kctx, ns, MethodData);
	return true;
}

static kbool_t subproc_setupPackage(KonohaContext *kctx, kNameSpace *ns, isFirstTime_t isFirstTime, KTraceInfo *trace)
{
	return true;
}

// --------------------------------------------------------------------------

KDEFINE_PACKAGE* subproc_init(void)
{
	static KDEFINE_PACKAGE d = {
		KPACKNAME("subproc", "1.0"),
		.initPackage    = subproc_initPackage,
		.setupPackage   = subproc_setupPackage,
	};
	return &d;
}

#ifdef __cplusplus
}
#endif
