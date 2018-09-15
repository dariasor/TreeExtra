#!/usr/bin/env python
"""
(C) Alexander Sorokin, 2009
License: BSD
"""

import os,sys, getopt

#This is the default BIN location. You don't need to change it.
#You should use --ag_bin= command line argument to override it.
TE_BIN_LOCATION="/home/syrnick/daria/TreeExtra.1.2/additive_groves/"


def usage():
    print """
Usage: build_te_model.py all_parameters
Parameters:
-h, --help       print this message
-b, --ag_bin=    location for AG binaries.
-w, --work_dir=  location for the intermediate files and output AG models
-t, --train=     training file
-v, --val=       validation file
-r, --attr=      attribute file
-s, --speed=     training mode (fast,slow); default is fast
-e, --extra=     extra arguments for the model

If the output model exists, it will do noting.
"""

def get_next_action(data_root):
    log_fn=os.path.join(data_root,'log.txt');
    log_line=open(os.path.expanduser(log_fn),'r').readlines()[-2].strip();
    if log_line.startswith("Suggested action: "):
        return log_line.replace("Suggested action: ","")
    return None


def make_predictions_with_model(data_root,model_file):

    cmd="%sag_predict -p %s/data.test -r %s/data.attr -m %s -o %s/preds.txt" % (
        TE_BIN_LOCATION,data_root,data_root,model_file,data_root)
    os.system(cmd)
    pass

def build_model(data_root,ag_bin=None,train_file='data.train',val_file='data.valid',attr_file='data.attr',speed='slow',extra_args='-n 2'):
    if ag_bin is None:
        ag_bin=TE_BIN_LOCATION

    ag_train=os.path.join(ag_bin,'ag_train')
    
    model_filename=os.path.join(data_root,'model.bin');
    if os.path.exists(model_filename):
        print "Model exists", model_filename
        return

    #train_file=os.path.join(data_root,'data.train')
    #val_file=os.path.join(data_root,'data.valid')
    #test_file=os.path.join(data_root,'data.test')
    #attr_file=os.path.join(data_root,'data.attr')


    true_dir=os.getcwd()

    os.chdir(data_root);
    train_cmd = "%s -t %s -v %s -r %s -s %s %s " % ( ag_train, train_file, val_file, attr_file, speed, extra_args )
    print "Next action is ",train_cmd
    train_status = os.system(train_cmd)

    done_training=False
    while not done_training:
        action = get_next_action("./")
        print "Next action is ",action
        action_cmd=os.path.join(ag_bin,action);
        action_status = os.system(action_cmd)        

        if action.startswith('ag_save'):
            done_training=True;

    os.chdir(true_dir);


if __name__=="__main__":

    optlist, args = getopt.getopt(sys.argv[1:], "t:v:r:s:w:b:he:", ["help", "ag_bin=", "train=","val=","attr=","work_dir=","speed=","extra="])

    ag_bin="";
    train="data.train"
    validation="data.valid"
    attr="data.attr"
    speed="fast"
    extra_args=""

    work_dir="./"
    
    for (field, val) in optlist:
        print field,val
        if field in ("-h", "--help"):
            usage()
            sys.exit()
        elif field in ("-b","--ag_bin"):
            ag_bin=val
        elif field in ("-t","--train"):
            train=val
        elif field in ("-v","--val"):
            validation=val
        elif field in ("-r","--attr"):
            attr=val
        elif field in ("-w","--work_dir"):
            work_dir=val
        elif field in ("-s","--speed"):
            speed=val
        elif field in ("-e","--extra"):
            extra_args=val

    if work_dir is None:
        usage();
        sys.exit()

    build_model(work_dir,ag_bin,train,validation,attr,speed,extra_args);
