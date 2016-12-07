package main

import "fmt"


type Integer int  
 
func (a Integer) Less(b Integer) bool { 
        return  a < b 
}


func main() {
fmt.Print("hello, world!\n");fmt.Print("hello, world1!\n")

var a Integer = 1;
if a.Less(2) {
    fmt.Println("less");
}
}
