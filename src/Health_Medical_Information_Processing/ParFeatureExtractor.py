import numpy as np
from getFeatures import *
import warnings
from tqdm import tqdm


def ParFeatureExtractor(emg, stimulus, repetition, deadzone, winsize, wininc, featFunc, ker=1):
    datawin = np.ones([winsize, 1])
    numwin = emg.shape[0]  # 1797052
    nSignals = emg.shape[1]  # 12
    edges = np.linspace(-3, 3, 21)
    # print(datawin, numwin, nSignals, edges)

    # ----------------------------------------------------------------
    # allocate memory
    if featFunc == 'getHISTfeat':
        emg = np.divide((emg - np.tile(np.mean(emg, axis=0), (emg.shape[0], 1))),
                        np.tile(np.std(emg, axis=0), (emg.shape[0], 1)))
        # print(emg[0])
        feat = np.zeros([numwin, nSignals * len(edges)])
    elif featFunc == 'getTDfeat':
        feat = np.zeros([numwin, nSignals * 5])
    elif featFunc == 'getmDWTfeat':
        feat = np.zeros([numwin, nSignals * 4])  # 4 = (Level DWT+1)
    else:
        feat = np.zeros([numwin, nSignals])
    featStim = np.zeros([numwin, 1])
    featRep = np.zeros([numwin, 1])
    checkStimRep = np.zeros([numwin, 1])

    # ---------------------------------------------------------------
    # compute feat, featStim & featRep
    for winInd in tqdm(range(numwin - winsize)):
        if winInd % wininc == 0:
            curStimWin = stimulus[winInd:winInd + winsize + 1]
            curRepWin = repetition[winInd:winInd + winsize + 1]

            if len(np.unique(curStimWin)) == 1 and len(np.unique(curRepWin)) == 1:  # 确保每个时间窗只有一种标签
                checkStimRep[winInd] = 1
                featStim[winInd] = curStimWin[0]
                featRep[winInd] = curRepWin[0]

                curwin = emg[winInd:winInd + winsize]
                # print(curwin.shape)

                if featFunc == 'getrmsfeat':
                    # GETRMSFEAT Gets the RMS feature (Root Mean Square).
                    feat[winInd] = getrmsfeat(curwin, winsize, wininc)
                elif featFunc == 'getTDfeat':
                    # Compute Time Domain features according to HUDGINS, A New Strategy for Multifunction Myoelectric
                    # Control
                    feat[winInd] = getTDfeat(curwin, deadzone, winsize, wininc)
                elif featFunc == 'getmavfeat':
                    # GETMAVFEAT Gets the MAV feature (Mean Absolute Value).
                    feat[winInd] = getmavfeat(curwin, winsize, wininc, datawin)
                elif featFunc == 'getzcfeat':
                    # GETZCFEAT Gets the ZC feature (zero crossing).
                    feat[winInd] = getzcfeat(curwin, deadzone, winsize, wininc, datawin)
                elif featFunc == 'getsscfeat':
                    # GETSSCFEAT Gets the slope sign change feature.
                    feat[winInd] = getsscfeat(curwin, deadzone, winsize, wininc, datawin)
                elif featFunc == 'getwlfeat':
                    # GETWLFEAT Gets the waveform length feature.
                    feat[winInd] = getwlfeat(curwin, winsize, wininc, datawin)
                elif featFunc == 'getarfeat':
                    # GETARFEAT Gets the AR feature (autoregressive).
                    order = 1
                    feat[winInd] = getarfeat(curwin, order, winsize, wininc, datawin)
                elif featFunc == 'getiavfeat':
                    # GETIAVFEAT Gets the IAV feature (Integrated Absolute Value).
                    feat[winInd] = getiavfeat(curwin, winsize, wininc, datawin)
                elif featFunc == 'getHISTfeat':
                    # getHISTfeat Gets the histogram feature as described in Ninapro papers
                    feat[winInd] = getHISTfeat(curwin, winsize, wininc, edges)
                elif featFunc == 'getmDWTfeat':
                    feat[winInd] = getmDWTfeat(curwin, winsize, wininc)
                else:
                    warnings.warn('feature not yet implemented in FeatureExtractor')

    # --------------------------------------------------------------------------
    # Remove features that correspond to windows without unique stimulus and repetition
    z = np.where(checkStimRep == 0)[0]
    # print(sum(checkStimRep))
    feat = np.delete(feat, z, axis=0)
    featStim = np.delete(featStim, z, axis=0)
    featRep = np.delete(featRep, z, axis=0)
    return feat, featStim, featRep
