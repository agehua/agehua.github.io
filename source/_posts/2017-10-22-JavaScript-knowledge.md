---
layout: post
title:  JavaScript知识点积累
category: accumulation
tags:
    - JavaScript
    - Basic Knowledge
keywords: JavaScript
banner: http://obxk8w81b.bkt.clouddn.com/Bridge%20and%20Houses%20on%20the%20Corner%20of%20Herengracht-Prinsessegracht,%20The%20Hague.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Bridge%20and%20Houses%20on%20the%20Corner%20of%20Herengracht-Prinsessegracht,%20The%20Hague.jpg
toc: true
---

## JavaScript积累

> 'Java and JavaScript are similar like Car and Carpet are similar' - Greg Hewgill

本文大部分内容来自: [廖雪峰--JavaScript教程](https://www.liaoxuefeng.com/wiki/001434446689867b27157e896e74d51a89c25cc8b43bdb3000)

### for ... of循环介绍
ES6引入了for ... of循环遍历集合，你可能会有疑问，for ... of循环和for ... in循环有何区别？

for ... in循环由于历史遗留问题，它遍历的实际上是对象的属性名称。一个Array数组实际上也是一个对象，它的每个元素的索引被视为一个属性。

<!--more-->
当我们手动给Array对象添加了额外的属性后，for ... in循环将带来意想不到的意外效果：
~~~ JavaScript
var a = ['A', 'B', 'C'];
a.name = 'Hello';
for (var x in a) {
    console.log(x); // '0', '1', '2', 'name'
}
~~~
for ... in循环将把name包括在内，但Array的length属性却不包括在内。

for ... of循环则完全修复了这些问题，它只循环集合本身的元素：
~~~ JavaScript
var a = ['A', 'B', 'C'];
a.name = 'Hello';
for (var x of a) {
    console.log(x); // 'A', 'B', 'C'
}
~~~
这就是为什么要引入新的for ... of循环。

然而，更好的方式是直接使用iterable内置的forEach方法，它接收一个函数，每次迭代就自动回调该函数。以Array为例：

~~~ JavaScript
'use strict';
var a = ['A', 'B', 'C'];
a.forEach(function (element, index, array) {
    // element: 指向当前元素的值
    // index: 指向当前索引
    // array: 指向Array对象本身
    console.log(element + ', index = ' + index);
});
~~~
> 注意，forEach()方法是ES5.1标准引入的，你需要测试浏览器是否支持。

Set与Array类似，但Set没有索引，因此回调函数的前两个参数都是元素本身：
~~~ JavaScript
var s = new Set(['A', 'B', 'C']);
s.forEach(function (element, sameElement, set) {
    console.log(element);
});
~~~
Map的回调函数参数依次为value、key和map本身：
~~~ JavaScript
var m = new Map([[1, 'x'], [2, 'y'], [3, 'z']]);
m.forEach(function (value, key, map) {
    console.log(value);
});
~~~
如果对某些参数不感兴趣，由于JavaScript的函数调用不要求参数必须一致，因此可以忽略它们。例如，只需要获得Array的element：
~~~ JavaScript
var a = ['A', 'B', 'C'];
a.forEach(function (element) {
    console.log(element);
});
~~~

### rest参数
JavaScript还有一个免费赠送的关键字arguments，它只在函数内部起作用，并且永远指向当前函数的调用者传入的所有参数。arguments类似Array但它不是一个Array.
由于JavaScript函数允许接收任意个参数，于是我们就不得不用arguments来获取所有参数：
~~~ JavaScript
function foo(a, b) {
    var i, rest = [];
    if (arguments.length > 2) {
        for (i = 2; i<arguments.length; i++) {
            rest.push(arguments[i]);
        }
    }
    console.log('a = ' + a);
    console.log('b = ' + b);
    console.log(rest);
}
~~~

为了获取除了已定义参数a、b之外的参数，我们不得不用arguments，并且循环要从索引2开始以便排除前两个参数，这种写法很别扭，只是为了获得额外的rest参数，有没有更好的方法？

ES6标准引入了rest参数，上面的函数可以改写为：
~~~ JavaScript
function foo(a, b, ...rest) {
    console.log('a = ' + a);
    console.log('b = ' + b);
    console.log(rest);
}

foo(1, 2, 3, 4, 5);
// 结果:
// a = 1
// b = 2
// Array [ 3, 4, 5 ]

foo(1);
// 结果:
// a = 1
// b = undefined
// Array []
~~~
rest参数只能写在最后，前面用...标识，从运行结果可知，传入的参数先绑定a、b，多余的参数以数组形式交给变量rest，所以，不再需要arguments我们就获取了全部参数。

如果传入的参数连正常定义的参数都没填满，也不要紧，rest参数会接收一个空数组（注意不是undefined）。

### 全局作用域
不在任何函数内定义的变量就具有全局作用域。实际上，JavaScript默认有一个全局对象window，全局作用域的变量实际上被绑定到window的一个属性

你可能猜到了，由于函数定义有两种方式，以变量方式var foo = function () {}定义的函数实际上也是一个全局变量，因此，顶层函数的定义也被视为一个全局变量，并绑定到window对象：
~~~ JavaScript
'use strict';

function foo() {
    alert('foo');
}

foo(); // 直接调用foo()
window.foo(); // 通过window.foo()调用
~~~

### 名字空间

全局变量会绑定到window上，不同的JavaScript文件如果使用了相同的全局变量，或者定义了相同名字的顶层函数，都会造成命名冲突，并且很难被发现。

减少冲突的一个方法是把自己的所有变量和函数全部绑定到一个全局变量中。例如：
~~~ JavaScript
// 唯一的全局变量MYAPP:
var MYAPP = {};

// 其他变量:
MYAPP.name = 'myapp';
MYAPP.version = 1.0;

// 其他函数:
MYAPP.foo = function () {
    return 'foo';
};
~~~
把自己的代码全部放入唯一的名字空间MYAPP中，会大大减少全局变量冲突的可能。

许多著名的JavaScript库都是这么干的：jQuery，YUI，underscore等等。
### 局部作用域

由于JavaScript的变量作用域实际上是函数内部，我们在for循环等语句块中是无法定义具有局部作用域的变量的：
~~~ JavaScript
'use strict';

function foo() {
    for (var i=0; i<100; i++) {
        //
    }
    i += 100; // 仍然可以引用变量i
}
~~~
为了解决块级作用域，ES6引入了新的关键字let，用let替代var可以申明一个块级作用域的变量：
~~~ JavaScript
'use strict';

function foo() {
    var sum = 0;
    for (let i=0; i<100; i++) {
        sum += i;
    }
    // SyntaxError:
    i += 1;
}
~~~
ES6标准引入了新的关键字const来定义常量，const与let都具有块级作用域.

### 解构赋值

从ES6开始，JavaScript引入了**解构赋值**，可以同时对一组变量进行赋值
如果数组本身还有嵌套，也可以通过下面的形式进行解构赋值，注意嵌套层次和位置要保持一致：
~~~ JavaScript
let [x, [y, z]] = ['hello', ['JavaScript', 'ES6']];
x; // 'hello'
y; // 'JavaScript'
z; // 'ES6'
~~~
解构赋值还可以忽略某些元素：
~~~ JavaScript
let [, , z] = ['hello', 'JavaScript', 'ES6']; // 忽略前两个元素，只对z赋值第三个元素
z; // 'ES6'
~~~
对一个对象进行解构赋值时，同样可以直接对嵌套的对象属性进行赋值，只要保证对应的层次是一致的：
~~~ JavaScript
var person = {
    name: '小明',
    age: 20,
    gender: 'male',
    passport: 'G-12345678',
    school: 'No.4 middle school',
    address: {
        city: 'Beijing',
        street: 'No.1 Road',
        zipcode: '100001'
    }
};
var {name, address: {city, zip}} = person;
name; // '小明'
city; // 'Beijing'
zip; // undefined, 因为属性名是zipcode而不是zip
// 注意: address不是变量，而是为了让city和zip获得嵌套的address对象的属性:
address; // Uncaught ReferenceError: address is not defined
~~~
使用解构赋值对对象属性进行赋值时，如果对应的属性不存在，变量将被赋值为undefined，这和引用一个不存在的属性获得undefined是一致的。如果要使用的变量名和属性名不一致，可以用下面的语法获取：
~~~ JavaScript
var person = {
    name: '小明',
    age: 20,
    gender: 'male',
    passport: 'G-12345678',
    school: 'No.4 middle school'
};

// 把passport属性赋值给变量id:
let {name, passport:id} = person;
name; // '小明'
id; // 'G-12345678'
// 注意: passport不是变量，而是为了让变量id获得passport属性:
passport; // Uncaught ReferenceError: passport is not defined
~~~
解构赋值还可以使用默认值，这样就避免了不存在的属性返回undefined的问题：
~~~ JavaScript
var person = {
    name: '小明',
    age: 20,
    gender: 'male',
    passport: 'G-12345678'
};
~~~
// 如果person对象没有single属性，默认赋值为true:
~~~ JavaScript
var {name, single=true} = person;
name; // '小明'
single; // true
~~~
有些时候，如果变量已经被声明了，再次赋值的时候，正确的写法也会报语法错误：
~~~ JavaScript
// 声明变量:
var x, y;
// 解构赋值:
{x, y} = { name: '小明', x: 100, y: 200};
// 语法错误: Uncaught SyntaxError: Unexpected token =
~~~
这是因为JavaScript引擎把{开头的语句当作了块处理，于是=不再合法。解决方法是用小括号括起来：
~~~ JavaScript
({x, y} = { name: '小明', x: 100, y: 200});
~~~
**使用场景**

解构赋值在很多时候可以大大简化代码。例如，交换两个变量x和y的值，可以这么写，不再需要临时变量：
~~~ JavaScript
var x=1, y=2;
[x, y] = [y, x]
~~~
快速获取当前页面的域名和路径：
~~~ JavaScript
var {hostname:domain, pathname:path} = location;
~~~
如果一个函数接收一个对象作为参数，那么，可以使用解构直接把对象的属性绑定到变量中。例如，下面的函数可以快速创建一个Date对象：
~~~ JavaScript
function buildDate({year, month, day, hour=0, minute=0, second=0}) {
    return new Date(year + '-' + month + '-' + day + ' ' + hour + ':' + minute + ':' + second);
}
~~~
它的方便之处在于传入的对象只需要year、month和day这三个属性：
~~~ JavaScript
buildDate({ year: 2017, month: 1, day: 1 });
// Sun Jan 01 2017 00:00:00 GMT+0800 (CST)
~~~
也可以传入hour、minute和second属性：
~~~ JavaScript
buildDate({ year: 2017, month: 1, day: 1, hour: 20, minute: 15 });
// Sun Jan 01 2017 20:15:00 GMT+0800 (CST)
~~~
使用解构赋值可以减少代码量，但是，需要在支持ES6解构赋值特性的现代浏览器中才能正常运行。目前支持解构赋值的浏览器包括Chrome，Firefox，Edge等。

### 方法this、apply和call
在一个方法内部，this是一个特殊变量，它始终指向当前对象
~~~ JavaScript
'use strict';

var xiaoming = {
    name: '小明',
    birth: 1990,
    age: function () {
        var y = new Date().getFullYear();
        return y - this.birth;
    }
};

var fn = xiaoming.age;
fn(); // Uncaught TypeError: Cannot read property 'birth' of undefined
~~~
this指针只在age方法的函数内指向xiaoming，在函数内部定义的函数，this又指向undefined了！（在非strict模式下，它重新指向全局对象window！）

~~~ JavaScript
'use strict';

var xiaoming = {
    name: '小明',
    birth: 1990,
    age: function () {
        var that = this; // 在方法内部一开始就捕获this
        function getAgeFromBirth() {
            var y = new Date().getFullYear();
            return y - that.birth; // 用that而不是this
        }
        return getAgeFromBirth();
    }
};

xiaoming.age(); // 25
~~~
用var that = this;，你就可以放心地在方法内部定义其他函数，而不是把所有语句都堆到一个方法中。

虽然在一个独立的函数调用中，根据是否是strict模式，this指向undefined或window，不过，我们还是可以控制this的指向的！

要指定函数的this指向哪个对象，可以用**函数**本身的apply方法，它接收两个参数，第一个参数就是需要绑定的this变量，第二个参数是Array，表示函数本身的参数。

用apply修复getAge()调用：
~~~ JavaScript
function getAge() {
    var y = new Date().getFullYear();
    return y - this.birth;
}

var xiaoming = {
    name: '小明',
    birth: 1990,
    age: getAge
};

xiaoming.age(); // 25
getAge.apply(xiaoming, []); // 25, this指向xiaoming, 参数为空
~~~
另一个与apply()类似的方法是call()，唯一区别是：

- apply()把参数打包成Array再传入；
- call()把参数按顺序传入。

比如调用Math.max(3, 5, 4)，分别用apply()和call()实现如下：
~~~ JavaScript
Math.max.apply(null, [3, 5, 4]); // 5
Math.max.call(null, 3, 5, 4); // 5
~~~
对普通函数调用，我们通常把this绑定为null。

### 装饰器

利用apply()，我们还可以动态改变函数的行为。

JavaScript的所有对象都是动态的，即使内置的函数，我们也可以重新指向新的函数。

现在假定我们想统计一下代码一共调用了多少次parseInt()，可以把所有的调用都找出来，然后手动加上count += 1，不过这样做太傻了。最佳方案是用我们自己的函数替换掉默认的parseInt()：
~~~ JavaScript
'use strict';

var count = 0;
var oldParseInt = parseInt; // 保存原函数

window.parseInt = function () {
    count += 1;
    return oldParseInt.apply(null, arguments); // 调用原函数
};
~~~

### 高阶函数
JavaScript的函数其实都指向某个变量。既然变量可以指向函数，函数的参数能接收变量，那么一个函数就可以接收另一个函数作为参数，这种函数就称之为高阶函数。

**map**
map()方法定义在JavaScript的Array中，我们调用Array的map()方法，传入我们自己的函数，就得到了一个新的Array作为结果
~~~ JavaScript
'use strict';

function pow(x) {
    return x * x;
}
var arr = [1, 2, 3, 4, 5, 6, 7, 8, 9];
var results = arr.map(pow); // [1, 4, 9, 16, 25, 36, 49, 64, 81]
console.log(results);
~~~
> 注意：map()传入的参数是pow，即函数对象本身。

把Array的所有数字转为字符串：
~~~ JavaScript
var arr = [1, 2, 3, 4, 5, 6, 7, 8, 9];
arr.map(String); // ['1', '2', '3', '4', '5', '6', '7', '8', '9']
~~~
只需要一行代码。

**reduce**
Array的reduce()把一个函数作用在这个Array的[x1, x2, x3...]上，这个函数必须接收两个参数，reduce()把结果继续和序列的下一个元素做累积计算，其效果就是：
~~~ JavaScript
[x1, x2, x3, x4].reduce(f) = f(f(f(x1, x2), x3), x4)
~~~

要把[1, 3, 5, 7, 9]变换成整数13579，reduce()也能派上用场：
~~~ JavaScript
var arr = [1, 3, 5, 7, 9];
arr.reduce(function (x, y) {
    return x * 10 + y;
}); // 13579
~~~
**filter**
filter也是一个常用的操作，它用于把Array的某些元素过滤掉，然后返回剩下的元素。

和map()类似，Array的filter()也接收一个函数。和map()不同的是，filter()把传入的函数依次作用于每个元素，然后根据返回值是true还是false决定保留还是丢弃该元素。

把一个Array中的空字符串删掉，可以这么写：
~~~ JavaScript
var arr = ['A', '', 'B', null, undefined, 'C', '  '];
var r = arr.filter(function (s) {
    return s && s.trim(); // 注意：IE9以下的版本没有trim()方法
});
r; // ['A', 'B', 'C']
~~~

filter()接收的回调函数，其实可以有多个参数。通常我们仅使用第一个参数，表示Array的某个元素。回调函数还可以接收另外两个参数，表示元素的位置和数组本身：
~~~ JavaScript
var arr = ['A', 'B', 'C'];
var r = arr.filter(function (element, index, self) {
    console.log(element); // 依次打印'A', 'B', 'C'
    console.log(index); // 依次打印0, 1, 2
    console.log(self); // self就是变量arr
    return true;
});
~~~
利用filter，可以巧妙地去除Array的重复元素：
~~~ JavaScript
'use strict';

var
    r,
    arr = ['apple', 'strawberry', 'banana', 'pear', 'apple', 'orange', 'orange', 'strawberry'];

r = arr.filter(function (element, index, self) {
    return self.indexOf(element) === index;
});
console.log(r.toString());
~~~
> 去除重复元素依靠的是indexOf总是返回第一个元素的位置，后续的重复元素位置与indexOf返回的位置不相等，因此被filter滤掉了。

**sort()**
sort()方法会直接对Array进行修改，它返回的结果仍是当前Array：
~~~ JavaScript
var a1 = ['B', 'A', 'C'];
var a2 = a1.sort();
a1; // ['A', 'B', 'C']
a2; // ['A', 'B', 'C']
a1 === a2; // true, a1和a2是同一对象
~~~

### 闭包
高阶函数除了可以接受函数作为参数外，还可以把函数作为结果值返回。

返回闭包时牢记的一点就是：**返回函数不要引用任何循环变量，或者后续会发生变化的变量**
如果一定要引用循环变量怎么办？方法是再创建一个函数，用该函数的参数绑定循环变量当前的值，无论该循环变量后续如何更改，已绑定到函数参数的值不变：
~~~ JavaScript
function count() {
    var arr = [];
    for (var i=1; i<=3; i++) {
        arr.push((function (n) {
            return function () {
                return n * n;
            }
        })(i));
    }
    return arr;
}

var results = count();
var f1 = results[0];
var f2 = results[1];
var f3 = results[2];

f1(); // 1
f2(); // 4
f3(); // 9
~~~

注意这里用了一个“创建一个匿名函数并立刻执行”的语法：
~~~ JavaScript
(function (x) {
    return x * x;
})(3); // 9
~~~
理论上讲，创建一个匿名函数并立刻执行可以这么写：
~~~ JavaScript
function (x) { return x * x } (3);
~~~
但是由于JavaScript语法解析的问题，会报SyntaxError错误，因此需要用括号把整个函数定义括起来：
~~~ JavaScript
(function (x) { return x * x }) (3);
~~~
通常，一个立即执行的匿名函数可以把函数体拆开，一般这么写：
~~~ JavaScript
(function (x) {
    return x * x;
})(3)
~~~

闭包有非常强大的功能。举个栗子：

在面向对象的程序设计语言里，比如Java和C++，要在对象内部封装一个私有变量，可以用private修饰一个成员变量。

在没有class机制，只有函数的语言里，借助闭包，同样可以封装一个私有变量。我们用JavaScript创建一个计数器：
~~~ JavaScript
'use strict';

function create_counter(initial) {
    var x = initial || 0;
    return {
        inc: function () {
            x += 1;
            return x;
        }
    }
}
~~~
它用起来像这样：
~~~ JavaScript
var c1 = create_counter();
c1.inc(); // 1
c1.inc(); // 2
c1.inc(); // 3

var c2 = create_counter(10);
c2.inc(); // 11
c2.inc(); // 12
c2.inc(); // 13
~~~
在返回的对象中，实现了一个闭包，该闭包携带了局部变量x，并且，从外部代码根本无法访问到变量x。换句话说，闭包就是携带状态的函数，并且它的状态可以完全对外隐藏起来。

闭包还可以把多参数的函数变成单参数的函数。例如，要计算xy可以用Math.pow(x, y)函数，不过考虑到经常计算x2或x3，我们可以利用闭包创建新的函数pow2和pow3：
~~~ JavaScript
'use strict';

function make_pow(n) {
    return function (x) {
        return Math.pow(x, n);
    }
}

// 创建两个新函数:
var pow2 = make_pow(2);
var pow3 = make_pow(3);

console.log(pow2(5)); // 25
console.log(pow3(7)); // 343
~~~
**使用闭包的注意点：**
- 1）由于闭包会使得函数中的变量都被保存在内存中，内存消耗很大，所以不能滥用闭包，否则会造成网页的性能问题，在IE中可能导致内存泄露。解决方法是，在退出函数之前，将不使用的局部变量全部删除。
- 2）闭包会在父函数外部，改变父函数内部变量的值。所以，如果你把父函数当作对象（object）使用，把闭包当作它的公用方法（Public Method），把内部变量当作它的私有属性（private value），这时一定要小心，不要随便改变父函数内部变量的值。

### 闭包的思考
关于闭包的总结也可以再看下这篇文章：[学习Javascript闭包（Closure）](http://www.ruanyifeng.com/blog/2009/08/learning_javascript_closures.html)

上面提到的这篇文章的最后，列出了一道思考题：
如果你能理解下面两段代码的运行结果，应该就算理解闭包的运行机制了。
代码片段一。
~~~ JavaScript
var name = "The Window";
var object = {
　　name : "My Object",
　　getNameFunc : function(){
　　　　return function(){
　　　　　　return this.name;
　　　　};
　　}
};
alert(object.getNameFunc()());
~~~
代码片段二。
~~~ JavaScript
var name = "The Window";
var object = {
　　name : "My Object",
　　getNameFunc : function(){
　　　　var that = this;
　　　　return function(){
　　　　　　return that.name;
　　　　};
　　}
};
alert(object.getNameFunc()());
~~~


我先说结果，代码段一的结果是：The Window；而代码段二的结果是：My Object。

关于this关键字，前面已经介绍过了，它代表的实例会根据环境不同而变化的. 
> In JavaScript this always refers to the “owner” of the function we're executing, or rather, to the object that a function is a method of. 

上面这段话，摘自：[The this keyword](http://www.quirksmode.org/js/this.html)。

我理解的意思是，**this永远指向我们要执行的方法的“owner”，或者更确切的说，是指向function作为某个object的一个方法（method）的那个object**。

在代码片段一里，object.getNameFunc()返回的是function，假设是f，那么:
**alert(object.getNameFunc()());  等价于===》 alert(f());**
f()的执行已经和object没有关系了，this这里应该指向的是window对象，所以name值应该是："The Window"。

在代码片段二里，在object.getNameFunc()执行的时候，this代表object对象，**用that变量（that不是关键字）**保存了起来，而最里面的这个function是可以访问that变量的，那么
**alert(object.getNameFunc()());  应该等价于===》 alert(f(that));**
所以name的值应该是：“My Object”。

#### 箭头函数
ES6标准新增了一种新的函数：Arrow Function（箭头函数）。
箭头函数有两种格式，一种像上面的，只包含一个表达式，连{ ... }和return都省略掉了。还有一种可以包含多条语句，这时候就不能省略{ ... }和return：。如果参数不是一个，就需要用括号()括起来：
~~~ JavaScript
// 两个参数:
(x, y) => x * x + y * y

// 无参数:
() => 3.14

// 可变参数:
(x, y, ...rest) => {
    var i, sum = x + y;
    for (i=0; i<rest.length; i++) {
        sum += rest[i];
    }
    return sum;
}
~~~
如果要返回一个对象，就要注意，如果是单表达式，这么写的话会报错：
~~~ JavaScript
// SyntaxError:
x => { foo: x }
~~~
因为和函数体的{ ... }有语法冲突，所以要改为：
~~~ JavaScript
// ok:
x => ({ foo: x })
~~~

**this**

箭头函数看上去是匿名函数的一种简写，但实际上，箭头函数和匿名函数有个明显的区别：**箭头函数内部的this是词法作用域，由上下文确定**。

回顾前面的例子，由于JavaScript函数对this绑定的错误处理，下面的例子无法得到预期结果：
~~~ JavaScript
var obj = {
    birth: 1990,
    getAge: function () {
        var b = this.birth; // 1990
        var fn = function () {
            return new Date().getFullYear() - this.birth; // this指向window或undefined
        };
        return fn();
    }
};
~~~
现在，箭头函数完全修复了this的指向，this总是指向词法作用域，也就是外层调用者obj：
~~~ JavaScript
var obj = {
    birth: 1990,
    getAge: function () {
        var b = this.birth; // 1990
        var fn = () => new Date().getFullYear() - this.birth; // this指向obj对象
        return fn();
    }
};
obj.getAge(); // 25
~~~
如果使用箭头函数，以前的那种hack写法：
~~~ JavaScript
var that = this;
~~~
就不再需要了。
由于this在箭头函数中已经按照词法作用域绑定了，所以，用call()或者apply()调用箭头函数时，无法对this进行绑定，即传入的第一个参数被忽略：
~~~ JavaScript
var obj = {
    birth: 1990,
    getAge: function (year) {
        var b = this.birth; // 1990
        var fn = (y) => y - this.birth; // this.birth仍是1990
        return fn.call({birth:2000}, year);
    }
};
obj.getAge(2015); // 25
~~~

关于箭头函数中this的解释，可以看这篇文章：[箭头函数中this的用法](https://github.com/zhengweikeng/blog/blob/master/posts/2016/%E7%AE%AD%E5%A4%B4%E5%87%BD%E6%95%B0%E4%B8%ADthis%E7%9A%84%E7%94%A8%E6%B3%95.md)
和[深入浅出ES6（七）：箭头函数 Arrow Functions](http://www.infoq.com/cn/articles/es6-in-depth-arrow-functions)这篇

### 标准对象

总结一下，有这么几条规则需要遵守：

- 不要使用new Number()、new Boolean()、new String()创建包装对象；
- 用parseInt()或parseFloat()来转换任意类型到number；
- 用String()来转换任意类型到string，或者直接调用某个对象的toString()方法；
- 通常不必把任意类型转换为boolean再判断，因为可以直接写if (myVar) {...}；
- typeof操作符可以判断出number、boolean、string、function和undefined；
- 判断Array要使用Array.isArray(arr)；
- 判断null请使用myVar === null；
- 判断某个全局变量是否存在用typeof window.myVar === 'undefined'；
- 函数内部判断某个变量是否存在用typeof myVar === 'undefined'。

任何对象都有toString()方法吗？null和undefined就没有！确实如此，这两个特殊值要除外，虽然null还伪装成了object类型。

更细心的同学指出，number对象调用toString()报SyntaxError：
~~~ JavaScript
123.toString(); // SyntaxError
~~~
遇到这种情况，要特殊处理一下：
~~~ JavaScript
123..toString(); // '123', 注意是两个点！
(123).toString(); // '123'
~~~

**Date对象**

如果要创建一个指定日期和时间的Date对象，可以用：
~~~ JavaScript
var d = new Date(2015, 5, 19, 20, 15, 30, 123);
~~~
> JavaScript的Date对象月份值从0开始，牢记0=1月，1=2月，2=3月，……，11=12月。

~~~ JavaScript
var d = Date.parse('2015-06-24T19:49:22.875+08:00');
d; // 1435146562875
~~~
~~~ JavaScript
var d = new Date(1435146562875);
d; // Wed Jun 24 2015 19:49:22 GMT+0800 (CST)
d.getMonth(); // 5
~~~
> 使用Date.parse()时传入的字符串使用实际月份01~12，转换为Date对象后getMonth()获取的月份值为0~11。

要获取当前时间戳，可以用：
~~~ JavaScript
'use strict';

if (Date.now) {
    console.log(Date.now()); // 老版本IE没有now()方法
} else {
    console.log(new Date().getTime());
}
~~~

**RegExp**，

正则表达式的内容参考：[标准对象-RegExp](https://www.liaoxuefeng.com/wiki/001434446689867b27157e896e74d51a89c25cc8b43bdb3000/001434499503920bb7b42ff6627420da2ceae4babf6c4f2000)


### 构造函数
除了直接用{ ... }创建一个对象外，JavaScript还可以用一种构造函数的方法来创建对象。它的用法是，先定义一个构造函数：
~~~ JavaScript
function Student(name) {
    this.name = name;
    this.hello = function () {
        alert('Hello, ' + this.name + '!');
    }
}
~~~
你会问，咦，这不是一个普通函数吗？

这确实是一个普通函数，但是在JavaScript中，可以用关键字new来调用这个函数，并返回一个对象：
~~~ JavaScript
var xiaoming = new Student('小明');
xiaoming.name; // '小明'
xiaoming.hello(); // Hello, 小明!
~~~
> 注意，如果不写new，这就是一个普通函数，它返回undefined。但是，如果写了new，它就变成了一个构造函数，它绑定的this指向新创建的对象，并默认返回this，也就是说，不需要在最后写return this;。

新创建的xiaoming的原型链是：
~~~ JavaScript
xiaoming ----> Student.prototype ----> Object.prototype ----> null
~~~

xiaoming的原型指向函数Student的原型。如果你又创建了xiaohong、xiaojun，那么这些对象的原型与xiaoming是一样的：
~~~ JavaScript
xiaoming ↘
xiaohong -→ Student.prototype ----> Object.prototype ----> null
xiaojun  ↗
~~~

用new Student()创建的对象还从原型上获得了一个constructor属性，它指向函数Student本身：
~~~ JavaScript
xiaoming.constructor === Student.prototype.constructor; // true
Student.prototype.constructor === Student; // true
Object.getPrototypeOf(xiaoming) === Student.prototype; // true
xiaoming instanceof Student; // true
~~~
看晕了吧？用一张图来表示这些乱七八糟的关系就是：

![](/images/blogimages/2017/js/js_function_construction.png)

不过还有一个小问题，注意观察：
~~~ JavaScript
xiaoming.name; // '小明'
xiaohong.name; // '小红'
xiaoming.hello; // function: Student.hello()
xiaohong.hello; // function: Student.hello()
xiaoming.hello === xiaohong.hello; // false
~~~
xiaoming和xiaohong各自的name不同，这是对的，否则我们无法区分谁是谁了。

xiaoming和xiaohong各自的hello是一个函数，但它们是两个不同的函数，虽然函数名称和代码都是相同的！

如果我们通过new Student()创建了很多对象，这些对象的hello函数实际上只需要共享同一个函数就可以了，这样可以节省很多内存。

要让创建的对象共享一个hello函数，根据对象的属性查找原则，我们只要把hello函数移动到xiaoming、xiaohong这些对象共同的原型上就可以了，也就是Student.prototype：

![](/images/blogimages/2017/js/js_function_construction_1.png)

修改代码如下：
~~~ JavaScript
function Student(name) {
    this.name = name;
}

Student.prototype.hello = function () {
    alert('Hello, ' + this.name + '!');
};
~~~

**忘记写new怎么办**

如果一个函数被定义为用于创建对象的构造函数，但是调用时忘记了写new怎么办？

在strict模式下，this.name = name将报错，因为this绑定为undefined，在非strict模式下，this.name = name不报错，因为this绑定为window，于是无意间创建了全局变量name，并且返回undefined，这个结果更糟糕。

所以，调用构造函数千万不要忘记写new。为了区分普通函数和构造函数，按照约定，构造函数首字母应当大写，而普通函数首字母应当小写，这样，一些语法检查工具如jslint将可以帮你检测到漏写的new。

最后，我们还可以编写一个createStudent()函数，在内部封装所有的new操作。一个常用的编程模式像这样：
~~~ JavaScript
function Student(props) {
    this.name = props.name || '匿名'; // 默认值为'匿名'
    this.grade = props.grade || 1; // 默认值为1
}

Student.prototype.hello = function () {
    alert('Hello, ' + this.name + '!');
};

function createStudent(props) {
    return new Student(props || {})
}
~~~
这个createStudent()函数有几个巨大的优点：一是不需要new来调用，二是参数非常灵活，可以不传，也可以这么传：
~~~ JavaScript
var xiaoming = createStudent({
    name: '小明'
});

xiaoming.grade; // 1
~~~
如果创建的对象有很多属性，我们只需要传递需要的某些属性，剩下的属性可以用默认值。由于参数是一个Object，我们无需记忆参数的顺序。如果恰好从JSON拿到了一个对象，就可以直接创建出xiaoming。

**原型继承**
在传统的基于Class的语言如Java、C++中，继承的本质是扩展一个已有的Class，并生成新的Subclass。
JavaScript由于采用原型继承，我们无法直接扩展一个Class，因为根本不存在Class这种类型。但是办法还是有的。

我们还是用上面的Student函数，现在，我们要基于Student扩展出PrimaryStudent，可以先定义出PrimaryStudent：
~~~ JavaScript
function PrimaryStudent(props) {
    // 调用Student构造函数，绑定this变量:
    Student.call(this, props);
    this.grade = props.grade || 1;
}
~~~
但是，调用了Student构造函数不等于继承了Student，PrimaryStudent创建的对象的原型是：
~~~ JavaScript
new PrimaryStudent() ----> PrimaryStudent.prototype ----> Object.prototype ----> null
~~~
必须想办法把原型链修改为：
~~~ JavaScript
new PrimaryStudent() ----> PrimaryStudent.prototype ----> Student.prototype ----> Object.prototype ----> null
~~~

我们必须借助一个中间对象来实现正确的原型链，这个中间对象的原型要指向Student.prototype。为了实现这一点，参考道爷（就是发明JSON的那个道格拉斯）的代码，中间对象可以用一个空函数F来实现：
~~~ JavaScript
// PrimaryStudent构造函数:
function PrimaryStudent(props) {
    Student.call(this, props);
    this.grade = props.grade || 1;
}

// 空函数F:
function F() {
}

// 把F的原型指向Student.prototype:
F.prototype = Student.prototype;

// 把PrimaryStudent的原型指向一个新的F对象，F对象的原型正好指向Student.prototype:
PrimaryStudent.prototype = new F();

// 把PrimaryStudent原型的构造函数修复为PrimaryStudent:
PrimaryStudent.prototype.constructor = PrimaryStudent;

// 继续在PrimaryStudent原型（就是new F()对象）上定义方法：
PrimaryStudent.prototype.getGrade = function () {
    return this.grade;
};

// 创建xiaoming:
var xiaoming = new PrimaryStudent({
    name: '小明',
    grade: 2
});
xiaoming.name; // '小明'
xiaoming.grade; // 2

// 验证原型:
xiaoming.__proto__ === PrimaryStudent.prototype; // true
xiaoming.__proto__.__proto__ === Student.prototype; // true

// 验证继承关系:
xiaoming instanceof PrimaryStudent; // true
xiaoming instanceof Student; // true
~~~
用一张图来表示新的原型链：

![](/images/blogimages/2017/js/js_function_construction_2.png)

> 注意，函数F仅用于桥接，我们仅创建了一个new F()实例，而且，没有改变原有的Student定义的原型链。

如果把继承这个动作用一个inherits()函数封装起来，还可以隐藏F的定义，并简化代码：
~~~ JavaScript
function inherits(Child, Parent) {
    var F = function () {};
    F.prototype = Parent.prototype;
    Child.prototype = new F();
    Child.prototype.constructor = Child;
}
~~~
这个inherits()函数可以复用：
~~~ JavaScript
function Student(props) {
    this.name = props.name || 'Unnamed';
}

Student.prototype.hello = function () {
    alert('Hello, ' + this.name + '!');
}

function PrimaryStudent(props) {
    Student.call(this, props);
    this.grade = props.grade || 1;
}

// 实现原型继承链:
inherits(PrimaryStudent, Student);

// 绑定其他方法到PrimaryStudent原型:
PrimaryStudent.prototype.getGrade = function () {
    return this.grade;
};
~~~

总结JavaScript的原型继承实现方式就是：

- 定义新的构造函数，并在内部用call()调用希望“继承”的构造函数，并绑定this；
- 借助中间函数F实现原型链继承，最好通过封装的inherits函数完成；
- 继续在新的构造函数的原型上定义新方法。

### class继承
上面的我们看到了JavaScript的对象模型是基于原型实现的，特点是简单，缺点是理解起来比传统的类－实例模型要困难，最大的缺点是继承的实现需要编写大量代码，并且需要正确实现原型链。

新的关键字class从ES6开始正式被引入到JavaScript中。class的目的就是让定义类更简单。

如果用新的class关键字来编写Student，可以这样写：
~~~ JavaScript
class Student {
    constructor(name) {
        this.name = name;
    }

    hello() {
        alert('Hello, ' + this.name + '!');
    }
}
~~~
比较一下就可以发现，class的定义包含了构造函数constructor和定义在原型对象上的函数hello()（注意没有function关键字），这样就避免了Student.prototype.hello = function () {...}这样分散的代码。

最后，创建一个Student对象代码和前面章节完全一样：
~~~ JavaScript
var xiaoming = new Student('小明');
xiaoming.hello();
~~~

**class继承**
原型继承的中间对象，原型对象的构造函数等等都不需要考虑了，直接通过extends来实现：
~~~ JavaScript
class PrimaryStudent extends Student {
    constructor(name, grade) {
        super(name); // 记得用super调用父类的构造方法!
        this.grade = grade;
    }

    myGrade() {
        alert('I am at grade ' + this.grade);
    }
}
~~~
注意PrimaryStudent的定义也是class关键字实现的，而extends则表示原型链对象来自Student。子类的构造函数可能会与父类不太相同，例如，PrimaryStudent需要name和grade两个参数，并且需要通过**super(name)**来调用父类的构造函数，否则父类的name属性无法正常初始化。

PrimaryStudent已经自动获得了父类Student的hello方法，我们又在子类中定义了新的myGrade方法。

ES6引入的class和原有的JavaScript原型继承有什么区别呢？实际上它们没有任何区别，class的作用就是让JavaScript引擎去实现原来需要我们自己编写的原型链代码。简而言之，用class的好处就是极大地简化了原型链代码。

你一定会问，class这么好用，能不能现在就用上？

现在用还早了点，因为不是所有的主流浏览器都支持ES6的class。如果一定要现在就用上，就需要一个工具把class代码转换为传统的prototype代码，可以试试[Babel](https://babeljs.io/)这个工具。

### 操作DOM
由于HTML文档被浏览器解析后就是一棵DOM树，要改变HTML的结构，就需要通过JavaScript来操作DOM。

始终记住DOM是一个树形结构。操作一个DOM节点实际上就是这么几个操作：
- 更新：更新该DOM节点的内容，相当于更新了该DOM节点表示的HTML的内容；
- 遍历：遍历该DOM节点下的子节点，以便进行进一步操作；
- 添加：在该DOM节点下新增一个子节点，相当于动态增加了一个HTML节点；
- 删除：将该节点从HTML中删除，相当于删掉了该DOM节点的内容以及它包含的所有子节点。

在操作一个DOM节点前，我们需要通过各种方式先拿到这个DOM节点。最常用的方法是**document.getElementById()**和**document.getElementsByTagName()**，以及CSS选择器**document.getElementsByClassName()**。

由于ID在HTML文档中是唯一的，所以document.getElementById()可以直接定位唯一的一个DOM节点。document.getElementsByTagName()和document.getElementsByClassName()总是返回一组DOM节点。要精确地选择DOM，可以先定位父节点，再从父节点开始选择，以缩小范围。

例如：
~~~ JavaScript
// 返回ID为'test'的节点：
var test = document.getElementById('test');

// 先定位ID为'test-table'的节点，再返回其内部所有tr节点：
var trs = document.getElementById('test-table').getElementsByTagName('tr');

// 先定位ID为'test-div'的节点，再返回其内部所有class包含red的节点：
var reds = document.getElementById('test-div').getElementsByClassName('red');

// 获取节点test下的所有直属子节点:
var cs = test.children;

// 获取节点test下第一个、最后一个子节点：
var first = test.firstElementChild;
var last = test.lastElementChild;
~~~
第二种方法是使用querySelector()和querySelectorAll()，需要了解selector语法，然后使用条件来获取节点，更加方便：
~~~ JavaScript
// 通过querySelector获取ID为q1的节点：
var q1 = document.querySelector('#q1');

// 通过querySelectorAll获取q1节点内的符合条件的所有节点：
var ps = q1.querySelectorAll('div.highlighted > p');
~~~
> 注意：低版本的IE<8不支持querySelector和querySelectorAll。IE8仅有限支持

严格地讲，我们这里的DOM节点是指Element，但是DOM节点实际上是Node，在HTML中，Node包括Element、Comment、CDATA_SECTION等很多种，以及根节点Document类型，但是，绝大多数时候我们只关心Element，也就是实际控制页面结构的Node，其他类型的Node忽略即可。根节点Document已经自动绑定为全局变量document。

