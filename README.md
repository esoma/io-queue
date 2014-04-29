IO-Queue
=====

IO-Queue is a header only library that implements a C++11 Unbounded
Non-Intrusive Lockless Single Consumer Multiple Producer FIFO Queue. It excels
at taking I/O bound work from many threads and instead performing that I/O
bound work on a seperate thread.


Usage
-----

Simply include io_queue.hpp, there are no dependencies (other than a compiler
that supports the atomic C++11 standard library components).

Create a queue using the IOQueue class. The IOQueue has a similar interface
to the std::queue:
```cpp
IOQueue<int> my_queue;
```

To put data in the queue use the push method. Pushing supports copying and
moving. Pushing is considered a producer operation. Any thread can safely
execute this operation at any time.
```cpp
IOQueue<int> my_queue;
my_queue.push(1);
```

To check for data in the queue the bool operator or the size method can be
used. It should be noted that the bool operator will always be faster as no
internal size count is kept. The size method actually counts all of the items
in the queue on call. The bool operator and the size method are considered
consumer operations, only one thread may safely execute these at one time.
```cpp
IOQueue<int> my_queue;
my_queue.push(1);
if (my_queue)
{
	std::cout << "Items in Queue: " << my_queue.size() << std::endl;
}
```

The above code will output:
````
Items in Queue: 1
````

To get data out of the queue the front method is used. You should always check
that there is data in queue before calling the front method as there is no
built in check. If no data is in the queue when front is called a segfault is
likely. The front method is considered a consumer operation, only one thread
may safely execute this at one time.
```cpp
IOQueue<int> my_queue;
my_queue.push(1);
if (my_queue)
{
	std::cout << "Item: " << my_queue.front() << std::endl;
}
```

The above code will output:
````
Item: 1
````

To remove an item from the queue the pop method is used. You should always
check that there is data in queue before calling the pop method as there is no
built in check. If no data is in the queue when pop is called a segfault is
likely. The pop method is considered a consumer operation, only one thread
may safely execute this at one time.
```cpp
IOQueue<int> my_queue;
my_queue.push(1);
my_queue.push(2);
while(my_queue)
{
	std::cout << "Item: " << my_queue.front() << std::endl;
	my_queue.pop();
}
```

The above code will output:
````
Item: 1
Item: 2
````


Test
-----
Tests are successful if there is no output. Example build command with gcc on
windows:
````
g++ -ggdb -Wall --std=c++11 test.cpp -o test.exe
````