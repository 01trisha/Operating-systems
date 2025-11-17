// uthread.c
#include "uthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

static uthread_t uthreads[MAX_THREADS_COUNT];
static int uthread_count = 0;
static uthread_t current_thread = NULL;
static ucontext_t main_context;
static ucontext_t scheduler_context;
static void *scheduler_stack = NULL;
static int scheduler_initialized = 0;
static uthread_t ready_queue = NULL;
static uthread_t ready_queue_tail = NULL;
void uthread_scheduler(void);

static void add_to_ready_queue(uthread_t thread) {
    if (thread->state == UTHREAD_FINISHED) return;
    thread->state = UTHREAD_READY;
    thread->next = NULL;
    if (ready_queue_tail) {
        ready_queue_tail->next = thread;
        ready_queue_tail = thread;
    } else {
        ready_queue = ready_queue_tail = thread;
    }
}
static uthread_t pop_from_ready_queue(void) {
    if (ready_queue == NULL) return NULL;
    uthread_t thread = ready_queue;
    ready_queue = ready_queue->next;
    if (ready_queue == NULL) {
        ready_queue_tail = NULL;
    }
    thread->next = NULL;
    return thread;
}
static void thread_wrapper(uthread_t thread) {
    void *retval = thread->start_routine(thread->arg);
    uthread_exit(retval);
}
static int create_stack(void **stack) {
    *stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (*stack == MAP_FAILED) return -1;
    memset(*stack, 0x7f, STACK_SIZE);
    return 0;
}
static void uthread_scheduler_init(void) {
    if (scheduler_initialized) return;
    if (getcontext(&main_context) == -1) {
        perror("uthread_scheduler_init: getcontext main failed");
        exit(1);
    }
    scheduler_stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (scheduler_stack == MAP_FAILED) {
        perror("uthread_scheduler_init: mmap failed");
        exit(1);
    }
    if (getcontext(&scheduler_context) == -1) {
        perror("uthread_scheduler_init: getcontext failed");
        exit(1);
    }
    scheduler_context.uc_stack.ss_sp = scheduler_stack;
    scheduler_context.uc_stack.ss_size = STACK_SIZE;
    scheduler_context.uc_stack.ss_flags = 0;
    scheduler_context.uc_link = &main_context;
    makecontext(&scheduler_context, uthread_scheduler, 0);
    scheduler_initialized = 1;
}
void uthread_scheduler(void) {
    while (1) {
        uthread_t next_thread = pop_from_ready_queue();
        if (next_thread == NULL) {
            setcontext(&main_context);
        }
        current_thread = next_thread;
        current_thread->state = UTHREAD_RUNNING;
        swapcontext(&scheduler_context, &current_thread->ucontext);
    }
}
int uthread_create(uthread_t *thread, void *(*start_routine)(void *), void *arg) {
    uthread_scheduler_init();
    if (uthread_count >= MAX_THREADS_COUNT) {
        fprintf(stderr, "uthread_create: too many threads\n");
        return -1;
    }
    if (!start_routine) {
        fprintf(stderr, "uthread_create: null start_routine\n");
        return -1;
    }
    void *stack;
    if (create_stack(&stack) == -1) {
        perror("uthread_create: stack allocation failed");
        return -1;
    }
    uthread_t mythread = (uthread_t)((char*)stack + STACK_SIZE - sizeof(uthread_struct_t));
    if (getcontext(&mythread->ucontext) == -1) {
        perror("uthread_create: getcontext failed");
        munmap(stack, STACK_SIZE);
        return -1;
    }
    mythread->ucontext.uc_stack.ss_sp = stack;
    mythread->ucontext.uc_stack.ss_size = STACK_SIZE - sizeof(uthread_struct_t);
    mythread->ucontext.uc_stack.ss_flags = 0;
    mythread->ucontext.uc_link = &scheduler_context;
    mythread->uthread_id = uthread_count;
    mythread->start_routine = start_routine;
    mythread->arg = arg;
    mythread->retval = NULL;
    mythread->state = UTHREAD_READY;
    mythread->stack = stack;
    mythread->next = NULL;
    makecontext(&mythread->ucontext, (void (*)(void))thread_wrapper, 1, mythread);
    uthreads[uthread_count] = mythread;
    *thread = mythread;
    uthread_count++;
    add_to_ready_queue(mythread);
    return 0;
}
int uthread_join(uthread_t thread, void **retval) {
    if (!thread) {
        return -1;
    }
    if (thread->state != UTHREAD_FINISHED) {
        if (current_thread == NULL) {
            if (swapcontext(&main_context, &scheduler_context) == -1) {
                perror("uthread_join: swapcontext failed");
                return -1;
            }
        } else {
            while (thread->state != UTHREAD_FINISHED) {
                uthread_yield();
            }
        }
    }
    if (retval) {
        *retval = thread->retval;
    }
    if (thread->stack) {
        void *stack_to_free = thread->stack;
        thread->stack = NULL;
        munmap(stack_to_free, STACK_SIZE);
    }
    return 0;
}
void uthread_yield(void) {
    if (!current_thread) return;
    if (current_thread->state == UTHREAD_RUNNING) {
        current_thread->state = UTHREAD_READY;
        add_to_ready_queue(current_thread);
    }
    swapcontext(&current_thread->ucontext, &scheduler_context);
}
void uthread_exit(void *retval) {
    if (current_thread) {
        current_thread->retval = retval;
        current_thread->state = UTHREAD_FINISHED;
    }
    setcontext(&scheduler_context);
}
