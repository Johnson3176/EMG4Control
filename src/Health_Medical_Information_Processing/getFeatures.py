import numpy as np
from scipy.signal import lfilter
import pywt
import librosa


def getHISTfeat(x, winsize=None, wininc=None, edges=None):
    if winsize is None:
        winsize = x.shape[0]
    if wininc is None:
        wininc = winsize
    if edges is None:
        edges = np.linspace(-3, 3, 21)

    datasize = x.shape[0]
    Nsignals = x.shape[1]
    numwin = int(np.floor((datasize - winsize) / wininc) + 1)

    # allocate memory
    feat = np.zeros([numwin, Nsignals*len(edges)])

    st = 0
    en = winsize

    for i in range(numwin):
        curwin = x[st:en] * np.tile(np.ones([winsize, 1]), (1, Nsignals))
        # histc
        F0 = np.zeros([Nsignals, len(edges)])
        for j in range(Nsignals):
            F0[j, 0:-1] = np.histogram(curwin[:, j].reshape(-1), edges)[0]

        feat[i] = F0.reshape(-1)

        st = st + wininc
        en = en + wininc

    return feat


def getiavfeat(x, winsize=None, wininc=None, datawin=None):
    if winsize is None:
        winsize = x.shape[0]
    if wininc is None:
        wininc = winsize
    if datawin is None:
        datawin = np.ones([winsize, 1])

    datasize = x.shape[0]
    Nsignals = x.shape[1]
    numwin = int(np.floor((datasize - winsize) / wininc) + 1)

    # allocate memory
    feat = np.zeros([numwin, Nsignals])

    st = 0
    en = winsize

    for i in range(numwin):
        curwin = x[st:en] * np.tile(datawin, (1, Nsignals))
        feat[i] = np.sum(abs(curwin), axis=0)

        st = st + wininc
        en = en + wininc

    return feat


def getmavfeat(x, winsize=None, wininc=None, datawin=None):
    if winsize is None:
        winsize = x.shape[0]
    if wininc is None:
        wininc = winsize
    if datawin is None:
        datawin = np.ones([winsize, 1])

    datasize = x.shape[0]
    Nsignals = x.shape[1]
    numwin = int(np.floor((datasize - winsize) / wininc) + 1)

    # allocate memory
    feat = np.zeros([numwin, Nsignals])

    st = 0
    en = winsize

    for i in range(numwin):
        curwin = x[st:en] * np.tile(datawin, (1, Nsignals))
        feat[i] = np.mean(abs(curwin), axis=0)

        st = st + wininc
        en = en + wininc

    return feat


def getmavsfeat(x, winsize=None, wininc=None, datawin=None):
    if winsize is None:
        winsize = x.shape[0]
    if wininc is None:
        wininc = winsize
    if datawin is None:
        datawin = np.ones([winsize, 1])

    datasize = x.shape[0]
    Nsignals = x.shape[1]
    numwin = int(np.floor((datasize - winsize) / wininc) + 1)

    # allocate memory
    feat = np.zeros([numwin, Nsignals])

    st = 0
    en = winsize

    for i in range(numwin-1):
        curwin = x[st:en] * np.tile(datawin, (1, Nsignals))
        curwinSucc = x[st + wininc:en + wininc] * np.tile(datawin, (1, Nsignals))

        feat[i] = np.mean(abs(curwinSucc), axis=0) - np.mean(abs(curwin), axis=0)

        st = st + wininc
        en = en + wininc

    return feat


def getmDWTfeat(x, winsize=None, wininc=None):
    if winsize is None:
        winsize = x.shape[0]
    if wininc is None:
        wininc = winsize

    datasize = x.shape[0]
    Nsignals = x.shape[1]
    numwin = int(np.floor((datasize - winsize) / wininc) + 1)

    # allocate memory
    feat = np.zeros([numwin, Nsignals*4])

    st = 0
    en = winsize

    for i in range(numwin):
        curwin = x[st:en]

        # -----------------
        # mDWT NEW NEW
        m_xk = np.zeros((curwin.shape[1], 4))
        for colInd in range(curwin.shape[1]):
            C = pywt.wavedec(curwin[:, colInd], 'db7', mode='sym', level=3)

            sReal = [0, 3, 2, 1]

            for s in range(4):
                d_xk = C[s]
                MaxSum = int(min(np.ceil(x.shape[0]/(pow(2, sReal[s])-1 if sReal[s] != 0 else 1e-5)), d_xk.shape[0]))
                m_xk[colInd][s] = np.sum(abs(d_xk[:MaxSum]))

        feat[i] = m_xk.reshape(-1)

        st = st + wininc
        en = en + wininc

    return feat


def getrmsfeat(x, winsize=None, wininc=None, datawin=None):
    if winsize is None:
        winsize = x.shape[0]
    if wininc is None:
        wininc = winsize
    if datawin is None:
        datawin = np.ones([winsize, 1])

    datasize = x.shape[0]
    Nsignals = x.shape[1]
    numwin = int(np.floor((datasize - winsize) / wininc) + 1)

    # allocate memory
    feat = np.zeros([numwin, Nsignals])

    st = 0
    en = winsize

    for i in range(numwin):
        curwin = x[st:en] * np.tile(datawin, (1, Nsignals))
        feat[i] = np.sqrt(np.mean(np.square(curwin), axis=0))

        st = st + wininc
        en = en + wininc

    return feat


def getsscfeat(x, deadzone, winsize=None, wininc=None, datawin=None):
    if winsize is None:
        winsize = x.shape[0]
    if wininc is None:
        wininc = winsize
    if datawin is None:
        datawin = np.ones([winsize, 1])

    x = np.concatenate((np.zeros([1, x.shape[1]]), np.diff(x, axis=0)), axis=0)
    # print(x.shape)
    datasize = x.shape[0]
    Nsignals = x.shape[1]
    numwin = int(np.floor((datasize - winsize) / wininc) + 1)

    # allocate memory
    feat = np.zeros([numwin, Nsignals])

    st = 0
    en = winsize

    for i in range(numwin):
        y = x[st:en] * np.tile(datawin, (1, Nsignals))
        pos = np.reshape(y > deadzone, -1).astype(int)
        neg = np.reshape(y < -deadzone, -1).astype(int)
        y = np.reshape(pos - neg, (winsize, Nsignals))
        # print(y.shape)
        # forces the zeros towards either the positive or negative
        # the filter is chosen so that the most recent +1 or -1 has
        # the most influence on the state of the zero.
        a = 1
        b = np.exp(-np.linspace(1, int(winsize / 2), int(winsize / 2)))
        z = lfilter(b, a, y, axis=0)
        pos = np.reshape(z > 0, -1).astype(int)
        neg = np.reshape(z < 0, -1).astype(int)
        z = np.reshape(pos - neg, (winsize, Nsignals))
        dz = np.diff(z, axis=0)

        feat[i] = np.sum(np.array(abs(dz) == 2).astype(int), axis=0)

        st = st + wininc
        en = en + wininc

    return feat


def getTDfeat(x, deadzone=None, winsize=None, wininc=None, datawin=None):
    if deadzone is None:
        deadzone = 0
    if winsize is None:
        winsize = x.shape[0]
    if wininc is None:
        wininc = winsize
    if datawin is None:
        datawin = np.ones([winsize, 1])

    # ----------------
    # 1.Mean Absolute Value
    feat1 = getmavfeat(x, winsize, wininc)

    # ----------------
    # 2.Mean Absolute Value Slopej
    feat2 = getmavsfeat(x, winsize, wininc, datawin)

    # ----------------
    # 3.Zero Crossing
    feat3 = getzcfeat(x, deadzone, winsize, wininc)

    # ----------------
    # 4.Slope Sign Change
    feat4 = getsscfeat(x, deadzone, winsize, wininc)

    # ----------------
    # 5.Waveform Length (WL) (I,A)
    feat5 = getwlfeat(x, winsize, wininc)

    feat = np.concatenate((feat1, feat2, feat3, feat4, feat5), axis=1)

    return feat


def getwlfeat(x, winsize=None, wininc=None, datawin=None):
    if winsize is None:
        winsize = x.shape[0]
    if wininc is None:
        wininc = winsize
    if datawin is None:
        datawin = np.ones([winsize, 1])

    datasize = x.shape[0]
    Nsignals = x.shape[1]
    numwin = int(np.floor((datasize - winsize) / wininc) + 1)

    # allocate memory
    feat = np.zeros([numwin, Nsignals])

    st = 0
    en = winsize

    for i in range(numwin):
        curwin = x[st:en] * np.tile(datawin, (1, Nsignals))
        feat[i] = np.sum(abs(np.diff(curwin, 2, axis=0)), axis=0)

        st = st + wininc
        en = en + wininc

    return feat


def getzcfeat(x, deadzone, winsize=None, wininc=None, datawin=None):
    if winsize is None:
        winsize = x.shape[0]
    if wininc is None:
        wininc = winsize
    if datawin is None:
        datawin = np.ones([winsize, 1])

    datasize = x.shape[0]
    Nsignals = x.shape[1]
    numwin = int(np.floor((datasize - winsize) / wininc) + 1)

    # allocate memory
    feat = np.zeros([numwin, Nsignals])

    st = 0
    en = winsize

    for i in range(numwin):
        y = x[st:en] * np.tile(datawin, (1, Nsignals))
        pos = np.reshape(y > deadzone, -1).astype(int)
        neg = np.reshape(y < -deadzone, -1).astype(int)
        y = np.reshape(pos - neg, (winsize, Nsignals))
        # print(y.shape)
        # forces the zeros towards either the positive or negative
        # the filter is chosen so that the most recent +1 or -1 has
        # the most influence on the state of the zero.
        a = 1
        b = np.exp(-np.linspace(1, int(winsize/2), int(winsize/2)))
        z = lfilter(b, a, y, axis=0)
        pos = np.reshape(z > 0, -1).astype(int)
        neg = np.reshape(z < 0, -1).astype(int)
        z = np.reshape(pos - neg, (winsize, Nsignals))
        dz = np.diff(z, axis=0)

        feat[i] = np.sum(np.array(abs(dz) == 2).astype(int), axis=0)

        st = st + wininc
        en = en + wininc

    return feat


def getarfeat(x, order, winsize=None, wininc=None, datawin=None):
    if winsize is None:
        winsize = x.shape[0]
    if wininc is None:
        wininc = winsize
    if datawin is None:
        datawin = np.ones([winsize, 1])

    datasize = x.shape[0]
    Nsignals = x.shape[1]
    numwin = int(np.floor((datasize - winsize) / wininc) + 1)

    # allocate memory
    feat = np.zeros([numwin, Nsignals*order])

    st = 0
    en = winsize

    for i in range(numwin):
        curwin = x[st:en] * np.tile(datawin, (1, Nsignals))
        cur_xlpc = librosa.lpc(curwin, order=order, axis=0)
        cur_xlpc = cur_xlpc[1:order+1]
        feat[i] = cur_xlpc

        st = st + wininc
        en = en + wininc

    return feat
