# 二分探索で平方根を求める
# target: 元の値, left: 探索の左端, right: 探索の右端, reps: 残りの反復回数

def double BinarySearch(double target, double left, double right, int reps)
  if reps <= 0 then
    (left + right) / 2.0
  else
    if (((left + right) / 2.0) * ((left + right) / 2.0)) < target then
      BinarySearch(target, ((left + right) / 2.0), right, reps - 1)
    else
      BinarySearch(target, left, ((left + right) / 2.0), reps - 1)