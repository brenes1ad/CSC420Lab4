#ProducerConsumer.py -- simple program to do multi-threaded producer-consumer "toy" in python
#
#T. Brenes -Code from ChatGPT
#10/25/25
#CSC420 F25

import threading
import queue
import time
import random

# Configuration
QUEUE_CAPACITY = 6
NUM_PRODUCERS = 3
NUM_CONSUMERS = 4
ITEMS_PER_PRODUCER = 7

# Use a unique sentinel object as poison pill (safer than a special integer)
POISON_PILL = object()

# Shared queue (thread-safe in Python)
"""Queues in Python internally use mutex locks to prevent race conditions"""
q = queue.Queue(maxsize=QUEUE_CAPACITY)

# Stats dictionaries (accessed with a lock)
producer_stats = {}
consumer_stats = {}
"""this lock is used for updating info regarding timings. It enforces mutual exclusion with updates to
#producer_stats and consumer_stats. Only one thread can update them at a time"""
stats_lock = threading.Lock()

start_time = time.time()

def now_ms():
    return int((time.time() - start_time) * 1000)

def producer_fn(pid):
    t0 = time.time()
    produced = 0
    try:
        for j in range(ITEMS_PER_PRODUCER):
            item = pid * 100 + j  # unique integer per producer and item
            """put() will wait if the queue is full until a space becomes available. Mutual Exclusion
            # Then signals a consumer to get() """
            q.put(item)  # blocks if queue is full; Queue provides internal locking
            produced += 1
            print(f"[{now_ms():6d} ms] Producer {pid} produced item {item}")
            # simulate work
            time.sleep(0.05 + random.random() * 0.1)
    finally:
        elapsed = int((time.time() - t0) * 1000)
        with stats_lock:
            producer_stats[pid] = {"produced": produced, "elapsed_ms": elapsed}
        print(f"[{now_ms():6d} ms] Producer {pid} done (elapsed {elapsed} ms)")

def consumer_fn(cid):
    t0 = time.time()
    consumed = 0
    try:
        while True:
            """get() waits if there is nothing in the queue until something becomes available. Mutual exclusion"""
            item = q.get()  # blocks until item available; Queue handles mutual exclusion
            # Recognize poison pill by identity
            if item is POISON_PILL:
                print(f"[{now_ms():6d} ms] Consumer {cid} received poison pill, exiting")
                break
            consumed += 1
            print(f"[{now_ms():6d} ms] Consumer {cid} consumed item {item}")
            # simulate processing
            time.sleep(0.07 + random.random() * 0.12)
    finally:
        elapsed = int((time.time() - t0) * 1000)
        with stats_lock:
            consumer_stats[cid] = {"consumed": consumed, "elapsed_ms": elapsed}
        print(f"[{now_ms():6d} ms] Consumer {cid} done (elapsed {elapsed} ms)")

# Create and start producer threads
"""Each of the producers and consumers are put into their own thread but all run concurrently."""
producer_threads = []
for i in range(NUM_PRODUCERS):
    t = threading.Thread(target=producer_fn, args=(i,), name=f"Producer-{i}")
    producer_threads.append(t)
    t.start()

# Create and start consumer threads
consumer_threads = []
for i in range(NUM_CONSUMERS):
    t = threading.Thread(target=consumer_fn, args=(i,), name=f"Consumer-{i}")
    consumer_threads.append(t)
    t.start()

# Wait for all producers to finish
for t in producer_threads:
    t.join()

# After all producers finished, send one poison pill per consumer
print(f"[{now_ms():6d} ms] All producers finished. Sending {NUM_CONSUMERS} poison pills...")
for _ in range(NUM_CONSUMERS):
    q.put(POISON_PILL)

# Wait for all consumers to finish
for t in consumer_threads:
    t.join()

end_time = time.time()
total_ms = int((end_time - start_time) * 1000)

# Final summary
print("\n=== Summary ===")
with stats_lock:
    for pid in sorted(producer_stats):
        p = producer_stats[pid]
        print(f"Producer {pid}: produced={p['produced']}, elapsed={p['elapsed_ms']} ms")
    for cid in sorted(consumer_stats):
        c = consumer_stats[cid]
        print(f"Consumer {cid}: consumed={c['consumed']}, elapsed={c['elapsed_ms']} ms")

print(f"Total runtime: {total_ms} ms")
