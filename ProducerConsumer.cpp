// ProducerConsumer.cpp -- simple program to do multi-threaded producer-consumer "toy" in c++ using pthreads.
//
// T. Brenes -Code from ChatGPT
// 10/27/25
// CSC420 F25
//
//
//
// This program creates NUM_PRODUCERS producers and NUM_CONSUMERS consumers.
// Producers generate ITEMS_PER_PRODUCER items and put them into a bounded buffer.
// After all producers finish, the main thread inserts one "poison pill" per consumer
// so consumers can exit cleanly. The bounded buffer is protected by a pthread mutex
// and two condition variables (not_full, not_empty), implementing the monitor protocol.

#include <pthread.h>
#include <unistd.h>         // for usleep (optional)
#include <deque>
#include <vector>
#include <map>
#include <iostream>
#include <chrono>
#include <random>
#include <atomic>
#include <sstream>
#include <iomanip>

using namespace std;

// Configuration
constexpr int QUEUE_CAPACITY = 6;
constexpr int NUM_PRODUCERS  = 3;
constexpr int NUM_CONSUMERS  = 4;
constexpr int ITEMS_PER_PRODUCER = 7;

// Poison pill sentinel. Producers only produce non-negative ints.
// Use a distinct negative value for poison.
constexpr int POISON_PILL = -1;

// Shared buffer (bounded)
deque<int> buffer;

// Monitor / synchronization primitives
//Explicit creation of mutux lock and other variables to help support
//This is the monitor lock that handles mutual exclusion. If anything wants to act, it must hold this lock(buffer_mutex)
//Those extra variables help with the wait() and signal() in a normal monitor
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  not_full     = PTHREAD_COND_INITIALIZER;
pthread_cond_t  not_empty    = PTHREAD_COND_INITIALIZER;

// Stats protected by their own mutex
map<int, pair<int,long long>> producer_stats; // pid -> (produced_count, elapsed_ms)
map<int, pair<int,long long>> consumer_stats; // cid -> (consumed_count, elapsed_ms)
//This is a separate mutex lock with similar use to the python example. It handles only modification to the
//storage and display of stats about runtime.
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

// Start time for timestamping ms relative to program start
auto program_start = chrono::steady_clock::now();

long long now_ms() {
    auto diff = chrono::steady_clock::now() - program_start;
    return chrono::duration_cast<chrono::milliseconds>(diff).count();
}

// Utility to print time-stamped messages in a safe way (not strictly atomic)
void log_msg(const string &s) {
    // We don't use an extra I/O mutex here; output interleaving is acceptable for demo.
    cout << "[" << setw(6) << now_ms() << " ms] " << s << "\n";
}

// Bounded buffer put (monitor-style): waits while full
void buffer_put(int item) {
    pthread_mutex_lock(&buffer_mutex);
    while ((int)buffer.size() >= QUEUE_CAPACITY) {
        // Wait until a consumer signals there's space.
        pthread_cond_wait(&not_full, &buffer_mutex);
    }
    buffer.push_back(item);
    // Signal that buffer is not empty (wake one waiting consumer)
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&buffer_mutex);
}

// Bounded buffer get (monitor-style): waits while empty
int buffer_get() {
    pthread_mutex_lock(&buffer_mutex);
    while (buffer.empty()) {
        // Wait until a producer signals there's an item.
        pthread_cond_wait(&not_empty, &buffer_mutex);
    }
    int item = buffer.front();
    buffer.pop_front();
    // Signal that buffer is not full (wake one waiting producer)
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&buffer_mutex);
    return item;
}

// Thread functions must have C signature for pthreads
extern "C" void* producer_thread(void* arg) {
    int pid = *reinterpret_cast<int*>(arg);
    auto t0 = chrono::steady_clock::now();
    int produced = 0;

    // Randomness for simulated work time
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist_ms(50, 150); // between 50 and 150 ms

    for (int j = 0; j < ITEMS_PER_PRODUCER; ++j) {
        int item = pid * 100 + j; // unique non-negative item
        buffer_put(item);         // may block if buffer full (wait on not_full)
        ++produced;
        {
            ostringstream oss;
            oss << "Producer " << pid << " produced item " << item;
            log_msg(oss.str());
        }
        // Simulate work
        int sleep_ms = dist_ms(gen);
        this_thread::sleep_for(chrono::milliseconds(sleep_ms));
    }

    // Record stats under stats_mutex
    auto t1 = chrono::steady_clock::now();
    long long elapsed_ms = chrono::duration_cast<chrono::milliseconds>(t1 - t0).count();
    pthread_mutex_lock(&stats_mutex);
    producer_stats[pid] = make_pair(produced, elapsed_ms);
    pthread_mutex_unlock(&stats_mutex);

    {
        ostringstream oss;
        oss << "Producer " << pid << " done (elapsed " << elapsed_ms << " ms)";
        log_msg(oss.str());
    }

    return nullptr;
}

extern "C" void* consumer_thread(void* arg) {
    int cid = *reinterpret_cast<int*>(arg);
    auto t0 = chrono::steady_clock::now();
    int consumed = 0;

    // Randomness for simulated processing time
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist_ms(70, 190); // between 70 and 190 ms

    while (true) {
        int item = buffer_get(); // may block if buffer empty (wait on not_empty)

        if (item == POISON_PILL) {
            ostringstream oss;
            oss << "Consumer " << cid << " received poison pill, exiting";
            log_msg(oss.str());
            // Important: when a consumer sees poison, we must ensure the poison pill
            // isn't "lost" if other consumers must also receive one. We assume
            // main enqueues one poison per consumer.
            break;
        }

        ++consumed;
        {
            ostringstream oss;
            oss << "Consumer " << cid << " consumed item " << item;
            log_msg(oss.str());
        }

        // Simulate processing
        int sleep_ms = dist_ms(gen);
        this_thread::sleep_for(chrono::milliseconds(sleep_ms));
    }

    // Record stats
    auto t1 = chrono::steady_clock::now();
    long long elapsed_ms = chrono::duration_cast<chrono::milliseconds>(t1 - t0).count();

    pthread_mutex_lock(&stats_mutex);
    consumer_stats[cid] = make_pair(consumed, elapsed_ms);
    pthread_mutex_unlock(&stats_mutex);

    {
        ostringstream oss;
        oss << "Consumer " << cid << " done (elapsed " << elapsed_ms << " ms)";
        log_msg(oss.str());
    }

    return nullptr;
}

int main() {
    program_start = chrono::steady_clock::now();
    vector<pthread_t> producers(NUM_PRODUCERS);
    vector<pthread_t> consumers(NUM_CONSUMERS);
    vector<int> prod_ids(NUM_PRODUCERS);
    vector<int> cons_ids(NUM_CONSUMERS);

    // Start consumers first (optional)
    for (int i = 0; i < NUM_CONSUMERS; ++i) {
        cons_ids[i] = i;
        if (pthread_create(&consumers[i], nullptr, consumer_thread, &cons_ids[i]) != 0) {
            perror("Failed to create consumer thread");
            return 1;
        }
    }

    // Start producers
    for (int i = 0; i < NUM_PRODUCERS; ++i) {
        prod_ids[i] = i;
        if (pthread_create(&producers[i], nullptr, producer_thread, &prod_ids[i]) != 0) {
            perror("Failed to create producer thread");
            return 1;
        }
    }

    // Wait for all producers to finish
    for (int i = 0; i < NUM_PRODUCERS; ++i) {
        pthread_join(producers[i], nullptr);
    }

    {
        ostringstream oss;
        oss << "All producers finished. Sending " << NUM_CONSUMERS << " poison pills...";
        log_msg(oss.str());
    }

    // Send one poison pill per consumer. Use buffer_put (which blocks if full),
    // thus following the same monitor protocol as normal items.
    for (int i = 0; i < NUM_CONSUMERS; ++i) {
        buffer_put(POISON_PILL);
    }

    // Wait for all consumers to finish
    for (int i = 0; i < NUM_CONSUMERS; ++i) {
        pthread_join(consumers[i], nullptr);
    }

    // Print summary
    long long total_ms = now_ms();
    cout << "\n=== Summary ===\n";
    pthread_mutex_lock(&stats_mutex);
    for (const auto &p : producer_stats) {
        cout << "Producer " << p.first << ": produced=" << p.second.first
             << ", elapsed=" << p.second.second << " ms\n";
    }
    for (const auto &c : consumer_stats) {
        cout << "Consumer " << c.first << ": consumed=" << c.second.first
             << ", elapsed=" << c.second.second << " ms\n";
    }
    pthread_mutex_unlock(&stats_mutex);

    cout << "Total runtime: " << total_ms << " ms\n";

    // Destroy mutexes/conds
    pthread_mutex_destroy(&buffer_mutex);
    pthread_mutex_destroy(&stats_mutex);
    pthread_cond_destroy(&not_full);
    pthread_cond_destroy(&not_empty);

    return 0;
}
