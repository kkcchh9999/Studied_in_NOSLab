import pandas as pd
import time

print("--- mnist 읽기 ---")
#csv
start_time = time.time()     #시작시간
data = pd.read_csv("/media/kch/Test/testdata/mnist/mnist.csv")
print("csvRead : ", time.time() - start_time) #현재시간 - 시작시간

#json
start_time = time.time()     #시작시간
data = pd.read_json("/media/kch/Test/testdata/mnist/mnist.json")
print("jsonRead : ", time.time() - start_time) #현재시간 - 시작시간

#xlsx
start_time = time.time()     #시작시간
data = pd.read_excel("/media/kch/Test/testdata/mnist/mnist.xlsx")
print("excelRead : ", time.time() - start_time) #현재시간 - 시작시간

print("--- fashion_mnist 읽기 ---")
#csv
start_time = time.time()     #시작시간
data = pd.read_csv("/media/kch/Test/testdata/fashion_mnist/fashion_mnist.csv")
print("csvRead : ", time.time() - start_time) #현재시간 - 시작시간

#json
start_time = time.time()     #시작시간
data = pd.read_json("/media/kch/Test/testdata/fashion_mnist/fashion_mnist.json")
print("jsonRead : ", time.time() - start_time) #현재시간 - 시작시간

#xlsx
start_time = time.time()     #시작시간
data = pd.read_excel("/media/kch/Test/testdata/fashion_mnist/fashion_mnist.xlsx")
print("excelRead : ", time.time() - start_time) #현재시간 - 시작시간


