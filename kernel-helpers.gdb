define print_task_list
  set $_head = $arg0
  set $_node = $_head->next
  while $_node != $_head
    set $_t = (struct task *)(((char *)$_node) - 8)
    set $_wp = 0
    if $_t->waitflag
      set $_wp = *(uint64_t*)$_t->waitflag
    end
    printf "node: %p, t:%p %s, waitflag: %ld\n", $_node, $_t, $_t->name, $_wp
    set $_node = $_node->next
  end
end

define list_tasks
  set $_sched = $arg0
  printf "sched is %p\n", $_sched
  printf "running:\n"
  print_task_list &$_sched->running
  printf "timer_waiting:\n"
  print_task_list &$_sched->timer_waiting
  printf "flag_waiting:\n"
  print_task_list &$_sched->flag_waiting
end
