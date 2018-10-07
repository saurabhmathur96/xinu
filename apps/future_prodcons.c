uint future_prod(future_t* fut,int n) {
  future_set(fut, n);
  printf("Produced %d\n",n);
  return OK;
}

uint future_cons(future_t* fut) {
  int i, status;
  status = (int)future_get(fut, &i);
  if (status < 1) {
    printf("future_get failed\n");
    return -1;
  }
  printf("Consumed %d\n", i);
  return OK;
}