package main

import "fmt"

type Integer int

func (a Integer) Less (b Integer) bool {
    return a < b
}

func (a *Integer) Add (b Integer) {
    *a += b
}

type LessAdder interface {
    Less (b Integer) bool
    Add (b Integer)
}

func main() {
    var a Integer = 3
    fmt.Println(a.Less(2))
    fmt.Println(a.Less(3))
    fmt.Println(a)

    var b LessAdder = &a
    fmt.Println(b.Less(4));
    fmt.Println(b.Less(3))
    fmt.Println(b.Less(1));
    fmt.Println(b)
}
