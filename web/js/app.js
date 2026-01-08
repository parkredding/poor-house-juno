/**
 * Poor House Juno - Web Application
 * Main entry point for the browser-based test environment
 */

import { AudioEngine } from './audio.js';
import { MidiHandler } from './midi.js';
import { UI } from './ui.js';

class App {
    constructor() {
        this.audioEngine = null;
        this.midiHandler = null;
        this.ui = null;
        this.running = false;
    }

    async init() {
        console.log('Poor House Juno - Initializing...');

        // Initialize UI
        this.ui = new UI();
        this.ui.setStatus('Ready - Click "Start Audio" to begin');

        // Setup event listeners
        this.setupEventListeners();

        // Initialize MIDI (doesn't require user gesture)
        this.midiHandler = new MidiHandler();
        await this.midiHandler.init();

        // Update MIDI device list
        this.updateMidiDevices();

        console.log('Initialization complete');
    }

    setupEventListeners() {
        // Start Audio button
        document.getElementById('start-audio').addEventListener('click', async () => {
            await this.startAudio();
        });

        // Play/Stop note buttons
        document.getElementById('play-note').addEventListener('click', () => {
            this.playTestNote();
        });

        document.getElementById('stop-note').addEventListener('click', () => {
            this.stopTestNote();
        });

        // DCO Parameter controls
        document.getElementById('saw-level').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 100;
            document.getElementById('saw-level-value').textContent = `${e.target.value}%`;
            if (this.audioEngine) this.audioEngine.setSawLevel(value);
        });

        document.getElementById('pulse-level').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 100;
            document.getElementById('pulse-level-value').textContent = `${e.target.value}%`;
            if (this.audioEngine) this.audioEngine.setPulseLevel(value);
        });

        document.getElementById('sub-level').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 100;
            document.getElementById('sub-level-value').textContent = `${e.target.value}%`;
            if (this.audioEngine) this.audioEngine.setSubLevel(value);
        });

        document.getElementById('noise-level').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 100;
            document.getElementById('noise-level-value').textContent = `${e.target.value}%`;
            if (this.audioEngine) this.audioEngine.setNoiseLevel(value);
        });

        document.getElementById('pulse-width').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 100;
            document.getElementById('pulse-width-value').textContent = `${e.target.value}%`;
            if (this.audioEngine) this.audioEngine.setPulseWidth(value);
        });

        document.getElementById('pwm-depth').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 100;
            document.getElementById('pwm-depth-value').textContent = `${e.target.value}%`;
            if (this.audioEngine) this.audioEngine.setPwmDepth(value);
        });

        document.getElementById('detune').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value);
            document.getElementById('detune-value').textContent = value.toFixed(1);
            if (this.audioEngine) this.audioEngine.setDetune(value);
        });

        document.getElementById('drift-enabled').addEventListener('change', (e) => {
            if (this.audioEngine) this.audioEngine.setDriftEnabled(e.target.checked);
        });

        // LFO controls
        document.getElementById('lfo-rate').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value);
            document.getElementById('lfo-rate-value').textContent = `${value.toFixed(1)} Hz`;
            if (this.audioEngine) this.audioEngine.setLfoRate(value);
        });

        document.getElementById('lfo-target').addEventListener('change', (e) => {
            const value = parseInt(e.target.value);
            if (this.audioEngine) this.audioEngine.setLfoTarget(value);
        });

        // Filter controls
        document.getElementById('filter-cutoff').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 100;
            document.getElementById('filter-cutoff-value').textContent = `${e.target.value}%`;
            if (this.audioEngine) this.audioEngine.setFilterCutoff(value);
        });

        document.getElementById('filter-resonance').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 100;
            document.getElementById('filter-resonance-value').textContent = `${e.target.value}%`;
            if (this.audioEngine) this.audioEngine.setFilterResonance(value);
        });

        document.getElementById('filter-env-amount').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 100;
            document.getElementById('filter-env-amount-value').textContent = `${e.target.value}%`;
            if (this.audioEngine) this.audioEngine.setFilterEnvAmount(value);
        });

        document.getElementById('filter-key-track').addEventListener('change', (e) => {
            const value = parseInt(e.target.value);
            if (this.audioEngine) this.audioEngine.setFilterKeyTrack(value);
        });

        // Filter envelope controls
        document.getElementById('filter-env-attack').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 1000;  // Convert ms to seconds
            document.getElementById('filter-env-attack-value').textContent = `${e.target.value}ms`;
            if (this.audioEngine) this.audioEngine.setFilterEnvAttack(value);
        });

        document.getElementById('filter-env-decay').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 1000;
            document.getElementById('filter-env-decay-value').textContent = `${e.target.value}ms`;
            if (this.audioEngine) this.audioEngine.setFilterEnvDecay(value);
        });

        document.getElementById('filter-env-sustain').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 100;
            document.getElementById('filter-env-sustain-value').textContent = `${e.target.value}%`;
            if (this.audioEngine) this.audioEngine.setFilterEnvSustain(value);
        });

        document.getElementById('filter-env-release').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 1000;
            document.getElementById('filter-env-release-value').textContent = `${e.target.value}ms`;
            if (this.audioEngine) this.audioEngine.setFilterEnvRelease(value);
        });

        // MIDI device selection
        document.getElementById('midi-input').addEventListener('change', (e) => {
            this.selectMidiDevice(e.target.value);
        });
    }

    async startAudio() {
        if (this.running) return;

        try {
            this.ui.setStatus('Starting audio engine...');

            // Initialize audio engine
            this.audioEngine = new AudioEngine();
            await this.audioEngine.init();

            // Connect MIDI to audio engine
            if (this.midiHandler) {
                this.midiHandler.setMidiCallback((data) => {
                    if (this.audioEngine) {
                        this.audioEngine.handleMidi(data);
                    }
                    this.ui.logMidi(data);
                });
            }

            // Update UI with audio info
            const info = this.audioEngine.getInfo();
            document.getElementById('sample-rate').textContent = info.sampleRate;
            document.getElementById('buffer-size').textContent = info.bufferSize;
            document.getElementById('latency').textContent = info.latency.toFixed(1);

            // Enable controls
            document.getElementById('start-audio').disabled = true;
            document.getElementById('play-note').disabled = false;
            document.getElementById('stop-note').disabled = false;

            this.running = true;
            this.ui.setStatus('Running');

            console.log('Audio engine started:', info);
        } catch (error) {
            console.error('Failed to start audio:', error);
            this.ui.setStatus(`Error: ${error.message}`);
        }
    }

    playTestNote() {
        if (this.audioEngine) {
            // Send MIDI Note On for A4 (note 69)
            this.audioEngine.handleMidi([0x90, 69, 100]);
            this.ui.logMidi([0x90, 69, 100]);
        }
    }

    stopTestNote() {
        if (this.audioEngine) {
            // Send MIDI Note Off for A4
            this.audioEngine.handleMidi([0x80, 69, 0]);
            this.ui.logMidi([0x80, 69, 0]);
        }
    }

    updateMidiDevices() {
        if (!this.midiHandler) return;

        const devices = this.midiHandler.getDevices();
        const select = document.getElementById('midi-input');

        // Clear existing options (except first)
        while (select.options.length > 1) {
            select.remove(1);
        }

        // Add devices
        devices.forEach(device => {
            const option = document.createElement('option');
            option.value = device.id;
            option.textContent = device.name;
            select.appendChild(option);
        });
    }

    selectMidiDevice(deviceId) {
        if (!this.midiHandler) return;

        if (deviceId) {
            this.midiHandler.selectDevice(deviceId);
            this.ui.setStatus(`MIDI: ${this.midiHandler.getSelectedDeviceName()}`);
        } else {
            this.midiHandler.selectDevice(null);
        }
    }
}

// Initialize app when DOM is ready
document.addEventListener('DOMContentLoaded', async () => {
    const app = new App();
    await app.init();
    window.app = app; // For debugging
});
