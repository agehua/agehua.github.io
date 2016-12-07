// memory layout for one layer virtual inheritance.  

#include <iostream>
#include <stdio.h>

using namespace std;

#define COUT_FUNC(a, b) (cout << a << " | " << b << " | ") 
#define COUT_CNUM(a, b) (cout << a << " | " << b) 
#define COUT_ARG3(a, b, c) (COUT_FUNC(a, b) << c)
#define COMMENT(a) printf("\e[32m%s\e[0m\n", (a));

class top{
    public:
        top(long i):_i(i){}
        top(){}
        virtual void a() {cout << "this is top::a()" << endl;}
        virtual void b() {cout << "this is top::b()" << endl;}
        long _i;
};

class middle1: virtual public top{
    public:
        middle1(long j):top(100),_j(j){}
        virtual void c() {cout << "this is middle1::c()" << endl;}
        virtual void b() {cout << "this is middle1::b()" << endl;}
        long _j;
};

typedef void (*func)(void);

int main(void) {
    long* pobj;
    func pfunc;

    cout << "---memory of middle1---" << endl;

    middle1 m1(101);
    pobj = (long*)&m1;

    pfunc = (func)((long*)*((long*)(pobj[0])));
    COUT_FUNC("middle1[0]:    vtab(0)", (long*)(pobj[0])); pfunc(); 
    COUT_CNUM("middle1[1]:         _j", (long)pobj[1]) << endl;

    pfunc = (func)((long*)*((long*)(pobj[2])));
    COUT_FUNC("    top[0]:    vtab(0)", (long*)(pobj[2])); pfunc();
    COUT_CNUM("    top[1]:         _i", (long)pobj[3]) << endl;

    cout << endl << "---vtable of middle1---" << endl;

    // class middle1
    COUT_ARG3("middle1[0]: offset(-3)", (long*)pobj[0] - 3, (long)*((long*)pobj[0] - 3)); COMMENT("\t\t\t# offset to vbase");
    COUT_ARG3("            offset(-2)", (long*)pobj[0] - 2, (long)*((long*)pobj[0] - 2)); COMMENT("\t\t\t# offset to begin");
    COUT_ARG3("            offset(-1)", (long*)pobj[0] - 1, (long*)*((long*)pobj[0] - 1)); COMMENT("\t\t# typeinfo for middle1");

    pfunc = (func)((long*)*((long*)(pobj[0])));
    COUT_FUNC("               vtab(0)", (long*)pobj[0]); pfunc();
    pfunc = (func)((long*)*((long*)(pobj[0]) + 1));
    COUT_FUNC("               vtab(1)", (long*)pobj[0] + 1); pfunc();
    COUT_ARG3("               vtab(2)", (long*)pobj[0] + 2, *((long*)(pobj[0])+2)); COMMENT("\t\t\t# next class offset to begin");

    // class top
    COUT_ARG3("    top[0]: offset(-3)", (long*)pobj[2] - 3, (long)*((long*)pobj[2] - 3)); COMMENT("\t\t\t# offset to vbase");
    COUT_ARG3("            offset(-2)", (long*)pobj[2] - 2, (long)*((long*)pobj[2] - 2)); COMMENT("\t\t\t# offset to begin");
    COUT_ARG3("            offset(-1)", (long*)pobj[2] - 1, (long*)*((long*)pobj[2] - 1)); COMMENT("\t\t# typeinfo for middle1");

    pfunc = (func)((long*)*((long*)(pobj[2])));
    COUT_FUNC("               vtab(0)", (long*)pobj[2]); pfunc();
    COUT_ARG3("               vtab(1)", (long*)(pobj[2]) + 1, (long*)*((long*)(pobj[2])+1)); COMMENT("\t\t# virtual thunk to middle1::b()");
    COUT_ARG3("               vtab(2)", (long*)(pobj[2]) + 2, *((long*)(pobj[2])+2)); COMMENT("\t\t\t# end of vtable");

    // VTT for middle1
    COUT_ARG3("      VTT:      vtt(0)", (long*)(pobj[2]) + 3, (long*)*((long*)(pobj[2])+3)); COMMENT("\t\t# for middle1");
    COUT_ARG3("                vtt(1)", (long*)(pobj[2]) + 4, (long*)*((long*)(pobj[2])+4)); COMMENT("\t\t# for top");
}
