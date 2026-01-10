/**
 * Web Audio AudioWorklet processor
 * This runs in the audio thread and calls into WASM
 */

import createSynthProcessor from './synth-processor.js';

// Import the WASM module
let wasmModule = null;
let synthInstance = null;

class SynthProcessor extends AudioWorkletProcessor {
    constructor(options) {
        super();

        // Port for receiving messages from main thread
        this.port.onmessage = this.handleMessage.bind(this);

        // Buffer pointers (will be allocated in WASM heap)
        this.leftPtr = 0;
        this.rightPtr = 0;
        this.bufferSize = 128;

        // Signal that we're ready
        this.port.postMessage({ type: 'ready' });
    }

    handleMessage(event) {
        const { type, data } = event.data;

        if (!synthInstance && type !== 'init') return;

        switch (type) {
            case 'init':
                this.initWasm(data);
                break;

            case 'midi':
                if (data.length >= 3) {
                    synthInstance.handleMidi(data[0], data[1], data[2]);
                }
                break;

            // DCO parameters
            case 'setSawLevel':
                synthInstance.setSawLevel(data);
                break;

            case 'setPulseLevel':
                synthInstance.setPulseLevel(data);
                break;

            case 'setSubLevel':
                synthInstance.setSubLevel(data);
                break;

            case 'setNoiseLevel':
                synthInstance.setNoiseLevel(data);
                break;

            case 'setPulseWidth':
                synthInstance.setPulseWidth(data);
                break;

            case 'setPwmDepth':
                synthInstance.setPwmDepth(data);
                break;

            case 'setLfoTarget':
                synthInstance.setLfoTarget(data);
                break;

            case 'setLfoRate':
                synthInstance.setLfoRate(data);
                break;

            case 'setLfoDelay':
                synthInstance.setLfoDelay(data);
                break;

            case 'setDetune':
                synthInstance.setDetune(data);
                break;

            case 'setDriftEnabled':
                synthInstance.setDriftEnabled(data);
                break;

            // Filter parameters
            case 'setFilterCutoff':
                synthInstance.setFilterCutoff(data);
                break;

            case 'setFilterResonance':
                synthInstance.setFilterResonance(data);
                break;

            case 'setFilterEnvAmount':
                synthInstance.setFilterEnvAmount(data);
                break;

            case 'setFilterLfoAmount':
                synthInstance.setFilterLfoAmount(data);
                break;

            case 'setFilterKeyTrack':
                synthInstance.setFilterKeyTrack(data);
                break;

            // M11: HPF parameter
            case 'setFilterHpfMode':
                synthInstance.setFilterHpfMode(data);
                break;

            // Filter envelope parameters
            case 'setFilterEnvAttack':
                synthInstance.setFilterEnvAttack(data);
                break;

            case 'setFilterEnvDecay':
                synthInstance.setFilterEnvDecay(data);
                break;

            case 'setFilterEnvSustain':
                synthInstance.setFilterEnvSustain(data);
                break;

            case 'setFilterEnvRelease':
                synthInstance.setFilterEnvRelease(data);
                break;

            // Amplitude envelope parameters
            case 'setAmpEnvAttack':
                synthInstance.setAmpEnvAttack(data);
                break;

            case 'setAmpEnvDecay':
                synthInstance.setAmpEnvDecay(data);
                break;

            case 'setAmpEnvSustain':
                synthInstance.setAmpEnvSustain(data);
                break;

            case 'setAmpEnvRelease':
                synthInstance.setAmpEnvRelease(data);
                break;

            // Chorus parameters
            case 'setChorusMode':
                synthInstance.setChorusMode(data);
                break;

            // M11: Performance parameters
            case 'setPitchBendRange':
                synthInstance.setPitchBendRange(data);
                break;

            case 'setPortamentoTime':
                synthInstance.setPortamentoTime(data);
                break;

            // M13: Performance parameters
            case 'setModWheel':
                synthInstance.setModWheel(data);
                break;

            case 'setVcaMode':
                synthInstance.setVcaMode(data);
                break;

            case 'setFilterEnvPolarity':
                synthInstance.setFilterEnvPolarity(data);
                break;

            // M14: Range & Voice Control parameters
            case 'setDcoRange':
                synthInstance.setDcoRange(data);
                break;

            case 'setVcaLevel':
                synthInstance.setVcaLevel(data);
                break;

            case 'setMasterTune':
                synthInstance.setMasterTune(data);
                break;

            case 'setVelocityToFilter':
                synthInstance.setVelocityToFilter(data);
                break;

            case 'setVelocityToAmp':
                synthInstance.setVelocityToAmp(data);
                break;

            // M16: Voice Allocation Mode
            case 'setVoiceAllocationMode':
                synthInstance.setVoiceAllocationMode(data);
                break;

            // Legacy
            case 'setFrequency':
                synthInstance.setFrequency(data);
                break;

            case 'setNoteOn':
                synthInstance.setNoteOn(data);
                break;
        }
    }

    async initWasm(moduleData) {
        try {
            console.log('AudioWorklet: Initializing WASM module...');
            
            // createSynthProcessor is the default export from synth-processor.js
            // We provide a locateFile function to ensure the WASM file is found
            wasmModule = await createSynthProcessor({
                locateFile: (path, prefix) => {
                    // Force the loader to look in the same directory as the worklet
                    return path;
                }
            });
            
            console.log('AudioWorklet: WASM module initialized');

            // Create synth instance
            synthInstance = new wasmModule.WebSynth(sampleRate);
            console.log('AudioWorklet: Synth instance created, sampleRate:', sampleRate);

            // Allocate buffers in WASM heap
            this.leftPtr = wasmModule._malloc(this.bufferSize * 4);  // 4 bytes per float
            this.rightPtr = wasmModule._malloc(this.bufferSize * 4);
            console.log('AudioWorklet: Memory allocated in WASM heap');

            this.port.postMessage({ type: 'initialized' });
        } catch (error) {
            console.error('AudioWorklet: Failed to initialize WASM:', error);
            this.port.postMessage({ type: 'error', error: error.message });
        }
    }

    process(inputs, outputs, parameters) {
        if (!synthInstance) {
            // Not initialized yet, output silence
            return true;
        }

        const output = outputs[0];
        const leftChannel = output[0];
        const rightChannel = output[1];

        // Call WASM synth to fill buffers
        synthInstance.process(this.leftPtr, this.rightPtr, leftChannel.length);

        // Copy from WASM heap to output buffers
        const leftHeap = new Float32Array(
            wasmModule.HEAPF32.buffer,
            this.leftPtr,
            leftChannel.length
        );
        const rightHeap = new Float32Array(
            wasmModule.HEAPF32.buffer,
            this.rightPtr,
            rightChannel.length
        );

        leftChannel.set(leftHeap);
        rightChannel.set(rightHeap);

        return true;
    }
}

registerProcessor('synth-processor', SynthProcessor);
