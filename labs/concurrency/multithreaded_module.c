#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kthread.h>
#include <linux/delay.h>

MODULE_AUTHOR("Halyna Butovych");
MODULE_DESCRIPTION("Trying threads in Linux Kernel Training");
MODULE_LICENSE("Dual BSD/GPL");

static int global_counter;
static struct task_struct *thread1;
static struct task_struct *thread2;

static int threadfn(void *data)
{
	pr_info("threadfn init\n");
	for (int i = 0; i < 1000000 && !kthread_should_stop(); i++) {
		global_counter++;
		ndelay(1);
	}
	pr_info("threadfn exit\n");
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
}

module_init(multithreaded_module_init);
module_exit(multithreaded_module_exit);
