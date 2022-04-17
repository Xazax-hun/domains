# Domains

Toy language to help experiment with numerical domains for abstract interpretation. The language is inspired by the following book:

```
  Rival, Xavier, and Kwangkeun Yi. Introduction to static analysis: an abstract interpretation perspective. Mit Press, 2020.
```

The goal is to build a framework that helps experimenting with numerical domains and static analysis methods to
infer which regions in the 2D space are reachable by the programs written in the graphical language.

# Running the interpreter

Consider the following example program from the book:
```
init(50, 50, 50, 50);
translation(10, 0);
iter{
    { translation(10, 0) } or { rotation(0, 0, 90) }
}
```

Running `./domains example.tr --svg` will output the SVG to the standard output
that visualizes an execution trace of the program.
An execution is defined as a random walk of the control flow graph.

## Output:

![Example output](examples/example.png "Example output")

# Running the analyzer

Consider the following small program:
```
init(50, 50, 50, 50);
iter {
    {
        translation(10, 0)
    } or {
        translation(0, 10)
    }
};
rotation(0, 0, 180)
```
Running `./domains signDemo.tr --analyze sign` will print out the same program along with
some annotations. The annotations will contain the facts that the analyzer was able to
infer about the program.


## Output:
```
init(50, 50, 50, 50) /* { x: Positive, y: Positive } */;
iter {
  {
    translation(10, 0) /* { x: Positive, y: Positive } */
  } or {
    translation(0, 10) /* { x: Positive, y: Positive } */
  }
};
rotation(0, 0, 180) /* { x: Negative, y: Negative } */
```

# Dependencies

* Meson
* C++20 compatible compiler
* fmt
* cairo (optional for svg)
