#############################################
# experiment.sh:
#  main program for the experiment
#  input: mus.txt(1) train.tsv(2) test.tsv(3) response_name(4) topk(5) alphas.txt(6) 
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


mkdir result_"$test_data_name"

######## preprocess

#prescreening of features: exclude any feature that only has one value or has missing value (for now..)
#output result_"$test_data_name"/preprocess_unused.txt and result_"$test_data_name"/trueY.txt
python preprocess.py "$2" "$3" "$4" result_"$test_data_name"

#converting format
bash /home/cuize/Desktop/experiment/ag_scripts-master/General/tsv_to_dta.sh "$train_data_name_fullpath" "$4" result_"$test_data_name"/preprocess_unused.txt 
bash /home/cuize/Desktop/experiment/ag_scripts-master/General/tsv_to_dta.sh "$test_data_name_fullpath" "$4" result_"$test_data_name"/preprocess_unused.txt
rm "$test_data_name_fullpath".attr
mv "$train_data_name_fullpath".dta "$train_data_name_fullpath".attr "$test_data_name_fullpath".dta result_"$test_data_name"/
#now $train_data_name.dta, $train_data_name.attr and $test_data_name.dta are in result_"$test_data_name" folder

######### model training and prediction for different alpha

cat "$6"|      #read alphas.txt
while read alpha
do

	####BT model
	# input: traindata testdata attr topk
	# output: preds,model,attr for BTtopk


	/home/cuize/Desktop/experiment/TreeExtra/Bin/bt_train -t result_"$test_data_name"/"$train_data_name".dta -v result_"$test_data_name"/"$test_data_name".dta -r result_"$test_data_name"/"$train_data_name".attr -k $5 -a $alpha -m BT_alpha"$alpha".bin -o BT_preds_alpha"$alpha".txt
	mv BT_alpha"$alpha".bin result_"$test_data_name"/
	mv BT_preds_alpha"$alpha".txt result_"$test_data_name"/
	mv "$train_data_name".fs.attr result_"$test_data_name"/"$train_data_name"_BT_alpha"$alpha"fs.attr
	rm bagging_rms.txt  correlations.txt  log.txt feature_scores.txt

	####BTtopk model
	# input: traindata testdata topk_attr
	# output: preds,model,feature_scores
	/home/cuize/Desktop/experiment/TreeExtra/Bin/bt_train -t result_"$test_data_name"/"$train_data_name".dta -v result_"$test_data_name"/"$test_data_name".dta -r result_"$test_data_name"/"$train_data_name"_BT_alpha"$alpha"fs.attr -m BTt"$5"_alpha"$alpha".bin -o BTt"$5"_preds_alpha"$alpha".txt -a $alpha
	mv BTt"$5"_alpha"$alpha".bin result_"$test_data_name"/
	mv BTt"$5"_preds_alpha"$alpha".txt result_"$test_data_name"/
	mv feature_scores.txt result_"$test_data_name"/feature_scores_BTt"$5"_alpha"$alpha".txt
	rm "$train_data_name"_BT_alpha"$alpha"fs.fs.attr bagging_rms.txt  correlations.txt  log.txt

	####GBFS/GBDT(GBFS with mu=0) models
	# input: mus traindata testdata attr topk
	# output: preds,attr for GBFStopk 
	cat "$1"|      #read mus.txt
	while read row
	do
		/home/cuize/Desktop/experiment/TreeExtra/Bin/gbt_train -t result_"$test_data_name"/"$train_data_name".dta -v result_"$test_data_name"/"$test_data_name".dta -r result_"$test_data_name"/"$train_data_name".attr -mu $row -k $5 -a $alpha
		mv preds.txt result_"$test_data_name"/GBFS_mu"$row"_preds_alpha"$alpha".txt
		mv "$train_data_name".fs.attr result_"$test_data_name"/"$train_data_name"_GBFS_mu"$row"_alpha"$alpha".attr
		mv feature_scores.txt result_"$test_data_name"/feature_scores_GBFS_mu"$row"_alpha"$alpha".txt
		mv boosting_rms.txt result_"$test_data_name"/boosting_rms_GBFS_mu"$row"_alpha"$alpha".txt
		mv log.txt result_"$test_data_name"/log_GBFS_mu"$row"_alpha"$alpha".txt
	done

	####GBFStopk/GBDTtopk(GBFStopk with mu=0) models
	# input: mus traindata testdata topk_attr
	# output: preds,feature_scores 
	cat "$1"|      #read mus.txt
	while read row
	do
		/home/cuize/Desktop/experiment/TreeExtra/Bin/gbt_train -t result_"$test_data_name"/"$train_data_name".dta -v result_"$test_data_name"/"$test_data_name".dta -r result_"$test_data_name"/"$train_data_name"_GBFS_mu"$row"_alpha"$alpha".attr -mu $row -k -1 -a $alpha
		mv preds.txt result_"$test_data_name"/GBFSt"$5"_mu"$row"_preds_alpha"$alpha".txt
		mv feature_scores.txt result_"$test_data_name"/feature_scores_GBFSt"$5"_mu"$row"_alpha"$alpha".txt
		mv boosting_rms.txt result_"$test_data_name"/boosting_rms_GBFSt"$5"_mu"$row"_alpha"$alpha".txt
		mv log.txt result_"$test_data_name"/log_GBFSt"$5"_mu"$row"_alpha"$alpha".txt
		rm "$train_data_name"_GBFS_mu"$row"_alpha"$alpha".fs.attr 
	done
done


######## postprocess
