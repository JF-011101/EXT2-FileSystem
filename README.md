
# MY-FS
MY-FS is a youth EXT2 file system designed and implemented based on fuse that can run on Linux.

[My school OS experiment course intruction](http://hitsz-lab.gitee.io/os-labs-2021/lab5/part1/)
## About FUSE
The traditional Linux based file system development is carried out in the form of kernel module, which makes the development and debugging extremely cumbersome. In addition, the kernel development is different from the traditional C program development. You need to use the kernel API. User state functions such as malloc are no longer applicable in the kernel development.

Due to the difficulty of kernel file system development, fuse architecture has been put on the table. The full name of fuse is filesystem in user space. It moves the implementation of the file system from the kernel state to the user state, that is, we can implement a file system in the user state.

Therefore, MY-FS is based on the fuse architecture, and implements our youth EXT2 file system in the user state, thus perfectly accessing Linux.

You can learn more about fuse [here](https://en.wikipedia.org/wiki/Fuse_(electrical))