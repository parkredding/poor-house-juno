/**
 * Virtual MIDI Keyboard
 * Provides mouse and computer keyboard input for playing notes
 */

export class VirtualKeyboard {
    constructor(midiCallback) {
        this.midiCallback = midiCallback;
        this.activeNotes = new Set();
        this.octave = 4; // Default octave (C4 = middle C)
        this.velocity = 100;

        // Map computer keyboard keys to MIDI note offsets
        this.keyMap = {
            // Bottom row (C to E)
            'z': 0,  // C
            's': 1,  // C#
            'x': 2,  // D
            'd': 3,  // D#
            'c': 4,  // E
            'v': 5,  // F
            'g': 6,  // F#
            'b': 7,  // G
            'h': 8,  // G#
            'n': 9,  // A
            'j': 10, // A#
            'm': 11, // B
            ',': 12, // C (next octave)
            'l': 13, // C#
            '.': 14, // D
            ';': 15, // D#
            '/': 16  // E
        };

        this.setupEventListeners();
        this.createKeyboard();
    }

    setupEventListeners() {
        // Computer keyboard events
        document.addEventListener('keydown', (e) => {
            if (e.repeat) return; // Ignore key repeat
            this.handleKeyDown(e.key.toLowerCase());
        });

        document.addEventListener('keyup', (e) => {
            this.handleKeyUp(e.key.toLowerCase());
        });

        // Octave controls
        document.getElementById('octave-down')?.addEventListener('click', () => {
            this.changeOctave(-1);
        });

        document.getElementById('octave-up')?.addEventListener('click', () => {
            this.changeOctave(1);
        });

        document.getElementById('velocity-slider')?.addEventListener('input', (e) => {
            this.velocity = parseInt(e.target.value);
            document.getElementById('velocity-value').textContent = this.velocity;
        });
    }

    handleKeyDown(key) {
        if (!(key in this.keyMap)) return;

        const offset = this.keyMap[key];
        const note = this.octave * 12 + offset;

        if (this.activeNotes.has(note)) return; // Already playing

        this.activeNotes.add(note);
        this.sendNoteOn(note, this.velocity);
        this.highlightKey(note, true);
    }

    handleKeyUp(key) {
        if (!(key in this.keyMap)) return;

        const offset = this.keyMap[key];
        const note = this.octave * 12 + offset;

        if (!this.activeNotes.has(note)) return;

        this.activeNotes.delete(note);
        this.sendNoteOff(note);
        this.highlightKey(note, false);
    }

    changeOctave(delta) {
        const newOctave = this.octave + delta;
        if (newOctave < 0 || newOctave > 8) return;

        // Release all currently playing notes before changing octave
        this.releaseAllNotes();

        this.octave = newOctave;
        document.getElementById('octave-display').textContent = this.octave;
    }

    releaseAllNotes() {
        for (const note of this.activeNotes) {
            this.sendNoteOff(note);
            this.highlightKey(note, false);
        }
        this.activeNotes.clear();
    }

    createKeyboard() {
        const container = document.getElementById('virtual-keyboard');
        if (!container) return;

        // Create 2 octaves starting from C
        const numOctaves = 2;
        const startNote = 60; // Middle C (C4)

        for (let octave = 0; octave < numOctaves; octave++) {
            const octaveDiv = document.createElement('div');
            octaveDiv.className = 'keyboard-octave';

            // White keys pattern: C D E F G A B
            const whiteKeys = [0, 2, 4, 5, 7, 9, 11];
            const blackKeys = [1, 3, 6, 8, 10]; // C# D# F# G# A#

            // Create white keys
            whiteKeys.forEach(offset => {
                const note = startNote + octave * 12 + offset;
                const key = document.createElement('div');
                key.className = 'piano-key white';
                key.dataset.note = note;

                // Add note label for C notes
                if (offset === 0) {
                    const label = document.createElement('span');
                    label.className = 'key-label';
                    label.textContent = `C${Math.floor(note / 12) - 1}`;
                    key.appendChild(label);
                }

                key.addEventListener('mousedown', () => this.handleMouseDown(note));
                key.addEventListener('mouseup', () => this.handleMouseUp(note));
                key.addEventListener('mouseenter', (e) => {
                    if (e.buttons === 1) this.handleMouseDown(note);
                });
                key.addEventListener('mouseleave', () => this.handleMouseUp(note));

                octaveDiv.appendChild(key);
            });

            // Create black keys (positioned absolutely over white keys)
            blackKeys.forEach((offset, index) => {
                const note = startNote + octave * 12 + offset;
                const key = document.createElement('div');
                key.className = 'piano-key black';
                key.dataset.note = note;
                key.style.left = `${this.getBlackKeyPosition(offset)}%`;

                key.addEventListener('mousedown', () => this.handleMouseDown(note));
                key.addEventListener('mouseup', () => this.handleMouseUp(note));
                key.addEventListener('mouseenter', (e) => {
                    if (e.buttons === 1) this.handleMouseDown(note);
                });
                key.addEventListener('mouseleave', () => this.handleMouseUp(note));

                octaveDiv.appendChild(key);
            });

            container.appendChild(octaveDiv);
        }
    }

    getBlackKeyPosition(offset) {
        // Position black keys between white keys
        const positions = {
            1: 7.1,    // C#
            3: 21.4,   // D#
            6: 42.8,   // F#
            8: 57.1,   // G#
            10: 71.4   // A#
        };
        return positions[offset] || 0;
    }

    handleMouseDown(note) {
        if (this.activeNotes.has(note)) return;

        this.activeNotes.add(note);
        this.sendNoteOn(note, this.velocity);
        this.highlightKey(note, true);
    }

    handleMouseUp(note) {
        if (!this.activeNotes.has(note)) return;

        this.activeNotes.delete(note);
        this.sendNoteOff(note);
        this.highlightKey(note, false);
    }

    highlightKey(note, active) {
        const key = document.querySelector(`[data-note="${note}"]`);
        if (key) {
            if (active) {
                key.classList.add('active');
            } else {
                key.classList.remove('active');
            }
        }
    }

    sendNoteOn(note, velocity) {
        if (this.midiCallback) {
            this.midiCallback([0x90, note, velocity]);
        }
    }

    sendNoteOff(note) {
        if (this.midiCallback) {
            this.midiCallback([0x80, note, 0]);
        }
    }

    // Handle external MIDI to highlight keys
    handleExternalMidi(data) {
        const [status, note, velocity] = data;
        const command = status & 0xF0;

        if (command === 0x90 && velocity > 0) {
            // Note on
            this.highlightKey(note, true);
        } else if (command === 0x80 || (command === 0x90 && velocity === 0)) {
            // Note off
            this.highlightKey(note, false);
        }
    }
}
