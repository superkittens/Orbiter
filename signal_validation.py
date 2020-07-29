import numpy as np
import matplotlib.pyplot as plt
import h5py
import soundfile as sf

%matplotlib qt

#%%
def createFadeInOutEnvelope(length, fadeInOut):
	envelope = np.zeros(length)

	for i in range(0, length):
		if fadeInOut == 'fade_in':
			envelope[i] = np.power(np.sin(i * np.pi / (2 * length)), 2)

		if fadeInOut == 'fade_out':
			envelope[i] = np.power(np.cos(i * np.pi / (2 * length)), 2)

	return envelope


def overlapAndAdd(olaBuffer, x, writeIndex, numSamplesToWrite):

	x_index = 0
	ola_index = writeIndex
	buffer_size = len(olaBuffer)

	for i in range(writeIndex, writeIndex + numSamplesToWrite):

		if ola_index >= buffer_size:
			ola_index = 0

		olaBuffer[ola_index] += x[x_index]
		x_index += 1
		ola_index += 1

	return olaBuffer



#	HRTFProcessor sample HRIR and audio input
#%%
hrir_length = 512
audio_block_length = 256
zero_padded_exponent = int(np.log2(hrir_length + audio_block_length)) + 1
zero_padded_length = np.power(2, zero_padded_exponent)


hrir_impulse = np.zeros(hrir_length)
hrir_impulse[0] = 1.0

hrir_dc = np.ones(hrir_length)

fade_out_envelope = createFadeInOutEnvelope(audio_block_length, fadeInOut='fade_out')
fade_in_envelope = createFadeInOutEnvelope(audio_block_length, fadeInOut='fade_in')

hrir_impulse_zp = np.zeros(zero_padded_length)
hrir_impulse_zp[0:hrir_length] = hrir_impulse

hrir_dc_zp = np.zeros(zero_padded_length)
hrir_dc_zp[0:hrir_length] = hrir_dc

x = np.zeros(zero_padded_length)
x[0] = 1.0

ola_buffer = np.zeros(zero_padded_length)


#	This block corresponds to the "HRTF Appplication" test in HRTFProcessorTest
#	ie. feed an impulse signal into the HRTFProcessor
#	The result from HRTFProessor and this script should match
y = np.fft.ifft(np.fft.fft(x) * np.fft.fft(hrir_impulse_zp))

ola_buffer = overlapAndAdd(ola_buffer, y, 0, zero_padded_length)


#	This block corresponds to the "Changing HRTF" test
y_old = np.fft.ifft(np.fft.fft(x) * np.fft.fft(hrir_impulse_zp))
y_new = np.fft.ifft(np.fft.fft(x) * np.fft.fft(hrir_dc_zp))

y_old[0:audio_block_length] = y_old[0:audio_block_length] * fade_out_envelope
y_new[0:audio_block_length] = y_new[0:audio_block_length] * fade_in_envelope

y = y_old + y_new

ola_buffer = overlapAndAdd(ola_buffer, y, audio_block_length, zero_padded_length - audio_block_length)



#	This block corresponds to "Regular Operation" test in HRTFProcessorTest
#%%
f0 = 1000
fs = 44100

x = np.sin(np.arange(0, 2048, 1) * 2 * np.pi * f0 / fs)

ola_buffer = np.zeros(zero_padded_length)

for block in range(0, 3):
	x_zp = np.zeros(zero_padded_length)
	x_zp[0 : audio_block_length] = x[block * audio_block_length : block * audio_block_length + audio_block_length]
	
	y = np.fft.ifft(np.fft.fft(x_zp) * np.fft.fft(hrir_impulse_zp))

	ola_buffer = overlapAndAdd(ola_buffer, y, block * audio_block_length, zero_padded_length - (audio_block_length * block))



#	Test using a real HRTF
#%%
sofa_filepath = '/Users/superkittens/projects/sound_prototypes/hrtf/hrtfs/BRIRs_from_a_room/B/002.sofa'

f = h5py.File(sofa_filepath, 'r')

Fs = f['Data.SamplingRate']
Ns = int(np.size(f['N']))
EPos = f['EmitterPosition']
Azimuth = f['SourcePosition']
HRIR = f['Data.IR']

hrir_index = 54

hrir_left = HRIR[hrir_index, 0, :]
hrir_right = HRIR[hrir_index, 1, :]

num_samples = 131072
N = 16384
M = 15000
ola = np.zeros((2,N))
test_signal = np.sin(np.arange(0, num_samples, 1) * 2 * np.pi * 1000 / 44100)
output = np.zeros((2,num_samples + N))

num_blocks = int(num_samples / audio_block_length)
ola_index = 0

for block in range(0, num_blocks):
	print(block)

	x = np.zeros(N)
	h_left = np.zeros(N)
	h_right = np.zeros(N)

	x[0 : audio_block_length] = test_signal[block * audio_block_length : (block * audio_block_length) + audio_block_length]
	h_left[0 : M] = hrir_left[0 : M]
	h_right[0 : M] = hrir_right[0 : M]

	y_left = np.fft.ifft(np.fft.fft(x, N) * np.fft.fft(h_left, N))
	y_right = np.fft.ifft(np.fft.fft(x, N) * np.fft.fft(h_right, N))

	for i in range(ola_index, ola_index + audio_block_length):
		ola[0][i] = 0
		ola[1][i] = 0

	ola_index += audio_block_length

	if ola_index >= N:
		ola_index = 0

	ola[0] = overlapAndAdd(ola[0], y_left, ola_index, N)
	ola[1] = overlapAndAdd(ola[1], y_right, ola_index, N)

	output[0][block * audio_block_length : (block * audio_block_length) + audio_block_length] = ola[0][ola_index : ola_index + audio_block_length]
	output[1][block * audio_block_length : (block * audio_block_length) + audio_block_length] = ola[1][ola_index : ola_index + audio_block_length]



sf.write('/Users/superkittens/projects/juce_projects/Orbiter/validation.wav', output.T, 44100)





