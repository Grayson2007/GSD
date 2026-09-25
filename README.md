# Grayson's System Distrbution (GSD)

## Introduction

GSD is an open source Kernel and standard designed for hobbiests. This project is primarily an expirment on various ideas of mine when it comes to operating system design. I describe this kernel as a hybrid between a monolithic kernel and a microkernel. Although the 'kernel' itself is designed to be as simple and tiny as possible where everything else is dynamically loaded on top of it.

## Architecture

GSD itself is meant to be a small baseplate that just manages your system through a database and provides various forms of protection.  Everything else is meant to be loaded on top of the baseplate. I imagine the base kernel itself being incredibly tiny potentially less then 2 mb tiny.

My design in my head currently borrows ideas from 
both UNIX and NT. Although this kernel will be neither of those two. Natively I'm not planning on posix compatibility and instead the kernel having it's own super flexible interface. 


## Plans 

Some Features I'm planning to add to this kernel are  
- Installable System Calls (syscalls that can be dynamically loaded and registered at runtime)
- Advanced Memory Protection 
- Strict Permission Systems
- Registry System similar to NT
- Object like System also similar to NT 
- Memory Managment (Inspired by the Linux Kernel)


## Status So far 

Currently I've been working on the UEFI Loader for the kernel and I'm first targetting x86_64 based machines. 


It also cannot be built or ran currently in a VM but that will change in the near future.
Once I have something working I'll post instructions below on how to try my kernel yourself in a VM.

