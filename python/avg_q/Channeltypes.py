# Copyright (C) 2012-2014,2016-2018 Bernd Feige
# This file is part of avg_q and released under the GPL v3 (see avg_q/COPYING).
"""
Utility module defining global default channel sets
"""

import collections
# This is for finding EEG channels given a generic name such as 'C3'
# Note that the order is significant: The first occurrance is preferred
# in case of multiple matches.
equivalent_channels=collections.OrderedDict({
 'EEG C3': 'C3',
 'EEG C4': 'C4',
 'EEG Fz': 'Fz',
 'EEG Cz': 'Cz',
 'EEG Pz': 'Pz',
 'EEG Oz': 'Oz',
 'EEG_C3': 'C3',
 'EEG_C4': 'C4',
 # Two ESVO files and two tDCS files...
 'EEGC3': 'C3',
 'EEGC4': 'C4',
 'EEGFz': 'Fz',
 'EEGCz': 'Cz',
 'EEGPz': 'Pz',
 'EEGOz': 'Oz',
 'C3A2': 'C3',
 'C4A1': 'C4',
 'O1A2': 'O1',
 'O2A1': 'O2',
 'EEG C3-A2': 'C3',
 'EEG C4-A1': 'C4',
 'EEG Fz-A1A2': 'Fz',
 'EEG Cz-A1A2': 'Cz',
 'EEG Pz-A1A2': 'Pz',
 'EEG C3-A1+A2': 'C3',
 'EEG C4-A1+A2': 'C4',
 'C3*': 'C3',
 'C4*': 'C4',
 'M1': 'A1',
 'M2': 'A2',
 'FP1': 'Fp1',
 'FP2': 'Fp2',
 # Pneumologie...
 'EEG1': 'C3',
 'EEG2': 'C4',
})

# Unipolar EOG channels may qualify as EEG
BipolarEOGchannels=set([
 'Eog',
 'EOG',
 'VEOG',
 'HEOG',
 'EOG LOC',
 'EOG ROC',
 'EOGV',
 'EOGl:M2',
 'EOGr:M1',
 'EOGl',
 'EOGr',
])
ECGchannels=set([
 'Ekg1','Ekg2',
 'Ekg',
 'EKG',
 'ECG',
 'ECG 2',
])
EMGchannels=set([
 'EMG',
 'EMG1',
 'EMG2',
 'EMG3',
 'EMG4',
 'EMG_L',
 'EMG_R',
 'BEMG1',
 'BEMG2',
 'EMG EMGchin',
 'EMG EMGTible',
 'EMG EMGTibri',
 'EMGchin',
 'PLMr',
 'PLMl',
])
AUXchannels=set([
 'EDA',
 'EDAShape',
 'GSR_MR_100_EDA',
 'EVT',
 'Airflow',
 'Flow Th',
 'Thorax',
 'Abdomen',
 'Plethysmogram',
 'Pleth',
 'SpO2',
 'SPO2',
 'Pulse',
 'Snoring',
 'Lage',
 'Licht',
 'Akku',
 'Beweg.',
 'AUX',
 'Summe Effort',
])
NonEEGChannels=set.union(BipolarEOGchannels,ECGchannels,EMGchannels,AUXchannels)
