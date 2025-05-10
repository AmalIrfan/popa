Popa
====

A cheap copy of Forth that works like a virtual machine.

## Build

```
make
```

## Run

```
./main 2> log
<CTRL-D>
```

or

```
./main
<CTRL-D>
```

## Example

```
./main 2> log
load echo.txt
echo hello
<CTRL-D>
```

## Notes

[ALGORITHM.txt](./ALGORITHM.txt) is a hardly readable poor attempt at explaining the workings.

the execute function is what \`runs\` the code, it's just a huge switch case.

important functions: `main`, `loop`, `intrepret`, `compile`, `table\_find`, `table\_try`, `execute`

some explanation
- `main`: call loop, if a file is loaded close it and call loop again, else quit
- `loop`: get a word, call intrepret or compile depending on mode
- `intrepret`: run special word or call `table\_try`
- `compile`: store bytecode for bytecode words or store address given by `table\_find`
- `table\_find`: search for a word in `mtable` (dictionary)
- `table\_try`: if `table\_find` gives an address call execute on it with return address `-1`
- `execute`: execute bytecode until a exit bytecode with return address `-1` is reached

