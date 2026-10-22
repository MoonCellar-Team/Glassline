#include <stdio.h>
#include <stdlib.h>

extern "C" {

void __Unwind_SjLj_Register(void*) {}
void __Unwind_SjLj_Unregister(void*) {}

void __Unwind_SjLj_Resume(void*) {
    fprintf(stderr, "medecomp: исключение при раскрутке SjLj — на PC не поддерживается\n");
    abort();
}

}
