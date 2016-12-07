#!/bin/bash

echo "### compile Sample1.java"
javac Sample1.java

echo "### generate jni headerfile Sample1.h"
javah -classpath ./ -jni Sample1

echo "### compile cpp Sample1.so"
g++ -I /usr/lib/jvm/java-1.6.0-openjdk/include/ -I /usr/lib/jvm/java-1.6.0-openjdk/include/linux/ Sample1.cpp -fPIC -shared -o libSample1.so

export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH

echo "### run cpp"
echo "---------------"
java Sample1
echo "--------------- end"

rm libSample1.so 


echo "### compile c Sample1.so"
gcc -I /usr/lib/jvm/java-1.6.0-openjdk/include/ -I /usr/lib/jvm/java-1.6.0-openjdk/include/linux/ Sample1.c -fPIC -shared -o libSample1.so

echo "### run c"
echo "---------------"
java Sample1

#rm libSample1.so Sample1.class Sample1.h

