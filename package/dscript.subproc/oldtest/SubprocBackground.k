// written by shinpei

import ("konoha.new");
import( "dscript.subproc");
import ("konoha.array");

void test () {
	Subproc p = new Subproc("echo $SHELL", false);
	p.enablePipemodeOUT(true);
	p.bg();
	// need enough time
	String[] ret = p.communicate("");
	assert(ret[0] == "$SHELL\n");
}

test();

