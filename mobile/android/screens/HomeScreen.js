// ─────────────────────────────────────────
//  EchoNav — android/screens/HomeScreen.js
//  Main screen the user sees when they
//  open the app. Has:
//   - Connect / Disconnect button
//   - Voice command button (hold to speak)
//   - Connection status indicator
//   - Quick navigation to other screens
// ─────────────────────────────────────────

import React, { useState, useEffect } from 'react';
import {
    View, Text, TouchableOpacity,
    StyleSheet, Alert, ActivityIndicator
} from 'react-native';

import BLEManager from '../bluetooth/BLEManager';
import Voice      from '@react-native-voice/voice';
import Tts        from 'react-native-tts';


export default function HomeScreen({ navigation }) {

    const [connected,    setConnected]    = useState(false);
    const [scanning,     setScanning]     = useState(false);
    const [listening,    setListening]    = useState(false);
    const [voiceText,    setVoiceText]    = useState('');
    const [lastAlert,    setLastAlert]    = useState(null);


    // ── Setup ─────────────────────────────
    useEffect(() => {
        // Listen for BLE disconnection
        BLEManager.onDisconnect = () => {
            setConnected(false);
            Tts.speak('EchoNav disconnected');
        };

        // Show latest detection on home screen
        BLEManager.onDetection = (packet) => {
            setLastAlert(packet);
        };

        // Voice recognition callbacks
        Voice.onSpeechResults = onSpeechResults;
        Voice.onSpeechError   = () => setListening(false);

        return () => {
            Voice.destroy().then(Voice.removeAllListeners);
        };
    }, []);


    // ── BLE connect / disconnect ──────────
    const handleConnect = () => {
        if (connected) {
            BLEManager.disconnect();
            setConnected(false);
            return;
        }

        setScanning(true);
        BLEManager.scan(() => {
            setScanning(false);
            setConnected(true);
            Tts.speak('EchoNav connected');
        });

        // Stop scanning after 10 seconds if not found
        setTimeout(() => {
            if (!BLEManager.isConnected()) {
                setScanning(false);
                Alert.alert(
                    'Not Found',
                    'Could not find EchoNav glasses. Make sure they are powered on.'
                );
            }
        }, 10000);
    };


    // ── Voice commands ────────────────────
    const startListening = async () => {
        try {
            setListening(true);
            setVoiceText('');
            await Voice.start('en-US');
        } catch (e) {
            console.error('[VOICE]', e);
            setListening(false);
        }
    };

    const stopListening = async () => {
        try {
            await Voice.stop();
            setListening(false);
        } catch (e) {
            console.error('[VOICE]', e);
        }
    };

    const onSpeechResults = (event) => {
        const text = event.value[0].toLowerCase();
        setVoiceText(text);
        setListening(false);
        handleVoiceCommand(text);
    };

    const handleVoiceCommand = (command) => {
        // Simple keyword matching for common commands
        if (command.includes('live feed') || command.includes('show feed')) {
            navigation.navigate('LiveFeed');
        } else if (command.includes('settings')) {
            navigation.navigate('Settings');
        } else if (command.includes('testing') || command.includes('test session')) {
            navigation.navigate('Testing');
        } else if (command.includes('connect')) {
            handleConnect();
        } else if (command.includes('disconnect')) {
            BLEManager.disconnect();
            setConnected(false);
        } else {
            Tts.speak('Command not recognised. Try: live feed, settings, or connect.');
        }
    };


    // ── Render ────────────────────────────
    return (
        <View style={styles.container}>

            {/* Logo / title */}
            <Text style={styles.title}>EchoNav</Text>
            <Text style={styles.subtitle}>AI Navigation Glasses</Text>

            {/* Connection status */}
            <View style={[
                styles.statusBadge,
                { backgroundColor: connected ? '#2ecc71' : '#e74c3c' }
            ]}>
                <Text style={styles.statusText}>
                    {connected ? '● Connected' : '○ Disconnected'}
                </Text>
            </View>

            {/* Connect button */}
            <TouchableOpacity
                style={[styles.button, scanning && styles.buttonDisabled]}
                onPress={handleConnect}
                disabled={scanning}
            >
                {scanning
                    ? <ActivityIndicator color="#fff" />
                    : <Text style={styles.buttonText}>
                        {connected ? 'Disconnect' : 'Connect to Glasses'}
                      </Text>
                }
            </TouchableOpacity>

            {/* Voice command button — hold to speak */}
            <TouchableOpacity
                style={[styles.voiceButton, listening && styles.voiceButtonActive]}
                onPressIn={startListening}
                onPressOut={stopListening}
            >
                <Text style={styles.voiceIcon}>🎤</Text>
                <Text style={styles.voiceLabel}>
                    {listening ? 'Listening...' : 'Hold to speak'}
                </Text>
            </TouchableOpacity>

            {/* Show last voice command */}
            {voiceText !== '' && (
                <Text style={styles.voiceText}>"{voiceText}"</Text>
            )}

            {/* Last detection alert */}
            {lastAlert && (
                <View style={[
                    styles.alertBox,
                    { borderColor: lastAlert.color }
                ]}>
                    <Text style={[styles.alertTitle, { color: lastAlert.color }]}>
                        {PRIORITY_LABELS[lastAlert.priority]} ALERT
                    </Text>
                    <Text style={styles.alertBody}>
                        {lastAlert.label} — {lastAlert.distance}m
                        ({DIR_LABELS[lastAlert.direction]})
                    </Text>
                    <Text style={styles.alertTime}>{lastAlert.timestamp}</Text>
                </View>
            )}

            {/* Navigation buttons */}
            <View style={styles.navRow}>
                <TouchableOpacity
                    style={styles.navButton}
                    onPress={() => navigation.navigate('LiveFeed')}
                >
                    <Text style={styles.navButtonText}>Live Feed</Text>
                </TouchableOpacity>

                <TouchableOpacity
                    style={styles.navButton}
                    onPress={() => navigation.navigate('Settings')}
                >
                    <Text style={styles.navButtonText}>Settings</Text>
                </TouchableOpacity>

                <TouchableOpacity
                    style={styles.navButton}
                    onPress={() => navigation.navigate('Testing')}
                >
                    <Text style={styles.navButtonText}>Testing</Text>
                </TouchableOpacity>
            </View>

        </View>
    );
}

// Labels used in JSX above
const PRIORITY_LABELS = { 0: 'LOW', 1: 'MEDIUM', 2: 'HIGH' };
const DIR_LABELS = { 0: 'LEFT', 1: 'CENTER', 2: 'RIGHT' };


// ── Styles ────────────────────────────────
const styles = StyleSheet.create({
    container: {
        flex: 1,
        backgroundColor: '#1a1a2e',
        alignItems: 'center',
        justifyContent: 'center',
        padding: 24,
    },
    title: {
        fontSize: 42,
        fontWeight: 'bold',
        color: '#ffffff',
        letterSpacing: 4,
    },
    subtitle: {
        fontSize: 14,
        color: '#8888aa',
        marginBottom: 32,
        letterSpacing: 2,
    },
    statusBadge: {
        paddingHorizontal: 20,
        paddingVertical: 6,
        borderRadius: 20,
        marginBottom: 24,
    },
    statusText: {
        color: '#ffffff',
        fontWeight: 'bold',
        fontSize: 14,
    },
    button: {
        backgroundColor: '#4a90e2',
        paddingVertical: 16,
        paddingHorizontal: 40,
        borderRadius: 12,
        marginBottom: 20,
        width: '100%',
        alignItems: 'center',
    },
    buttonDisabled: {
        backgroundColor: '#555577',
    },
    buttonText: {
        color: '#ffffff',
        fontSize: 17,
        fontWeight: 'bold',
    },
    voiceButton: {
        backgroundColor: '#2d2d4e',
        borderWidth: 2,
        borderColor: '#4a90e2',
        paddingVertical: 18,
        paddingHorizontal: 40,
        borderRadius: 12,
        marginBottom: 12,
        width: '100%',
        alignItems: 'center',
        flexDirection: 'row',
        justifyContent: 'center',
        gap: 10,
    },
    voiceButtonActive: {
        borderColor: '#e74c3c',
        backgroundColor: '#3d1a1a',
    },
    voiceIcon: {
        fontSize: 22,
    },
    voiceLabel: {
        color: '#ffffff',
        fontSize: 16,
    },
    voiceText: {
        color: '#8888aa',
        fontStyle: 'italic',
        marginBottom: 16,
        fontSize: 13,
    },
    alertBox: {
        borderWidth: 2,
        borderRadius: 10,
        padding: 14,
        width: '100%',
        marginBottom: 20,
    },
    alertTitle: {
        fontWeight: 'bold',
        fontSize: 16,
        marginBottom: 4,
    },
    alertBody: {
        color: '#ffffff',
        fontSize: 14,
    },
    alertTime: {
        color: '#8888aa',
        fontSize: 11,
        marginTop: 4,
    },
    navRow: {
        flexDirection: 'row',
        gap: 10,
        width: '100%',
    },
    navButton: {
        flex: 1,
        backgroundColor: '#2d2d4e',
        paddingVertical: 12,
        borderRadius: 10,
        alignItems: 'center',
    },
    navButtonText: {
        color: '#aaaacc',
        fontSize: 13,
        fontWeight: '600',
    },
});