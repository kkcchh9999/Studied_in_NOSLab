import pandas as pd

#csv 파일 읽기 
data = pd.read_csv("/home/kch/mnist/mnist.csv", index_col=0)

#csv to json
data.to_json("/home/kch/mnist.json")

