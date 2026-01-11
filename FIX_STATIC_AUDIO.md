# Fix for Static/Garbled Audio Output

## Problem
You're hearing static/unintelligible noise instead of a clear C major chord when the test tone plays.

## Root Cause
The filter stability fix (commit 34a4419) was merged but the binary wasn't rebuilt. The old binary is still running with the buggy filter code that lacks critical feedback compensation, causing NaN values and garbled audio.

## Solution: Rebuild and Reinstall

### Quick Fix (Recommended)
Run the rebuild script that automates the entire process:

```bash
cd ~/poor-house-juno
./rebuild.sh
```

### Manual Steps (if rebuild.sh doesn't work)
If you need to do it manually:

```bash
cd ~/poor-house-juno

# Stop the service
sudo systemctl stop poor-house-juno

# Clean and rebuild
rm -rf build
mkdir build
cd build
cmake ..
make -j$(nproc)

# Install the new binary
sudo cp poor-house-juno /usr/local/bin/

# Restart the service
sudo systemctl start poor-house-juno
```

### Verify the Fix

1. Check that the service started successfully:
   ```bash
   sudo systemctl status poor-house-juno
   ```

2. View the logs to see the build timestamp and test output:
   ```bash
   sudo journalctl -u poor-house-juno -f
   ```

3. Look for:
   - **Build timestamp** - should show today's date
   - **"(C major triad: C4, E4, G4)"** - test chord announcement
   - Should hear a CLEAR chord (not static)

4. If you still hear static after rebuild, check the build timestamp in the logs:
   - If it shows an old date, the rebuild didn't work
   - If it shows today's date but still sounds bad, there may be another issue

## What Was Fixed

### Technical Details
The previous filter code was missing the critical stability compensation factor:

**Before (BROKEN - causes NaN/static):**
```cpp
float inputWithFeedback = input - feedback;
```

**After (FIXED - stable):**
```cpp
float G = g_ / (1.0f + g_);
float G4 = G * G * G * G;
float inputCompensated = (input - feedback) / (1.0f + k_ * G4);  // ← Stability factor!
```

Without the `(1.0f + k_ * G4)` denominator, the filter feedback loop can exceed unity gain and blow up, producing NaN values that sound like static/white noise.

### Additional Safety Improvements
I also added extra validation to make the filter even more robust:
- NaN detection for cutoff frequency
- Clamping of the `g_` coefficient to prevent extreme values
- Clamping of resonance parameter before scaling

## Troubleshooting

### If audio is still garbled after rebuild:

1. **Check the build timestamp:**
   ```bash
   sudo journalctl -u poor-house-juno | grep "Build timestamp"
   ```
   It should show today's date/time. If not, the new binary isn't running.

2. **Manually verify binary was updated:**
   ```bash
   ls -la /usr/local/bin/poor-house-juno
   ```
   The timestamp should be recent (today).

3. **Check for build errors:**
   ```bash
   cd ~/poor-house-juno/build
   make 2>&1 | grep -i error
   ```

4. **Try a clean rebuild:**
   ```bash
   cd ~/poor-house-juno
   rm -rf build
   ./rebuild.sh
   ```

### If you get compilation errors:

Make sure you have all dependencies installed:
```bash
sudo apt-get update
sudo apt-get install -y cmake g++ libasound2-dev
```

## Expected Result

After rebuilding, you should hear:
- **A clear, rich C major chord** (C4, E4, G4)
- **Warm Juno-style tone** with slight chorus effect
- **NO static, NO white noise, NO garbled sound**

The chord plays for 3 seconds when the service starts and no MIDI device is connected.

## Reference
- Filter fix commit: `34a4419` - "Fix critical filter instability causing NaN/garbled audio"
- Original implementation: Based on Vadim Zavalishin's "The Art of VA Filter Design" (ZDF 4-pole ladder)
