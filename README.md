IO-Queue
=====

IO-Queue is a header only library that implements a C11 Unbounded Non-Intrusive
Lockless Single Consumer Multiple Producer FIFO Queue. It excels at taking I/O
bound work from many threads and instead performing that I/O bound work on a
seperate thread.


Usage
-----

Simply include io_queue.h, there are no dependencies (other than a compiler
that supports the atomic C11 standard library components).

Create a queue using the IoQueue structure:
```c
IoQueue my_queue;
io_queue_init(&my_queue, sizeof(int));
```

To put data in the queue use the io_queue_push function. Pushing supports
copying and moving. Pushing is considered a producer operation. Any thread can
safely execute this operation at any time.
```c
int in = 1;
io_queue_push(&my_queue, &in);
```

To check for data in the queue the io_queue_has_front function should be used.
Checking the front is considered a consumer operation, only one thread may
safely execute this at one time.
```c
if (io_queue_has_front(&my_queue) == IO_QUEUE_RESULT_TRUE)
{
	// do something
}
```

To get data out of the queue the io_queue_front function is used. You should
always check that there is data in queue before calling front as there is no
built in check. If no data is in the queue when front is called a segfault is
likely (or an assertion failure). Getting data is considered a consumer
operation, only one thread may safely execute this at one time.
```c
if (io_queue_has_front(&my_queue) == IO_QUEUE_RESULT_TRUE)
{
	int out;
	io_queue_front(&my_queue, &out);
}
```

To remove an item from the queue the io_queue_pop function is used. You should
always check that there is data in queue before popping as there is no built in
check. If no data is in the queue when pop is called a segfault is
likely (or an assertion failure). Popping is considered a consumer operation,
only one thread may safely execute this at one time.
```c
if (io_queue_has_front(&my_queue) == IO_QUEUE_RESULT_TRUE)
{
	io_queue_pop(&my_queue);
}
```


Test
-----
Tests are successful if there is no output. Example build command with gcc on
windows:
````
gcc -ggdb -Wall test.c -o test.exe
````