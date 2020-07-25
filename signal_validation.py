import numpy as np
import matplotlib.pyplot as plt

%matplotlib qt

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
	buffer_size = len(olaBuffer)

	for i in range(writeIndex, writeIndex + numSamplesToWrite):
		olaBuffer[i] += x[x_index]
		x_index += 1

	return olaBuffer



#	HRTFProcessor sample HRIR and audio input
#%%
hrir_length = 512
audio_block_length = 256
zero_padded_exponent = int(np.log2(hrir_length + audio_block_length)) + 1
zero_padded_length = np.power(2, zero_packed_exponent)


hrir_impulse = np.zeros(hrir_length)
hrir_impulse[int(hrir_length / 2) - 1] = 1.0

hrir_dc = np.ones(hrir_length)

fade_out_envelope = createFadeInOutEnvelope(audio_block_length, fadeInOut='fade_out')
fade_in_envelope = createFadeInOutEnvelope(audio_block_length, fadeInOut='fade_in')

hrir_impulse_zp = np.zeros(zero_packed_length)
hrir_impulse_zp[0:hrir_length] = hrir_impulse

hrir_dc_zp = np.zeros(zero_packed_length)
hrir_dc_zp[0:hrir_length] = hrir_dc

x = np.zeros(zero_packed_length)
x[0] = 1.0

ola_buffer = np.zeros(zero_packed_length)


#	This block corresponds to the "HRTF Appplication" test in HRTFProcessorTest
#	ie. feed an impulse signal into the HRTFProcessor
#	The result from HRTFProessor and this script should match
y = np.fft.ifft(np.fft.fft(x) * np.fft.fft(hrir_impulse_zp))

ola_buffer = overlapAndAdd(ola_buffer, y, 0, zero_packed_length)


#	This block corresponds to the "Changing HRTF" test
y_old = np.fft.ifft(np.fft.fft(x) * np.fft.fft(hrir_impulse_zp))
y_new = np.fft.ifft(np.fft.fft(x) * np.fft.fft(hrir_dc_zp))

y_old[0:audio_block_length] = y_old[0:audio_block_length] * fade_out_envelope
y_new[0:audio_block_length] = y_new[0:audio_block_length] * fade_in_envelope

y = y_old + y_new

ola_buffer = overlapAndAdd(ola_buffer, y, audio_block_length, zero_packed_length - audio_block_length)

plt.plot(ola_buffer)









