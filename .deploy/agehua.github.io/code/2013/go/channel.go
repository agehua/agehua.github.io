package main   

import  "fmt" 

func Count(ch chan int, i int ) { 
    fmt.Printf("Counting %d\n", i) 
    ch <- i  
} 

func main() { 
    chs :=  make([] chan int, 10) 
    for i := 0; i < 10; i++ { 
        chs[i] = make( chan int ) 
        go Count(chs[i], i) 
    } 

    var j int
     for _, ch := range(chs) { 
        j = <-ch 
        fmt.Printf("geting %d\n", j)
     }  
}
