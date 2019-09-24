def fib(x)
  # TODO 3.5
  # Delete "1" and write fib here
  if x == 0 then
    1
  else
    if x == 1 then
      1
    else
      fib(x-1) + fib(x-2)