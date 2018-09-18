#############################################
# aux_exp.sh:
#  main program for the control outcomes
#  input: topk.txt(1) train.tsv(2) test.tsv(3) response_name(4) alpha.txt (5)
#  output: AUC.txt table etc.
#############################
# subprograms include:
#################
# preprocess.py 
# description: 
#  prescreening of features: exclude any feature that only has one value 
#  input: $2(train.tsv) $3(test.tsv)  $4(response_name)
#  output: preprocess_unused.txt trueY.txt 
################# 
# tsv_to_dta.sh
# description:
#  convert data file to required format for TreeExtra pacakge and generate .attr file 
#  input:  stem(original dataset name without extension,like train in $2(train.tsv)) $4(response_name) 
#          preprocess_unused.txt 
#  output: stem.dta stem.attr
#################
# bt_train
# description:
#   train a BT model based on train.dta train.attr, then rank the features and output predictions on test data 
#   input: train.dta test.dta train.attr topk
#   output: feature_score.txt preds.txt
#################
# gbt_train
# description:
#   train a GBDT model based on train.dta train.attr,then rank the features and output predictions on test data 
#   for each mu>0 in mus.txt implement the GBFS with penalty mu on new split variables
#   input: train.dta test.dta train.attr topk mu
#   output: feature_score.txt preds.txt
#################
# postprocess.py
# description:
#   take true Y value and preds.txt from different model and 
#   output all kinds of performance (Now only AUC and RMSE) indexes and plots
#   input: all preds.txt and model informations
#   output: AUC.txt RMSE.txt

# get datanames

train_data_name_fullpath_ext=$2
test_data_name_fullpath_ext=$3
train_data_name_fullpath="${train_data_name_fullpath_ext%.*}"
test_data_name_fullpath="${test_data_name_fullpath_ext%.*}"
train_data_name=$(basename -- "$train_data_name_fullpath")
test_data_name=$(basename -- "$test_data_name_fullpath")


mkdir aux_result_"$test_data_name"

######## preprocess

#prescreening of features: exclude any feature that only has one value or has missing value (for now..)
#output aux_result_"$test_data_name"/preprocess_unused.txt and aux_result_"$test_data_name"/trueY.txt
python preprocess.py "$2" "$3" "$4" aux_result_"$test_data_name"

#converting format
bash /home/cuize/Desktop/experiment/ag_scripts-master/General/tsv_to_dta.sh "$train_data_name_fullpath" "$4" aux_result_"$test_data_name"/preprocess_unused.txt 
bash /home/cuize/Desktop/experiment/ag_scripts-master/General/tsv_to_dta.sh "$test_data_name_fullpath" "$4" aux_result_"$test_data_name"/preprocess_unused.txt
rm "$test_data_name_fullpath".attr
mv "$train_data_name_fullpath".dta "$train_data_name_fullpath".attr "$test_data_name_fullpath".dta aux_result_"$test_data_name"/
#now $train_data_name.dta, $train_data_name.attr and $test_data_name.dta are in aux_result_"$test_data_name" folder

######### model training and prediction for different alpha

cat "$5"|      #read alphas.txt
while read alpha
do

	####BT/GBDT topk model
	# input: topk.txt traindata testdata attr 
	# output: preds etc
	cat "$1"|      #read topk.txt
	while read row
	do
		############### BT
		/home/cuize/Desktop/experiment/TreeExtra/Bin/bt_train -t aux_result_"$test_data_name"/"$train_data_name".dta -v aux_result_"$test_data_name"/"$test_data_name".dta -r aux_result_"$test_data_name"/"$train_data_name".attr -k "$row" -a $alpha
		rm preds.txt model.bin feature_scores.txt bagging_rms.txt  correlations.txt  log.txt
		mv "$train_data_name".fs.attr aux_result_"$test_data_name"/"$train_data_name"_BTt"$row"_alpha"$alpha".attr

		/home/cuize/Desktop/experiment/TreeExtra/Bin/bt_train -t aux_result_"$test_data_name"/"$train_data_name".dta -v aux_result_"$test_data_name"/"$test_data_name".dta -r aux_result_"$test_data_name"/"$train_data_name"_BTt"$row"_alpha"$alpha".attr -m BTt"$row"_alpha"$alpha".bin -o BTt"$row"_preds_alpha"$alpha".txt -a $alpha
		rm correlations.txt  log.txt
		mv BTt"$row"_alpha"$alpha".bin BTt"$row"_preds_alpha"$alpha".txt aux_result_"$test_data_name"/
		mv bagging_rms.txt aux_result_"$test_data_name"/bagging_rms_BTt"$row"_alpha"$alpha".txt


		############### GBDT
		/home/cuize/Desktop/experiment/TreeExtra/Bin/gbt_train -t aux_result_"$test_data_name"/"$train_data_name".dta -v aux_result_"$test_data_name"/"$test_data_name".dta -r aux_result_"$test_data_name"/"$train_data_name".attr -k "$row" -a $alpha
		rm preds.txt feature_scores.txt boosting_rms.txt log.txt
		mv "$train_data_name".fs.attr aux_result_"$test_data_name"/"$train_data_name"_GBDTt"$row"_alpha"$alpha".attr


		/home/cuize/Desktop/experiment/TreeExtra/Bin/gbt_train -t aux_result_"$test_data_name"/"$train_data_name".dta -v aux_result_"$test_data_name"/"$test_data_name".dta -r aux_result_"$test_data_name"/"$train_data_name"_GBDTt"$row"_alpha"$alpha".attr -a $alpha
		rm log.txt
		mv preds.txt aux_result_"$test_data_name"/GBDTt"$row"_preds_alpha"$alpha".txt
		mv boosting_rms.txt aux_result_"$test_data_name"/boosting_rms_GBDTt"$row"_alpha"$alpha".txt
		

	done

done

######## postprocess
