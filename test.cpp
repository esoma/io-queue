/*

The MIT License (MIT)

Copyright (c) 2012-2014 Erik Soma

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

// standard library
#include <assert.h>
#include <atomic>
#include <cstdlib>
#include <functional>
#include <thread>
// io-queue
#include "io_queue.hpp"

static void test_basic_operations();
static void test_thread_safety();

/*
    This program tests the IOQueue.
*/
int main(int argc, const char* argv[])
{
    test_basic_operations();
    test_thread_safety();
    return EXIT_SUCCESS;
}

static void test_basic_operations()
{
    IOQueue<int> queue;
    assert(!queue);
    
    queue.push(1);
    assert(queue);
    assert(queue.front() == 1);
    assert(queue.size() == 1);
    
    queue.pop();
    assert(!queue);
    assert(queue.size() == 0);
    
    queue.push(2);
    assert(queue);
    assert(queue.front() == 2);
    assert(queue.size() == 1);
    
    queue.push(3);
    assert(queue);
    assert(queue.front() == 2);
    assert(queue.size() == 2);
    
    queue.pop();
    assert(queue);
    assert(queue.front() == 3);
    assert(queue.size() == 1);
    
    queue.pop();
    assert(!queue);
    assert(queue.size() == 0);
}

static void test_thread_safety()
{
    std::atomic<int> count(0);
    std::atomic<int> push_count(0);
    IOQueue<std::function<void()>> queue;
    
    auto consumer = std::thread(
        [&queue, &count]
        {
            while(count.load() < 9000000)
            {
                if (queue)
                {
                    queue.front()();
                    queue.pop();
                }
            }
        }
    );
    
    auto producer_func =
        [&queue, &push_count, &count]
        {
            while(push_count.fetch_add(1) < 9000000)
            {
                queue.push([&count]{ ++count; });
            }
        };
        
    auto producer_1 = std::thread([&producer_func]{ producer_func(); });
    auto producer_2 = std::thread([&producer_func]{ producer_func(); });
    auto producer_3 = std::thread([&producer_func]{ producer_func(); });
    auto producer_4 = std::thread([&producer_func]{ producer_func(); });
    auto producer_5 = std::thread([&producer_func]{ producer_func(); });
    auto producer_6 = std::thread([&producer_func]{ producer_func(); });
    auto producer_7 = std::thread([&producer_func]{ producer_func(); });
    
    producer_1.join();
    producer_2.join();
    producer_3.join();
    producer_4.join();
    producer_5.join();
    producer_6.join();
    producer_7.join();

    consumer.join();
}