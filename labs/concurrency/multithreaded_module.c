#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm-generic/barrier.h>

MODULE_AUTHOR("Halyna Butovych");
MODULE_DESCRIPTION("Trying threads in Linux Kernel Training");
MODULE_LICENSE("Dual BSD/GPL");

#define MM_LOCK_BIT 0

static int global_counter;
static struct task_struct *thread1;
static struct task_struct *thread2;
static unsigned long lock_var;

static int shared_data;
DECLARE_RWSEM(sem_for_shared_data);
static struct task_struct *reader1;
static struct task_struct *reader2;
static struct task_struct *writer1;
static atomic_t readers_count = ATOMIC_INIT(0);

static inline void lock(unsigned long *l)
{
	while (unlikely(test_and_set_bit_lock(MM_LOCK_BIT, l)))
		;
}

static inline void unlock(unsigned long *l)
{
	clear_bit_unlock(MM_LOCK_BIT, l);
	/* if clear_bit is used for locking purposes, I should call
	 * smp_mb__before_clear_bit and/or smp_mb__after_clear_bit in order to
	 * ensure changes are visible on other processors
	 */
	smp_mb__after_atomic();
}

static int threadfn(void *data)
{
	pr_info("threadfn init\n");
	for (int i = 0; i < 1000000 && !kthread_should_stop(); i++) {
		lock(&lock_var);
		global_counter++;
		unlock(&lock_var);
		ndelay(1);
	}
	pr_info("threadfn exit\n");
	return 0;
}

static int reader(void *data)
{
	const char *name = data;
	BUG_ON(!name);
	pr_info("reader %s init\n", name);
	while (!kthread_should_stop()) {
		down_read(&sem_for_shared_data);
		int count = atomic_inc_return(&readers_count);
		pr_info("reader %s reads: %d\n", name, shared_data);
		/* blink before reading */
		mdelay(1);
		if (count > 1)
			pr_info_once("the book seems to be popular\n");
		atomic_dec(&readers_count);
		up_read(&sem_for_shared_data);
	}
	pr_info("reader %s exit\n", name);
	return 0;
}

static int writer(void *data)
{
	const char *name = data;
	BUG_ON(!name);
	pr_info("reader %s init\n", name);
	while (!kthread_should_stop()) {
		/* think about what to write */
		mdelay(100);
		down_write(&sem_for_shared_data);
		BUG_ON(atomic_read(&readers_count));
		/* think about what to write once more */
		mdelay(10);
		++shared_data;
		up_write(&sem_for_shared_data);
	}
	pr_info("reader %s exit\n", name);
	return 0;
}


static int __init multithreaded_module_init(void)
{
	pr_info("multithreaded_module_init\n");
	thread1 = kthread_create(threadfn, NULL, "th1");
	BUG_ON(!thread1);
	get_task_struct(thread1);
	thread2 = kthread_create(threadfn, NULL, "th2");
	BUG_ON(!thread2);
	get_task_struct(thread2);
	wake_up_process(thread1);
	wake_up_process(thread2);

	reader1 = kthread_run(reader, "reader1", "reader1");
	reader2 = kthread_run(reader, "reader2", "reader2");
	writer1 = kthread_run(writer, "writer1", "writer1");

	return 0;
}

static void __exit multithreaded_module_exit(void)
{
	pr_info("multithreaded_module_exit\n");
	if (thread1) {
		int rc = kthread_stop(thread1);
		WARN_ON(-EINTR == rc);
		put_task_struct(thread1);
	}
	if (thread2) {
		int rc = kthread_stop(thread2);
		WARN_ON(-EINTR == rc);
		put_task_struct(thread2);
	}
	pr_info("global_counter is :%d", global_counter);

	if (reader1) {
		int rc = kthread_stop(reader1);
		WARN_ON(-EINTR == rc);
	}

	if (reader2) {
		int rc = kthread_stop(reader2);
		WARN_ON(-EINTR == rc);
	}

	if (writer1) {
		int rc = kthread_stop(writer1);
		WARN_ON(-EINTR == rc);
	}
}

module_init(multithreaded_module_init);
module_exit(multithreaded_module_exit);
