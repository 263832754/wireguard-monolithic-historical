/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018 Thomas Gschwantner <tharre3@gmail.com>. All Rights Reserved.
 * Copyright (C) 2018 Jason A. Donenfeld <Jason@zx2c4.com>. All Rights Reserved.
 */

#ifdef DEBUG

#include "../mpmc_ptr_ring.h"
#include <linux/kthread.h>
#include <linux/ptr_ring.h>

#define THREADS_PRODUCER 20
#define THREADS_CONSUMER 20
#define ELEMENT_COUNT 100000000LL /* divisible by threads_{consumer,producer} */
#define QUEUE_SIZE 1024

#define EXPECTED_TOTAL ((ELEMENT_COUNT * (ELEMENT_COUNT + 1)) / 2)
#define PER_PRODUCER (ELEMENT_COUNT / THREADS_PRODUCER)
#define PER_CONSUMER (ELEMENT_COUNT / THREADS_CONSUMER)
#define THREADS_TOTAL (THREADS_PRODUCER + THREADS_CONSUMER)

struct worker_producer {
	struct ptr_ring *ring;
	struct completion completion;
	int thread_num;
};

struct worker_consumer {
	struct ptr_ring *ring;
	struct completion completion;
	uint64_t total;
	uint64_t count;
};

static __init int producer_function(void *data)
{
	struct worker_producer *td = data;
	uint64_t i;

	for (i = td->thread_num * PER_PRODUCER + 1; i <= (td->thread_num + 1) * PER_PRODUCER; ++i) {
		while (ptr_ring_produce(td->ring, (void *)i)) {
			if (need_resched())
				schedule();
		}
	}
	complete_all(&td->completion);
	return 0;
}

static __init int consumer_function(void *data)
{
	struct worker_consumer *td = data;
	uint64_t i;

	for (i = 0; i < PER_CONSUMER; ++i) {
		uintptr_t value;
		while (!(value = (uintptr_t)ptr_ring_consume(td->ring))) {
			if (need_resched())
				schedule();
		}

		td->total += value;
		++td->count;
	}
	complete_all(&td->completion);
	return 0;
}

bool __init mpmc_ring_selftest(void)
{
	struct worker_producer *producers;
	struct worker_consumer *consumers;
	struct ptr_ring ring;
	int64_t total = 0, count = 0;
	int i;

	producers = kmalloc_array(THREADS_PRODUCER, sizeof(*producers), GFP_KERNEL);
	consumers = kmalloc_array(THREADS_CONSUMER, sizeof(*consumers), GFP_KERNEL);

	BUG_ON(!producers || !consumers);
	BUG_ON(ptr_ring_init(&ring, QUEUE_SIZE, GFP_KERNEL));

	for (i = 0; i < THREADS_PRODUCER; ++i) {
		producers[i].ring = &ring;
		producers[i].thread_num = i;
		init_completion(&producers[i].completion);
		kthread_run(producer_function, &producers[i], "producer %d", i);
	}

	for (i = 0; i < THREADS_CONSUMER; ++i) {
		consumers[i].ring = &ring;
		consumers[i].total = 0;
		consumers[i].count = 0;
		init_completion(&consumers[i].completion);
		kthread_run(consumer_function, &consumers[i], "consumer %d", i);
	}

	for (i = 0; i < THREADS_PRODUCER; ++i)
		wait_for_completion(&producers[i].completion);
	for (i = 0; i < THREADS_CONSUMER; ++i)
		wait_for_completion(&consumers[i].completion);

	BUG_ON(!ptr_ring_empty(&ring));
	ptr_ring_cleanup(&ring, NULL);

	for (i = 0; i < THREADS_CONSUMER; ++i) {
		total += consumers[i].total;
		count += consumers[i].count;
	}

	kfree(producers);
	kfree(consumers);

	if (count == ELEMENT_COUNT && total == EXPECTED_TOTAL) {
		pr_info("mpmc_ring self-tests: pass");
		return true;
	}

	pr_info("mpmc_ring self-test failed:");
	pr_info("Count: %llu, expected: %llu", count, ELEMENT_COUNT);
	pr_info("Total: %llu, expected: %llu", total, EXPECTED_TOTAL);

	return false;
}

#endif
