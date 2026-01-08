/**
 * UI utilities and visualizations
 */

export class UI {
    constructor() {
        this.midiLogLines = [];
        this.maxLogLines = 10;
    }

    setStatus(status) {
        const statusEl = document.getElementById('status');
        if (statusEl) {
            statusEl.textContent = status;
        }
    }

    logMidi(data) {
        const [status, data1, data2] = data;
        const statusType = status & 0xF0;
        const channel = (status & 0x0F) + 1;

        let message = '';

        switch (statusType) {
            case 0x90:
                if (data2 > 0) {
                    message = `Note ON  ch${channel}: ${data1} vel=${data2}`;
                } else {
                    message = `Note OFF ch${channel}: ${data1}`;
                }
                break;
            case 0x80:
                message = `Note OFF ch${channel}: ${data1}`;
                break;
            case 0xB0:
                message = `CC ch${channel}: ${data1} = ${data2}`;
                break;
            default:
                message = `MIDI: ${data.map(b => b.toString(16).padStart(2, '0')).join(' ')}`;
        }

        this.midiLogLines.push(message);

        // Keep only last N lines
        if (this.midiLogLines.length > this.maxLogLines) {
            this.midiLogLines.shift();
        }

        // Update log display
        const logEl = document.getElementById('midi-log');
        if (logEl) {
            logEl.innerHTML = this.midiLogLines
                .map(line => `<div>${line}</div>`)
                .join('');
            logEl.scrollTop = logEl.scrollHeight;
        }
    }

    // Simple oscilloscope visualization
    startOscilloscope(analyser) {
        const canvas = document.getElementById('oscilloscope');
        if (!canvas) return;

        const ctx = canvas.getContext('2d');
        const bufferLength = analyser.fftSize;
        const dataArray = new Float32Array(bufferLength);

        const draw = () => {
            requestAnimationFrame(draw);

            analyser.getFloatTimeDomainData(dataArray);

            // Clear canvas
            ctx.fillStyle = 'rgba(0, 0, 0, 0.1)';
            ctx.fillRect(0, 0, canvas.width, canvas.height);

            // Draw waveform
            ctx.lineWidth = 2;
            ctx.strokeStyle = '#00d4ff';
            ctx.beginPath();

            const sliceWidth = canvas.width / bufferLength;
            let x = 0;

            for (let i = 0; i < bufferLength; i++) {
                const v = dataArray[i];
                const y = (v + 1) * canvas.height / 2;

                if (i === 0) {
                    ctx.moveTo(x, y);
                } else {
                    ctx.lineTo(x, y);
                }

                x += sliceWidth;
            }

            ctx.stroke();
        };

        draw();
    }
}
