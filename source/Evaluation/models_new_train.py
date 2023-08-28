import pickle
import sys
import math

import matplotlib.pyplot as plt
import numpy as np
from matplotlib import rc, rcParams
from mlxtend.plotting import plot_decision_regions
from pandas import read_csv
from pandas.plotting import scatter_matrix
from sklearn import datasets, linear_model, tree
from sklearn.discriminant_analysis import LinearDiscriminantAnalysis
from sklearn.ensemble import AdaBoostClassifier, RandomForestClassifier
from sklearn.gaussian_process import GaussianProcessClassifier
from sklearn.gaussian_process.kernels import RBF
from sklearn.linear_model import LogisticRegression
from sklearn import metrics
from sklearn.metrics import det_curve
from sklearn.metrics import (accuracy_score, auc, classification_report,
                             confusion_matrix, mean_squared_error,
                             ConfusionMatrixDisplay, r2_score, roc_curve)
from sklearn.model_selection import (StratifiedKFold, cross_val_score,
                                     cross_validate, train_test_split)
from sklearn.naive_bayes import GaussianNB
from sklearn.neighbors import KNeighborsClassifier
from sklearn.neural_network import MLPClassifier
from sklearn.svm import SVC
from sklearn.tree import DecisionTreeClassifier
from scipy import interp


def main():
    rc('font', **{'family': 'serif', 'serif': ['Computer Modern']})
    rc('text', usetex=True)
    rcParams['font.size'] = 14
    f = []
    for arg in sys.argv[1:]:
        f.append(arg)
        print(arg)

    for filein in f:
        # Load dataset
        names = ['C1', 'C2', 'C3', 'C4', 'C5', 'C6', 'C7', 'C8', 'C9', 'C10', 'C11', 'C12', 'C13', 'C14', 'C15', 'C16', 'T1',
                'T2', 'T3', 'T4', 'T5', 'T6', 'T7', 'T8', 'T9', 'T10', 'T11', 'T12', 'T13', 'T14', 'T15', 'T16', 'exits', 'class']
        # names = ['Cache', 'exits', 'times','Cache1', 'exits1', 'times1','Cache2', 'exits2', 'times2','Cache3', 'exits3', 'times3',
        #        'Cache4', 'exits4', 'times4', 'Cache5', 'exits5', 'times5', 'Cache6', 'exits6', 'times6', 'Cache7', 'exits7', 'times7','Cache8',
        #        'exits8', 'times8','Cache9', 'exits9', 'times9','Cache10', 'exits10', 'times10','Cache11', 'exits11', 'times11', 'Cache12', 'exits12',
        #        'times12', 'Cache13', 'exits13', 'times13', 'Cache14', 'exits14', 'times14', 'Cache15', 'exits15', 'times15', 'class']
        dataset = read_csv(filein, names=names)
        # dtype={"tlb": int, "exits": int, "times": int, "class": "string"}

        # shape
        print(dataset.shape)

        # descriptions
        print(dataset.describe())
        # class distribution
        print(dataset.groupby('class').size())

        # Some further analysis
        # dataset.plot(kind='box', subplots=True, layout=(3,1), sharex=False, sharey=False)
        # plt.show()

        # histograms
        #dataset.hist()
        #plt.show()

        # scatter plot matrix
        # scatter_matrix(dataset)
        # plt.show()

        # Split-out validation dataset
        array = dataset.values
        # X = array[:,0:12]
        # X = array[:,[0,1,3,4,6,7,9,10]]
        # y = array[:,12]
        # X = array[:,[0,1]]
        # X = array[:,[0,1,3,4,6,7,9,10,12,13,15,16,18,19,21,22]]
        X_Data = array[:, 0:16]
        X_Data_tim = array[:, 16:32]
        exits_data = array[:, 32]
        # X = array[:,[0,3,6,9,12,15,18,21,24,27,30,33,36,39,42,45,1]]
        # X = array[:,[0,1,3,4,6,7,9,10,12,13,15,16,18,19,21,22,24,25,27,28,30,31,33,34,36,37,39,40,42,43,45,46]]
        y_data = array[:, 33]

        ND = []

        for i in range(0, len(y_data)):
            for j in range(0, 16):
                if (X_Data_tim[i, j]>270):
                    #ND.append(int(X_Data[i, j]))
                    ND.append(1)
                else:
                    ND.append(0)


        r1=[]
        r2=[]
        r3=[]
        r4=[]
        r5=[]
        r6=[]
        #lr = [1,2,4,8,16,32]
        lr = [4,8,16,32,48,64]
        ct = 3
        for i in lr:
            X = []
            y = []
            ct = ct - 1
            if ct < 0:
                ct = 0
            li = int((len(ND)-(len(ND)%i))/i) 
            for j in range(0, li):
                if (j < int(len(ND)/(2*i))):
                    y.append(0)
                else:
                    y.append(1)
                row = []
                # row = (ND[(j*i):(j*i+i)]).split()
                # for k in range(len(row)):
                #    row[k] = int(row[k])
                for k in range((j*i), (j*i+i)):
                    row.append(int(ND[k]))
                X.append(row)
            X = np.array(X)
            print(X.size)
            print(X[0, :])
            
            ts1 = 1- (0.40/(2**ct))
            ts2 = 1- (0.15/(2**ct))
            ts3 = 1- (0.03/(2**ct)) 
            """
            ts1 = 1- (0.30)
            ts2 = 1- (0.2)
            ts3 = 1- (0.02)
            """
            X_train, X_validation, Y_train, Y_validation = train_test_split(
                X, y, test_size=ts1, random_state=1, shuffle=True)
            X_train1, X_validation, Y_train1, Y_validation = train_test_split(
                X, y, test_size=ts2, random_state=1, shuffle=True)
            X_tra, X_kk, Y_tra, Y_kk = train_test_split(
                X, y, test_size=ts3, random_state=1, shuffle=True)
            print(X_train[1, :])
            print(Y_train[1])
            print(X_train.size)
            print(X_validation.size)

            # ROC curve for the threshold approach
            stimated_out = []
            for j in range(0, len(Y_train)):
                c = 0
                id = math.floor(j/16)
                #if (exits_data[id]==0):
                for k in range(0, len(X_train[1, :])):
                    if (X_train[j, k] == 1):
                        c += 1
                stimated_out.append(c)

            # Compute ROC curve and area the curve
            mean_tpr = 0.0
            fpr, tpr, thresholds = roc_curve(Y_train, stimated_out)
            fpr1, fnr, thresholds1 = det_curve(Y_train, stimated_out)

            print(Y_train.count(1))

            print("Computed thresholds false positives and true positives")
            print(thresholds[1:thresholds.shape[0]])
            print(thresholds1[::-1])
            print(fpr)
            print(fpr1)
            print(tpr)
            print(fnr)

            tprf=[]
            fprf=[]
            fnrf=[]

            for j in range(0,thresholds.shape[0]):
                tval = thresholds[j]
                ch = -1
                for k in range(0,thresholds1.shape[0]):
                    if (thresholds1[k]==tval):
                        tprf.append(tpr[j])
                        fprf.append(fpr[j])
                        fnrf.append(fnr[k])

            truepos = (np.array(tprf))*Y_train.count(1)
            falseposr = np.array(fprf)
            falsepos = np.array(falseposr)*Y_train.count(0)
            trueneg = (1 - falseposr)*Y_train.count(0) #true neg rate * number of reg
            falsenegr = np.array(fnrf)
            falseneg = falsenegr*Y_train.count(1)
            #trueneg = 1 - falsepos

            arr = np.array([2*truepos, falsepos, falseneg])
            # print(arr)
            f1 = np.divide(2*truepos, arr.sum(axis=0))
            acc = np.divide((truepos+trueneg), (truepos+trueneg+falsepos+falseneg))
            pre = np.divide(truepos, (truepos+falsepos))
            rec = 1 - falsenegr
            print("accuracy precission recall f1")
            print(acc)
            print(pre)
            print(rec)
            print(f1)

            ra = []
            rp = []
            rre = []
            rf1 = []
            names = []

            best = 0
            fbest = 0.0
            for j in range(0,len(f1)):
                if (f1[j]>fbest):
                    fbest = f1[j]
                    best = j
            print(best,acc[best],pre[best],rec[best],fbest)
            
            names.append('Threshold based')
            ra.append(acc[best])
            rp.append(pre[best])
            rre.append(rec[best])
            rf1.append(fbest)
            nnei = 8
            if (i == 1):
                r5.append(fbest)
                nnei = 4
            if (i == 2):
                r6.append(fbest)
                nnei = 4
            if (i == 4):
                r1.append(fbest)
                nnei = 8
            if (i == 8):
                r2.append(fbest)
                nnei = 16
            if (i == 16):
                r3.append(fbest)
                nnei = 16
            if (i == 32):
                r4.append(fbest)
                nnei = 16
            if (i == 48):
                r5.append(fbest)
                nnei = 16
            if (i == 64):
                r6.append(fbest)
                nnei = 16   

            # mean_tpr += interp(mean_fpr, fpr, tpr)
            # mean_tpr[0] = 0.0

            """ roc_auc = auc(fpr, tpr)
            plt.plot(fpr, tpr, lw=1, label='ROC fold %d (area = %0.2f)' %
                    (i, roc_auc))
            plt.ylabel('ROC Curve')
            plt.tight_layout()
            plt.show()

            display = metrics.DetCurveDisplay(fpr=fpr1, fnr=fnr)
            display.plot()
            plt.ylabel('DET Curve')
            plt.tight_layout()
            plt.show() """

            # Test the decision tree algorithm
            model = DecisionTreeClassifier()
            model = model.fit(X_train, Y_train)
            # fig, axes = plt.subplots(nrows = 1,ncols = 1,figsize = (4,4), dpi=300)

            print("Decision tree")
            predictions = model.predict(X_validation)

            # Evaluate predictions
            print(accuracy_score(Y_validation, predictions))
            print(confusion_matrix(Y_validation, predictions))
            print(classification_report(Y_validation, predictions))

            # tree.plot_tree(model,filled=True)

            # Evaluate predictions
            # print("KNN")
            # model = KNeighborsClassifier(n_neighbors=8)
            # model = model.fit(X_train, Y_train)
            # predictions = model.predict(X_validation)

            # print(accuracy_score(Y_validation, predictions))
            # print(confusion_matrix(Y_validation, predictions))
            # print(classification_report(Y_validation, predictions))

            # plot_decision_regions(X_validation, Y_validation, model)
            print("Now testing models")
            # Spot Check Algorithms
            models = []
            # models.append(('SVM',SVC(kernel="linear", C=0.025)))
            # models.append(('SVM', SVC(gamma='auto')))
            models.append(('Logistic \nRegression', LogisticRegression(
                solver='lbfgs', multi_class='auto', max_iter=500,n_jobs=4)))
            models.append(('Lineal \nDiscriminant \nAnalysis',
                        LinearDiscriminantAnalysis()))
            models.append(('KNN', KNeighborsClassifier(n_neighbors=nnei,n_jobs=4)))
            models.append(('Decission \nTree', DecisionTreeClassifier()))
            models.append(('Random \nForest', RandomForestClassifier(
                max_depth=5, n_estimators=20, max_features=1,n_jobs=4)))
            models.append(('Gaussian\n Naive \n Bayes', GaussianNB()))
            # models.append(('Gaussian Process Classifier',GaussianProcessClassifier(1.0 * RBF(1.0))))
            models.append(('Ada Boost \nClassifier', AdaBoostClassifier()))
            models.append(('Neural \nNetwork', MLPClassifier(hidden_layer_sizes=(
                20,), activation="logistic", alpha=0.01, max_iter=500)))
            models.append(('SVM', SVC(gamma='auto')))
            # models.append(('SVM',SVC(kernel="linear", C=0.025)))
            # evaluate each model in turn
            results_ac = []
            results_pr = []
            results_re = []
            results_f1 = []
            scoring = {'acc': 'accuracy',
                    'prec': 'precision',
                    'rec': 'recall',
                    'f1': 'f1'}
            for name, model in models:
                if (name == 'SVM'):
                    print('SVN uff')
                    kfold = StratifiedKFold(
                        n_splits=10, random_state=1, shuffle=True)
                    cv_results = cross_validate(
                        model, X_train1, Y_train1, cv=kfold, scoring=scoring)
                    # cv_results = cross_val_score(model, X_train1, Y_train1, cv=kfold, scoring=scoring)
                    results_ac.append(cv_results['test_acc'])
                    results_pr.append(cv_results['test_prec'])
                    results_re.append(cv_results['test_rec'])
                    results_f1.append(cv_results['test_f1'])
                    ra.append(cv_results['test_acc'].mean())
                    rp.append(cv_results['test_prec'].mean())
                    rre.append(cv_results['test_rec'].mean())
                    rf1.append(cv_results['test_f1'].mean())
                    if (i == 1):
                        r5.append(cv_results['test_f1'].mean())
                    if (i == 2):
                        r6.append(cv_results['test_f1'].mean())
                    if (i == 4):
                        r1.append(cv_results['test_f1'].mean())
                    if (i == 8):
                        r2.append(cv_results['test_f1'].mean())
                    if (i == 16):
                        r3.append(cv_results['test_f1'].mean())
                    if (i == 32):
                        r4.append(cv_results['test_f1'].mean())
                    if (i == 48):
                        r5.append(cv_results['test_f1'].mean())
                    if (i == 64):
                        r6.append(cv_results['test_f1'].mean())
                    names.append(name)
                    print('%s: %f (%f)' % (
                        name, (cv_results['test_acc']).mean(), (cv_results['test_acc']).std()))
                    print('precision: %f (%f)' % (
                        (cv_results['test_prec']).mean(), (cv_results['test_prec']).std()))
                    print('recall: %f (%f)' % (
                        (cv_results['test_rec']).mean(), (cv_results['test_rec']).std()))
                    print('f1: %f (%f)' %
                        ((cv_results['test_f1']).mean(), (cv_results['test_f1']).std()))
                else:
                    kfold = StratifiedKFold(
                        n_splits=10, random_state=1, shuffle=True)
                    if (name == 'KNN'):
                        cv_results = cross_validate(
                            model, X_tra, Y_tra, cv=kfold, scoring=scoring,n_jobs=4)
                    else:
                    # cv_results = cross_val_score(model, X_train, Y_train, cv=kfold, scoring='accuracy')
                        cv_results = cross_validate(
                            model, X_train, Y_train, cv=kfold, scoring=scoring,n_jobs=4)
                    # results.append(cv_results)
                    results_ac.append(cv_results['test_acc'])
                    results_pr.append(cv_results['test_prec'])
                    results_re.append(cv_results['test_rec'])
                    results_f1.append(cv_results['test_f1'])
                    ra.append(cv_results['test_acc'].mean())
                    rp.append(cv_results['test_prec'].mean())
                    rre.append(cv_results['test_rec'].mean())
                    rf1.append(cv_results['test_f1'].mean())
                    if (i == 1):
                        r5.append(cv_results['test_f1'].mean())
                    if (i == 2):
                        r6.append(cv_results['test_f1'].mean())
                    if (i == 4):
                        r1.append(cv_results['test_f1'].mean())
                    if (i == 8):
                        r2.append(cv_results['test_f1'].mean())
                    if (i == 16):
                        r3.append(cv_results['test_f1'].mean())
                    if (i == 32):
                        r4.append(cv_results['test_f1'].mean())
                    if (i == 48):
                        r5.append(cv_results['test_f1'].mean())
                    if (i == 64):
                        r6.append(cv_results['test_f1'].mean())
                    names.append(name)
                    print('%s: %f (%f)' % (
                        name, (cv_results['test_acc']).mean(), (cv_results['test_acc']).std()))
                    print('precision: %f (%f)' % (
                        (cv_results['test_prec']).mean(), (cv_results['test_prec']).std()))
                    print('recall: %f (%f)' % (
                        (cv_results['test_rec']).mean(), (cv_results['test_rec']).std()))
                    print('f1: %f (%f)' %
                        ((cv_results['test_f1']).mean(), (cv_results['test_f1']).std()))
                    # print('%s: %f (%f)' % (name, cv_results.mean(), cv_results.std()))


            # Complete figure
            # plt.figure(figsize=(15, 6))
            # x=range(0,len(names))
            # plt.plot(x,ra)
            # plt.plot(x,rp)
            # plt.plot(x,rre)
            # plt.plot(x,rf1)
            # plt.xticks(x, names)
            # plt.legend(["Accuracy", "Precision","Recall","F1"],loc='best')
            # plt.ylabel('Percentage')
            # plt.tight_layout()
            # #plt.show()
            # filor = str(filein).split('.')
            # figname = str(filor[0])+'_w_'+str(i)+'.png'
            # plt.savefig(figname)
            # #plt.show()

        print("Final results")
        print(r5)
        print(r6)
        print(r1)
        print(r2)
        print(r3)
        print(r4)

        test_names = ['Threshold','Logistic \nRegression','Lineal \nDiscriminant \nAnalysis','KNN','Decission \nTree','Random \nForest','Gaussian\n Naive \n Bayes','Ada Boost \nClassifier','Neural \nNetwork','SVM']

        k = np.arange(len(test_names))  # the label locations
        width = 0.3 # the width of the bars

        fig, ax = plt.subplots()
        rects5 = ax.bar(k - 5*(width/6), r5, width/3,label='1')
        rects6 = ax.bar(k - 3*(width/6), r6, width/3,label='2')
        rects1 = ax.bar(k - (width/6), r1, width/3,label='4')
        rects2 = ax.bar(k + (width/6), r2, width/3,label='8')
        rects3 = ax.bar(k + 3*(width/6), r3, width/3,label='16')
        rects4 = ax.bar(k + 5*(width/6), r4, width/3,label='32')

        #rects11 = ax.bar(k + 9*width/24, r11, width/12,label='24')
        #rects12 = ax.bar(k + 11*width/24, r12, width/12,label='26')

        # Add some text for labels, title and custom x-axis tick labels, etc.
        ax.set_ylabel('Percentage')
        ax.set_title('F1 scores')
        ax.set_xticks(k)
        ax.set_xticklabels(test_names)
        ax.set_ylim([0.5, 1])
        ax.legend()

        #ax.bar_label(rects1, padding=3)
        #ax.bar_label(rects2, padding=3)

        #plt.grid(True)
        fig.tight_layout()
        filor = str(filein).split('.')
        figname = str(filor[0])+'_w_'+str(i)+'.png'
        plt.savefig(figname)
        #plt.savefig('misses_saved.eps', bbox_inches = "tight")
        #plt.show()


if __name__ == "__main__":
    main()
