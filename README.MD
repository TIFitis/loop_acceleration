# Underapproximating Loops for Fast Counterexample Detection

http://www.kroening.com/papers/fmsd2015.pdf

## How to build

``` bash
    $ cd Debug
    $ make cbmc
    $ make all
```

## What it does

The program takes a cbmc IR file, which can be generated using: 

```bash 
    $ goto-cc a.c -o a.gb
```

Feed this file as a commandline arg to this program, 

```bash 
    $ ./loop_acceleration a.gb
```
This generates a new file ` a.gb.gb `, which can be fed to cbmc
for verification. This new file has accelerated paths where
possible. 

```bash 
    $ cbmc ./a.gb.gb --z3 --unwind 2
```
   
## What has been handled

* For acceleration, we assume the closed form expression
to be in the form of the polynomial given in the paper. If such
a closed form cannot be formed for ANY variable in the loop body,
we do not accelerate that path.
* Compile with -DNOSYNTACTIC flag to use only the polynomial 
based accleration (Default). If that flag is not present, 
then we will try to accelerate with syntactic matching (Sec 3.1)
in the paper. 
* Only **monotonic** loop conditions are supported as of now. 
* For each if/else branch in the loop, we create two aux paths - 
one assuming taken and one assuming not taken. There is no 
alternation b/w the paths - if taken once, then that branch is
assumed taken for the rest of the acceleration process. 
* Only integral values are accelerated. 
 
