/**
 * Web MIDI API handler
 */

export class MidiHandler {
    constructor() {
        this.midiAccess = null;
        this.devices = [];
        this.selectedDevice = null;
        this.midiCallback = null;
    }

    async init() {
        if (!navigator.requestMIDIAccess) {
            console.warn('Web MIDI API not supported');
            return false;
        }

        try {
            this.midiAccess = await navigator.requestMIDIAccess();
            console.log('MIDI access granted');

            // Enumerate devices
            this.updateDevices();

            // Listen for device changes
            this.midiAccess.onstatechange = () => {
                this.updateDevices();
            };

            return true;
        } catch (error) {
            console.error('Failed to get MIDI access:', error);
            return false;
        }
    }

    updateDevices() {
        if (!this.midiAccess) return;

        this.devices = [];

        for (const input of this.midiAccess.inputs.values()) {
            this.devices.push({
                id: input.id,
                name: input.name,
                manufacturer: input.manufacturer,
                input: input
            });
        }

        console.log('MIDI devices:', this.devices);
    }

    getDevices() {
        return this.devices;
    }

    selectDevice(deviceId) {
        // Disconnect previous device
        if (this.selectedDevice) {
            this.selectedDevice.input.onmidimessage = null;
            this.selectedDevice = null;
        }

        if (!deviceId) return;

        // Find and connect new device
        const device = this.devices.find(d => d.id === deviceId);
        if (device) {
            this.selectedDevice = device;
            device.input.onmidimessage = (event) => {
                this.handleMidiMessage(event);
            };
            console.log('Selected MIDI device:', device.name);
        }
    }

    getSelectedDeviceName() {
        return this.selectedDevice ? this.selectedDevice.name : 'None';
    }

    handleMidiMessage(event) {
        const data = Array.from(event.data);

        // Flash MIDI indicator
        const indicator = document.getElementById('midi-activity');
        if (indicator) {
            indicator.classList.add('active');
            setTimeout(() => indicator.classList.remove('active'), 50);
        }

        // Call callback if set
        if (this.midiCallback) {
            this.midiCallback(data);
        }
    }

    setMidiCallback(callback) {
        this.midiCallback = callback;
    }
}
