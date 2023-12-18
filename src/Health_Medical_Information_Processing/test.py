import scipy.io as scio
from ParFeatureExtractor import *


data = scio.loadmat('E:\\课\\健康与医学信息处理\\期末作业\\PaWFE---Parallel-Window-Feature-Extraction-master\\2_FeatureExtraction'
                    '\\S1_E1_A1.mat')
feat, featStim, featRep = ParFeatureExtractor(data['emg'], data['stimulus'], data['repetition'], 1e-5, 400, 20, 'getarfeat')
print(feat[1])
