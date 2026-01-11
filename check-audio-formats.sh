#!/bin/bash
# Check what audio formats hw:2,0 supports

echo "========================================="
echo "Audio Device Capabilities Check"
echo "========================================="
echo ""

echo "1. List all audio devices:"
aplay -l
echo ""

echo "2. Check what formats hw:2,0 supports:"
echo ""

# Use speaker-test to probe formats
echo "Testing FLOAT_LE format..."
if speaker-test -D hw:2,0 -c 2 -f dat -t sine -l 1 -s 1 2>&1 | grep -q "Sample format not available"; then
    echo "  ✗ FLOAT_LE: NOT supported"
else
    echo "  ✓ FLOAT_LE: Supported"
fi

echo ""
echo "Testing S32_LE format..."
if speaker-test -D hw:2,0 -c 2 -r 48000 -F S32_LE -t sine -l 1 -s 1 2>&1 | grep -q "Sample format not available"; then
    echo "  ✗ S32_LE: NOT supported"
else
    echo "  ✓ S32_LE: Supported"
fi

echo ""
echo "Testing S16_LE format..."
if speaker-test -D hw:2,0 -c 2 -r 48000 -F S16_LE -t sine -l 1 -s 1 2>&1 | grep -q "Sample format not available"; then
    echo "  ✗ S16_LE: NOT supported"
else
    echo "  ✓ S16_LE: Supported"
fi

echo ""
echo "3. Get detailed device info:"
cat /proc/asound/card2/stream0 2>/dev/null || cat /proc/asound/card2/pcm0p/info 2>/dev/null || echo "Could not read device info"
echo ""

echo "========================================="
echo "Recommendation"
echo "========================================="
echo "Most Raspberry Pi audio devices support S16_LE (16-bit signed integer)."
echo "I'll update the code to try multiple formats automatically."
echo ""
