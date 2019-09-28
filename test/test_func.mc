# 型エラーを出すサンプル

def int f1 (int x, int y)          # OK
  x + y
def double f2 (int x, double y)    # NG
  x + y
def double f3 (double x, int y)    # NG
  x + y
def double f4 (double x, double y) # OK
  x + y

def double f5 (int x, int y)       # NG
  x + y
def int f6 (double x, double y)    # NG
  x + y