# `overseerd`

Freeze resource intensive apps when you aren't using them.

## Motivation

Many applications nowadays are made using inefficient frameworks and libraries such as Electron, slowing down the rest of your system even when you aren't actively using the apps. This Linux program solves this by freezing select apps when their windows aren't in focus.

## Build & Install

```
$ make
$ sudo make install
```

## Usage

```
$ overseerd Discord # Freeze processes with the name Discord. If overseerd is working properly, nothing should be outputed to the console.
```