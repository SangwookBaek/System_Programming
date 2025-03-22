# Phase 2 README
In phase 2, we add pipeline('|') function which makes it possible to put one command's output to another command's input.

e.g. ls | grep history -> find files which contains "history" in ls output

- How to Compile : make
- How to clean : make clean
- How to execute : ./shellex

---

- You can use pipeline which are not in double **quote(" ") or single quote('')**
    - e.g. ls '|' grep txt In this example, '|' is not recognized as pipeline.
- The text after grep ignore **double quote(””) or single quote(’’)**
    - e.g. ls | grept txt == ls | grept “txt” == ls | grept ‘txt’
    - In ls | grep “”txt”  case, grep will find file contains “txt”.

- You can also use builtin function as input or output.
- History will record the entire line. 