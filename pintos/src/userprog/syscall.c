#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
}
void
syscall_exit (int status)
{
  struct thread *cur = thread_current ();

  cur->exit_status->exit_code = status;

  /* Decrement self REF_COUNT and try to free self EXIT_STATUS. */
  lock_acquire (&cur->exit_status->lock);
  cur->exit_status->ref_count--;
  if (cur->exit_status->ref_count == 0)
    free (cur->exit_status);
  else
    sema_up (&cur->exit_status->sema);
  lock_release (&cur->exit_status->lock);

  /* Decrement REF_COUNT of all children and try to free child's EXIT_STATUS. */
  while (!list_empty (&cur->child_status_list))
    {
      struct list_elem *e = list_pop_front (&cur->child_status_list);
      struct exit_status_t *exit_status = list_entry (e, struct exit_status_t, elem);
      lock_acquire (&exit_status->lock);
      exit_status->ref_count--;
      if (exit_status->ref_count == 0)
        free (exit_status);
      else
        lock_release (&exit_status->lock);
    }

  printf ("%s: exit(%d)\n", (char *) &cur->name, status);
  thread_exit ();
}