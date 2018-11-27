#include "duktape.h"

static extstr_intern_check_fn extstr_intern_check_func = NULL;
static extstr_free_fn extstr_free_func= NULL;

const char* duk_extstr_intern_check(void* _udata, void* _str, unsigned long _blen) {
    if (extstr_intern_check_func) {
        return extstr_intern_check_func(_udata, _str, _blen);
    }
    return NULL;
}

void duk_extstr_free(void* _udata, const void * _extdata) {
    if (extstr_free_func) { extstr_free_func(_udata, _extdata); }

}
void duk_extstr_set_handler(extstr_intern_check_fn _check, extstr_free_fn _free) {
    extstr_intern_check_func = _check;
    extstr_free_func = _free;
}

