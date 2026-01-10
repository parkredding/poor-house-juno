/**
 * Poor House Juno - Web Application
 * Main entry point for the browser-based test environment
 */

import { AudioEngine } from './audio.js';
import { MidiHandler } from './midi.js';
import { UI } from './ui.js';
import { VirtualKeyboard } from './keyboard.js';
import { PresetManager } from './presets.js';

class App {
    constructor() {
        this.audioEngine = null;
        this.midiHandler = null;
        this.ui = null;
        this.keyboard = null;
        this.presetManager = null;
        this.running = false;
        this.activeNotes = new Map(); // Track active notes for voice indicator
        this.cpuUpdateInterval = null;
    }

    async init() {
        console.log('Poor House Juno - Initializing...');

        // Initialize UI
        this.ui = new UI();
        this.ui.setStatus('Ready - Click "Start Audio" to begin');

        // Initialize preset manager
        this.presetManager = new PresetManager();
        this.populatePresetList();

        // Setup event listeners
        this.setupEventListeners();

        // Initialize virtual keyboard (will be connected to audio engine when started)
        this.keyboard = new VirtualKeyboard((data) => this.handleKeyboardMidi(data));

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

        // M14: DCO Range control
        document.getElementById('dco-range').addEventListener('change', (e) => {
            const value = parseInt(e.target.value);
            if (this.audioEngine) this.audioEngine.setDcoRange(value);
        });

        // LFO controls
        document.getElementById('lfo-rate').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value);
            document.getElementById('lfo-rate-value').textContent = `${value.toFixed(1)} Hz`;
            if (this.audioEngine) this.audioEngine.setLfoRate(value);
        });

        // M12: LFO Delay control
        document.getElementById('lfo-delay').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value);
            document.getElementById('lfo-delay-value').textContent = `${value.toFixed(1)}s`;
            if (this.audioEngine) this.audioEngine.setLfoDelay(value);
        });

        document.getElementById('lfo-target').addEventListener('change', (e) => {
            const value = parseInt(e.target.value);
            if (this.audioEngine) this.audioEngine.setLfoTarget(value);
        });

        // Chorus controls
        document.getElementById('chorus-mode').addEventListener('change', (e) => {
            const value = parseInt(e.target.value);
            if (this.audioEngine) this.audioEngine.setChorusMode(value);
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

        // M11: HPF control
        document.getElementById('filter-hpf').addEventListener('change', (e) => {
            const value = parseInt(e.target.value);
            if (this.audioEngine) this.audioEngine.setFilterHpfMode(value);
        });

        // M11: Filter LFO Amount
        document.getElementById('filter-lfo-amount').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 100;
            document.getElementById('filter-lfo-amount-value').textContent = `${e.target.value}%`;
            if (this.audioEngine) this.audioEngine.setFilterLfoAmount(value);
        });

        // M11: Performance controls
        document.getElementById('pitch-bend-range').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value);
            document.getElementById('pitch-bend-range-value').textContent = `±${value}`;
            if (this.audioEngine) this.audioEngine.setPitchBendRange(value);
        });

        document.getElementById('portamento-time').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value);
            document.getElementById('portamento-time-value').textContent = `${value.toFixed(1)}s`;
            if (this.audioEngine) this.audioEngine.setPortamentoTime(value);
        });

        // M13: Performance controls
        document.getElementById('mod-wheel').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 100;
            document.getElementById('mod-wheel-value').textContent = `${e.target.value}%`;
            if (this.audioEngine) this.audioEngine.setModWheel(value);
        });

        document.getElementById('vca-mode').addEventListener('change', (e) => {
            const value = parseInt(e.target.value);
            if (this.audioEngine) this.audioEngine.setVcaMode(value);
        });

        document.getElementById('filter-env-polarity').addEventListener('change', (e) => {
            const value = parseInt(e.target.value);
            if (this.audioEngine) this.audioEngine.setFilterEnvPolarity(value);
        });

        // M14: Range & Voice Control
        document.getElementById('vca-level').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 100;
            document.getElementById('vca-level-value').textContent = `${e.target.value}%`;
            if (this.audioEngine) this.audioEngine.setVcaLevel(value);
        });

        document.getElementById('master-tune').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value);
            document.getElementById('master-tune-value').textContent = `${value}`;
            if (this.audioEngine) this.audioEngine.setMasterTune(value);
        });

        document.getElementById('velocity-to-filter').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 100;
            document.getElementById('velocity-to-filter-value').textContent = `${e.target.value}%`;
            if (this.audioEngine) this.audioEngine.setVelocityToFilter(value);
        });

        document.getElementById('velocity-to-amp').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 100;
            document.getElementById('velocity-to-amp-value').textContent = `${e.target.value}%`;
            if (this.audioEngine) this.audioEngine.setVelocityToAmp(value);
        });

        // M16: Voice Allocation Mode
        document.getElementById('voice-allocation-mode').addEventListener('change', (e) => {
            const value = parseInt(e.target.value);
            if (this.audioEngine) this.audioEngine.setVoiceAllocationMode(value);
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

        // Amplitude envelope controls
        document.getElementById('amp-env-attack').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 1000;  // Convert ms to seconds
            document.getElementById('amp-env-attack-value').textContent = `${e.target.value}ms`;
            if (this.audioEngine) this.audioEngine.setAmpEnvAttack(value);
        });

        document.getElementById('amp-env-decay').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 1000;
            document.getElementById('amp-env-decay-value').textContent = `${e.target.value}ms`;
            if (this.audioEngine) this.audioEngine.setAmpEnvDecay(value);
        });

        document.getElementById('amp-env-sustain').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 100;
            document.getElementById('amp-env-sustain-value').textContent = `${e.target.value}%`;
            if (this.audioEngine) this.audioEngine.setAmpEnvSustain(value);
        });

        document.getElementById('amp-env-release').addEventListener('input', (e) => {
            const value = parseFloat(e.target.value) / 1000;
            document.getElementById('amp-env-release-value').textContent = `${e.target.value}ms`;
            if (this.audioEngine) this.audioEngine.setAmpEnvRelease(value);
        });

        // MIDI device selection
        document.getElementById('midi-input').addEventListener('change', (e) => {
            this.selectMidiDevice(e.target.value);
        });

        // Preset controls
        document.getElementById('preset-load').addEventListener('click', () => {
            this.loadPreset();
        });

        document.getElementById('preset-save').addEventListener('click', () => {
            this.savePreset();
        });

        document.getElementById('preset-delete').addEventListener('click', () => {
            this.deletePreset();
        });

        document.getElementById('preset-select').addEventListener('change', (e) => {
            if (e.target.value) {
                document.getElementById('preset-name').value = e.target.value;
            }
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
                    if (this.keyboard) {
                        this.keyboard.handleExternalMidi(data);
                    }
                    this.ui.logMidi(data);
                    this.trackVoiceActivity(data);
                    this.flashMidiIndicator();
                });
            }

            // Start oscilloscope
            const analyser = this.audioEngine.getAnalyser();
            if (analyser) {
                this.ui.startOscilloscope(analyser);
            }

            // Start CPU monitoring
            this.startCpuMonitoring();

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
            const data = [0x90, 69, 100];
            this.audioEngine.handleMidi(data);
            this.ui.logMidi(data);
            this.trackVoiceActivity(data);
            this.flashMidiIndicator();
        }
    }

    stopTestNote() {
        if (this.audioEngine) {
            // Send MIDI Note Off for A4
            const data = [0x80, 69, 0];
            this.audioEngine.handleMidi(data);
            this.ui.logMidi(data);
            this.trackVoiceActivity(data);
            this.flashMidiIndicator();
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

    handleKeyboardMidi(data) {
        if (this.audioEngine) {
            this.audioEngine.handleMidi(data);
        }
        this.ui.logMidi(data);
        this.trackVoiceActivity(data);
        this.flashMidiIndicator();
    }

    trackVoiceActivity(data) {
        const [status, note, velocity] = data;
        const command = status & 0xF0;

        if (command === 0x90 && velocity > 0) {
            // Note on
            this.activeNotes.set(note, Date.now());
        } else if (command === 0x80 || (command === 0x90 && velocity === 0)) {
            // Note off
            this.activeNotes.delete(note);
        }

        this.updateVoiceIndicator();
    }

    updateVoiceIndicator() {
        const voiceCount = Math.min(this.activeNotes.size, 6);
        document.getElementById('voice-count').textContent = `${voiceCount}/6`;

        // Update voice dots
        for (let i = 0; i < 6; i++) {
            const dot = document.getElementById(`voice-${i}`);
            if (dot) {
                if (i < voiceCount) {
                    dot.classList.add('active');
                } else {
                    dot.classList.remove('active');
                }
            }
        }
    }

    flashMidiIndicator() {
        const indicator = document.getElementById('midi-activity');
        if (indicator) {
            indicator.classList.add('active');
            setTimeout(() => {
                indicator.classList.remove('active');
            }, 100);
        }
    }

    // Preset management
    populatePresetList() {
        const select = document.getElementById('preset-select');
        const names = this.presetManager.getPresetNames();

        // Clear existing options (except first)
        while (select.options.length > 1) {
            select.remove(1);
        }

        // Add presets
        names.forEach(name => {
            const option = document.createElement('option');
            option.value = name;
            option.textContent = name;
            select.appendChild(option);
        });
    }

    loadPreset() {
        const select = document.getElementById('preset-select');
        const presetName = select.value;

        if (!presetName) {
            alert('Please select a preset to load');
            return;
        }

        try {
            const params = this.presetManager.loadPreset(presetName);
            this.presetManager.applyPresetToUI(params);

            // Apply to audio engine
            if (this.audioEngine) {
                this.applyParametersToEngine(params);
            }

            this.ui.setStatus(`Loaded preset: ${presetName}`);
        } catch (error) {
            alert(`Failed to load preset: ${error.message}`);
        }
    }

    savePreset() {
        const nameInput = document.getElementById('preset-name');
        const presetName = nameInput.value.trim();

        if (!presetName) {
            alert('Please enter a preset name');
            return;
        }

        try {
            const params = this.presetManager.captureCurrentState();
            this.presetManager.savePreset(presetName, params);
            this.populatePresetList();

            // Select the newly saved preset
            document.getElementById('preset-select').value = presetName;

            this.ui.setStatus(`Saved preset: ${presetName}`);
        } catch (error) {
            alert(`Failed to save preset: ${error.message}`);
        }
    }

    deletePreset() {
        const select = document.getElementById('preset-select');
        const presetName = select.value;

        if (!presetName) {
            alert('Please select a preset to delete');
            return;
        }

        if (!confirm(`Delete preset "${presetName}"?`)) {
            return;
        }

        try {
            this.presetManager.deletePreset(presetName);
            this.populatePresetList();

            // Clear selection
            select.value = '';
            document.getElementById('preset-name').value = '';

            this.ui.setStatus(`Deleted preset: ${presetName}`);
        } catch (error) {
            alert(`Failed to delete preset: ${error.message}`);
        }
    }

    applyParametersToEngine(params) {
        if (!this.audioEngine) return;

        // DCO
        this.audioEngine.setSawLevel(params.sawLevel);
        this.audioEngine.setPulseLevel(params.pulseLevel);
        this.audioEngine.setSubLevel(params.subLevel);
        this.audioEngine.setNoiseLevel(params.noiseLevel);
        this.audioEngine.setPulseWidth(params.pulseWidth);
        this.audioEngine.setPwmDepth(params.pwmDepth);
        this.audioEngine.setDetune(params.detune);
        this.audioEngine.setDriftEnabled(params.driftEnabled);

        // LFO
        this.audioEngine.setLfoRate(params.lfoRate);
        this.audioEngine.setLfoDelay(params.lfoDelay || 0.0);
        this.audioEngine.setLfoTarget(params.lfoTarget);

        // Filter
        this.audioEngine.setFilterCutoff(params.filterCutoff);
        this.audioEngine.setFilterResonance(params.filterResonance);
        this.audioEngine.setFilterEnvAmount(params.filterEnvAmount);
        this.audioEngine.setFilterKeyTrack(params.filterKeyTrack);

        // Filter Envelope
        this.audioEngine.setFilterEnvAttack(params.filterEnvAttack);
        this.audioEngine.setFilterEnvDecay(params.filterEnvDecay);
        this.audioEngine.setFilterEnvSustain(params.filterEnvSustain);
        this.audioEngine.setFilterEnvRelease(params.filterEnvRelease);

        // Amp Envelope
        this.audioEngine.setAmpEnvAttack(params.ampEnvAttack);
        this.audioEngine.setAmpEnvDecay(params.ampEnvDecay);
        this.audioEngine.setAmpEnvSustain(params.ampEnvSustain);
        this.audioEngine.setAmpEnvRelease(params.ampEnvRelease);

        // Chorus
        this.audioEngine.setChorusMode(params.chorusMode);

        // M13: Performance Controls
        if (params.vcaMode !== undefined) {
            this.audioEngine.setVcaMode(params.vcaMode);
        }
        if (params.filterEnvPolarity !== undefined) {
            this.audioEngine.setFilterEnvPolarity(params.filterEnvPolarity);
        }

        // M14: Range & Voice Control
        if (params.dcoRange !== undefined) {
            this.audioEngine.setDcoRange(params.dcoRange);
        }
        if (params.vcaLevel !== undefined) {
            this.audioEngine.setVcaLevel(params.vcaLevel);
        }
        if (params.masterTune !== undefined) {
            this.audioEngine.setMasterTune(params.masterTune);
        }
        if (params.velocityToFilter !== undefined) {
            this.audioEngine.setVelocityToFilter(params.velocityToFilter);
        }
        if (params.velocityToAmp !== undefined) {
            this.audioEngine.setVelocityToAmp(params.velocityToAmp);
        }

        // M16: Voice Allocation Mode
        if (params.voiceAllocationMode !== undefined) {
            this.audioEngine.setVoiceAllocationMode(params.voiceAllocationMode);
        }
    }

    startCpuMonitoring() {
        // Simulate CPU usage (would need WASM profiling for real data)
        this.cpuUpdateInterval = setInterval(() => {
            // Estimate based on active voices and effects
            const voiceLoad = (this.activeNotes.size / 6) * 30; // Max 30% for voices
            const baseLoad = 5; // Base processing
            const chorusMode = parseInt(document.getElementById('chorus-mode').value);
            const chorusLoad = chorusMode > 0 ? 10 : 0;

            const estimatedCpu = Math.min(baseLoad + voiceLoad + chorusLoad, 100);
            document.getElementById('cpu-value').textContent = `${Math.round(estimatedCpu)}%`;
        }, 500);
    }

    stopCpuMonitoring() {
        if (this.cpuUpdateInterval) {
            clearInterval(this.cpuUpdateInterval);
            this.cpuUpdateInterval = null;
        }
    }
}

// Initialize app when DOM is ready
document.addEventListener('DOMContentLoaded', async () => {
    const app = new App();
    await app.init();
    window.app = app; // For debugging
});
