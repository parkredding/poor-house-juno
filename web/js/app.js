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

        // Parameter controls
        document.getElementById('frequency').addEventListener('input', (e) => {
            const freq = parseFloat(e.target.value);
            document.getElementById('frequency-value').textContent = `${freq} Hz`;
            if (this.audioEngine) {
                this.audioEngine.setFrequency(freq);
            }
        });

        document.getElementById('amplitude').addEventListener('input', (e) => {
            const amp = parseFloat(e.target.value) / 100;
            document.getElementById('amplitude-value').textContent = `${e.target.value}%`;
            if (this.audioEngine) {
                this.audioEngine.setAmplitude(amp);
            }
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
