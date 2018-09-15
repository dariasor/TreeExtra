import pandas as pd
import numpy as np
import sys
import csv
def generate_unused(train,test,response_name):
	train=pd.read_csv(train,sep="\t")
	train_names=list(train.columns.values)
	unused=[]
	for name in train_names:
		if name==response_name:
			continue
		else:
			unique=train[name].unique()
			dtype=unique[0]
			try:
				dtype.isalnum()
				unused.append(name)
			except:
				if len(unique)==1 or any([np.isnan(x) for x in unique]):
					unused.append(name)

	txtfile1="result/preprocess_unused.txt"
	txtfile2="result/trueY.txt"
	pd.read_csv(test,sep="\t")[response_name].to_csv(txtfile2, index=False)
	with open(txtfile1, "w") as output:
		writer = csv.writer(output, lineterminator='\n',escapechar=' ',quoting=csv.QUOTE_NONE)
		for val in unused:
			writer.writerow([val])


if __name__ == '__main__':
	train=sys.argv[1]
	test=sys.argv[2]
	response_name=sys.argv[3]
	generate_unused(train,test,response_name)
