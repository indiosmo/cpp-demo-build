#ifndef LAB_CONCURRENT_QUEUE_HPP
#define LAB_CONCURRENT_QUEUE_HPP

#include "readerwriterqueue.h"

/*
 * SPSC lock-free queue, aliased from moodycamel::ReaderWriterQueue so a swap
 * is a one-header change. Wires the two cross-thread edges of the three-thread
 * topology.
 */

namespace lab {

template <typename T>
using concurrent_queue = moodycamel::ReaderWriterQueue<T>;

template <typename T>
using blocking_concurrent_queue = moodycamel::BlockingReaderWriterQueue<T>;

} // namespace lab

#endif /* LAB_CONCURRENT_QUEUE_HPP */
