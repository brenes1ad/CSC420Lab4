/**
 * ProducerConsumer.java -- simple program to run multi-threaded consumers and producers
 *
 * T. Brenes -Code from ChatGPT
 * 10/21/25
 * CSC420 F25
 */


/**
 * The ArrayBlockingQueue has atomic and thread safe versions of put() and take() which means
 * no two threads could modify the queue at the same time
 */

//Test to see if documentation is showing

import java.util.concurrent.*;
import java.util.*;

public class ProducerConsumer {
    // Configuration
    private static final int QUEUE_CAPACITY = 5;
    private static final int NUM_PRODUCERS = 3;
    private static final int NUM_CONSUMERS = 4;
    private static final int ITEMS_PER_PRODUCER = 5;

    // Poison pill to signal termination
    private static final int POISON_PILL = Integer.MIN_VALUE;

   public static void main(String[] args) throws InterruptedException {
        //ArrayBlockingQueue automatically handles mutual exclusion. It makes producers wait if queue is full
        //and makes consumers wait if it's empty doing so with an internal lock
       //Because of that, there are no explicit wait or signal calls because it does it all under the hood.
        BlockingQueue<Integer> queue = new ArrayBlockingQueue<>(QUEUE_CAPACITY);
        List<Thread> threads = new ArrayList<>();

        long startTime = System.currentTimeMillis();

        // --- Producers ---
       //Each producer runs in its own thread, prodces something, then sleeps to act like it's busy working.
        for (int i = 0; i < NUM_PRODUCERS; i++) {
            int id = i;
            Thread producer = new Thread(() -> {
                long t0 = System.currentTimeMillis();
                try {
                    for (int j = 0; j < ITEMS_PER_PRODUCER; j++) {
                        int item = id * 100 + j;
                        queue.put(item);
                        System.out.printf("[%6d ms] Producer %d produced %d%n",
                                System.currentTimeMillis() - startTime, id, item);
                        Thread.sleep(100);
                    }
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                } finally {
                    System.out.printf("[%6d ms] Producer %d done (elapsed %d ms)%n",
                            System.currentTimeMillis() - startTime, id,
                            System.currentTimeMillis() - t0);
                }
            }, "Producer-" + i);
            threads.add(producer);
            producer.start();
        }

        // --- Consumers ---
       //Each consumer also runs in its own thread just constantly calling take() to eat until killed off
        for (int i = 0; i < NUM_CONSUMERS; i++) {
            int id = i;
            Thread consumer = new Thread(() -> {
                long t0 = System.currentTimeMillis();
                try {
                    while (true) {
                        int item = queue.take();
                        if (item == POISON_PILL) {
                            System.out.printf("[%6d ms] Consumer %d received poison pill%n",
                                    System.currentTimeMillis() - startTime, id);
                            break;
                        }
                        System.out.printf("[%6d ms] Consumer %d consumed %d%n",
                                System.currentTimeMillis() - startTime, id, item);
                        Thread.sleep(150);
                    }
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                } finally {
                    System.out.printf("[%6d ms] Consumer %d done (elapsed %d ms)%n",
                            System.currentTimeMillis() - startTime, id,
                            System.currentTimeMillis() - t0);
                }
            }, "Consumer-" + i);
            threads.add(consumer);
            consumer.start();
        }

        // Wait for all producers to finish
        for (int i = 0; i < NUM_PRODUCERS; i++) {
            threads.get(i).join();
        }

        // Send poison pills (one per consumer)
       //Poison pill kills consumers when the producers are done producing items so they're not infinitely waiting
       // for something to consume.
        System.out.println("\nAll producers finished. Sending poison pills...");
        for (int i = 0; i < NUM_CONSUMERS; i++) {
            queue.put(POISON_PILL);
        }

        // Wait for all consumers to finish
        for (int i = NUM_PRODUCERS; i < threads.size(); i++) {
            threads.get(i).join();
        }

        long endTime = System.currentTimeMillis();
        System.out.printf("%nAll threads done. Total runtime: %d ms%n", (endTime - startTime));
    }
}


