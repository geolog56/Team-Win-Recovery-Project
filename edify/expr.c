/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#include "expr.h"

// Functions should:
//
//    - return a malloc()'d string
//    - if Evaluate() on any argument returns NULL, return NULL.

int BooleanString(const char* s) {
    return s[0] != '\0';
}

char* Evaluate(State* state, Expr* expr) {
    return expr->fn(expr->name, state, expr->argc, expr->argv);
}

char* ConcatFn(const char* name, State* state, int argc, Expr* argv[]) {
    if (argc == 0) {
        return strdup("");
    }
    char** strings = malloc(argc * sizeof(char*));
    int i;
    for (i = 0; i < argc; ++i) {
        strings[i] = NULL;
    }
    char* result = NULL;
    int length = 0;
    for (i = 0; i < argc; ++i) {
        strings[i] = Evaluate(state, argv[i]);
        if (strings[i] == NULL) {
            goto done;
        }
        length += strlen(strings[i]);
    }

    result = malloc(length+1);
    int p = 0;
    for (i = 0; i < argc; ++i) {
        strcpy(result+p, strings[i]);
        p += strlen(strings[i]);
    }
    result[p] = '\0';

  done:
    for (i = 0; i < argc; ++i) {
        free(strings[i]);
    }
    free(strings);
    return result;
}

char* IfElseFn(const char* name, State* state, int argc, Expr* argv[]) {
    if (argc != 2 && argc != 3) {
        free(state->errmsg);
        state->errmsg = strdup("ifelse expects 2 or 3 arguments");
        return NULL;
    }
    char* cond = Evaluate(state, argv[0]);
    if (cond == NULL) {
        return NULL;
    }

    if (BooleanString(cond) == true) {
        free(cond);
        return Evaluate(state, argv[1]);
    } else {
        if (argc == 3) {
            free(cond);
            return Evaluate(state, argv[2]);
        } else {
            return cond;
        }
    }
}

char* AbortFn(const char* name, State* state, int argc, Expr* argv[]) {
    char* msg = NULL;
    if (argc > 0) {
        msg = Evaluate(state, argv[0]);
    }
    free(state->errmsg);
    if (msg) {
        state->errmsg = msg;
    } else {
        state->errmsg = strdup("called abort()");
    }
    return NULL;
}

char* AssertFn(const char* name, State* state, int argc, Expr* argv[]) {
    int i;
    for (i = 0; i < argc; ++i) {
        char* v = Evaluate(state, argv[i]);
        if (v == NULL) {
            return NULL;
        }
        int b = BooleanString(v);
        free(v);
        if (!b) {
            int prefix_len;
            int len = argv[i]->end - argv[i]->start;
            char* err_src = malloc(len + 20);
            strcpy(err_src, "assert failed: ");
            prefix_len = strlen(err_src);
            memcpy(err_src + prefix_len, state->script + argv[i]->start, len);
            err_src[prefix_len + len] = '\0';
            free(state->errmsg);
            state->errmsg = err_src;
            return NULL;
        }
    }
    return strdup("");
}

char* SleepFn(const char* name, State* state, int argc, Expr* argv[]) {
    char* val = Evaluate(state, argv[0]);
    if (val == NULL) {
        return NULL;
    }
    int v = strtol(val, NULL, 10);
    sleep(v);
    return val;
}

char* StdoutFn(const char* name, State* state, int argc, Expr* argv[]) {
    int i;
    for (i = 0; i < argc; ++i) {
        char* v = Evaluate(state, argv[i]);
        if (v == NULL) {
            return NULL;
        }
        fputs(v, stdout);
        free(v);
    }
    return strdup("");
}

char* LogicalAndFn(const char* name, State* state,
                   int argc, Expr* argv[]) {
    char* left = Evaluate(state, argv[0]);
    if (left == NULL) return NULL;
    if (BooleanString(left) == true) {
        free(left);
        return Evaluate(state, argv[1]);
    } else {
        return left;
    }
}

char* LogicalOrFn(const char* name, State* state,
                  int argc, Expr* argv[]) {
    char* left = Evaluate(state, argv[0]);
    if (left == NULL) return NULL;
    if (BooleanString(left) == false) {
        free(left);
        return Evaluate(state, argv[1]);
    } else {
        return left;
    }
}

char* LogicalNotFn(const char* name, State* state,
                   int argc, Expr* argv[]) {
    char* val = Evaluate(state, argv[0]);
    if (val == NULL) return NULL;
    bool bv = BooleanString(val);
    free(val);
    if (bv) {
        return strdup("");
    } else {
        return strdup("t");
    }
}

char* SubstringFn(const char* name, State* state,
                  int argc, Expr* argv[]) {
    char* needle = Evaluate(state, argv[0]);
    if (needle == NULL) return NULL;
    char* haystack = Evaluate(state, argv[1]);
    if (haystack == NULL) {
        free(needle);
        return NULL;
    }

    char* result = strdup(strstr(haystack, needle) ? "t" : "");
    free(needle);
    free(haystack);
    return result;
}

char* EqualityFn(const char* name, State* state, int argc, Expr* argv[]) {
    char* left = Evaluate(state, argv[0]);
    if (left == NULL) return NULL;
    char* right = Evaluate(state, argv[1]);
    if (right == NULL) {
        free(left);
        return NULL;
    }

    char* result = strdup(strcmp(left, right) == 0 ? "t" : "");
    free(left);
    free(right);
    return result;
}

char* InequalityFn(const char* name, State* state, int argc, Expr* argv[]) {
    char* left = Evaluate(state, argv[0]);
    if (left == NULL) return NULL;
    char* right = Evaluate(state, argv[1]);
    if (right == NULL) {
        free(left);
        return NULL;
    }

    char* result = strdup(strcmp(left, right) != 0 ? "t" : "");
    free(left);
    free(right);
    return result;
}

char* SequenceFn(const char* name, State* state, int argc, Expr* argv[]) {
    char* left = Evaluate(state, argv[0]);
    if (left == NULL) return NULL;
    free(left);
    return Evaluate(state, argv[1]);
}

char* LessThanIntFn(const char* name, State* state, int argc, Expr* argv[]) {
    if (argc != 2) {
        free(state->errmsg);
        state->errmsg = strdup("less_than_int expects 2 arguments");
        return NULL;
    }

    char* left;
    char* right;
    if (ReadArgs(state, argv, 2, &left, &right) < 0) return NULL;

    bool result = false;
    char* end;

    long l_int = strtol(left, &end, 10);
    if (left[0] == '\0' || *end != '\0') {
        fprintf(stderr, "[%s] is not an int\n", left);
        goto done;
    }

    long r_int = strtol(right, &end, 10);
    if (right[0] == '\0' || *end != '\0') {
        fprintf(stderr, "[%s] is not an int\n", right);
        goto done;
    }

    result = l_int < r_int;

  done:
    free(left);
    free(right);
    return strdup(result ? "t" : "");
}

char* GreaterThanIntFn(const char* name, State* state, int argc, Expr* argv[]) {
    if (argc != 2) {
        free(state->errmsg);
        state->errmsg = strdup("greater_than_int expects 2 arguments");
        return NULL;
    }

    Expr* temp[2];
    temp[0] = argv[1];
    temp[1] = argv[0];

    return LessThanIntFn(name, state, 2, temp);
}

char* Literal(const char* name, State* state, int argc, Expr* argv[]) {
    return strdup(name);
}

Expr* Build(Function fn, YYLTYPE loc, int count, ...) {
    va_list v;
    va_start(v, count);
    Expr* e = malloc(sizeof(Expr));
    e->fn = fn;
    e->name = "(operator)";
    e->argc = count;
    e->argv = malloc(count * sizeof(Expr*));
    int i;
    for (i = 0; i < count; ++i) {
        e->argv[i] = va_arg(v, Expr*);
    }
    va_end(v);
    e->start = loc.start;
    e->end = loc.end;
    return e;
}

// -----------------------------------------------------------------
//   the function table
// -----------------------------------------------------------------

static int fn_entries = 0;
static int fn_size = 0;
NamedFunction* fn_table = NULL;

void RegisterFunction(const char* name, Function fn) {
    if (fn_entries >= fn_size) {
        fn_size = fn_size*2 + 1;
        fn_table = realloc(fn_table, fn_size * sizeof(NamedFunction));
    }
    fn_table[fn_entries].name = name;
    fn_table[fn_entries].fn = fn;
    ++fn_entries;
}

static int fn_entry_compare(const void* a, const void* b) {
    const char* na = ((const NamedFunction*)a)->name;
    const char* nb = ((const NamedFunction*)b)->name;
    return strcmp(na, nb);
}

void FinishRegistration() {
    qsort(fn_table, fn_entries, sizeof(NamedFunction), fn_entry_compare);
}

Function FindFunction(const char* name) {
    NamedFunction key;
    key.name = name;
    NamedFunction* nf = bsearch(&key, fn_table, fn_entries,
                                sizeof(NamedFunction), fn_entry_compare);
    if (nf == NULL) {
        return NULL;
    }
    return nf->fn;
}

void RegisterBuiltins() {
    RegisterFunction("ifelse", IfElseFn);
    RegisterFunction("abort", AbortFn);
    RegisterFunction("assert", AssertFn);
    RegisterFunction("concat", ConcatFn);
    RegisterFunction("is_substring", SubstringFn);
    RegisterFunction("stdout", StdoutFn);
    RegisterFunction("sleep", SleepFn);

    RegisterFunction("less_than_int", LessThanIntFn);
    RegisterFunction("greater_than_int", GreaterThanIntFn);
}


// -----------------------------------------------------------------
//   convenience methods for functions
// -----------------------------------------------------------------

// Evaluate the expressions in argv, giving 'count' char* (the ... is
// zero or more char** to put them in).  If any expression evaluates
// to NULL, free the rest and return -1.  Return 0 on success.
int ReadArgs(State* state, Expr* argv[], int count, ...) {
    char** args = malloc(count * sizeof(char*));
    va_list v;
    va_start(v, count);
    int i;
    for (i = 0; i < count; ++i) {
        args[i] = Evaluate(state, argv[i]);
        if (args[i] == NULL) {
            va_end(v);
            int j;
            for (j = 0; j < i; ++j) {
                free(args[j]);
            }
            free(args);
            return -1;
        }
        *(va_arg(v, char**)) = args[i];
    }
    va_end(v);
    free(args);
    return 0;
}

// Evaluate the expressions in argv, returning an array of char*
// results.  If any evaluate to NULL, free the rest and return NULL.
// The caller is responsible for freeing the returned array and the
// strings it contains.
char** ReadVarArgs(State* state, int argc, Expr* argv[]) {
    char** args = (char**)malloc(argc * sizeof(char*));
    int i = 0;
    for (i = 0; i < argc; ++i) {
        args[i] = Evaluate(state, argv[i]);
        if (args[i] == NULL) {
            int j;
            for (j = 0; j < i; ++j) {
                free(args[j]);
            }
            free(args);
            return NULL;
        }
    }
    return args;
}

// Use printf-style arguments to compose an error message to put into
// *state.  Returns NULL.
char* ErrorAbort(State* state, char* format, ...) {
    char* buffer = malloc(4096);
    va_list v;
    va_start(v, format);
    vsnprintf(buffer, 4096, format, v);
    va_end(v);
    free(state->errmsg);
    state->errmsg = buffer;
    return NULL;
}
