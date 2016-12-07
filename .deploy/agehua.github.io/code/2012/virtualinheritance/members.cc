// 成员函数的调用

#include <iostream>

using namespace std;

class top{
    public:
        void printA(){cout << "this is in top::printA" << endl;}
        void printB(){cout << "this is in top::printB" << endl;}
        void printC(){cout << "this is in top::printC" << endl;}
};

class middle1: virtual public top{
    public:
        void printB(){cout << "this is in middle1::printB" << endl;}
        void printC(){cout << "this is in middle1::printC" << endl;}
};

class middle2: virtual public top{
    public:
        void printC(){cout << "this is in middle2::printC" << endl;}
};

class bottom: public middle1, public middle2{
    public:
        void printC(){cout << "this is in bottom::printC" << endl;}
};

int main(void){
    bottom bo;
    bo.printA();
    bo.printB();
    bo.printC();
}
