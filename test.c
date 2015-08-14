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
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
// io-queue
#include "io_queue.h"
// pthread
#include <pthread.h>

static void test_basic_operations();
static void test_thread_safety();

int main(int argc, const char* argv[])
{
    test_basic_operations();
    test_thread_safety();
    return EXIT_SUCCESS;
}

static void test_basic_operations()
{
    IoQueue io_queue;
    {
        IoQueueResult result = io_queue_init(&io_queue, sizeof(int));
        assert(result == IO_QUEUE_RESULT_SUCCESS);
    }
    // initial queue is empty
    assert(io_queue_has_front(&io_queue) == IO_QUEUE_RESULT_FALSE);
    // push one item on empty queue
    {
        int in = 1;
        int out = 0;
        {
            IoQueueResult result = io_queue_push(&io_queue, &in);
            assert(result == IO_QUEUE_RESULT_SUCCESS);
        }
        assert(io_queue_has_front(&io_queue) == IO_QUEUE_RESULT_TRUE);
        {
            IoQueueResult result = io_queue_front(&io_queue, &out);
            assert(result == IO_QUEUE_RESULT_SUCCESS);
        }
        assert(out == in);
    }
    // pop one item out of a single item queue
    {
        IoQueueResult result = io_queue_pop(&io_queue);
        assert(result == IO_QUEUE_RESULT_SUCCESS);
    }
    assert(io_queue_has_front(&io_queue) == IO_QUEUE_RESULT_FALSE);
    // push many items on the queue
    for(size_t i = 0; i < 64; ++i)
    {
        int in = i;
        int out = -1;
        {
            IoQueueResult result = io_queue_push(&io_queue, &in);
            assert(result == IO_QUEUE_RESULT_SUCCESS);
        }
        assert(io_queue_has_front(&io_queue) == IO_QUEUE_RESULT_TRUE);
        {
            IoQueueResult result = io_queue_front(&io_queue, &out);
            assert(result == IO_QUEUE_RESULT_SUCCESS);
        }
        assert(out == 0);
    }
    // pop many items from the queue
    for(size_t i = 0; i < 32; ++i)
    {
        {
            int out = -1;
            IoQueueResult result = io_queue_front(&io_queue, &out);
            assert(result == IO_QUEUE_RESULT_SUCCESS);
            assert(out == i);
        }
        {
            IoQueueResult result = io_queue_pop(&io_queue);
            assert(result == IO_QUEUE_RESULT_SUCCESS);
        }
    }
    // clear the queue
    assert(io_queue_has_front(&io_queue) == IO_QUEUE_RESULT_TRUE);
    {
        IoQueueResult result = io_queue_clear(&io_queue);
        assert(result == IO_QUEUE_RESULT_SUCCESS);
    }
    assert(io_queue_has_front(&io_queue) == IO_QUEUE_RESULT_FALSE);
}

#define IO_QUEUE_TEST_THREAD_SAFETY_COUNT 90000000
#define IO_QUEUE_TEST_THREAD_PRODUCER_COUNT 7

typedef struct IoQueueTestThreadSafety
{
    atomic_int consume_count;
    atomic_int producer_count;
    IoQueue io_queue;
} IoQueueTestThreadSafety;

static void* test_thread_safety_consumer(void* arg)
{
    IoQueueTestThreadSafety* test = (IoQueueTestThreadSafety*)arg;
    while(atomic_load(&test->consume_count) < IO_QUEUE_TEST_THREAD_SAFETY_COUNT)
    {
        if (io_queue_has_front(&test->io_queue))
        {
            atomic_fetch_add(&test->consume_count, 1);
            {
                IoQueueResult result = io_queue_pop(&test->io_queue);
                assert(result == IO_QUEUE_RESULT_SUCCESS);
            }
        }
    }
    return NULL;
}

static void* test_thread_safety_producer(void* arg)
{
    IoQueueTestThreadSafety* test = (IoQueueTestThreadSafety*)arg;
    while(1)
    {
        int in = atomic_fetch_add(&test->producer_count, 1);
        if (in >= IO_QUEUE_TEST_THREAD_SAFETY_COUNT)
        {
            break;
        }
        IoQueueResult result = io_queue_push(&test->io_queue, &in);
        assert(result == IO_QUEUE_RESULT_SUCCESS);
    }
    return NULL;
}

static void test_thread_safety()
{
    IoQueueTestThreadSafety test;
    atomic_init(&test.consume_count, 0);
    atomic_init(&test.producer_count, 0);
    {
        IoQueueResult result = io_queue_init(&test.io_queue, sizeof(int));
        assert(result == IO_QUEUE_RESULT_SUCCESS);
    }
    // create threads
    pthread_t consumer;
    pthread_t producers[IO_QUEUE_TEST_THREAD_PRODUCER_COUNT];
    {
        int p_result = pthread_create(
            &consumer,
            NULL,
            test_thread_safety_consumer,
            &test
        );
        assert(p_result == 0);
    }
    for(size_t i = 0; i < IO_QUEUE_TEST_THREAD_PRODUCER_COUNT; ++i)
    {
        int p_result = pthread_create(
            &producers[i],
            NULL,
            test_thread_safety_producer,
            &test
        );
        assert(p_result == 0);
    }
    // wait for everything to finish
    for(size_t i = 0; i < IO_QUEUE_TEST_THREAD_PRODUCER_COUNT; ++i)
    {
        int p_result = pthread_join(producers[i], NULL);
        assert(p_result == 0);
    }
    {
        int p_result = pthread_join(consumer, NULL);
        assert(p_result == 0);
    }
    // make sure the queue is empty
    assert(io_queue_has_front(&test.io_queue) == IO_QUEUE_RESULT_FALSE);
}