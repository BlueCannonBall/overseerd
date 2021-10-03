# `discordd`

Freeze DiScOrD when you aren't using it.

## Motivation

DiScOrD is a shitty, inefficient, slow electron app constantly robbing your system resources, even when you aren't using it. This daemon solves this (at least on Linux) by "freezing" Discord (using SIGSTOP) when its window isn't in focus.

## Build & Install

```
$ make
# make install
```
*Note that `discordd` depends on `libxdo`.*
*Also note that `#` represents a root shell.*