/**
 * Web Audio engine integration
 */

export class AudioEngine {
    constructor() {
        this.audioContext = null;
        this.workletNode = null;
        this.analyser = null;
        this.initialized = false;
    }

    async init() {
        // Create audio context
        this.audioContext = new (window.AudioContext || window.webkitAudioContext)({
            sampleRate: 48000,
            latencyHint: 'interactive'
        });

        console.log('AudioContext created:', this.audioContext.sampleRate, 'Hz');

        // Load and register AudioWorklet
        try {
            await this.audioContext.audioWorklet.addModule('synth-processor.js');
            console.log('AudioWorklet module loaded');
        } catch (error) {
            console.error('Failed to load AudioWorklet:', error);
            throw new Error('AudioWorklet not supported or failed to load');
        }

        // Create worklet node
        this.workletNode = new AudioWorkletNode(this.audioContext, 'synth-processor', {
            numberOfInputs: 0,
            numberOfOutputs: 1,
            outputChannelCount: [2]
        });

        // Create analyser for visualization
        this.analyser = this.audioContext.createAnalyser();
        this.analyser.fftSize = 2048;

        // Connect: worklet -> analyser -> destination
        this.workletNode.connect(this.analyser);
        this.analyser.connect(this.audioContext.destination);

        // Setup message handler
        this.workletNode.port.onmessage = (event) => {
            this.handleWorkletMessage(event.data);
        };

        // Initialize WASM in worklet
        this.workletNode.port.postMessage({
            type: 'init',
            data: {}
        });

        this.initialized = true;

        console.log('Audio engine initialized');
    }

    handleWorkletMessage(message) {
        const { type, data, error } = message;

        switch (type) {
            case 'ready':
                console.log('Worklet ready');
                break;

            case 'initialized':
                console.log('WASM synth initialized in worklet');
                break;

            case 'error':
                console.error('Worklet error:', error);
                break;
        }
    }

    handleMidi(data) {
        if (!this.initialized || !this.workletNode) return;

        this.workletNode.port.postMessage({
            type: 'midi',
            data: Array.from(data)
        });
    }

    // DCO parameter methods
    setSawLevel(level) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setSawLevel', data: level });
    }

    setPulseLevel(level) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setPulseLevel', data: level });
    }

    setSubLevel(level) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setSubLevel', data: level });
    }

    setNoiseLevel(level) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setNoiseLevel', data: level });
    }

    setPulseWidth(width) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setPulseWidth', data: width });
    }

    setPwmDepth(depth) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setPwmDepth', data: depth });
    }

    setLfoTarget(target) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setLfoTarget', data: target });
    }

    setLfoRate(rate) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setLfoRate', data: rate });
    }

    setDetune(cents) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setDetune', data: cents });
    }

    setDriftEnabled(enabled) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setDriftEnabled', data: enabled });
    }

    // Legacy methods (for compatibility)
    setFrequency(freq) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setFrequency', data: freq });
    }

    getInfo() {
        return {
            sampleRate: this.audioContext.sampleRate,
            bufferSize: 128, // Worklet uses 128 by default
            latency: this.audioContext.baseLatency * 1000 // Convert to ms
        };
    }

    getAnalyser() {
        return this.analyser;
    }

    shutdown() {
        if (this.audioContext) {
            this.audioContext.close();
        }
    }
}
