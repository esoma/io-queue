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

#ifndef IO_QUEUE_HPP
#define IO_QUEUE_HPP

// standard library
#include <atomic>

/*
    This container is:
        - Unbounded
        - Non Intrusive
        - FIFO Queue
        - Lockless
        - Thread Safe for Multiple Producers and a Single Consumer
        - Exception Safe
    
    What is it good for?
        Taking slow or non-parallel work from many threads and giving it to
        another thread dedicated to executing that work.
        
        For example a logger: you may have multiple threads writing to the
        same i/o stream. Rather than acquiring a lock everytime they need to
        write to the stream simply put the message on the queue and let a 
        dedicated thread handle the actual work.
*/
template<class T>
class IOQueue
{
    private:
    
        /*
            Structure used to create a linked list for the internal
            implementation of the queue.
        */
        struct Node
        {
            Node(const T& t): item(t), next(0){};
            Node(T&& t): item(std::move(t)), next(0){};
            Node* get_next()
            {
                return next.load(std::memory_order_seq_cst);
            }
            void set_next(Node* node)
            {
                next.store(node, std::memory_order_seq_cst);
            }
            T item;
            std::atomic<Node*> next;
        };
        
    public:
    
        /*
            Constructor
            
            Creates an empty queue.
        =====================================================================*/
        IOQueue():
            head(0),
            tail(0)
        {
        }
        
        /*
            bool
            
            Checks whether the queue contains anything.
        =====================================================================*/
        operator bool() const
        {
            return head.load(std::memory_order_seq_cst) != 0;
        }
        
        /*
            front
            
            Gets the next item in the queue.
            
            Typically you will always want to check that something is at the 
            front of the queue using the bool operator because no exception is
            thrown when this is called when the queue is empty. You are likely
            to get a segfault without checking.
        =====================================================================*/
        T& front()
        {
            return head.load(std::memory_order_seq_cst)->item;
        }
        
        /*
            pop
            
            Removes the next item from the queue.
            
            Like the front method, typically you will always want to check that
            something is at the front of the queue using the bool operator 
            because no exception is thrown when this is called when the queue
            is empty. You are likely to get a segfault without checking.
            
            If T's destructor throws an exception the queue is guarenteed to
            not be in a corrupted state.
        =====================================================================*/
        void pop()
        {
            auto popped = head.load(std::memory_order_seq_cst);
            auto compare = popped;
            if (tail.compare_exchange_strong(
                    compare,
                    0,
                    std::memory_order_seq_cst
            ))
            {
                compare = popped;
                head.compare_exchange_strong(
                    compare,
                    0,
                    std::memory_order_seq_cst
                );
            }
            else
            {
                Node* new_head = popped->get_next();
                while(!new_head)
                {
                    new_head = popped->get_next();
                }
                head.store(new_head, std::memory_order_seq_cst); 
            }
            delete popped;
        }
        
        /*
            push
            
            Puts the item at the end of the queue. Push supports copying and
            moving (by constructor).
            
            If T's constructor throws an Exception the queue is guarenteed to
            not be in a corrupted state.
        =====================================================================*/
        void push(const T& item)
        {
            push(new Node(item));
        }
        
        void push(T&& item)
        {
            push(new Node(std::move(item)));
        }
        
        /*
            size
            
            Gets the number of items currently in the queue. O(n) complexity.
        =====================================================================*/
        unsigned int size() const
        {
            unsigned int count = 0;
            auto node = head.load(std::memory_order_seq_cst);
            while(node)
            {
                ++count;
                node = node->get_next();
            }
            return count;
        }
        
        /*
            Destructor
        =====================================================================*/
        ~IOQueue()
        {
            while(*this)
            {
                pop();
            }
        }
        
    private:
    
        std::atomic<Node*> head;
        
        std::atomic<Node*> tail;
        
        /*
            push (node)
            
            Internal method used to modify the linked list of Nodes.
        =====================================================================*/
        void push(Node* node)
        {
            auto old_tail = tail.exchange(node, std::memory_order_seq_cst);
            if (old_tail)
            {
                old_tail->set_next(node);
            }
            else
            {  
                head.store(node, std::memory_order_seq_cst);
            }
        }
};

#endif