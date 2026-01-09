/**
 * Preset Management
 * Save and load synth parameter presets to localStorage
 */

export class PresetManager {
    constructor() {
        this.storageKey = 'poorHouseJunoPresets';
        this.presets = this.loadPresetsFromStorage();
        this.currentParameters = {};
    }

    loadPresetsFromStorage() {
        try {
            const stored = localStorage.getItem(this.storageKey);
            if (stored) {
                return JSON.parse(stored);
            }
        } catch (error) {
            console.error('Failed to load presets:', error);
        }

        // Return default presets
        return {
            'Init': this.getInitPreset(),
            'Classic Juno': this.getClassicJunoPreset(),
            'Bass': this.getBassPreset(),
            'Pad': this.getPadPreset(),
            'Lead': this.getLeadPreset()
        };
    }

    savePresetsToStorage() {
        try {
            localStorage.setItem(this.storageKey, JSON.stringify(this.presets));
        } catch (error) {
            console.error('Failed to save presets:', error);
        }
    }

    getInitPreset() {
        return {
            sawLevel: 0.5,
            pulseLevel: 0,
            subLevel: 0,
            noiseLevel: 0,
            pulseWidth: 0.5,
            pwmDepth: 0,
            detune: 0,
            driftEnabled: true,
            lfoRate: 2.0,
            lfoDelay: 0.0,
            lfoTarget: 0,
            filterCutoff: 0.8,
            filterResonance: 0,
            filterEnvAmount: 0,
            filterKeyTrack: 0,
            filterEnvAttack: 0.01,
            filterEnvDecay: 0.3,
            filterEnvSustain: 0.7,
            filterEnvRelease: 0.5,
            ampEnvAttack: 0.005,
            ampEnvDecay: 0.3,
            ampEnvSustain: 0.8,
            ampEnvRelease: 0.3,
            chorusMode: 0
        };
    }

    getClassicJunoPreset() {
        return {
            sawLevel: 0.6,
            pulseLevel: 0.4,
            subLevel: 0.0,
            noiseLevel: 0.0,
            pulseWidth: 0.5,
            pwmDepth: 0.3,
            detune: 0,
            driftEnabled: true,
            lfoRate: 4.5,
            lfoDelay: 0.0,
            lfoTarget: 2,
            filterCutoff: 0.65,
            filterResonance: 0.4,
            filterEnvAmount: 0.5,
            filterKeyTrack: 1,
            filterEnvAttack: 0.01,
            filterEnvDecay: 0.8,
            filterEnvSustain: 0.4,
            filterEnvRelease: 0.5,
            ampEnvAttack: 0.01,
            ampEnvDecay: 0.5,
            ampEnvSustain: 0.7,
            ampEnvRelease: 0.4,
            chorusMode: 2
        };
    }

    getBassPreset() {
        return {
            sawLevel: 0.8,
            pulseLevel: 0.2,
            subLevel: 0.6,
            noiseLevel: 0.0,
            pulseWidth: 0.3,
            pwmDepth: 0.0,
            detune: 0,
            driftEnabled: true,
            lfoRate: 0.5,
            lfoDelay: 0.0,
            lfoTarget: 0,
            filterCutoff: 0.4,
            filterResonance: 0.6,
            filterEnvAmount: 0.8,
            filterKeyTrack: 2,
            filterEnvAttack: 0.001,
            filterEnvDecay: 0.4,
            filterEnvSustain: 0.1,
            filterEnvRelease: 0.2,
            ampEnvAttack: 0.001,
            ampEnvDecay: 0.3,
            ampEnvSustain: 0.9,
            ampEnvRelease: 0.1,
            chorusMode: 0
        };
    }

    getPadPreset() {
        return {
            sawLevel: 0.5,
            pulseLevel: 0.5,
            subLevel: 0.0,
            noiseLevel: 0.05,
            pulseWidth: 0.6,
            pwmDepth: 0.5,
            detune: 3.0,
            driftEnabled: true,
            lfoRate: 0.3,
            lfoDelay: 0.0,
            lfoTarget: 2,
            filterCutoff: 0.7,
            filterResonance: 0.3,
            filterEnvAmount: 0.3,
            filterKeyTrack: 0,
            filterEnvAttack: 1.5,
            filterEnvDecay: 2.0,
            filterEnvSustain: 0.8,
            filterEnvRelease: 2.5,
            ampEnvAttack: 1.2,
            ampEnvDecay: 1.0,
            ampEnvSustain: 0.9,
            ampEnvRelease: 2.0,
            chorusMode: 3
        };
    }

    getLeadPreset() {
        return {
            sawLevel: 1.0,
            pulseLevel: 0.0,
            subLevel: 0.0,
            noiseLevel: 0.0,
            pulseWidth: 0.5,
            pwmDepth: 0.0,
            detune: 0,
            driftEnabled: true,
            lfoRate: 5.0,
            lfoDelay: 0.0,
            lfoTarget: 1,
            filterCutoff: 0.75,
            filterResonance: 0.5,
            filterEnvAmount: 0.7,
            filterKeyTrack: 2,
            filterEnvAttack: 0.005,
            filterEnvDecay: 0.2,
            filterEnvSustain: 0.5,
            filterEnvRelease: 0.3,
            ampEnvAttack: 0.002,
            ampEnvDecay: 0.1,
            ampEnvSustain: 1.0,
            ampEnvRelease: 0.1,
            chorusMode: 1
        };
    }

    savePreset(name, parameters) {
        if (!name || name.trim() === '') {
            throw new Error('Preset name cannot be empty');
        }

        this.presets[name] = { ...parameters };
        this.savePresetsToStorage();
    }

    loadPreset(name) {
        if (!(name in this.presets)) {
            throw new Error(`Preset "${name}" not found`);
        }

        return { ...this.presets[name] };
    }

    deletePreset(name) {
        if (!(name in this.presets)) {
            throw new Error(`Preset "${name}" not found`);
        }

        delete this.presets[name];
        this.savePresetsToStorage();
    }

    getPresetNames() {
        return Object.keys(this.presets).sort();
    }

    captureCurrentState() {
        // Capture all current UI values
        const params = {};

        // DCO
        params.sawLevel = parseFloat(document.getElementById('saw-level').value) / 100;
        params.pulseLevel = parseFloat(document.getElementById('pulse-level').value) / 100;
        params.subLevel = parseFloat(document.getElementById('sub-level').value) / 100;
        params.noiseLevel = parseFloat(document.getElementById('noise-level').value) / 100;
        params.pulseWidth = parseFloat(document.getElementById('pulse-width').value) / 100;
        params.pwmDepth = parseFloat(document.getElementById('pwm-depth').value) / 100;
        params.detune = parseFloat(document.getElementById('detune').value);
        params.driftEnabled = document.getElementById('drift-enabled').checked;

        // LFO
        params.lfoRate = parseFloat(document.getElementById('lfo-rate').value);
        params.lfoDelay = parseFloat(document.getElementById('lfo-delay').value);
        params.lfoTarget = parseInt(document.getElementById('lfo-target').value);

        // Filter
        params.filterCutoff = parseFloat(document.getElementById('filter-cutoff').value) / 100;
        params.filterResonance = parseFloat(document.getElementById('filter-resonance').value) / 100;
        params.filterEnvAmount = parseFloat(document.getElementById('filter-env-amount').value) / 100;
        params.filterKeyTrack = parseInt(document.getElementById('filter-key-track').value);

        // Filter Envelope
        params.filterEnvAttack = parseFloat(document.getElementById('filter-env-attack').value) / 1000;
        params.filterEnvDecay = parseFloat(document.getElementById('filter-env-decay').value) / 1000;
        params.filterEnvSustain = parseFloat(document.getElementById('filter-env-sustain').value) / 100;
        params.filterEnvRelease = parseFloat(document.getElementById('filter-env-release').value) / 1000;

        // Amp Envelope
        params.ampEnvAttack = parseFloat(document.getElementById('amp-env-attack').value) / 1000;
        params.ampEnvDecay = parseFloat(document.getElementById('amp-env-decay').value) / 1000;
        params.ampEnvSustain = parseFloat(document.getElementById('amp-env-sustain').value) / 100;
        params.ampEnvRelease = parseFloat(document.getElementById('amp-env-release').value) / 1000;

        // Chorus
        params.chorusMode = parseInt(document.getElementById('chorus-mode').value);

        this.currentParameters = params;
        return params;
    }

    applyPresetToUI(parameters) {
        // DCO
        document.getElementById('saw-level').value = parameters.sawLevel * 100;
        document.getElementById('saw-level-value').textContent = `${Math.round(parameters.sawLevel * 100)}%`;

        document.getElementById('pulse-level').value = parameters.pulseLevel * 100;
        document.getElementById('pulse-level-value').textContent = `${Math.round(parameters.pulseLevel * 100)}%`;

        document.getElementById('sub-level').value = parameters.subLevel * 100;
        document.getElementById('sub-level-value').textContent = `${Math.round(parameters.subLevel * 100)}%`;

        document.getElementById('noise-level').value = parameters.noiseLevel * 100;
        document.getElementById('noise-level-value').textContent = `${Math.round(parameters.noiseLevel * 100)}%`;

        document.getElementById('pulse-width').value = parameters.pulseWidth * 100;
        document.getElementById('pulse-width-value').textContent = `${Math.round(parameters.pulseWidth * 100)}%`;

        document.getElementById('pwm-depth').value = parameters.pwmDepth * 100;
        document.getElementById('pwm-depth-value').textContent = `${Math.round(parameters.pwmDepth * 100)}%`;

        document.getElementById('detune').value = parameters.detune;
        document.getElementById('detune-value').textContent = parameters.detune.toFixed(1);

        document.getElementById('drift-enabled').checked = parameters.driftEnabled;

        // LFO
        document.getElementById('lfo-rate').value = parameters.lfoRate;
        document.getElementById('lfo-rate-value').textContent = `${parameters.lfoRate.toFixed(1)} Hz`;

        document.getElementById('lfo-delay').value = parameters.lfoDelay || 0.0;
        document.getElementById('lfo-delay-value').textContent = `${(parameters.lfoDelay || 0.0).toFixed(1)}s`;

        document.getElementById('lfo-target').value = parameters.lfoTarget;

        // Filter
        document.getElementById('filter-cutoff').value = parameters.filterCutoff * 100;
        document.getElementById('filter-cutoff-value').textContent = `${Math.round(parameters.filterCutoff * 100)}%`;

        document.getElementById('filter-resonance').value = parameters.filterResonance * 100;
        document.getElementById('filter-resonance-value').textContent = `${Math.round(parameters.filterResonance * 100)}%`;

        document.getElementById('filter-env-amount').value = parameters.filterEnvAmount * 100;
        document.getElementById('filter-env-amount-value').textContent = `${Math.round(parameters.filterEnvAmount * 100)}%`;

        document.getElementById('filter-key-track').value = parameters.filterKeyTrack;

        // Filter Envelope
        document.getElementById('filter-env-attack').value = parameters.filterEnvAttack * 1000;
        document.getElementById('filter-env-attack-value').textContent = `${Math.round(parameters.filterEnvAttack * 1000)}ms`;

        document.getElementById('filter-env-decay').value = parameters.filterEnvDecay * 1000;
        document.getElementById('filter-env-decay-value').textContent = `${Math.round(parameters.filterEnvDecay * 1000)}ms`;

        document.getElementById('filter-env-sustain').value = parameters.filterEnvSustain * 100;
        document.getElementById('filter-env-sustain-value').textContent = `${Math.round(parameters.filterEnvSustain * 100)}%`;

        document.getElementById('filter-env-release').value = parameters.filterEnvRelease * 1000;
        document.getElementById('filter-env-release-value').textContent = `${Math.round(parameters.filterEnvRelease * 1000)}ms`;

        // Amp Envelope
        document.getElementById('amp-env-attack').value = parameters.ampEnvAttack * 1000;
        document.getElementById('amp-env-attack-value').textContent = `${Math.round(parameters.ampEnvAttack * 1000)}ms`;

        document.getElementById('amp-env-decay').value = parameters.ampEnvDecay * 1000;
        document.getElementById('amp-env-decay-value').textContent = `${Math.round(parameters.ampEnvDecay * 1000)}ms`;

        document.getElementById('amp-env-sustain').value = parameters.ampEnvSustain * 100;
        document.getElementById('amp-env-sustain-value').textContent = `${Math.round(parameters.ampEnvSustain * 100)}%`;

        document.getElementById('amp-env-release').value = parameters.ampEnvRelease * 1000;
        document.getElementById('amp-env-release-value').textContent = `${Math.round(parameters.ampEnvRelease * 1000)}ms`;

        // Chorus
        document.getElementById('chorus-mode').value = parameters.chorusMode;

        return parameters;
    }
}
