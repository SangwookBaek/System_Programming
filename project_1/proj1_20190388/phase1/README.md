# Phase 1 README
In phase 1, we make simple shell program which is able to run some shell functions.


- How to Compile : make
- How to clean : make clean
- How to execute : ./shellex

---
You can execute builtin command and other shell functions.

You can use shell functions including **cat, ls , echo, vi, make …. etc**.

---

Also you can use builtin_commands including __history, !!, !#__

- __History__ command will print out every cmd history you have written. If you call history more than two times continuously, only first history will be recorded.
     - history -> history -> history : 1 history (only one recorded)

- **!!** will execute your last cmd.

- **!#** will execute #$_{th}$ cmd. 

- **!! , !#** will not be recorded in history. However, what those cmds execute will be recorded.

- **cd path** will change your directory to path.

- **cd ..** will change your directory to your upper directory.

- **cd** , **cd ~** will change your directory to home directory.
