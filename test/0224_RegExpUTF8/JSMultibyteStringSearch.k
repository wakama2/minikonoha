//written by Joseph

import("konoha.regexp");

void test() {
	assert("こんにちは、世界!".search(/世界/gi) == 6);
	assert("こんにちは、世界!".search(/世界/g) == 6);
	assert("こんにちは、世界!".search(/世界/i) == 6);
}

test();
