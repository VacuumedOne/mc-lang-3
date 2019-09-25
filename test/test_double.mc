def BinarySearch(target left right reps)
  if reps <= 0.0 then
    (left + right) / 2.0
  else
    if (((left + right) / 2.0) * ((left + right) / 2.0)) < target then
      BinarySearch(target, ((left + right) / 2.0), right, reps-1)
    else
      BinarySearch(target, left, ((left + right) / 2), reps-1)