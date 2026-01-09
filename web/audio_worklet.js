/**
 * Web Audio AudioWorklet processor
 * This runs in the audio thread and calls into WASM
 */

// Import the WASM module
// This will be loaded when the worklet is registered
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
            // Load WASM module
            const Module = await import('./synth-processor.js');
            wasmModule = await Module.default();

            // Create synth instance
            synthInstance = new wasmModule.WebSynth(sampleRate);

            // Allocate buffers in WASM heap
            this.leftPtr = wasmModule._malloc(this.bufferSize * 4);  // 4 bytes per float
            this.rightPtr = wasmModule._malloc(this.bufferSize * 4);

            this.port.postMessage({ type: 'initialized' });
        } catch (error) {
            console.error('Failed to initialize WASM:', error);
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
