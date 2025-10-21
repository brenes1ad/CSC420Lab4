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
        BlockingQueue<Integer> queue = new ArrayBlockingQueue<>(QUEUE_CAPACITY);
        List<Thread> threads = new ArrayList<>();

        long startTime = System.currentTimeMillis();

        // --- Producers ---
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


