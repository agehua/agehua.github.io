// memory layout for diamond virtual inheritance

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
        middle1(long j):top(100),_j(j),_l(151){}
        virtual void c() {cout << "this is middle1::c()" << endl;}
        virtual void b() {cout << "this is middle1::b()" << endl;}
        long _j;
        long _l;
};

class middle2: virtual public top{
    public:
        middle2(long k, long m):_k(k),_m(m){}
        virtual void d() {cout << "this is middle2::d()" << endl;}
        long _k;
        long _m;
};

class bottom: virtual public middle1, virtual public middle2{
    public:
        bottom():top(5936l), middle1(5937l), middle2(5938l, 5939l), _n(10000){}
        virtual void b() {cout << "this is bottom::b()" << endl;}
        virtual void e() {cout << "this is bottom::e()" << endl;}
        long _n;
};

typedef void (*func)(void);

int main(void) {
    long* pobj;
    func pfunc;

    cout << "--- member of bottom---" << endl;
    bottom bo;
    pobj = (long*) &bo;

    cout << "bo._n: " << bo._n << endl;
    cout << "   _j: " << bo._j << endl;
    cout << "   _l: " << bo._l << endl;
    cout << "   _i: " << bo._i << endl;
    cout << "   _k: " << bo._k << endl;
    cout << "   _m: " << bo._m << endl;

    cout << endl << endl << "--- memory of bottom---" << endl;

    pfunc = (func)((long*)*((long*)(pobj[0])));
    COUT_FUNC(" bottom[0]:    vtab(0)", (long*)(pobj[0])); pfunc(); 
    COUT_CNUM(" bottom[1]:         _n", (long)pobj[1]) << endl;

    pfunc = (func)((long*)*((long*)(pobj[2])));
    COUT_FUNC("middle1[0]:    vtab(0)", (long*)(pobj[2])); pfunc(); 
    COUT_CNUM("middle1[1]:         _j", (long)pobj[3]) << endl;
    COUT_CNUM("middle1[2]:         _l", (long)pobj[4]) << endl;

    pfunc = (func)((long*)*((long*)(pobj[5])));
    COUT_FUNC("    top[0]:    vtab(0)", (long*)(pobj[5])); pfunc(); 
    COUT_CNUM("    top[1]:         _i", (long)pobj[6]) << endl;

    pfunc = (func)((long*)*((long*)(pobj[7])));
    COUT_FUNC("middle2[0]:    vtab(0)", (long*)(pobj[7])); pfunc(); 
    COUT_CNUM("middle2[1]:         _k", (long)pobj[8]) << endl;
    COUT_CNUM("middle2[2]:         _m", (long)pobj[9]) << endl;

    cout << endl << endl << "--- vtable of bottom---" << endl;

    // class bottom
    COUT_ARG3(" bottom[0]: offset(-3)", (long*)pobj[0] - 3, (long)*((long*)pobj[0] - 3)); COMMENT("\t\t\t# offset to vbase");
    COUT_ARG3("            offset(-2)", (long*)pobj[0] - 2, (long)*((long*)pobj[0] - 2)); COMMENT("\t\t\t# offset to begin");
    COUT_ARG3("            offset(-1)", (long*)pobj[0] - 1, (long*)*((long*)pobj[0] - 1)); COMMENT("\t\t# typeinfo for bottom");
    pfunc = (func)((long*)*((long*)(pobj[0])));
    COUT_FUNC("               vtab(0)", (long*)(pobj[0])); pfunc(); 
    pfunc = (func)((long*)*((long*)(pobj[0]) + 1));
    COUT_FUNC("               vtab(1)", (long*)pobj[0] + 1); pfunc();
    COUT_ARG3("               vtab(2)", (long*)pobj[0] + 2, *((long*)(pobj[0])+2)); COMMENT("\t\t\t# next class offset to begin");

    // class middle1
    COUT_ARG3("middle1[0]: offset(-3)", (long*)pobj[2] - 3, (long)*((long*)pobj[2] - 3)); COMMENT("\t\t\t# offset to vbase");
    COUT_ARG3("            offset(-2)", (long*)pobj[2] - 2, (long)*((long*)pobj[2] - 2)); COMMENT("\t\t\t# offset to begin");
    COUT_ARG3("            offset(-1)", (long*)pobj[2] - 1, (long*)*((long*)pobj[2] - 1)); COMMENT("\t\t# typeinfo for bottom");

    pfunc = (func)((long*)*((long*)(pobj[2])));
    COUT_FUNC("               vtab(0)", (long*)pobj[2]); pfunc();
    COUT_ARG3("               vtab(1)", (long*)(pobj[2]) + 1, (long*)*((long*)(pobj[2])+1)); COMMENT("\t\t# virtual thunk to bottom::b()");
    COUT_ARG3("               vtab(2)", (long*)pobj[2] + 2, *((long*)(pobj[2])+2)); COMMENT("\t\t\t# next class offset to begin");

    // class top 
    COUT_ARG3("    top[0]: offset(-3)", (long*)pobj[5] - 3, (long)*((long*)pobj[5] - 3)); COMMENT("\t\t\t# offset to vbase");
    COUT_ARG3("            offset(-2)", (long*)pobj[5] - 2, (long)*((long*)pobj[5] - 2)); COMMENT("\t\t\t# offset to begin");
    COUT_ARG3("            offset(-1)", (long*)pobj[5] - 1, (long*)*((long*)pobj[5] - 1)); COMMENT("\t\t# typeinfo for middle1");

    pfunc = (func)((long*)*((long*)(pobj[5])));
    COUT_FUNC("               vtab(0)", (long*)pobj[5]); pfunc();
    COUT_ARG3("               vtab(1)", (long*)(pobj[5]) + 1, (long*)*((long*)(pobj[5])+1)); COMMENT("\t\t# virtual thunk to bottom::b()");
    COUT_ARG3("               vtab(2)", (long*)(pobj[5]) + 2, *((long*)(pobj[5])+2)); COMMENT("\t\t\t# end of vtable");

    // class middle2
    COUT_ARG3("middle2[0]: offset(-3)", (long*)pobj[7] - 3, (long)*((long*)pobj[7] - 3)); COMMENT("\t\t\t# offset to vbase");
    COUT_ARG3("            offset(-2)", (long*)pobj[7] - 2, (long)*((long*)pobj[7] - 2)); COMMENT("\t\t\t# offset to begin");
    COUT_ARG3("            offset(-1)", (long*)pobj[7] - 1, (long*)*((long*)pobj[7] - 1)); COMMENT("\t\t# typeinfo for bottom");

    pfunc = (func)((long*)*((long*)(pobj[7])));
    COUT_FUNC("               vtab(0)", (long*)pobj[7]); pfunc();
    COUT_ARG3("               vtab(1)", (long*)pobj[7] + 1, *((long*)(pobj[7])+1)); COMMENT("\t\t\t# end of vtable");

    // vtt
    COUT_ARG3("      VTT:      vtt(0)", (long*)(pobj[7]) + 2, (long*)*((long*)(pobj[7])+2)); COMMENT("\t\t\t# address align");
    COUT_ARG3("                vtt(1)", (long*)(pobj[7]) + 3, (long*)*((long*)(pobj[7])+3)); COMMENT("\t\t\t# address align");
    COUT_ARG3("                vtt(2)", (long*)(pobj[7]) + 4, (long*)*((long*)(pobj[7])+4)); COMMENT("\t\t# for bottom");
    COUT_ARG3("                vtt(3)", (long*)(pobj[7]) + 5, (long*)*((long*)(pobj[7])+5)); COMMENT("\t\t# for middle1");
    COUT_ARG3("                vtt(4)", (long*)(pobj[7]) + 6, (long*)*((long*)(pobj[7])+6)); COMMENT("\t\t# for top");
    COUT_ARG3("                vtt(5)", (long*)(pobj[7]) + 7, (long*)*((long*)(pobj[7])+7)); COMMENT("\t\t# for middle2");
}
