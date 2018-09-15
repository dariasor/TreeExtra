import pandas as pd
import numpy as np
import sys
import csv
def generate_unused(train,test,response_name,folder):
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
				if len(unique)==1:
					unused.append(name)

	txtfile1=folder+"/preprocess_unused.txt"
	txtfile2=folder+"/trueY.txt"
	pd.read_csv(test,sep="\t")[response_name].to_csv(txtfile2, index=False)
	with open(txtfile1, "w") as output:
		writer = csv.writer(output, lineterminator='\n',escapechar=' ',quoting=csv.QUOTE_NONE)
		for val in unused:
			writer.writerow([val])


if __name__ == '__main__':
	train=sys.argv[1]
	test=sys.argv[2]
	response_name=sys.argv[3]
	folder=sys.argv[4]
	generate_unused(train,test,response_name,folder)
