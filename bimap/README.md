## Problem

### Introduction

The task for you is to implement a [bimap](https://en.wikipedia.org/wiki/Bidirectional_map) (bidirectional map).

[Maps](http://en.cppreference.com/w/cpp/container/map) are data structures that store a set of pairs` and allow you to efficiently find the corresponding second element of the pair (called the _value_) by the first element of the pair (which is called the _key_). 
There are two common implementations of maps: search trees and hash tables.
Next, in this assignment, I will assume that map is arranged as a binary search tree.

Just like maps, bimaps store a set of pairs and allow you to efficiently find the second one by the first element of the pair and the first one by the second.
Thus the search can be done in both directions.
I will call the first element of the pair __left__, the second — __right__.

A naive bimap implementation could consist of two maps: forward and reverse.
The forward would be used to search from left to right, and the reverse would be used from right to left.
Such an implementation is not effective, since it allocates two objects in hip for each pair stored in bimap. You can make it more efficient if you create a node class of the following type:

```cpp
struct node {
    left_t  left_data;
    node*   left_left;
    node*   left_right;
    node*   left_parent;

    right_t right_data;
    node*   right_left;
    node*   right_right;
    node*   right_parent;
};
```

In the given class `left_data` and `right_data` are the values of the left and right elements of the pair, respectively.
`left_{left,right,parent}` form a binary search tree in which `left_data` is the key.
`right_{left,right,parent}` form a binary search tree in which right_data is the key.
Thus nodes are linked into two independent search trees.

The advantages of this implementation are:
- Reduced memory usage.
- Potentially faster search by reducing cache thrashing.
- Faster insertion and deletion, since one memory allocation is made, not two.

The usual binary search tree allows you to find the next largest element and the previous largest element.
Such a structure of nodes as in bitmap allows you to find a node with the next largest left, the next largest right, the previous largest left, the previous largest right.

In `std::map` iterators support two operations: move to the next element and move to the previous one.
`begin()` returns an iterator to the minimum element, `end()` — to the next one after the maximum.
Thus the loop from `begin()` to `end()` allows you to view all the elements of `std::map` in ascending order.

In bimap iterators can be done in the following way.
There are two types of iterators: `left_iterator` and `right_iterator`.
Passing from `begin_left()` to `end_left()` allows you to view all the left elements of pairs in ascending order.
Pass from `begin_right()` to `end_right()` — all right elements of pairs in ascending order.
Both kinds of iterators have a `flip()` function that returns an iterator of a different kind, but referring to the same pair.

### Task

In this task you have to implement a class with the following interface:

```cpp
struct bimap {
    // You can define these typedefs as you want.
    typedef ... left_t;
    typedef ... right_t;

    struct left_iterator;
    struct right_iterator;

    // Creates an empty bimap.
    bimap();

    // Destructor. Called when deleting bimap objects.
    // Invalidates all iterators referencing elements of this bimap
    // (including iterators referencing the elements following the last).
    ~bimap();

    // Inserting a pair (left, right), returns the iterator to left.
    // If such a left or such a right is already present in the bimap, the insertion is not
    // performed and end_left() is returned.
    left_iterator insert(left_t const& left, right_t const& right);

    // Deletes the element and its corresponding paired.
    // erase of an invalid iterator is undefined.
    // erase(end_left()) and erase(end_right()) are undefined.
    // Let `it` refer to some element `e`.
    // erase invalidates all iterators referring to `e` and to an element paired to `e`.
    void erase(left_iterator it);
    void erase(right_iterator it);

    // Returns an iterator by element. If the element is not found, returns end_left()/and_right(), respectively.
    left_iterator  find_left (left_t  const& left)  const;
    right_iterator find_right(right_t const& right) const;

    // Returns the iterator to the minimum value left.
    left_iterator begin_left() const;
    // Returns an iterator to the next largest left.
    left_iterator end_left() const;

    // Returns the iterator to the minimum value of right.
    right_iterator begin_right() const;
    // Returns an iterator to the next largest right.
    right_iterator end_right() const;
};

struct bimap::left_iterator {
    // The element currently referenced by the iterator.
    // Dereference of the end_left() iterator is undefined.
    // Dereference of an invalid iterator is undefined.
    left_t const& operator*() const;

    // Move to the next largest left.
    // The increment of the end_left() iterator is undefined.
    // The increment of the invalid iterator is undefined.
    left_iterator& operator++();
    left_iterator operator++(int);

    // The transition to the previous largest left.
    // The decrement of the begin_left() iterator is undefined.
    // The decrement of an invalid iterator is undefined.
    left_iterator& operator--();
    left_iterator operator--(int);
    
    // left_iterator refers to the left element of some pair.
    // This function returns an iterator to the right element of the same pair.
    // end_left().flip() returns end_right().
    // end_right().flip() returns end_left().
    // flip() of an invalid iterator is undefined.
    right_iterator flip() const;
};

struct bimap::right_iterator {
    // Here everything is similar to left_iterator.
    right_t const& operator*() const;

    right_iterator& operator++();
    right_iterator operator++(int);

    right_iterator& operator--();
    right_iterator operator--(int);
    
    left_iterator flip() const;
};
```

bimap stores a set of pairs (left, right).
`begin_left()`/`end_left()` allows you to go through all the left elements of pairs in ascending order.
`begin_right()`/`end_right()` allows you to go through all the right elements of pairs in ascending order.
The `flip()` iterator function allows standing on the left element of a pair to move to the right and vice versa.

It is necessary that bimap creates one object in memory for each pair stored in it.

It is necessary that bimap uses a search tree inside itself (not necessarily balanced),
and the search/insert/delete operations work in O(`h`), where `h` is the height of the tree.

Copy constructors and the bimap assignment operator should be prohibited.

In cases where this task does not prescribe a specific behavior,
any behavior of a specific implementation is considered to satisfy the task.
It is assumed that not all cases of undefined behavior specified in this task can be reduced to assert without sacrificing simplicity or efficiency of implementation.
In cases where it is appropriate, the recommended behavior is to trigger an assert.

`left_iterator` and `right_iterator` do not have to be two independent classes.
They can be, for example, typedefs for different instantiations of the same template.

## Implementation details

My code is located in the folder __src__.

The code in the __test__ folder was suggested by teacher for checking the task.
