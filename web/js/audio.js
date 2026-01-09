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

        // Resume audio context (required by browsers for user-initiated audio)
        await this.audioContext.resume();
        console.log('AudioContext resumed, state:', this.audioContext.state);

        // Load and register AudioWorklet
        try {
            await this.audioContext.audioWorklet.addModule('audio_worklet.js');
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

    // Filter parameter methods
    setFilterCutoff(cutoff) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setFilterCutoff', data: cutoff });
    }

    setFilterResonance(resonance) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setFilterResonance', data: resonance });
    }

    setFilterEnvAmount(amount) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setFilterEnvAmount', data: amount });
    }

    setFilterLfoAmount(amount) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setFilterLfoAmount', data: amount });
    }

    setFilterKeyTrack(mode) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setFilterKeyTrack', data: mode });
    }

    // M11: HPF parameter method
    setFilterHpfMode(mode) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setFilterHpfMode', data: mode });
    }

    // Filter envelope parameter methods
    setFilterEnvAttack(attack) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setFilterEnvAttack', data: attack });
    }

    setFilterEnvDecay(decay) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setFilterEnvDecay', data: decay });
    }

    setFilterEnvSustain(sustain) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setFilterEnvSustain', data: sustain });
    }

    setFilterEnvRelease(release) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setFilterEnvRelease', data: release });
    }

    // Amplitude envelope parameter methods
    setAmpEnvAttack(attack) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setAmpEnvAttack', data: attack });
    }

    setAmpEnvDecay(decay) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setAmpEnvDecay', data: decay });
    }

    setAmpEnvSustain(sustain) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setAmpEnvSustain', data: sustain });
    }

    setAmpEnvRelease(release) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setAmpEnvRelease', data: release });
    }

    // Chorus parameter methods
    setChorusMode(mode) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setChorusMode', data: mode });
    }

    // M11: Performance parameter methods
    setPitchBendRange(semitones) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setPitchBendRange', data: semitones });
    }

    setPortamentoTime(seconds) {
        if (!this.initialized || !this.workletNode) return;
        this.workletNode.port.postMessage({ type: 'setPortamentoTime', data: seconds });
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
