def BinarySearch(double target, double left, double right, int reps) : double
  if reps <= 0 then
    (left + right) / 2.0
  else
    if (((left + right) / 2.0) * ((left + right) / 2.0)) < target then
      BinarySearch(target, ((left + right) / 2.0), right, reps - 1)
    else
      BinarySearch(target, left, ((left + right) / 2.0), reps - 1)